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

#include "sql/stmt/update_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "sql/parser/value.h"
#include "storage/db/db.h"
#include "storage/field/field_meta.h"
#include "utlis/typecast.h"
#include "storage/table/table.h"
#include "string"
UpdateStmt::UpdateStmt(Table *table, std::vector<const FieldMeta *> field_metas, FilterStmt *filter_stmt, Value *values, int value_amount,bool b)
    : table_(table), field_metas_(field_metas), filter_stmt_(filter_stmt), values_(values), value_amount_(value_amount),has_multi_rows_(b)
{}


RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  // TODO
  bool has_multi_row = false;
  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }
  std::string table_name = update.relation_name;
  std::vector<UpdateRel> updateRel_list = update.updateRel_list;
  if (common::is_blank(table_name.c_str())) {
    LOG_WARN("invalid argument. relation name is blank.");
    return RC::INVALID_ARGUMENT;
  }
  for(UpdateRel uRel: updateRel_list){
    if (common::is_blank(uRel.attribute_name.c_str())) {
      LOG_WARN("invalid argument. update field name is blank.");
      return RC::INVALID_ARGUMENT;
    }
  }
  Table *table = db->find_table(table_name.c_str());
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name.c_str());
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  Value * values = new Value[updateRel_list.size()];
  std::vector<const FieldMeta *> field_metas;
  for(int i = 0; i < updateRel_list.size(); i++){
    auto uRel = updateRel_list[i];
    const FieldMeta *field_meta = table->table_meta().field(uRel.attribute_name.c_str());

    //添加subquery判断
    if(uRel.isSubquery==1){
      RC rc = RC::SUCCESS;
      SubqueryHelper subqueryHelper_;
      Stmt *stmt = nullptr;
      Value* left_value_list;
      int num = 0;
      std::vector<Value> tmp;
      SQLStageEvent *sql_event;
      rc = subqueryHelper_.handleSubQuery(db, *uRel.sub_query, tmp, sql_event,&num);

      if(rc!=RC::SUCCESS){
        LOG_WARN("can not convert right sub query into value list");
        return rc;
      }
      if(num>1){
        uRel.value=tmp[0];
        has_multi_row = true;
      }
      else if(num==0){
        uRel.value.set_null();
      }
      else{
        uRel.value=tmp[0];
      }
    }

    if (nullptr == field_meta) {
      LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), uRel.attribute_name.c_str());
      return RC::SCHEMA_FIELD_MISSING;
    }
    // if(uRel.value.isNull() && !field_meta->isNullable()){
    //   LOG_WARN("can not update not null col with null");
    //   return RC::INTERNAL;
    // }

    Value      *value = new Value();
    if(uRel.value.isNull() ){
        value->set_null();
    }else{
      if(field_meta->type() != uRel.value.attr_type()){
      if (field_meta->type() == AttrType::DATES && uRel.value.attr_type() == AttrType::CHARS) {
        value->set_date(uRel.value.get_int32());
      }else if((field_meta->type() == AttrType::INTS && uRel.value.attr_type() == AttrType::FLOATS)){
          value->set_int(round_ob(uRel.value.get_float()));
        }else if ((field_meta->type() == AttrType::FLOATS && uRel.value.attr_type() == AttrType::INTS)){
          value->set_float((float)(uRel.value.get_int()));
        }else if ((field_meta->type() == AttrType::FLOATS && uRel.value.attr_type() == AttrType::CHARS)){
          value->set_float(stringToNumber_ob(uRel.value.get_string()));
        }else if ((field_meta->type() == AttrType::INTS && uRel.value.attr_type() == AttrType::CHARS)){
          value->set_int(round_ob(stringToNumber_ob(uRel.value.get_string())));
        }else if ((field_meta->type() == AttrType::CHARS && uRel.value.attr_type() == AttrType::INTS)){
          value->set_string(std::to_string(uRel.value.get_int()).c_str());
        }else if ((field_meta->type() == AttrType::CHARS && uRel.value.attr_type() == AttrType::FLOATS)){
          value->set_string(std::to_string(uRel.value.get_float()).c_str());
        }else
        {
          return RC::INVALID_ARGUMENT;
        }
      }else
        value->set_value(uRel.value);
    }
      values[i] = *value;
      field_metas.emplace_back(field_meta);
  }



  std::unordered_map<std::string, Table *> table_map;
  table_map.insert(std::pair<std::string, Table *>(std::string(table_name), table));
  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(
      db, table, &table_map, update.conditions.data(), static_cast<int>(update.conditions.size()), filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create filter statement. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  stmt = new UpdateStmt(table, field_metas, filter_stmt, values, updateRel_list.size(),has_multi_row);
  return rc;
}
