#ifndef SEED_PARSER_DISTRIBUTION_H
#define SEED_PARSER_DISTRIBUTION_H

#include <string>
#include <functional>

bool parseDistribtion(
    std::string expr,
    std::string unit,
    std::function<double(double)>& res
);

#endif
