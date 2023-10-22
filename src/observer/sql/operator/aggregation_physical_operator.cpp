#include "common/log/log.h"
#include "sql/operator/aggregation_physical_operator.h"
#include "sql/parser/parse_defs.h"
#include "sql/parser/value.h"
#include "storage/field/agg_field.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include <cstring>
#include <vector>
#include "common/lang/comparator.h"

RC AggregationPhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  PhysicalOperator *child = children_[0].get();
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }
  
  for(AggField agg_field : agg_fields_){
    this->isFirst_.push_back(true);
    if(agg_field.is_star()){
      Value value (0);
      result_.push_back(value);
      index_.push_back(-1);
      continue;
    }
    AttrType attrType = agg_field.attr_type();
    AggOp op = agg_field.aggOp();
    const char * table_name = agg_field.table_name();
    const char * field_name = agg_field.field_name();
    for(int i =0; i < fields_.size(); i++){
      if (0 == strcmp(table_name, fields_[i].table_name()) && 0 == strcmp(field_name, fields_[i].field_name())) {
        index_.push_back(i);
        break;
      }
    }
    if(op == COUNT_AGGOP){
      Value value (0);
      result_.push_back(value);
      continue;
    }
    if(attrType == CHARS){
      Value value ("");
      value.set_null();
      result_.push_back(value);
    }
    if(attrType == INTS){
      Value value(0);
      value.set_null();
      result_.push_back(value);
    }
    if(attrType == FLOATS){
      Value value((float)0);
      value.set_null();
      result_.push_back(value);
    }
    if(attrType == DATES){
      Value value("");
      value.set_null();
      result_.push_back(value);
    }
  }
  return RC::SUCCESS;
}

RC AggregationPhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }
  PhysicalOperator *child = children_[0].get();
  int count = 0;
  std::vector<int> counts(agg_fields_.size());
  bool hasOutput = false;
  bool isFirst = true;
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }
    if(!hasOutput)
      hasOutput = true;
    count++;
    ProjectTuple *row_tuple = static_cast<ProjectTuple *>(tuple);
    for(int i =0; i <agg_fields_.size(); i++){
      int index = index_[i];
      Value v;
      row_tuple->cell_at(index,v);
      if(agg_fields_[i].aggOp() == COUNT_AGGOP){
        if(agg_fields_[i].is_star() || !v.isNull()){
          result_[i].set_int(result_[i].get_int()+1);
          continue;
        }
      }
      if(!v.isNull())
        counts[i]++;
      if(agg_fields_[i].attr_type() == INTS){
        int tmp = result_[i].get_int();
        if(v.isNull()){
          continue;
        }

        int cell_value = v.get_int();
        switch (agg_fields_[i].aggOp())
        {
        case MAX_AGGOP:
        {
          if(isFirst_[i])
            result_[i].set_int(cell_value);
          else
            result_[i].set_int(tmp > cell_value ? tmp : cell_value);
          break;
        }

        case MIN_AGGOP:
          if(isFirst_[i])
            result_[i].set_int(cell_value);
          else
            result_[i].set_int(tmp < cell_value ? tmp : cell_value);
          break; 
        case SUM_AGGOP:
        case AVG_AGGOP:
          result_[i].set_int(tmp+cell_value);
          break;        
        default:
          break;
        }
        isFirst_[i] = false;
      }
      if (agg_fields_[i].attr_type() == FLOATS){
        float tmp = result_[i].get_float();
        if(v.isNull()){
          continue;
        }
        float cell_value = v.get_float();
        switch (agg_fields_[i].aggOp())
        {
        case MAX_AGGOP:
          if(isFirst_[i])
            result_[i].set_int(cell_value);
          else
            result_[i].set_int(tmp > cell_value ? tmp : cell_value);
          break;
        case MIN_AGGOP:
          if(isFirst_[i])
            result_[i].set_int(cell_value);
          else
            result_[i].set_int(tmp < cell_value ? tmp : cell_value);
          break; 
        case SUM_AGGOP:
        case AVG_AGGOP:
          result_[i].set_float(tmp+cell_value);
          break;        
        default:
          break;
        }
        isFirst_[i] = false;
      }
      if (agg_fields_[i].attr_type() == DATES){
        int32_t tmp = result_[i].get_int32();
        if(v.isNull())
          continue;
        int32_t cell_value = v.get_int32();
        switch (agg_fields_[i].aggOp())
        {
        case MAX_AGGOP:
          if(isFirst_[i])
            result_[i].set_date(cell_value);
          else
            result_[i].set_date(tmp > cell_value ? tmp : cell_value);
          break;
        case MIN_AGGOP:
          if(isFirst_[i])
            result_[i].set_date(cell_value);
          else
            result_[i].set_date(tmp < cell_value ? tmp : cell_value);
          break;       
        default:
          break;
        }
        isFirst_[i] = false;
      }
      if (agg_fields_[i].attr_type() == CHARS){
        std::string tmp = result_[i].get_string();
        if(v.isNull())
          continue;
        std::string cell_value = v.get_string();
        int r = common::compare_string((void *)tmp.c_str(),tmp.length(),(void *)cell_value.c_str(),cell_value.length());
        switch (agg_fields_[i].aggOp())
        {
        case MAX_AGGOP:
          if(isFirst_[i])
            result_[i].set_string(cell_value.c_str());
          else
            result_[i].set_string(r > 0  ? tmp.c_str() : cell_value.c_str());
          break;
        case MIN_AGGOP:
          if(isFirst_[i])
            result_[i].set_string(cell_value.c_str());
          else
          result_[i].set_string(r < 0? tmp.c_str() : cell_value.c_str());
          break;       
        default:
          break;
        }
      }
      isFirst_[i] = false;
      }
    }
      for(int i =0; i <agg_fields_.size(); i++){
        if(agg_fields_[i].aggOp() == AVG_AGGOP){
          if(agg_fields_[i].attr_type() == INTS && !result_[i].isNull()){
            result_[i].set_float(1.0*result_[i].get_int()/counts[i]);
          }
          if(agg_fields_[i].attr_type() == FLOATS && !result_[i].isNull()){
            result_[i].set_float(result_[i].get_float()/counts[i]);
          }
        }
      }

  if (hasOutput)
    rc = RC::SUCCESS;
  return rc;
}

RC AggregationPhysicalOperator::close()
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }
  return RC::SUCCESS;
}

  

  

Tuple *AggregationPhysicalOperator::current_tuple()
{
  tuple_.set_cells(this->result_);
  return &tuple_;
}

// void AggregationPhysicalOperator::add_projection(const AggField field)
// {
//   // 对单表来说，展示的(alias) 字段总是字段名称，
//   // 对多表查询来说，展示的alias 需要带表名字
//   TupleCellSpec *spec = new TupleCellSpec(field.table_name(),field.field_name(),nullptr,field.aggOp());
//   tuple_.add_cell_spec(spec);
// }