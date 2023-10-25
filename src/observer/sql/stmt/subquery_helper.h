#pragma once


#include "sql/optimizer/optimize_stage.h"
#include "sql/executor/execute_stage.h"
#include "sql/parser/parse_stage.h"
#include "sql/query_cache/query_cache_stage.h"
#include "sql/stmt/select_stmtV2.h"

#include <memory>
class SubqueryHelper 
{
public:
  RC handleSubQuery(Db *db, ParsedSqlNode &sql_node,std::vector<Value>& value_list,SQLStageEvent *sql_event,int * num);


private:
  OptimizeStage   optimize_stage_;
  ExecuteStage    execute_stage_;
};