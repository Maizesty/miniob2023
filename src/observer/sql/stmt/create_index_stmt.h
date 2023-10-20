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
// Created by Wangyunlai on 2023/4/25.
//

#pragma once

#include <string>

#include "sql/stmt/stmt.h"

struct CreateIndexSqlNode;
class Table;
class FieldMeta;

/**
 * @brief 创建索引的语句
 * @ingroup Statement
 */
class CreateIndexStmt : public Stmt
{
public:
  // CreateIndexStmt(Table *table, const FieldMeta *field_meta, const std::string &index_name)
  //       : table_(table),
  //         field_meta_(field_meta),
  //         index_name_(index_name)
  // {}
  CreateIndexStmt(Table *table, std::vector<std::string> field_name_list, const std::string &index_name)
        : table_(table),
          field_name_list_(field_name_list),
          index_name_(index_name)
  {}
  virtual ~CreateIndexStmt() = default;

  StmtType type() const override { return StmtType::CREATE_INDEX; }

  Table *table() const { return table_; }
  // const FieldMeta *field_meta() const { return field_meta_; }
  std::vector<std::string> field_name_list() const { return field_name_list_;}
  const std::string &index_name() const { return index_name_; }

public:
  static RC create(Db *db, const CreateIndexSqlNode &create_index, Stmt *&stmt);

private:
  Table *table_ = nullptr;
  //后续就用个字段名，你传个元数据干什么？
  // const FieldMeta *field_meta_ = nullptr;
  std::string index_name_;
  std::vector<std::string> field_name_list_;
};
