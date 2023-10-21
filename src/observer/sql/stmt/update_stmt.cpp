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
#include "storage/table/table.h"
UpdateStmt::UpdateStmt(Table *table, std::vector<const FieldMeta *> field_metas, FilterStmt *filter_stmt, Value *values, int value_amount)
    : table_(table), field_metas_(field_metas), filter_stmt_(filter_stmt), values_(values), value_amount_(value_amount)
{}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  // TODO
  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }
  std::string table_name = update.relation_name;
  // std::string field_name = update.attribute_name;
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
    const FieldMeta *field_meta = table->table_meta().field(updateRel_list[i].attribute_name.c_str());
    if (nullptr == field_meta) {
      LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), updateRel_list[i].attribute_name.c_str());
      return RC::SCHEMA_FIELD_MISSING;
    }
    Value      *value = new Value();
    if (field_meta->type() == AttrType::DATES && updateRel_list[i].value.attr_type() == AttrType::CHARS) {
      value->set_date(updateRel_list[i].value.get_int32());
    } else {
      value->set_value(updateRel_list[i].value);
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
  stmt = new UpdateStmt(table, field_metas, filter_stmt, values, updateRel_list.size());
  return rc;
}
