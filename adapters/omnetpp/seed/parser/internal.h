#ifndef SEED_PARSER_INTERNAL_H
#define SEED_PARSER_INTERNAL_H

#include <string>
#include <unordered_map>
#include <vector>

typedef std::unordered_map<std::string, std::string> Attr;
typedef std::unordered_map<std::string, Attr> AttrMap;

enum Term
{
  VARIABLE,
  FUNCTION
};

bool parseNumber(std::string number, double &res);
bool parseNumber(std::string number, int& res);
std::pair<std::string, std::string> splitNumber(std::string str);
bool parseTime(std::string time, double &res);
bool parseSize(std::string size, double &res);
bool parseBandwidth(std::string bandwidth, double &res);
bool parseUnitNumber(std::string unitNumber, double& number, std::string& unit);
bool parseTerm
(
  const std::string &term,
  Term &result,
  std::string &name,
  std::vector<std::string> &args
);

#endif
