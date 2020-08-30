#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include <cctype>
#include <stdlib.h>

#include "lexer.h"
#include "inputbuf.h"
#include "parser.h"

//Forward Declarations
void syntax_error();

using namespace std;

//Parses the program
void Parser::parse_program()
{
	currentScope = "::"; //Prepare scopes
    scopes.push(currentScope);
    Token t1 = lexer.GetToken();
    if (t1.token_type == ID) //Check for Scope/var_list ID
	{
        Token t2 = lexer.GetToken();
		if (t2.token_type == COMMA || t2.token_type == SEMICOLON) //Check for var_list
		{
            lexer.UngetToken(t2);
            lexer.UngetToken(t1);
            parse_global_vars(); //Parse global_vars
            parse_scope(); //Parse scope
        }
		else if (t2.token_type == LBRACE) //Scope Only, global_vars is epsilon
		{
            lexer.UngetToken(t2);
            lexer.UngetToken(t1);
            parse_scope(); //Parse scope
        }
		else
            syntax_error();
    }
	else
        syntax_error();
}

//Parse the global_vars
void Parser::parse_global_vars() {
    Token t1 = lexer.GetToken();
    if (t1.token_type == ID)
	{
        lexer.UngetToken(t1);
        parse_var_list(true); //Parse the var_list
		t1 = lexer.GetToken();
		if (t1.token_type != SEMICOLON) //Check for semicolon
			syntax_error();
    }
	else //Epsilon, Move to Scope
        lexer.UngetToken(t1);
}

//Parse the var_list
void Parser::parse_var_list(bool ispublic)
{
    Token token = lexer.GetToken();
    if (token.token_type == ID)
	{
        Token token2 = lexer.GetToken();
        if (token2.token_type == COMMA)
		{
            parse_var_list(ispublic);
        }
		else
            lexer.UngetToken(token2);

        //Create new item for the current scope
        struct scopeItem newItem; 
        newItem.scope = currentScope;
        newItem.name = token.lexeme;
		newItem.ispublic = ispublic;
        
		//Store item in its scope
        struct scopeTable *newTable = new struct scopeTable;
        newTable->item = newItem;
        newTable->next = table;
        table = newTable;
    }
	else
        syntax_error(); 
}

//Parse the Scope
void Parser::parse_scope()
{
    Token t1 = lexer.GetToken();
	Token t2 = lexer.GetToken();
    if (t1.token_type == ID && t2.token_type == LBRACE) //Check ID and LBRACE
	{
        scopes.push(currentScope); //Store the current scope
        currentScope = t1.lexeme; //Get next scope
		
        parse_public_vars(); //Parse public_vars
        parse_private_vars(); //Parse private_vars
        parse_statement_list(); //Parse statement_list
		
        currentScope = scopes.top(); //Retrieve previous scope
        scopes.pop();
		
        t1 = lexer.GetToken();
        if (t1.token_type != RBRACE) //Check for RBRACE
            syntax_error();
    }
	else
        syntax_error();
}

//Check and parse for public vars
void Parser::parse_public_vars()
{
    Token t1 = lexer.GetToken();
    if (t1.token_type == PUBLIC) //Look for Public token
	{
        t1 = lexer.GetToken();
        if (t1.token_type == COLON) //Look for colon
		{
            parse_var_list(true); //Parse var_list
            t1 = lexer.GetToken();
            if (t1.token_type != SEMICOLON) //Check for semicolon
				syntax_error();
        }
		else
            syntax_error();
    }
	else //Epsilon, Move to Private
        lexer.UngetToken(t1);
}

//Check and parse for private vars
void Parser::parse_private_vars()
{
    Token t1 = lexer.GetToken();
    if (t1.token_type == PRIVATE) //Look for Private token
	{
        t1 = lexer.GetToken();
        if (t1.token_type == COLON) //Look for colon
		{
            cout << t1.lexeme; //Print colon token
			parse_var_list(false); //Parse var_list
            t1 = lexer.GetToken();
            if (t1.token_type != SEMICOLON) //Check for semicolon
                syntax_error();
        }
		else
            syntax_error();
    }
	else //Epsilon, Move to statement list
        lexer.UngetToken(t1);
}

//Parse the statement list
void Parser::parse_statement_list()
{
    Token t1 = lexer.GetToken();
    if (t1.token_type == ID) //Check for ID in statement
	{
        Token t2 = lexer.GetToken();
        if (t2.token_type == EQUAL) //Check for EQUAL in statement
		{
            lexer.UngetToken(t2);
            lexer.UngetToken(t1);
            parse_statement(); //Parse statement
        }
		else if (t2.token_type == LBRACE) //Check for LBRACE in scope
		{
            lexer.UngetToken(t2);
            lexer.UngetToken(t1);
            parse_scope(); //Parse scope
        }
		else
            syntax_error();
		
        Token t3 = lexer.GetToken(); 
        if (t3.token_type == ID) //Check for ID-EQUAL-ID pattern
		{
            lexer.UngetToken(t3);
            parse_statement_list(); //Parse statement list recursively
        }
		else //No more right expansion
            lexer.UngetToken(t3);
    }
	else
        syntax_error();
}

//Parses the statements
void Parser::parse_statement()
{
    Token t1 = lexer.GetToken();
    if (t1.token_type == ID) //Check for ID
	{
        Token t2 = lexer.GetToken();
        if (t2.token_type == EQUAL) //Check for EQUAL
		{
            t2 = lexer.GetToken();
            Token t3 = lexer.GetToken();
            if (t2.token_type == ID && t3.token_type == SEMICOLON) //Check for ID-EQUAL-ID-SEMICOLON
			{
                print_parse_statement(t1.lexeme, t2.lexeme); //Print
            }
			else
                syntax_error();
        }
		else //Parse scope
		{
            lexer.UngetToken(t2);
            lexer.UngetToken(t1);
            parse_scope(); //Parse the scope
        }
    }
	else
        syntax_error();
}

//Handles printing of statements
void Parser::print_parse_statement(string t1, string t2)
{
	cout << find_scope(t1) << t1 << " = " << find_scope(t2) << t2 << endl;
}

//Finds the scope current token belongs to
string Parser::find_scope(string id)
{
    struct scopeTable currItem = *table; //Iterator
    
	while (!((currItem.item.name.empty() || currItem.next == NULL) ||(currItem.item.name == id && (currItem.item.scope == currentScope || currItem.item.ispublic))))
	{
        currItem = *currItem.next;
    }
	
    if (!currItem.item.name.empty() && currItem.item.scope == "::") //Global Variable
        return "::";
	else if (!currItem.item.name.empty() && currItem.item.name == id ) //Same Scope
        return currItem.item.scope + ".";
	else //Not resolved
        return "?.";
}

//Handles syntax errors
void syntax_error()
{
    cout << "Syntax Error" << endl;
    exit(1);
}

//Code Driver
int main()
{
    Parser parser; //Create parser object
    parser.table = new struct scopeTable(); //Create scope table
    parser.parse_program(); //Run the parser
    return 0;
}