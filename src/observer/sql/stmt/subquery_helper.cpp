#include "subquery_helper.h"
#include "common/global_context.h"
#include "event/session_event.h"
#include "session/session.h"
#include "storage/trx/trx.h"
#include "storage/db/db.h"

RC SubqueryHelper::handleSubQuery(Db *db, ParsedSqlNode &sql_node,std::vector<Value>& value_list,SQLStageEvent *sql_event,int* num){
  RC rc = RC::SUCCESS;
  Stmt *stmt = nullptr;
  rc = Stmt::create_stmt(db, sql_node, stmt);
  if (rc != RC::SUCCESS && rc != RC::UNIMPLENMENT) {
    LOG_WARN("failed to create stmt. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  std::unique_ptr<LogicalOperator> logical_operator;
  std::unique_ptr<PhysicalOperator> physical_operator;
  rc = optimize_stage_.handle_request(stmt, logical_operator,physical_operator);
  TupleSchema schema;
  SelectStmtV2 *select_stmt = static_cast<SelectStmtV2 *>(stmt);
  bool with_table_name = select_stmt->tables().size() > 1;
  if(select_stmt->agg_fields().empty())
    for (const Field &field : select_stmt->query_fields()) {
      if (with_table_name) {
        schema.append_cell(field.table_name(), field.field_name());
      } else {
        schema.append_cell(field.field_name());
      }
    }
  else{
    for (const AggField &field : select_stmt->agg_fields()) {
    if (with_table_name) {
      if(field.is_star())
        schema.append_cell(field.table_name(), "*",field.aggOp());
      else
        schema.append_cell(field.table_name(), field.field_name(),field.aggOp());
    } else {
      if(field.is_star())
        schema.append_cell("*",field.aggOp());
      else
        schema.append_cell(field.field_name(),field.aggOp());
    }
  }
  }
  if(schema.cell_num()>1){
    return RC::INTERNAL;
  }
  // auto trx = sql_event->session_event()->sql_result()->session()->current_trx();
  auto trx = GCTX.trx_kit_->create_trx(db->clog_manager());
  trx->start_if_need();
  physical_operator->open(trx);
  while (RC::SUCCESS == (rc = physical_operator->next())) {
    Tuple *tuple = physical_operator->current_tuple();
    Value value;
    rc = tuple->cell_at(0, value);
    if (rc != RC::SUCCESS) {
        physical_operator->close();
        return rc;
    }
    value_list.push_back(value);
  }
  rc = physical_operator->close();
  if(rc != RC::SUCCESS){
    return rc;
  }
  *num = value_list.size();

  return rc;
}