#pragma once

#include <vector>

#include "sql/operator/logical_operator.h"
#include "sql/parser/parse_defs.h"
#include "storage/field/field_meta.h"


class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, std::vector<Value> values, std::vector<const FieldMeta *> field_metas);
  virtual ~UpdateLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::UPDATE;
  }

  Table *table() const { return table_; }
  const std::vector<Value> &values() const { return values_; }
  std::vector<Value> &values() { return values_; }
  std::vector<const FieldMeta *> field_metas() const{
    return field_metas_;
  }
private:
  Table *table_ = nullptr;
  std::vector<Value> values_;
  std::vector<const FieldMeta *> field_metas_;
};