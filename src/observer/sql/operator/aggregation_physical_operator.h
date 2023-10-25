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
// Created by WangYunlai on 2022/07/01.
//

#pragma once

#include "sql/operator/physical_operator.h"
#include "sql/parser/value.h"
#include "storage/field/agg_field.h"
#include "storage/field/field.h"
#include "gtest/gtest.h"
#include <vector>


class AggregationPhysicalOperator : public PhysicalOperator
{
public:
  AggregationPhysicalOperator(const std::vector<AggField> &agg_fields,const std::vector<Field> &fields)
  {
    for(auto agg_field: agg_fields){
      AggField agg(agg_field);
      agg_fields_.push_back(agg);
    }
    for(auto field: fields){
      Field f(field);
      fields_.push_back(f);
    }
  }

  virtual ~AggregationPhysicalOperator() = default;

  void add_expressions(std::vector<std::unique_ptr<Expression>> &&expressions)
  {
    
  }
  // void add_projection(const AggField field);

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::AGGREGATION;
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  int cell_num() const
  {
    return tuple_.cell_num();
  }

  Tuple *current_tuple() override;

private:
  ValueListTuple tuple_;
  std::vector<AggField> agg_fields_;
  std::vector<Field> fields_;
  std::vector<Value> result_;
  std::vector<int> index_;
  std::vector<bool> isFirst_;
};
