#include "parser/internal.h"

#include <sstream>

bool parseNumber(std::string number, int& res)
{
  if (!(std::istringstream(number) >> res))
    return false;
  return true;
}

bool parseNumber(std::string number, double& res)
{
  if (!(std::istringstream(number) >> res))
    return false;
  return true;
}

std::pair<std::string, std::string> splitNumber(std::string str)
{
  auto it = str.begin();
  for (; it != str.end() && (('0' <= *it && *it <= '9') || *it == '.'); ++it);
  return std::make_pair(std::string(str.begin(), it),
                        std::string(it, str.end()));
}

bool parseTime(std::string time, double &res)
{
  auto pair = splitNumber(time);
  if (!(std::istringstream(pair.first) >> res))
    return false;

  if (pair.second == "ns")
    res *= 1e-9;
  else if (pair.second == "us")
    res *= 1e-6;
  else if (pair.second == "ms")
    res *= 1e-3;
  else if (pair.second == "cs")
    res *= 1e-2;
  else if (pair.second == "ds")
    res *= 1e-1;
  else if (pair.second == "s")
    res *= 1e-0;
  else if (pair.second == "m")
    res *= 60e-0;
  else if (pair.second == "h")
    res *= 3600e-0;
  else return false;

  return true;
}

bool parseSize(std::string size, double &res)
{
  auto pair = splitNumber(size);
  if (!(std::istringstream(pair.first) >> res))
    return false;

  if (pair.second == "Tbit")
    res *= 1e12;
  else if (pair.second == "Gbit")
    res *= 1e9;
  else if (pair.second == "Mbit")
    res *= 1e6;
  else if (pair.second == "Kbit")
    res *= 1e3;
  else if (pair.second == "bit")
    res *= 1e-0;
  else if (pair.second == "Tbyte")
    res *= 8e12;
  else if (pair.second == "Gbyte")
    res *= 8e9;
  else if (pair.second == "Mbyte")
    res *= 8e6;
  else if (pair.second == "Kbyte")
    res *= 8e3;
  else if (pair.second == "byte")
    res *= 8e-0;
  else return false;

  return true;
}

bool parseBandwidth(std::string bandwidth, double &res)
{
  auto pair = splitNumber(bandwidth);
  if (!(std::istringstream(pair.first) >> res))
    return false;

  if (pair.second == "Tbps")
    res *= 1e12;
  else if (pair.second == "Gbps")
    res *= 1e9;
  else if (pair.second == "Mbps")
    res *= 1e6;
  else if (pair.second == "Kbps")
    res *= 1e3;
  else if (pair.second == "bps")
    res *= 1e-0;
  else return false;

  return true;
}

bool parseUnitNumber(std::string unitNumber, double& number, std::string& unit)
{
    if (parseTime(unitNumber, number)) unit = "s";
    else if (parseSize(unitNumber, number)) unit = "bit";
    else if (parseBandwidth(unitNumber, number)) unit = "bps";
    else if (parseNumber(unitNumber, number)) unit = "";
    else return false;
    return true;
}


bool parseTerm
(
  const std::string &term,
  Term &result,
  std::string &name,
  std::vector<std::string> &args
)
{
  std::size_t i = 0;
  for (; i < term.length() && term[i] != '('; ++i);
  name = term.substr(0, i);
  if (i == term.length())
  {
    result = VARIABLE;
    return true;
  }

  if (term[term.length() - 1] != ')') return false;
  std::string subterm = term.substr(i + 1, term.length() - i - 2);

  args.clear();
  std::size_t start = 0, end = 0;
  while ((end = subterm.find(',', start)) != std::string::npos) {
    args.push_back(subterm.substr(start, end - start));
    start = end + 1;
  }
  args.push_back(subterm.substr(start));

  result = FUNCTION;
  return true;
}
