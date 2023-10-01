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
  UpdatePhysicalOperator(Table *table, std::vector<Value> &&values, const FieldMeta* field_meta);

  virtual ~UpdatePhysicalOperator() = default;

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::UPDATE;
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override { return nullptr; }
  const FieldMeta* field_meta() const{
    return field_meta_;
  }

private:
  Table *table_ = nullptr;
  std::vector<Value> values_;
  Trx *trx_ = nullptr;
  const FieldMeta* field_meta_;
};