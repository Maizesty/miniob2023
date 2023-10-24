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
#include "sql/operator/sort_physical_operator.h"
#include "storage/record/record.h"
#include "storage/table/table.h"

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

  bool hasOutput = false;
  while (RC::SUCCESS == (rc = oper->next())) {
    Tuple *tuple = oper->current_tuple();
    if (nullptr == tuple) {
      rc = RC::INTERNAL;
      LOG_WARN("failed to get tuple from operator");
      break;
    }
    RowTuple* rowTuple =new RowTuple(*static_cast<RowTuple *>(tuple));
    
    tuples_.push_back(rowTuple);
    
  }
  auto sortLambda = [this](RowTuple * p1, RowTuple * p2) {
      return  this->SortCompare_.comparePairs(p1, p2);
  };
  std::sort(tuples_.begin(), tuples_.end(),sortLambda);
  std::reverse(tuples_.begin(), tuples_.end());
  if(tuples_.size() == count)
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
  return tuples_[count++];
}


