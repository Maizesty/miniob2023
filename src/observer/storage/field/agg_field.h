#pragma once

#include "storage/field/field.h"
#include "storage/table/table.h"
#include "storage/field/field_meta.h"

class AggField: public Field{
    public:
    AggField() = default;
    AggField(const Table *table, const FieldMeta *field, AggOp aggOp) : Field(table,field),aggOp_(aggOp)
    {}
    AggField(const AggField &) = default;

    AggOp aggOp() const{
        return aggOp_;
    }

    void set_aggOp(AggOp aggOp){
        aggOp_ = aggOp;
    }

    private:
    AggOp aggOp_;
};