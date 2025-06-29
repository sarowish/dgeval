#pragma once

#if !defined(yyFlexLexerOnce)
    #include <FlexLexer.h>
#endif
#include "driver.hpp"

class Lexer: public yyFlexLexer {
  public:
    auto yylex(Driver& driver) -> dgeval::Parser::symbol_type;
};
