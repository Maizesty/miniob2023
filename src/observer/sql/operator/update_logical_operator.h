#pragma once

#include <vector>

#include "sql/operator/logical_operator.h"
#include "sql/parser/parse_defs.h"
#include "storage/field/field_meta.h"


class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, std::vector<Value> values, std::vector<const FieldMeta *> field_metas,bool b = false);

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
  void set_field_has_mutli_row(bool field_has_mutli_row){
    this->field_has_mutli_row_ = field_has_mutli_row;
  }
  bool has_mutli_col(){
    return field_has_mutli_row_;
  }
private:
  Table *table_ = nullptr;
  std::vector<Value> values_;
  bool field_has_mutli_row_ = false;
  std::vector<const FieldMeta *> field_metas_;
  };