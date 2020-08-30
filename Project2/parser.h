#ifndef __PARSER__H__
#define __PARSER__H__

#include <stack>
#include "lexer.h"

struct scopeItem //For singular item in the table
{
    std::string name;
    std::string scope;
    bool ispublic;
};

struct scopeTable //For a full scope table
{
    scopeItem item;
    scopeTable* next = NULL;
    scopeTable* prev = NULL;
};

class Parser 
{
    public:
        void parse_program();
        struct scopeTable *table;

    private:
        LexicalAnalyzer lexer;
		
		std::stack<std::string> scopes;
        std::string currentScope;

        void parse_global_vars();
        void parse_var_list(bool);
        void parse_scope();
        void parse_public_vars();
        void parse_private_vars();
		void parse_statement_list();
        void parse_statement();
		std::string find_scope(std::string);
		void print_parse_statement(std::string, std::string);
};

#endif