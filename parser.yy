%language "c++"
%require "3.2"
%header

%define api.namespace { dgeval }
%define api.parser.class { Parser }
%define api.value.type variant
%define api.token.constructor

%code requires {
    #include <print>
    #include "context.hpp"

    class Driver;
    class Lexer;
}

%param { Driver& driver }
%parse-param { Lexer& lexer }

%locations
%define api.location.file none
%define parse.assert

%code {
    #include "scanner.hpp"
    #define yylex lexer.yylex
}

%token
    LEFT_PAREN       "("
    RIGHT_PAREN      ")"
    LEFT_BRACKET     "["
    RIGHT_BRACKET    "]"
    COMMA            ","
    ASSIGN           "="
    QUESTION_MARK    "?"
    COLON            ":"
    SEMICOLON        ";"
    AND              "&&"
    OR               "||"
    EQUAL            "=="
    NOT_EQUAL        "!="
    LESS             "<"
    GREATER          ">"
    LESS_EQUAL       "<="
    GREATER_EQUAL    ">="
    PLUS             "+"
    MINUS            "-"
    STAR             "*"
    SLASH            "/"
    NOT              "!"
    TRUE             "true"
    FALSE            "false"
    WAIT             "wait"
    THEN             "then"

%token <std::string> IDENTIFIER
%token <std::string> STRING
%token <double> NUMBER

%type <std::unique_ptr<ast::Expression>> expression literal assignment function_call array_access conditional argument_list
%type <std::unique_ptr<ast::Expression>> arithmetic_expression logical_expression comparison_expression negation_expression
%type <std::unique_ptr<ast::Statement>> statement expression_statement wait_statement
%type <std::unique_ptr<ast::StatementList>> statement_list
%type <std::vector<std::string>> identifier_list

%left ","
%right "="
%nonassoc "?" ":"
%left "&&" "||"
%left "==" "!=" "<" ">" "<=" ">="
%left "+" "-"
%left "*" "/"
%nonassoc "!"
%left "(" "["

%%

program : statement_list                    { driver.program = std::make_unique<ast::Program>(std::move($1)); } ;

statement_list : statement                  { $$ = std::make_unique<ast::StatementList>(); $$->inner.push_back(std::move($1)); }
               | statement_list statement   { $1->inner.push_back(std::move($2)); $$ = std::move($1); }
               ;

statement : expression_statement    { $$ = std::move($1); } 
          | wait_statement          { $$ = std::move($1); }
          ;

expression_statement : expression ";"         { $$ = std::make_unique<ast::ExpressionStatement>(@1.begin.line, std::move($1)); } ;

wait_statement : "wait" identifier_list "then" expression ";"          { $$ = std::make_unique<ast::WaitStatement>(@1.begin.line, std::move($2), std::move($4)); } ;

identifier_list : IDENTIFIER                        { $$ = std::vector<std::string>(); $$.push_back($1); }
                | identifier_list "," IDENTIFIER    { $1.push_back($3); $$ = std::move($1); }
                ;

expression : literal                    { $$ = std::move($1); }
           | assignment                 { $$ = std::move($1); }
           | function_call              { $$ = std::move($1); }
           | array_access               { $$ = std::move($1); }
           | conditional                { $$ = std::move($1); }
           | arithmetic_expression      { $$ = std::move($1); }
           | logical_expression         { $$ = std::move($1); }
           | comparison_expression      { $$ = std::move($1); }
           | negation_expression        { $$ = std::move($1); }
           | "(" expression ")"         { $$ = std::move($2); }
           | expression "," expression  { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Comma); }
           ;

literal : STRING                        { $$ = std::make_unique<ast::StringLiteral>(@1.begin.line, std::move($1)); }
        | NUMBER                        { $$ = std::make_unique<ast::NumberLiteral>(@1.begin.line, std::move($1)); }
        | IDENTIFIER                    { $$ = std::make_unique<ast::Identifier>(@1.begin.line, std::move($1)); }
        | "true"                        { $$ = std::make_unique<ast::BooleanLiteral>(@1.begin.line, true); }
        | "false"                       { $$ = std::make_unique<ast::BooleanLiteral>(@1.begin.line, false); }
        | "[" argument_list "]"         { $$ = std::make_unique<ast::ArrayLiteral>(@1.begin.line, std::move($2)); }
        ;

arithmetic_expression : expression "+" expression       { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Plus); }
                      | expression "-" expression       { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Minus); }
                      | expression "*" expression       { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Star); }
                      | expression "/" expression       { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Slash); }
                      ;                                                                                 
                                                                                                       
logical_expression : expression "&&" expression         { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::And); }
                   | expression "||" expression         { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Or); }
                   ;

comparison_expression : expression "==" expression      { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Equal); }
                      | expression "!=" expression      { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::NotEqual); }
                      | expression "<" expression       { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Less); }
                      | expression ">" expression       { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Greater); }
                      | expression "<=" expression      { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::LessEqual); }
                      | expression ">=" expression      { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::GreaterEqual); }
                      ;

negation_expression : "!" expression                    { $$ = std::make_unique<ast::UnaryExpression>(@1.begin.line, std::move($2), ast::Opcode::Not); }
                    | "-" expression                    { $$ = std::make_unique<ast::UnaryExpression>(@1.begin.line, std::move($2), ast::Opcode::Negate); }
                    ;

assignment : expression "=" expression                  { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Assign); } ;

conditional : expression "?" expression ":" expression  {
                                                          $$ = std::make_unique<ast::BinaryExpression>(
                                                              @2.begin.line,
                                                              std::move($1),
                                                              std::make_unique<ast::BinaryExpression>(
                                                                  @4.begin.line,
                                                                  std::move($3),
                                                                  std::move($5),
                                                                  ast::Opcode::Alt
                                                              ),
                                                              ast::Opcode::Conditional
                                                          );
                                                        } ;
 
array_access : expression "[" expression "]"            { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::ArrayAccess); } ;

function_call : expression "(" argument_list ")"        { $$ = std::make_unique<ast::BinaryExpression>(@2.begin.line, std::move($1), std::move($3), ast::Opcode::Call); } ;

argument_list : %empty
              | expression      { $$ = std::move($1); }
              ;


%%

void dgeval::Parser::error(const location_type& loc, const std::string& m) {
    std::println(driver.output, "Line {}: Syntax error.", loc.begin.line);
}
