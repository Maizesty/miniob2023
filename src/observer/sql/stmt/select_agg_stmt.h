#pragma once

#include <vector>
#include <memory>

#include "common/rc.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"
#include "storage/field/field.h"
#include "storage/field/agg_field.h"
#include "storage/field/order_field.h"

class FieldMeta;
class FilterStmt;
class Db;
class Table;

/**
 * @brief 表示带聚合的select语句
 * @ingroup SelectAggStmt
 */
class SelectAggStmt : public Stmt 
{
public:
  SelectAggStmt() = default;
  ~SelectAggStmt() override;

  StmtType type() const override
  {
    return StmtType::SELECT_AGG;
  }

public:
  static RC create(Db *db, const SelectSqlNode &select_sql, Stmt *&stmt);

public:
  const std::vector<Table *> &tables() const
  {
    return tables_;
  }
  const std::vector<Field> &query_fields() const
  {
    return query_fields_;
  }
  FilterStmt *filter_stmt() const
  {
    return filter_stmt_;
  }
  const std::vector<AggField> &agg_fields() const
  {
    return agg_fields_;
  }
  const std::vector<OrderFiled> &order_fileds() const
  {
    return order_fileds_;
  }

private:
  std::vector<Field> query_fields_;
  std::vector<AggField> agg_fields_;
  std::vector<Table *> tables_;
  FilterStmt *filter_stmt_ = nullptr;
  std::vector<OrderFiled> order_fileds_;
};
