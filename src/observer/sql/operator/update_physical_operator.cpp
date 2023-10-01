#include "sql/operator/update_physical_operator.h"
#include "sql/parser/value.h"
#include "sql/stmt/update_stmt.h"
#include "storage/field/field.h"
#include "storage/field/field_meta.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include <cstring>

using namespace std;

UpdatePhysicalOperator::UpdatePhysicalOperator(Table *table, vector<Value> &&values,const FieldMeta* field_meta)
    : table_(table), values_(std::move(values)), field_meta_(field_meta)
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
    size_t copy_len = field_meta_->len();
    if(field_meta_->type()==CHARS){
      const size_t data_len = values_[0].length();
      if(copy_len> data_len){
        copy_len = data_len + 1;
      }
    } 
    memcpy(record_data+field_meta_->offset(), values_[0].data(),copy_len);
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
