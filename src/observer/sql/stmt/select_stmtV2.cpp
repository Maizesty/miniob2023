/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/6/6.
//

#include "sql/stmt/select_stmtV2.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/filter_stmt.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/db/db.h"
#include "storage/field/order_field.h"
#include "storage/table/table.h"

SelectStmtV2::~SelectStmtV2()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

static void wildcard_fields(Table *table, std::vector<Field> &field_metas)
{
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num();
  for (int i = table_meta.sys_field_num(); i < field_num; i++) {
    field_metas.push_back(Field(table, table_meta.field(i)));
  }
}

RC SelectStmtV2::create(Db *db, const SelectSqlNode &select_sql, Stmt *&stmt,SQLStageEvent *sql_event)
{


  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }

  // collect tables in `from` statement
  std::vector<Table *> tables;
  std::unordered_map<std::string, Table *> table_map;
  for (size_t i = 0; i < select_sql.relations.size(); i++) {
    const char *table_name = select_sql.relations[i].c_str();
    if (nullptr == table_name) {
      LOG_WARN("invalid argument. relation name is null. index=%d", i);
      return RC::INVALID_ARGUMENT;
    }

    Table *table = db->find_table(table_name);
    if (nullptr == table) {
      LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }

    tables.push_back(table);
    table_map.insert(std::pair<std::string, Table *>(table_name, table));
  }

  // collect query fields in `select` statement
  std::vector<Field> query_fields;
  std::vector<AggField> agg_fields;

  for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--) {
    const RelAttrSqlNode &relation_attr = select_sql.attributes[i];
    
    if (common::is_blank(relation_attr.relation_name.c_str()) &&
        0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
      for (Table *table : tables) {
        wildcard_fields(table, query_fields);
      }
      if(relation_attr.aggOp !=NO_AGGOP)
        agg_fields.push_back(AggField(nullptr,nullptr,relation_attr.aggOp,true));

    } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
      const char *table_name = relation_attr.relation_name.c_str();
      const char *field_name = relation_attr.attribute_name.c_str();

      if (0 == strcmp(table_name, "*")) {
        if (0 != strcmp(field_name, "*")) {
          LOG_WARN("invalid field name while table is *. attr=%s", field_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        for (Table *table : tables) {
          wildcard_fields(table, query_fields);
          if(relation_attr.aggOp !=NO_AGGOP)
            agg_fields.push_back(AggField(nullptr,nullptr,relation_attr.aggOp,true));
        }
      } else {
        auto iter = table_map.find(table_name);
        if (iter == table_map.end()) {
          LOG_WARN("no such table in from list: %s", table_name);
          return RC::SCHEMA_FIELD_MISSING;
        }

        Table *table = iter->second;
        if (0 == strcmp(field_name, "*")) {
          wildcard_fields(table, query_fields);
        } else {
          const FieldMeta *field_meta = table->table_meta().field(field_name);
          if (nullptr == field_meta) {
            LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
            return RC::SCHEMA_FIELD_MISSING;
          }

          query_fields.push_back(Field(table, field_meta));
          if(relation_attr.aggOp !=NO_AGGOP)
            agg_fields.push_back(AggField(table,field_meta,relation_attr.aggOp,false));
        }
      }
    } else {
      if (tables.size() != 1) {
        LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
        return RC::SCHEMA_FIELD_MISSING;
      }

      Table *table = tables[0];
      const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
      if (nullptr == field_meta) {
        LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
        return RC::SCHEMA_FIELD_MISSING;
      }

      query_fields.push_back(Field(table, field_meta));
      if(relation_attr.aggOp !=NO_AGGOP)
        agg_fields.push_back(AggField(table,field_meta,relation_attr.aggOp,false));
    }

    //TODO 一些聚合函数的规则校验
    if(0 == strcmp(relation_attr.attribute_name.c_str(), "*") && (relation_attr.aggOp != COUNT_AGGOP &&  relation_attr.aggOp != NO_AGGOP)){
      LOG_WARN("invalid argument.Can not use this op %d on *",relation_attr.aggOp);
      return RC::INVALID_ARGUMENT;
    }
  }

  LOG_INFO("got %d tables in from stmt and %d fields in query stmt", tables.size(), query_fields.size());
  if(!agg_fields.empty()){
    for(auto attr : select_sql.attributes){
      if(attr.aggOp==NO_AGGOP){
      LOG_WARN("invalid argument.Can not have scalar col without group by");
      return RC::INVALID_ARGUMENT;
      }
    }
  }
  Table *default_table = nullptr;
  if (tables.size() == 1) {
    default_table = tables[0];
  }
  std::vector<OrderFiled> order_fileds;
  for(OrderByNode order : select_sql.order_by_node_list){
    RelAttrSqlNode rel = order.rel;
    
    if(common::is_blank(rel.relation_name.c_str())){
      if(tables.size()>1){
        LOG_WARN("invalid. I do not know the attr's table. attr=%s", rel.attribute_name.c_str());
        return RC::SCHEMA_FIELD_MISSING;
      }
      Table *table = tables[0];
      const FieldMeta *field_meta = table->table_meta().field(rel.attribute_name.c_str());
      if (nullptr == field_meta) {
        LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), rel.attribute_name.c_str());
        return RC::SCHEMA_FIELD_MISSING;
      }
      order_fileds.push_back(OrderFiled(table,field_meta,order.orderByType));
    }else{
      std::string table_name = rel.relation_name;
      Table *table = db->find_table(table_name.c_str());
      if (nullptr == table) {
      LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name.c_str());
      return RC::SCHEMA_TABLE_NOT_EXIST;
      }
      if(find(tables.begin(),tables.end(),table) == tables.end()){
        LOG_WARN("no such table in select query field,: ",table_name.c_str());
        return RC::INTERNAL;
      }
      const FieldMeta *field_meta = table->table_meta().field(rel.attribute_name.c_str());
      if (nullptr == field_meta) {
        LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), rel.attribute_name.c_str());
        return RC::SCHEMA_FIELD_MISSING;
      }
      order_fileds.push_back(OrderFiled(table,field_meta,order.orderByType));
    }
      
  }
  RC rc = RC::SUCCESS;
  SubqueryHelper subqueryHelper_;
  //侵入性修改：
  std::vector<ConditionSqlNode> conditions;
  for(auto condition : select_sql.conditions){
    if(condition.left_type == SUBQUERY){
      Stmt *stmt = nullptr;
      Value* left_value_list;
      int num = 0;
      std::vector<Value> tmp;
      rc = subqueryHelper_.handleSubQuery(db, *condition.left_sub_query, tmp, sql_event,&num);
      if(rc!=RC::SUCCESS){
        LOG_WARN("has complex subquery");
        return rc;
      }else if(IsNotSetOp(condition.comp)){
        if(num>1){
          LOG_WARN("subquery return too mant rows");
          return RC::INTERNAL;
        }
        condition.left_type = SINGLE_VALUE;
        if(num == 0)
          condition.left_value.set_null();
        else
          condition.left_value.set_value(tmp[0]);
      }else{
        condition.left_type = VALUE_LIST;
        if (num>0)
          condition.left_value_list.swap(tmp);
      }

    }
    if(condition.right_type == SUBQUERY){
      Stmt *stmt = nullptr;
      Value* right_value_list;
      int num = 0;
      std::vector<Value> tmp;
      rc = subqueryHelper_.handleSubQuery(db, *condition.right_sub_query, tmp, sql_event,&num);
      if(rc!=RC::SUCCESS){
        LOG_WARN("has complex subquery");
        return rc;

        }else if(IsNotSetOp(condition.comp)){
        if(num>1){
          LOG_WARN("subquery return too mant rows");
          return RC::INTERNAL;
        }
        condition.right_type = SINGLE_VALUE;
        if(num == 0)
          condition.right_value.set_null();
        else
          condition.right_value.set_value(tmp[0]);
      }else{
        condition.right_type = VALUE_LIST;
        if (num>0)
          condition.right_value_list.swap(tmp);
      }

    }
    

    
    conditions.push_back(condition);
  }
  // create filter statement in `where` statement
  FilterStmt *filter_stmt = nullptr;
  rc = FilterStmt::create(db,
      default_table,
      &table_map,
      conditions.data(),
      static_cast<int>(select_sql.conditions.size()),
      filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct filter stmt");
    return rc;
  }

  // everything alright
  SelectStmtV2 *select_stmt = new SelectStmtV2();
  // TODO add expression copy
  select_stmt->tables_.swap(tables);
  select_stmt->query_fields_.swap(query_fields);
  select_stmt->filter_stmt_ = filter_stmt;
  select_stmt->order_fileds_.swap(order_fileds);
  select_stmt->agg_fields_.swap(agg_fields);
  stmt = select_stmt;
  return RC::SUCCESS;
}
