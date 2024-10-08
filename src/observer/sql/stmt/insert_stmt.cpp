/* Copyright (c) 2021OceanBase and/or its affiliates. All rights reserved.
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

#include "sql/stmt/insert_stmt.h"
#include "common/log/log.h"
#include "sql/parser/value.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "utlis/date.h"
#include <cstdint>

InsertStmt::InsertStmt(Table *table, std::vector<std::vector<Value>> values_list, int value_amount)
    : table_(table), values_list_(values_list), value_amount_(value_amount)
{}

RC InsertStmt::create(Db *db, const InsertSqlNode &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name.c_str();
  // if (nullptr == db || nullptr == table_name || inserts.values.empty()) {
  //   LOG_WARN("invalid argument. db=%p, table_name=%p, value_num=%d",
  //       db, table_name, static_cast<int>(inserts.values.size()));
  //   return RC::INVALID_ARGUMENT;
  // }
  if (nullptr == db || nullptr == table_name || inserts.values_list.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, value_list_num=%d",
        db, table_name, static_cast<int>(inserts.values_list.size()));
    return RC::INVALID_ARGUMENT;
  }
  
  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }
  std::vector<std::vector<Value>> values_list;
  // check the fields number
  // const Value *values = inserts.values.data();
  for(std::vector<Value> values_arr:inserts.values_list){
    Value *values = const_cast<Value*>(values_arr.data());
    const int value_num = static_cast<int>(values_arr.size());
    const TableMeta &table_meta = table->table_meta();
    const int field_num = table_meta.field_num() - table_meta.sys_field_num();
    if (field_num != value_num) {
      LOG_WARN("schema mismatch. value num=%d, field num in schema=%d", value_num, field_num);
      return RC::SCHEMA_FIELD_MISSING;
    }

    // check fields type
    const int sys_field_num = table_meta.sys_field_num();
    for (int i = 0; i < value_num; i++) {
      const FieldMeta *field_meta = table_meta.field(i + sys_field_num);
      const AttrType field_type = field_meta->type();
      const AttrType value_type = values[i].attr_type();
      if(values[i].isNull() && !field_meta->isNullable()){
        LOG_ERROR("can not insert null into not null col");
        return RC::INTERNAL;
      }
      if(values[i].isNull()){
        Value v(NULLS,"nil",4);
        values[i]=v;
        continue;
      }
      if (field_type != value_type) { 
          // TODO try to convert the value type to field type
          if(!(field_type == AttrType::DATES && value_type == AttrType::CHARS)){
                  LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
                table_name, field_meta->name(), field_type, value_type);
              return RC::SCHEMA_FIELD_TYPE_MISMATCH;
          }else{
          int32_t date = -1;
          RC rc = string_to_date(values[i].data(), date);
          if(rc != RC::SUCCESS){
            LOG_WARN("field type mismatch. can not convert string %s to date, table=%s, field=%s, field type=%d, value_type=%d",
            values[i].data(),table_name, field_meta->name(), field_type, value_type);
            return rc;
          }
          Value v;
          v.set_date(date);
          values[i]=v;
        }

      }
    }
    std::vector<Value> values_vector(values, values + value_num);
    values_list.emplace_back(values_vector);
  }
    

  // everything alright
  stmt = new InsertStmt(table, values_list, values_list[0].size());
  return RC::SUCCESS;
}