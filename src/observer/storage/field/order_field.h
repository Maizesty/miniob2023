#pragma once

#include "sql/parser/parse_defs.h"
#include "storage/field/field.h"
#include "storage/table/table.h"
#include "storage/field/field_meta.h"

class OrderFiled: public Field{
    public:
    OrderFiled() = default;
    OrderFiled(const Table *table, const FieldMeta *field,OrderByType order) : Field(table,field), order_(order)
    {}
    OrderFiled(const OrderFiled &) = default;
    void set_order(OrderByType order){
      this->order_ = order;
    }

    OrderByType order(){
      return order_;
    }



    private:
    OrderByType order_;
};