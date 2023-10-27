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

#include "common/log/log.h"
#include "sql/expr/tuple.h"
#include "sql/operator/physical_operator.h"
#include "storage/field/order_field.h"
#include <cstddef>


class SortCompare{
  public:
  SortCompare(std::vector<OrderFiled> order_fields):order_fields_(order_fields)
  {
    for(auto order_field : order_fields_){
      TupleCellSpec tuple_cell_spec(order_field.table_name(),order_field.field_name());
      tuple_cell_specs_.push_back(tuple_cell_spec);
    }
  }
  public:
  std::vector<OrderFiled> order_fields_;
  std::vector<TupleCellSpec> tuple_cell_specs_;
  bool comparePairs(ProjectTuple * p1, ProjectTuple * p2) {
    bool result = false;
    for(int i = 0; i < tuple_cell_specs_.size();i++){
      Value v1,v2;
      p1->find_cell(tuple_cell_specs_[i], v1);
      p2->find_cell(tuple_cell_specs_[i], v2);
      int cmp_result = v1.compare(v2);
      if(cmp_result == 0 || (v1.isNull() && v2.isNull())){
        continue;
      }
      if(order_fields_[i].order() == ASC_TYPE){
        return cmp_result < 0;
      }else{
        return cmp_result > 0;
      }
    }
    return result;
  }
};


class SortPhysicalOperator : public PhysicalOperator
{
public:
  SortPhysicalOperator(std::vector<OrderFiled> order_fields,std::vector<Field> project_fields):order_fields_(order_fields),SortCompare_(order_fields),project_fields_(project_fields)
  {
    
  }

  virtual ~SortPhysicalOperator() = default;

  void add_expressions(std::vector<std::unique_ptr<Expression>> &&expressions)
  {
    
  }

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::SORT;
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
  RowTuple tuple_;
  std::vector<OrderFiled> order_fields_;
  std::vector<Field> project_fields_;
  SortCompare SortCompare_;
  std::vector<ProjectTuple*> tuples_;
  std::ifstream MergedFileInputStream_;
  size_t record_num_;
  int count = 0;
};


