#ifndef SEED_PARSER_PARSER_H
#define SEED_PARSER_PARSER_H

#include "context.h"
#include <string>

class Parser
{
  public:
    static bool parse
    (
      const std::string &bundle_path,
      Context &context,
      cModule *parent
    );
};

#endif
