#pragma once

#include <vector>
#include <memory>
#include "sql/operator/logical_operator.h"
#include "common/rc.h"
#include "sql/stmt/stmt.h"
#include "storage/field/field.h"
#include "storage/field/order_field.h"
#include "storage/field/agg_field.h"
#include "sql/optimizer/optimize_stage.h"
#include "subquery_helper.h"
class FieldMeta;
class FilterStmt;
class Db;
class Table;

/**
 * @brief 表示select语句
 * @ingroup Statement
 */
class SelectStmtV2 : public Stmt 
{
public:
  SelectStmtV2() = default;
  ~SelectStmtV2() override;

  StmtType type() const override
  {
    return StmtType::SELECT;
  }

public:
  static RC create(Db *db, const SelectSqlNode &select_sql, Stmt *&stmt,SQLStageEvent *sql_event=nullptr);

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
  const std::vector<OrderFiled> &order_fileds() const
  {
    return order_fileds_;
  }
  const std::vector<AggField> &agg_fields() const
  {
    return agg_fields_;
  }
private:
  std::vector<Field> query_fields_;
  std::vector<Table *> tables_;
  FilterStmt *filter_stmt_ = nullptr;
  std::vector<OrderFiled> order_fileds_;
  std::vector<AggField> agg_fields_;
};
