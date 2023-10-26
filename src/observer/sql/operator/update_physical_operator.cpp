#include "sql/operator/update_physical_operator.h"
#include "sql/parser/value.h"
#include "sql/stmt/update_stmt.h"
#include "storage/field/field.h"
#include "storage/field/field_meta.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include <cstring>

using namespace std;

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, vector<Value> &&values,vector<const FieldMeta *> &&field_metas,bool has_multi_row)
    : table_(table), values_(std::move(values)), field_metas_(field_metas),has_multi_row_(has_multi_row)
{}
RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  std::unique_ptr<PhysicalOperator> &child = children_[0];
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  trx_ = trx;

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    return RC::RECORD_EOF;
  }

  PhysicalOperator *child = children_[0].get();
  while (RC::SUCCESS == (rc = child->next())) {
    if(has_multi_row_){
      LOG_WARN("too many rows for update!");
      return RC::INTERNAL;
    }
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &record = row_tuple->record();
    Record newRecord;
    int record_size = table_->table_meta().record_size();
    char *record_data = (char *)malloc(record_size);
    memcpy(record_data, record.data(), record_size);
    char * bitmap = (char *)malloc(4);
    memcpy(bitmap,record.data(),4);
    for(int i = 0; i < field_metas_.size(); i++){
      size_t copy_len = field_metas_[i]->len();
      if(!field_metas_[i]->isNullable() && values_[i].isNull()){
          LOG_WARN("can not update not null col with null");
          return RC::INTERNAL;
      }
      int index = 3- field_metas_[i]->index()/8, byte = field_metas_[i]->index()%8;
      int isNull = bitmap[index] & (0x01 << byte);
      if(isNull && values_[i].isNull()){
        continue;
      }
      if(!isNull && values_[i].isNull()){
        bitmap[index] = bitmap[index] | (0x01 << byte);
        continue;
      }
      if(isNull && !values_[i].isNull()){
        char mask = ~(1 << byte);
        bitmap[index] = bitmap[index] & mask;
      }
      if(field_metas_[i]->type() == CHARS){
        const size_t data_len = values_[i].length();
        if(copy_len > data_len){
          copy_len = data_len + 1;
        }
      } 
      memcpy(record_data + field_metas_[i]->offset(), values_[i].data(), copy_len);
    }
    memcpy(record_data,bitmap,4);
    newRecord.set_data_owner(record_data, record_size);
    rc = trx_->update_record(table_, record,newRecord);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to delete record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
