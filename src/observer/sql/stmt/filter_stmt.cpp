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
// Created by Wangyunlai on 2022/5/22.
//

#include "common/rc.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"
#include "sql/parser/value.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "utlis/date.h"
#include <cstdint>

FilterStmt::~FilterStmt()
{
  for (FilterUnit *unit : filter_units_) {
    delete unit;
  }
  filter_units_.clear();
}

RC FilterStmt::create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt = nullptr;

  FilterStmt *tmp_stmt = new FilterStmt();
  for (int i = 0; i < condition_num; i++) {
    FilterUnit *filter_unit = nullptr;
    rc = create_filter_unit(db, default_table, tables, conditions[i], filter_unit);
    if (rc != RC::SUCCESS) {
      delete tmp_stmt;
      LOG_WARN("failed to create filter unit. condition index=%d", i);
      return rc;
    }
    tmp_stmt->filter_units_.push_back(filter_unit);
  }

  stmt = tmp_stmt;
  return rc;
}

RC get_table_and_field(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const RelAttrSqlNode &attr, Table *&table, const FieldMeta *&field)
{
  if (common::is_blank(attr.relation_name.c_str())) {
    table = default_table;
  } else if (nullptr != tables) {
    auto iter = tables->find(attr.relation_name);
    if (iter != tables->end()) {
      table = iter->second;
    }
  } else {
    table = db->find_table(attr.relation_name.c_str());
  }
  if (nullptr == table) {
    LOG_WARN("No such table: attr.relation_name: %s", attr.relation_name.c_str());
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  field = table->table_meta().field(attr.attribute_name.c_str());
  if (nullptr == field) {
    LOG_WARN("no such field in table: table %s, field %s", table->name(), attr.attribute_name.c_str());
    table = nullptr;
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  return RC::SUCCESS;
}

RC FilterStmt::create_filter_unit(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ConditionSqlNode &condition, FilterUnit *&filter_unit)
{
  RC rc = RC::SUCCESS;

  CompOp comp = condition.comp;
  if (comp < EQUAL_TO || comp >= NO_OP) {
    LOG_WARN("invalid compare operator : %d", comp);
    return RC::INVALID_ARGUMENT;
  }

  filter_unit = new FilterUnit;


  if (condition.left_type == ATTR) {
    Table *table = nullptr;
    const FieldMeta *field = nullptr;
    rc = get_table_and_field(db, default_table, tables, condition.left_attr, table, field);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    FilterObj filter_obj;
    filter_obj.init_attr(Field(table, field));
    filter_unit->set_left(filter_obj);
  } else if(condition.left_type == SINGLE_VALUE) {
    FilterObj filter_obj;
    filter_obj.init_value(condition.left_value);
    filter_unit->set_left(filter_obj);
  }else{
    FilterObj filter_obj;
    filter_obj.init_value_list(condition.left_value_list);
    filter_unit->set_left(filter_obj);
  }

  if (condition.right_type == ATTR) {
    Table *table = nullptr;
    const FieldMeta *field = nullptr;
    rc = get_table_and_field(db, default_table, tables, condition.right_attr, table, field);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    FilterObj filter_obj;
    filter_obj.init_attr(Field(table, field));
    filter_unit->set_right(filter_obj);
  } else if(condition.right_type == SINGLE_VALUE) {
    FilterObj filter_obj;
    filter_obj.init_value(condition.right_value);
    filter_unit->set_right(filter_obj);
  }else{
    FilterObj filter_obj;
    filter_obj.init_value_list(condition.right_value_list);
    filter_unit->set_right(filter_obj);
  }

  filter_unit->set_comp(comp);

  // 检查两个类型是否能够比较
  if(filter_unit->right().type != VALUE_LIST &&filter_unit->left().type != VALUE_LIST){
        AttrType left_type,right_type;
    if(filter_unit->left().is_attr){
      left_type=filter_unit->left().field.attr_type();
    }else{
      left_type=filter_unit->left().value.attr_type();
    }
    if(filter_unit->right().is_attr){
      right_type=filter_unit->right().field.attr_type();
    }else{
      right_type=filter_unit->right().value.attr_type();
    }
    if((left_type==AttrType::UNDEFINED )||right_type==AttrType::UNDEFINED){
      LOG_WARN("invalid compare value : %d,%d", left_type,right_type);
      return RC::INVALID_ARGUMENT;
    }
    if(left_type==AttrType::DATES && right_type==AttrType::CHARS){
      if (!condition.right_is_attr){
      FilterObj filter_obj;
      Value value;
      int32_t date=-1;
      rc = string_to_date(condition.right_value.data(), date);
      if(rc!=RC::SUCCESS){
        LOG_ERROR("can not convert right value : %s to date type", condition.right_value.data());
        return rc;
      }
      value.set_date(date);
      filter_obj.init_value(value);
      filter_unit->set_right(filter_obj);
      }
      return rc;
    }
    if(left_type==AttrType::CHARS&&right_type==AttrType::DATES){
      if (!condition.left_is_attr){
      FilterObj filter_obj;
      Value value;
      int32_t date=-1;
      rc = string_to_date(condition.left_value.data(), date);
      if(rc!=RC::SUCCESS){
        LOG_ERROR("can not convert left value : %s to date type", condition.left_value.data());
        return rc;
      }
      value.set_date(date);
      filter_obj.init_value(value);
      filter_unit->set_left(filter_obj);
      }
      return rc;
    }
    if((condition.comp == LIKE_WITH || condition.comp == NOT_LIKE_WITH) && (left_type != CHARS || right_type != CHARS)){
      LOG_ERROR("invalid compare value for like : %d,%d", left_type,right_type);
      return RC::INVALID_ARGUMENT;
    }
  }



  //没有考虑两边都是日期形式字符串这种
  // if((left_type!=right_type)&&!((left_type==INTS&&right_type==FLOATS)||(left_type==FLOATS&&right_type==INTS))){
  //   LOG_ERROR("invalid compare value : %d,%d", left_type,right_type);
  //   return RC::INVALID_ARGUMENT;
  // }
  return rc;
}
