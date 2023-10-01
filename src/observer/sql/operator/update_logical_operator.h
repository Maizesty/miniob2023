#pragma once

#include <vector>

#include "sql/operator/logical_operator.h"
#include "sql/parser/parse_defs.h"
#include "storage/field/field_meta.h"


class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, std::vector<Value> values,const FieldMeta* field_meta);
  virtual ~UpdateLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::UPDATE;
  }

  Table *table() const { return table_; }
  const std::vector<Value> &values() const { return values_; }
  std::vector<Value> &values() { return values_; }
  const  FieldMeta* field_meta() const{
    return field_meta_;
  }
private:
  Table *table_ = nullptr;
  std::vector<Value> values_;
  const FieldMeta* field_meta_;
};