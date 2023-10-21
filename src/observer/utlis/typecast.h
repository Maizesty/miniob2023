#include <string>
#include <regex>
#include <sstream>
#include <cmath>
float stringToNumber(std::string str){
    std::regex integerPattern("^-?\\d+$");
    std::regex floatPattern("^-?\\d+(\\.\\d+)?$");
    if(std::regex_match(str, integerPattern) || std::regex_match(str, floatPattern)){
      return std::stof(str); 
    }
    float result = 0.0;
    std::istringstream iss(str);
    iss >> result;
    return result;
}
int round(float f){
  return  static_cast<int>(std::round(f));
}