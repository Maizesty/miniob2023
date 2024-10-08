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
// Created by WangYunlai on 2022/6/7.
//

#pragma once

#include <iostream>
#include "sql/parser/parse_defs.h"
#include "storage/table/table.h"
#include "storage/field/field_meta.h"

class TupleCellSpec
{
public:
  TupleCellSpec(const char *table_name, const char *field_name, const char *alias = nullptr, const AggOp = NO_AGGOP);
  TupleCellSpec(const char *alias, const AggOp = NO_AGGOP);

  const char *table_name() const
  {
    return table_name_.c_str();
  }
  const char *field_name() const
  {
    return field_name_.c_str();
  }
  const char *alias() const
  {
    return alias_.c_str();
  }
  const AggOp aggOp() const{
    return aggOp_;
  }

private:
  std::string table_name_;
  std::string field_name_;
  std::string alias_;
  AggOp aggOp_;
};
