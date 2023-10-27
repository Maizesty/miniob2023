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
// Created by WangYunlai on 2022/07/01.
//

#include "common/log/log.h"
#include "common/rc.h"
#include "sql/expr/tuple.h"
#include "sql/operator/physical_operator.h"
#include "sql/operator/sort_physical_operator.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include <cstddef>
#include <fstream>
#include <string>

RC SortPhysicalOperator::open(Trx *trx)
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

  return RC::SUCCESS;
}

RC SortPhysicalOperator::next()
{
  RC rc = RC::SUCCESS;
  PhysicalOperator *oper = children_.front().get();

  bool hasOutput = false;
  //设置外排的阈值(桶的大小)
  size_t backet_size = 2;
  //桶的数量
  size_t backet_num = 0;
  //临时文件名前缀
  std::string filename_base = "sort_file";
  //先后顺序比较函数
  auto sortLambda = [this](ProjectTuple * p1, ProjectTuple * p2) {
      return  this->SortCompare_.comparePairs(p1, p2);
  };
  while (RC::SUCCESS == (rc = oper->next())) {
    Tuple *tuple = oper->current_tuple();
    if (nullptr == tuple) {
      rc = RC::INTERNAL;
      LOG_WARN("failed to get tuple from operator");
      break;
    }
    ProjectTuple* project_tuple =new ProjectTuple(*static_cast<ProjectTuple *>(tuple));
    tuples_.push_back(project_tuple);
    //数据量达到阈值
    if(tuples_.size() == backet_size){
      //桶内排序
      std::sort(tuples_.begin(), tuples_.end(),sortLambda);
      //写入元组到磁盘文件
      std::ofstream outputFile(filename_base + std::to_string(backet_num), std::ios::binary);
      if (outputFile) {
        size_t size = tuples_.size();
        outputFile.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
        for(size_t i = 0; i < size; i++){
          outputFile.write(reinterpret_cast<const char*>(tuples_[i]), sizeof(ProjectTuple));
          //释放内存
          // delete tuples_[i];
        }
        outputFile.close();
        LOG_WARN("向文件写入完成");
      } else {
        LOG_WARN("无法打开文件");
      }
      tuples_.clear();
      backet_num++;
    }
  }
  //将余下的记录写入磁盘文件(tuples_中可能还有记录)
  if(!tuples_.empty()){
    std::ofstream outputFile(filename_base + std::to_string(backet_num), std::ios::binary);
    size_t size = tuples_.size();
    outputFile.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
    for(size_t i = 0; i < tuples_.size(); i++){
      outputFile.write(reinterpret_cast<const char*>(tuples_[i]), sizeof(ProjectTuple));
      //释放内存
      // delete tuples_[i];
    }
    backet_num++;
    outputFile.close();
    tuples_.clear();
  }
  //2路归并排序
  std::string filename_base_new = "sort_file_new";
  size_t iter_num = 0;
  while(backet_num > 1){
    size_t merge_num = backet_num / 2;
    std::string current_filename_base = (iter_num & 1 ? filename_base_new : filename_base);
    std::string next_filename_base = (iter_num & 1 ? filename_base : filename_base_new);
    for(size_t i = 0; i < merge_num; i ++){
      std::string filename1 = current_filename_base + std::to_string(2 * i);
      std::string filename2 = current_filename_base + std::to_string(2 *i + 1);
      std::string filename_out = next_filename_base + std::to_string(i);
      std::ifstream inputFile1(filename1, std::ios::binary);
      std::ifstream inputFile2(filename2, std::ios::binary);
      size_t size1, size2;
      inputFile1.read(reinterpret_cast<char*>(&size1), sizeof(size_t));
      inputFile2.read(reinterpret_cast<char*>(&size2), sizeof(size_t));
      std::ofstream outputFile(filename_out, std::ios::binary);
      size_t output_size = size1 + size2;
      outputFile.write(reinterpret_cast<char*>(&output_size), sizeof(size_t));
      ProjectTuple* t1 = new ProjectTuple;
      ProjectTuple* t2 = new ProjectTuple;
      inputFile1.read(reinterpret_cast<char*>(t1), sizeof(ProjectTuple));
      inputFile2.read(reinterpret_cast<char*>(t2), sizeof(ProjectTuple));
      int c1 = 1, c2 = 1;
      while(!(c1 > size1 && c2 > size2)){
        if(SortCompare_.comparePairs(t1, t2)){
          if(c1 <= size1){
            outputFile.write(reinterpret_cast<char*>(t1), sizeof(ProjectTuple));
            inputFile1.read(reinterpret_cast<char*>(t1), sizeof(ProjectTuple));
            c1++;
          }else{
            while(c2 <= size2){
              outputFile.write(reinterpret_cast<char*>(t2), sizeof(ProjectTuple));
              inputFile2.read(reinterpret_cast<char*>(t2), sizeof(ProjectTuple));
              c2++;
            }
            // size_t pos1 = inputFile2.tellg();
            // inputFile2.seekg(0, std::ios::end);
            // size_t pos2 = inputFile2.tellg();
            // size_t rest_length = pos2 - pos1;
            // inputFile2.seekg(rest_length, std::ios::end);
            // char* buf = new char[rest_length];
            // inputFile2.read(buf, rest_length);
            // outputFile.write(buf, rest_length);
            // outputFile.flush();
            // delete[] buf;
            break;
          }
        }else{
          if(c2 <= size2){
            outputFile.write(reinterpret_cast<char*>(t2), sizeof(ProjectTuple));
            inputFile2.read(reinterpret_cast<char*>(t2), sizeof(ProjectTuple));
            c2++;
          }else{
            while(c1 <= size1){
              outputFile.write(reinterpret_cast<char*>(t1), sizeof(ProjectTuple));
              inputFile1.read(reinterpret_cast<char*>(t1), sizeof(ProjectTuple));
              c1++;
            }
            // size_t pos1 = inputFile1.tellg();
            // inputFile1.seekg(0, std::ios::end);
            // size_t pos2 = inputFile1.tellg();
            // size_t rest_length = pos2 - pos1;
            // inputFile1.seekg(rest_length, std::ios::end);
            // char* buf = new char[rest_length];
            // inputFile1.read(buf, rest_length);
            // outputFile.write(buf, rest_length);
            // outputFile.flush();
            // delete[] buf;
            break;
          }
        }
      }
      inputFile1.close();
      inputFile2.close();
      outputFile.close();
    }
    if(backet_num % 2 != 0){
      //重命名文件
      std::rename((current_filename_base + std::to_string(backet_num - 1)).c_str(),
        (next_filename_base + std::to_string(merge_num)).c_str());
      backet_num--;
      merge_num++;
    }
    //删除前一次归并的文件
    for(size_t i = 0; i < backet_num; i++){
      std::remove((current_filename_base + std::to_string(i)).c_str());
    }
    backet_num = merge_num;
    iter_num++;
  }
  std::string current_filename_base = (iter_num & 1 ? filename_base_new : filename_base);
  MergedFileInputStream_ = std::ifstream(current_filename_base + std::to_string(0), std::ios::binary);
  MergedFileInputStream_.read(reinterpret_cast<char*>(&record_num_), sizeof(size_t));
  if(count == record_num_)
    return  RC::RECORD_EOF;
  return RC::SUCCESS;
}

RC SortPhysicalOperator::close()
{
  if(MergedFileInputStream_.is_open()){
    MergedFileInputStream_.close();
  }
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
Tuple *SortPhysicalOperator::current_tuple()
{
  ProjectTuple* cur_tuple = new ProjectTuple;
  MergedFileInputStream_.seekg(sizeof(size_t) + count * sizeof(ProjectTuple));
  MergedFileInputStream_.read(reinterpret_cast<char*>(cur_tuple), sizeof(ProjectTuple));
  count++;
  return cur_tuple;
}


