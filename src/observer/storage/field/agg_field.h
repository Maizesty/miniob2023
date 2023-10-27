#pragma once

#include "storage/field/field.h"
#include "storage/table/table.h"
#include "storage/field/field_meta.h"

class AggField: public Field{
    public:
    AggField() = default;
    AggField(const Table *table, const FieldMeta *field, AggOp aggOp, bool is_star) : Field(table,field), aggOp_(aggOp), is_star_(is_star)
    {}
    AggField(const AggField &) = default;

    AggOp aggOp() const{
        return aggOp_;
    }
    bool is_star() const{
        return is_star_;
    }

    void set_aggOp(AggOp aggOp){
        aggOp_ = aggOp;
    }
    void set_is_star(bool is_star){
        is_star_ = is_star;
    }


    private:
    AggOp aggOp_;
    bool is_star_;
};