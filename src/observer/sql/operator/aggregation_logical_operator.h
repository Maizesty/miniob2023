#pragma once

#include <vector>
#include <memory>

#include "sql/operator/logical_operator.h"
#include "sql/expr/expression.h"
#include "storage/field/agg_field.h"
#include "storage/field/field.h"


class AggregationLogicalOperator : public LogicalOperator 
{
public:
  AggregationLogicalOperator(const std::vector<Field> &fields, std::vector<AggField> agg_fields_):fields_(fields), agg_fields_(agg_fields_)
  {
    
  }
  virtual ~AggregationLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::AGGREGATION;
  }

  std::vector<std::unique_ptr<Expression>> &expressions()
  {
    return expressions_;
  }
  const std::vector<std::unique_ptr<Expression>> &expressions() const
  {
    return expressions_;
  }
  const std::vector<Field> &fields() const
  {
    return fields_;
  }
  const std::vector<AggField> &agg_fields() const
  {
    return agg_fields_;
  }

private:
  std::vector<Field> fields_;
  std::vector<AggField> agg_fields_;
};
