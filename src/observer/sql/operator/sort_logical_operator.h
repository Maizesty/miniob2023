#pragma once

#include <vector>
#include <memory>

#include "sql/operator/logical_operator.h"
#include "sql/expr/expression.h"
#include "storage/field/field.h"
#include "storage/field/order_field.h"


class SortLogicalOperator : public LogicalOperator 
{
public:
  SortLogicalOperator(const std::vector<OrderFiled> &order_fields,std::vector<Field>      project_fields):order_fileds_(order_fields),project_fields_(project_fields)
  {
    
  }
  virtual ~SortLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::SORT;
  }

  std::vector<std::unique_ptr<Expression>> &expressions()
  {
    return expressions_;
  }
  const std::vector<std::unique_ptr<Expression>> &expressions() const
  {
    return expressions_;
  }
  const std::vector<OrderFiled> &order_fileds() const
  {
    return order_fileds_;
  }
  const std::vector<Field> &project_fields() const
  {
    return project_fields_;
  }
private:
  std::vector<OrderFiled> order_fileds_;
  std::vector<Field>      project_fields_;
};
