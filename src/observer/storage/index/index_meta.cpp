/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai.wyl on 2021/5/18.
//

#include "storage/index/index_meta.h"
#include "storage/field/field_meta.h"
#include "storage/table/table_meta.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "json/json.h"

const static Json::StaticString FIELD_NAME("name");
// const static Json::StaticString FIELD_FIELD_NAME("field_name");
const static Json::StaticString FIELD_FIELD_NAMES("field_names");
const static Json::StaticString FIELD_FIELD_ISUNIQUE("is_unique"); 
// RC IndexMeta::init(const char *name, const FieldMeta &field)
// {
//   if (common::is_blank(name)) {
//     LOG_ERROR("Failed to init index, name is empty.");
//     return RC::INVALID_ARGUMENT;
//   }

//   name_ = name;
//   field_meta_list_.emplace_back(field);
//   return RC::SUCCESS;
// }
RC IndexMeta::init(const char *name, std::vector<const FieldMeta*> field_meta_list,bool isUnique){
    if (common::is_blank(name)) {
    LOG_ERROR("Failed to init index, name is empty.");
    return RC::INVALID_ARGUMENT;
  }

  name_ = name;
  // field_meta_list_.swap(field_meta_list);
  for (auto field_meta:field_meta_list){
    this->field_name_list_.push_back(field_meta->name());
  }
  isUnique_ = isUnique;
  return RC::SUCCESS;
}
// RC IndexMeta::init(const char *name, std::string field_name)
// {
//   if (common::is_blank(name)) {
//     LOG_ERROR("Failed to init index, name is empty.");
//     return RC::INVALID_ARGUMENT;
//   }

//   name_ = name;
//   field_name_list_.push_back(field_name);
//   return RC::SUCCESS;
// }

RC init(const char *name, std::vector<std::string> field_name_list);


void IndexMeta::to_json(Json::Value &json_value) const
{
  json_value[FIELD_NAME] = name_;
  json_value[FIELD_FIELD_ISUNIQUE] = isUnique_;
  // json_value[FIELD_FIELD_NAME] = field_;
  Json::Value field_names_value;
  for (auto field_meta : field_name_list_) {
    Json::Value field_name_value;
    // index.to_json(index_value);
    field_name_value =field_meta;
    field_names_value.append(field_name_value);
  }
  json_value[FIELD_FIELD_NAMES] = field_names_value;
}

RC IndexMeta::from_json(const TableMeta &table, const Json::Value &json_value, IndexMeta &index)
{
  const Json::Value &name_value = json_value[FIELD_NAME];
  Json::Value index_name;
  const Json::Value &isUnique = json_value[FIELD_FIELD_ISUNIQUE];
  const Json::Value &fields_value = json_value[FIELD_FIELD_NAMES];
  if (!name_value.isString()) {
    LOG_ERROR("Index name is not a string. json value=%s", name_value.toStyledString().c_str());
    return RC::INTERNAL;
  }
  if(!isUnique.isBool()){
    LOG_ERROR("Invalid table meta. fields is not bool, json value=%s", isUnique.toStyledString().c_str());
    return RC::INTERNAL;
  }
  if (!fields_value.isArray() || fields_value.size() <= 0) {
    LOG_ERROR("Invalid table meta. fields is not array, json value=%s", fields_value.toStyledString().c_str());
    return RC::INTERNAL;
  }
  int                    field_num = fields_value.size();
  std::vector<const FieldMeta*> fields;
  for (int i = 0; i < field_num; i++) {
    const Json::Value &field_value = fields_value[i];
    const FieldMeta *field = table.field(field_value.asCString());
    if (nullptr == field) {
    LOG_ERROR("Deserialize index [%s]: no such field: %s", name_value.asCString(), field_value.asCString());
    return RC::SCHEMA_FIELD_MISSING;
    }
    fields.push_back(field);
  }

  return index.init(name_value.asCString(), fields,isUnique.asBool());
}

const char *IndexMeta::name() const
{
  return name_.c_str();
}

const char *IndexMeta::field(int index) const
{
  if (index > field_name_list_.size()){
    return "";
  }
  return field_name_list_[index].c_str();
}

const char *IndexMeta::fields() const{
  std::string *result = new std::string(field_name_list_[0]);
  for(int i = 1; i < field_name_list_.size(); i++){
    *result+=", "+ field_name_list_[i];
  }
  return result->c_str();
}
// std::vector<const FieldMeta*> IndexMeta::field_meta_list() const{
//   return field_meta_list_;
// }

void IndexMeta::desc(std::ostream &os) const
{
  os << "index name=" << name_ ;
  for(auto field_meta : field_name_list_){
    os << ", field=" << field_meta;
  }
}