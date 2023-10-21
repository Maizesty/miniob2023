#include "sql/operator/update_logical_operator.h"
#include "storage/field/field.h"

UpdateLogicalOperator::UpdateLogicalOperator(Table *table, std::vector<Value> values, std::vector<const FieldMeta> field_metas)
    : table_(table), values_(values), field_metas_(field_metas)
{
}