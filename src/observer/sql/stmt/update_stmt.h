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

#pragma once

#include "common/rc.h"
#include "sql/stmt/stmt.h"
#include <string>
#include "sql/stmt/filter_stmt.h"
#include "storage/field/field_meta.h"
#include "subquery_helper.h"

class Table;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt 
{
public:
  UpdateStmt() = default;
  UpdateStmt(Table *table, std::vector<const FieldMeta *> field_metas , FilterStmt *filter_stmt , Value *values, int value_amount,bool b=false);

    StmtType type() const override
  {
    return StmtType::UPDATE;
  }


public:
  static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

public:
  Table *table() const
  {
    return table_;
  }
  Value *values() const
  {
    return values_;
  }
  int value_amount() const
  {
    return value_amount_;
  }
  FilterStmt *filter_stmt() const
  {
    return filter_stmt_;
  }
  std::vector<const FieldMeta *> field_metas() const{
    return field_metas_;
  }
  void set_has_multi_rows(bool has_multi_rows){
    this->has_multi_rows_ = has_multi_rows;
  }
  bool has_multi_rows(){
    return has_multi_rows_;
  }
private:
  Table *table_ = nullptr;
  Value *values_ = nullptr;
  std::vector<const FieldMeta *> field_metas_;
  FilterStmt *filter_stmt_ = nullptr;
  int value_amount_ = 0;
  bool has_multi_rows_ = false;
};
