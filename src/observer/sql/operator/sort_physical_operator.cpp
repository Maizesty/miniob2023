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

#include "common/log/log.h"
#include "sql/expr/tuple.h"
#include "sql/operator/sort_physical_operator.h"
#include "sql/parser/value.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include <algorithm>
#include <vector>

RC SortPhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  PhysicalOperator *child = children_[0].get();
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC SortPhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  PhysicalOperator *oper = children_.front().get();
  int backet_num = 0;
  bool isFirst = true;
  std::string filename_base = "sort_file";
  auto sortLambda = [this](ValueListTuple * p1, ValueListTuple * p2) {
      return  this->SortCompare_.comparePairs(p1, p2);
  };
  while (RC::SUCCESS == (rc = oper->next())) {
    Tuple *tuple = oper->current_tuple();
    if (nullptr == tuple) {
      rc = RC::INTERNAL;
      LOG_WARN("failed to get tuple from operator");
      break;
    }
    std::vector<Value> value_list;
    for(int i =0;i<tuple->cell_num();i++){
      Value v;
      tuple->cell_at(i, v);
      value_list.push_back(v);
    }
    for(int i =0; i<tuple_cell_specs_.size();i++ ){
      Value v;
      tuple->find_cell(tuple_cell_specs_[i], v);
      value_list.push_back(v);
    }
    ValueListTuple* value_tuple = new ValueListTuple(value_list);

    value_list_tuples_.push_back(value_tuple);
    
  }
  if(!isSort){
    std::sort(value_list_tuples_.begin(),value_list_tuples_.end(),sortLambda);
    isSort = true;
  }

  if(value_list_tuples_.size() == count)
    return  RC::RECORD_EOF;
  return RC::SUCCESS;
}

RC SortPhysicalOperator::close()
{
  
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
Tuple *SortPhysicalOperator::current_tuple()
{
  value_list_tuples_[count]->pop(order_fields_.size());
  return value_list_tuples_[count++];
}


