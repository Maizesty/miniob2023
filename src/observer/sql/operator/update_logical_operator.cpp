#include "sql/operator/update_logical_operator.h"
#include "storage/field/field.h"

UpdateLogicalOperator::UpdateLogicalOperator(Table *table, std::vector<Value> values, const  FieldMeta* field_meta)
    : table_(table), values_(values), field_meta_(field_meta)
{
}