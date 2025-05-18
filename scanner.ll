%option noyywrap
%option yylineno
%option c++
%option yyclass="Lexer"

%{
#include "scanner.hpp"
#include "parser.hpp"

using namespace dgeval;

#undef YY_DECL
#define YY_DECL auto Lexer::yylex(Driver& driver) -> Parser::symbol_type
%}

int                 [1-9][0-9]*|0
fractional          [0-9]*[1-9]+
significand         {int}(\.{fractional})?|\.{fractional}
exponent            [eE][+-]?[1-9][0-9]*
number              {significand}{exponent}?

hex_escape          \\x[[:xdigit:]]{1,2}
whitespace          [ \t\r]
identifier          [a-zA-Z_][a-zA-Z0-9_]*

%{
#define YY_USER_ACTION loc.columns(yyleng);
%}

%x STRING

%%

%{
location& loc = driver.location;
loc.step();
%}

{whitespace}+    loc.step();
\n+              loc.lines(yyleng); loc.step();

"("              return Parser::make_LEFT_PAREN(loc);
")"              return Parser::make_RIGHT_PAREN(loc);
"["              return Parser::make_LEFT_BRACKET(loc);
"]"              return Parser::make_RIGHT_BRACKET(loc);
","              return Parser::make_COMMA(loc);
"="              return Parser::make_ASSIGN(loc);
"?"              return Parser::make_QUESTION_MARK(loc);
":"              return Parser::make_COLON(loc);
"&&"             return Parser::make_AND(loc);
"||"             return Parser::make_OR(loc);
"=="             return Parser::make_EQUAL(loc);
"!="             return Parser::make_NOT_EQUAL(loc);
"<"              return Parser::make_LESS(loc);
">"              return Parser::make_GREATER(loc);
"<="             return Parser::make_LESS_EQUAL(loc);
">="             return Parser::make_GREATER_EQUAL(loc);
"+"              return Parser::make_PLUS(loc);
"-"              return Parser::make_MINUS(loc);
"*"              return Parser::make_STAR(loc);
"/"              return Parser::make_SLASH(loc);
"!"              return Parser::make_NOT(loc);
";"              return Parser::make_SEMICOLON(loc);
"true"           return Parser::make_TRUE(loc);
"false"          return Parser::make_FALSE(loc);
"wait"           return Parser::make_WAIT(loc);
"then"           return Parser::make_THEN(loc);

{number}        { double number = atof(yytext); return Parser::make_NUMBER(number, loc); }
{identifier}    return Parser::make_IDENTIFIER(yytext, loc);

\"              { yy_push_state(STRING); driver.buffer.clear(); }

<STRING>{
  \"            { yy_pop_state(); return Parser::make_STRING(driver.buffer, loc); }
  [^\"\\\n]+    driver.buffer += yytext;
  \\n           driver.buffer += '\n';
  \\\\          driver.buffer += '\\';
  \\\"          driver.buffer += '\"';
  {hex_escape}  driver.buffer += strtol(yytext + 2, NULL, 16);
  \n            throw Parser::syntax_error(loc, "");
  <<EOF>>       throw Parser::syntax_error(loc, "");
  .             throw Parser::syntax_error(loc, "");
}

.               throw Parser::syntax_error(loc, "");
<<EOF>>         return Parser::make_YYEOF(loc);
%%
