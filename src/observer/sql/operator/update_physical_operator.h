#pragma once

#include <vector>
#include "sql/operator/physical_operator.h"
#include "sql/parser/parse.h"
#include "sql/stmt/update_stmt.h"
#include "storage/field/field_meta.h"

class UpdateStmt;

class UpdatePhysicalOperator : public PhysicalOperator
{
public:
  UpdatePhysicalOperator(Table *table, std::vector<Value> &&values, std::vector<const FieldMeta *> &&field_metas,bool has_multi_row = false);

  virtual ~UpdatePhysicalOperator() = default;

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::UPDATE;
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override { return nullptr; }
  std::vector<const FieldMeta *> field_metas() const{
    return field_metas_;
  }

private:
  Table *table_ = nullptr;
  std::vector<Value> values_;
  Trx *trx_ = nullptr;
  std::vector<const FieldMeta *> field_metas_;
  bool has_multi_row_ = false;
};