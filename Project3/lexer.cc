#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include <cctype>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "lexer.h"
#include "inputbuf.h"

using namespace std;

//Forward Declarations
void parse_program();
void print_list();
void parse_var_list();
void parse_unary_operator();
void update_type(int, int);
void parse_assignment_stmt();
void parse_case();
void parse_case_list();
void parse_switch_stmt();
void parse_while_stmt();
void parse_if_stmt();
void parse_stmt();
void parse_stmt_list();
void parse_body();
void parse_type_name();
void parse_var_decl();
void parse_var_decl_list();
void parse_global_vars();
		
bool is_binary_operator(int);
bool is_expression(int);
		
int parse_primary();
int parse_expression();
int parse_binary_operator();
		
//Mismatch Handling
void type_mismatch(int, int);
void syntax_error();
		
//List Functions
void add_to_list(std::string, int);
int search_table(std::string);

//For each individual variable
struct scopeTableItem
{
    std::string name;
    int line_no;
    int type;
    bool printed;
};

struct scopeTable
{
    scopeTableItem* item;
    scopeTable *next;
    scopeTable *prev;
};

scopeTable* symbolTable;

string reserved[] = { "END_OF_FILE", "INT", "REAL", "BOOL", "TR", "FA", "IF", "WHILE", "SWITCH", "CASE", "PUBLIC", 
					  "PRIVATE", "NUM", "REALNUM", "NOT", "PLUS", "MINUS", "MULT", "DIV", "GTEQ", "GREATER", "LTEQ", 
					  "NOTEQUAL", "LESS", "LPAREN", "RPAREN", "EQUAL", "COLON", "COMMA", "SEMICOLON", "LBRACE", "RBRACE", 
					  "ID", "ERROR"
};

//0 = EOF, 1 = INT, 2 = REAL, 3 = BOOL, 4 = TR, 5 = FA, 6 = IF, 7 = WHILE, 8 = SWITCH, 9 = CASE, 10 = PUBLIC
//11 = PRIVATE, 12 = NUM, 13 = REALNUM, 14 = NOT, 15 = PLUS, 16 = MINUS, 17 = MULT, 18 = DIV, 19 = GTEQ, 20 = GREATER, 21 = LTEQ
//22 = NOTEQUAL, 23 = LESS, 24 = LPAREN, 25 = RPAREN, 26 = EQUAL, 27 = COLON, 28 = COMMA, 29 = SEMICOLON, 30 = LBRACE, 31 = RBRACE
//32 = ID, 33 = ERROR

#define KEYWORDS_COUNT 11
string keyword[] = {"int", "real", "bool", "true", "false", "if", "while", "switch", "case", "public", "private"};

//*************************************
//START LEXER

LexicalAnalyzer lexer;
Token token;
int enumCount = 4;

//Formatted Print
void Token::Print()
{
    cout << "{" << this->lexeme << " , "
        << reserved[(int) this->token_type] << " , "
        << this->line_no << "}\n";
}

//Constructor
LexicalAnalyzer::LexicalAnalyzer()
{
    this->line_no = 1;
    tmp.lexeme = "";
    tmp.line_no = 1;
    tmp.token_type = ERROR;
}

//Skips spaces in the program
void LexicalAnalyzer::SkipSpace()
{
    char c;

    input.GetChar(c);
    line_no += (c == '\n');

    while (!input.EndOfInput() && isspace(c))
    {
        input.GetChar(c);
        line_no += (c == '\n');
    }

    if (!input.EndOfInput())
        input.UngetChar(c);
}

//Skips comments in the program
void LexicalAnalyzer::SkipComments()
{
    char c;
	
    if(input.EndOfInput()) //Check for EOF
        input.UngetChar(c);
	else
	{
		input.GetChar(c);
		if(c == '/') //Check for first /
		{
			input.GetChar(c);
			if(c == '/') //Check for second /
			{
				while(c != '\n') //Go through comment
				{
					input.GetChar(c);
				}
				line_no++; //Increment line
				SkipComments(); //Repeat
			}
			else //Else an error
				syntax_error();
		}
		else //Else not a comment
			input.UngetChar(c);
	}
}

//Checks if an item is a keyword
bool LexicalAnalyzer::IsKeyword(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++)
    {
        if (s == keyword[i])
            return true;
    }
    return false;
}

//Finds the location of a keyword
TokenType LexicalAnalyzer::FindKeywordIndex(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++)
    {
        if (s == keyword[i])
            return (TokenType) (i + 1);
    }
    return ERROR;
}

//Checks the type of number
Token LexicalAnalyzer::ScanNumber()
{
    char c;
    bool isREALNUM = false;
	
    input.GetChar(c);
    if (c == '0') //Check for only REALNUM
    {
        tmp.lexeme = "0";
        input.GetChar(c);
        if(c == '.') //Check for DOT
        {
            input.GetChar(c);
            if(isdigit(c)) //Is REALNUM
            {
                while (!input.EndOfInput() && isdigit(c)) //Get REALNUM
                {
                    tmp.lexeme += c;
                    isREALNUM = true;
                    input.GetChar(c);
                }
                input.UngetChar(c);
			}
            else //Else not REALNUM
                input.UngetChar(c);
        }
        else //Else not a REALNUM
            input.UngetChar(c);
    }
    else //Check for NUM & REALNUM
    {
        tmp.lexeme = "";
        while (!input.EndOfInput() && isdigit(c)) //Get NUM
        {
            tmp.lexeme += c;
            input.GetChar(c);
        }
		
        if(c == '.') //Check for DOT
        {               
            input.GetChar(c);
            if(isdigit(c)) //Is REALNUM
            {
                while (!input.EndOfInput() && isdigit(c)) //Get REALNUM
                {
                    tmp.lexeme += c;
                    isREALNUM = true;
                    input.GetChar(c);
                }
			}
            else //Else not REALNUM
				input.UngetChar(c);
        }
		
        if (!input.EndOfInput()) //Check for EOF
            input.UngetChar(c);
    }
	
    if(isREALNUM) //Is REALNUM
        tmp.token_type = REALNUM;
    else //Is NUM
        tmp.token_type = NUM;
	
    tmp.line_no = line_no;
    return tmp;
}

//Scans for a Keyword or ID
Token LexicalAnalyzer::ScanIdOrKeyword()
{
    char c;
    input.GetChar(c);

    if (isalpha(c))
    {
        tmp.lexeme = "";
        while (!input.EndOfInput() && isalnum(c))
        {
            tmp.lexeme += c;
            input.GetChar(c);
        }
        
		if (!input.EndOfInput())
            input.UngetChar(c);
        tmp.line_no = line_no;

        if (IsKeyword(tmp.lexeme))
             tmp.token_type = FindKeywordIndex(tmp.lexeme);
        else
            tmp.token_type = ID;
    }
    else
    {
        if (!input.EndOfInput())
            input.UngetChar(c);
        tmp.lexeme = "";
        tmp.token_type = ERROR;
    }
    return tmp;
}

//Puts a token back
TokenType LexicalAnalyzer::UngetToken(Token tok)
{
    tokens.push_back(tok);;
    return tok.token_type;
}

//Gets Token from the input
Token LexicalAnalyzer::GetToken()
{
    char c;
    if (!tokens.empty())
    {
        tmp = tokens.back();
        tokens.pop_back();
        return tmp;
    }

    SkipSpace(); //Skip spacing
    SkipComments(); //Skip comments
    SkipSpace(); //Skip spacing after comments
    tmp.lexeme = "";
    tmp.line_no = line_no;
    input.GetChar(c);
    switch (c)
    {
        case '!':
            tmp.token_type = NOT;
            return tmp;
        case '+':
            tmp.token_type = PLUS;
            return tmp;
        case '-':
            tmp.token_type = MINUS;
            return tmp;
        case '*':
            tmp.token_type = MULT;
            return tmp;
        case '/':
            tmp.token_type = DIV;
            return tmp;
        case '(':
            tmp.token_type = LPAREN;
            return tmp;
        case ')':
            tmp.token_type = RPAREN;
            return tmp;
        case '=':
            tmp.token_type = EQUAL;
            return tmp;
        case ':':
            tmp.token_type = COLON;
            return tmp;
        case ',':
            tmp.token_type = COMMA;
            return tmp;
        case ';':
            tmp.token_type = SEMICOLON;
            return tmp;
        case '{':
            tmp.token_type = LBRACE;
            return tmp;
        case '}':
            tmp.token_type = RBRACE;
            return tmp;
        case '<':
            input.GetChar(c);
            if(c == '=') //Check for Less than Equal
                tmp.token_type = LTEQ;
            else if (c == '>') //Check for Not Equal
                tmp.token_type = NOTEQUAL;
            else //Just Less than
            {
                input.UngetChar(c);
                tmp.token_type = LESS;
            }
            return tmp;
		case '>':
            input.GetChar(c);
            if(c == '=') //Check for Greater than Equal
                tmp.token_type = GTEQ;
            else //Just Greater than
            {
                input.UngetChar(c);
                tmp.token_type = GREATER;
            }
            return tmp;
        default:
            if (isdigit(c)) //Check for Num Type
            {
                input.UngetChar(c);
                return ScanNumber();
            }
            else if (isalpha(c)) //Check for ID
            {
                input.UngetChar(c);
                return ScanIdOrKeyword();
            }
            else if (input.EndOfInput()) //EOF
                tmp.token_type = END_OF_FILE;
            else
                tmp.token_type = ERROR;
            return tmp;
    }
}

//*************************************
//END LEXER

//*************************************
//START PARSER

//Parses the list of variables
void parse_var_list()
{
    token = lexer.GetToken();
    add_to_list(token.lexeme, 0);
    if(token.token_type == ID) //Check for ID
    {
        token = lexer.GetToken();
        if(token.token_type == COMMA) //Check for more variables
            parse_var_list(); //Parse var_list
        else if(token.token_type == COLON) //Check for var_decl
            lexer.UngetToken(token);
        else //Else an error
            syntax_error();
    }
    else //Else an error
        syntax_error();
}

//Parses Unary Operator NOT
void parse_unary_operator()
{
    token = lexer.GetToken();
    if(token.token_type != NOT)
        syntax_error();
}

//Checks type of binary operator & returns token value
int parse_binary_operator()
{
    token = lexer.GetToken();
	int binary = -1;
	
    if(token.token_type == PLUS) //Addition
        binary = 15;
    else if(token.token_type == MINUS) //Subtraction
        binary = 16;
    else if(token.token_type == MULT) //Multiplication
        binary = 17;
    else if(token.token_type == DIV) //Division
        binary = 18;
    else if(token.token_type == GREATER) // Greater than
        binary = 19;
    else if(token.token_type == LESS) //Less than
        binary = 23;
    else if(token.token_type == GTEQ) //Greater than Equal
        binary = 19;
    else if(token.token_type == LTEQ) //Less than Equal
        binary = 21;
    else if(token.token_type == EQUAL) //Equal
        binary = 26;
    else if(token.token_type == NOTEQUAL) //Not Equal
        binary = 22;
    else //Else there is an error
        syntax_error();
	
	return binary;
}

//Checks if type is an operator
bool is_binary_operator(int op)
{
    bool isOperator;
	switch (op)
	{
		case PLUS: //For ADDITION
			isOperator = true;
			break;
		case MINUS: //For SUBTRACTION
			isOperator = true;
			break;
		case MULT: //For MULTIPLICATION
			isOperator = true;
			break;
		case DIV: //For DIVISION
			isOperator = true;
			break;
		case LESS: //For LESS THAN
			isOperator = true;
			break;
		case GREATER: //For GREATER THAN
			isOperator = true;
			break;
		case GTEQ: //For GREATER THAN EQUAL
			isOperator = true;
			break;
		case LTEQ: //For LESS THAN EQUAL
			isOperator = true;
			break;
		case EQUAL: //For EQUAL
			isOperator = true;
			break;
		case NOTEQUAL: //For NOTEQUAL
			isOperator = true;
			break;
		default: //Else not an expression
			isOperator = false;
	}
	return isOperator;
}

//Parses the Primary Type of the item & returns token value
int parse_primary()
{
    token = lexer.GetToken();
	int numType = -1;
	
    if(token.token_type == ID) //For ID
        numType = search_table(token.lexeme);
    else if(token.token_type == NUM) //For Int
        numType = 1;
    else if(token.token_type == REALNUM) //For Real Num
        numType = 2;
    else if(token.token_type == TR) //For True
        numType = 3;
    else if(token.token_type == FA) //For False
        numType = 3;
    else //Else there is an error
        syntax_error();
	
	return numType;
}

//Parses expressions
int parse_expression()
{
    int type;
    token = lexer.GetToken();
    if(token.token_type == ID || token.token_type == NUM || token.token_type == REALNUM || token.token_type == TR || token.token_type == FA) //Check for primary
    {
        lexer.UngetToken(token);
        type = parse_primary(); //Parse primary
    }
    else if(is_binary_operator(token.token_type)) //Check for an operator
    {
        int LHS, RHS;
		lexer.UngetToken(token);
        type = parse_binary_operator(); //Parse binary operator
        LHS = parse_expression(); //Parse left expression
        RHS = parse_expression(); //Parse right expression
        if((LHS != RHS) || !(is_binary_operator(type)))
        {
            if(type == PLUS || type == MINUS || type == MULT || type == DIV) //Check which operator type
            {
                if(LHS <= 2 && RHS > 3)
                {
                    update_type(RHS, LHS);
                    RHS = LHS;
                }
                else if(LHS > 3 && RHS <= 2)
                {
                    update_type(RHS, LHS);
                    LHS = RHS;
                }
                else if(LHS > 3 && RHS > 3)
                {
                    update_type(RHS, LHS);
                    RHS = LHS;
                }
                else //Else a type mismatch
                    type_mismatch(token.line_no, 2);
            }
            else if(type == GTEQ || type == GREATER || type == LTEQ || type == LESS || type == NOTEQUAL || type == EQUAL) //Check which operator type
            {
                if(RHS > 3 && LHS > 3)
                {
                    update_type(RHS, LHS);
                    RHS = LHS;
                    return 3;
                }
                else //Else a type mismatch
                    type_mismatch(token.line_no, 2);
            }
            else //Else a type mismatch
                type_mismatch(token.line_no, 2);
        }
        if(type == GTEQ || type == GREATER || type == LTEQ || type == LESS || type == EQUAL) //Check for excluding NOTEQUAL
            type = 3;
        else
            type = RHS;
    }
    else if(token.token_type == NOT) //Check for NOT
    {
        lexer.UngetToken(token);
        parse_unary_operator(); //Parse unary_operator
        type = parse_expression(); //Parse expression
        if(type != 3)
            type_mismatch(token.line_no, 3);
    }
    else //Else an error
        syntax_error();
    return type;
}

//Updates the variables in the list
void update_type(int current, int newType)
{
    scopeTable* iterator = symbolTable;
    while(iterator->next != NULL) //Search the list of variables
    {
        if(iterator->item->type == current) //Found & updated
            iterator->item->type = newType;
        iterator = iterator->next; //Next node
    }
    if(iterator->item->type == current) //Check last node
        iterator->item->type = newType;
}

//Helper function for determining expressions
bool is_expression(int expressionType)
{
    bool isExpression;
	switch (expressionType)
	{
		case ID: //For ID
			isExpression = true;
			break;
		case NUM: //For NUM
			isExpression = true;
			break;
		case REALNUM: //For REALNUM
			isExpression = true;
			break;
		case TR: //For TRUE
			isExpression = true;
			break;
		case FA: //For FALSE
			isExpression = true;
			break;
		case PLUS: //For ADDITION
			isExpression = true;
			break;
		case MINUS: //For SUBTRACTION
			isExpression = true;
			break;
		case MULT: //For MULTIPLICATION
			isExpression = true;
			break;
		case DIV: //For DIVISION
			isExpression = true;
			break;
		case LESS: //For LESS THAN
			isExpression = true;
			break;
		case GREATER: //For GREATER THAN
			isExpression = true;
			break;
		case GTEQ: //For GREATER THAN EQUAL
			isExpression = true;
			break;
		case LTEQ: //For LESS THAN EQUAL
			isExpression = true;
			break;
		case EQUAL: //For EQUAL
			isExpression = true;
			break;
		case NOTEQUAL: //For NOTEQUAL
			isExpression = true;
			break;
		case NOT: //For NOT
			isExpression = true;
			break;
		default: //Else not an expression
			isExpression = false;
	}
	return isExpression;
}

//Parses the assignment statement
void parse_assignment_stmt()
{
    int LHS, RHS;
    token = lexer.GetToken();
    if(token.token_type == ID) //Check for ID
    {
        LHS = search_table(token.lexeme); //Search list of variables
        token = lexer.GetToken();
        if(token.token_type == EQUAL) //Check for EQUAL
        {
            token = lexer.GetToken();
            if(is_expression(token.token_type)) //Check for expression
            {
                lexer.UngetToken(token);
                RHS = parse_expression(); //Parse 
                if(LHS == 1 || LHS == 2 || LHS == 3) //Check for INT, REAL, BOOL
                {
                    if(LHS != RHS)
					{
                        if(LHS > 2) //Check for LHS - RHS compatible
                        {
                            update_type(RHS,LHS);
                            RHS = LHS;
                        }
                        else //Else a mismatch
							type_mismatch(token.line_no, 1);
                    }
                }
                else //Compatible
                {
                    update_type(LHS,RHS);
                    LHS = RHS;
                }
				
                token = lexer.GetToken();
                if(token.token_type != SEMICOLON) //Check for semicolon
                    syntax_error();
            }
            else //Else an error
                syntax_error();
        }
        else //Else an error
            syntax_error();
    }
    else //Else an error
        syntax_error();
}

//Parse the case
void parse_case()
{
    token = lexer.GetToken();
    if(token.token_type == CASE) //Check for CASE
    {
        token = lexer.GetToken();
        if(token.token_type == NUM) //Check for NUM
        {
            token = lexer.GetToken();
            if(token.token_type == COLON) //Check for COLON
                parse_body(); //Parse body
            else //Else an error
                syntax_error();
        }
        else //Else an error
            syntax_error();
    }
    else //Else an error
        syntax_error();
}

//Parse the list of cases
void parse_case_list()
{
    token = lexer.GetToken();
    if(token.token_type == CASE) //Check for CASE
    {
        lexer.UngetToken(token);
        parse_case(); //Parse case
		
        token = lexer.GetToken();
        if(token.token_type == CASE) //Check for CASE again
        {
            lexer.UngetToken(token);
            parse_case_list(); //Parse case_list again
        }
        else if(token.token_type == RBRACE) //Check for RBRACE in switch
            lexer.UngetToken(token);
    }
}

//Parses SWITCH statement structures
void parse_switch_stmt()
{
    token = lexer.GetToken();
    if(token.token_type == SWITCH) //Check for SWITCH
    {
        token = lexer.GetToken();
        if(token.token_type == LPAREN) //Check for LPAREN
        {
            int temp = parse_expression(); //Check for expression
            if(temp <= 3 && temp != 1)
                type_mismatch(token.line_no, 5);
			
            token = lexer.GetToken();
            if(token.token_type == RPAREN) //Check for RPAREN
            {
                token = lexer.GetToken();
                if(token.token_type == LBRACE) //Check for LBRACE
                {
                    parse_case_list(); //Parse case_list
                    token = lexer.GetToken();
                    if(token.token_type != RBRACE) //Check for RBRACE
                        syntax_error();
                }
                else //Else an error
                    syntax_error();
            }
            else //Else an error
                syntax_error();
        }
        else //Else an error
            syntax_error();
    }
    else //Else an error
        syntax_error();
}

//Parses WHILE statement structures
void parse_while_stmt()
{
    token = lexer.GetToken();
    if(token.token_type == WHILE) //Check for WHILE
    {
        token = lexer.GetToken();
        if(token.token_type == LPAREN) //Check for LPAREN
        {
            int temp = parse_expression(); //Check for expression
            if(temp != 3) //Check for matching BOOL
                type_mismatch(token.line_no, 4);
			
            token = lexer.GetToken();
            if(token.token_type == RPAREN) //Check for RPAREN
                parse_body();
            else //Else an error
                syntax_error();
        }
        else //Else an error
            syntax_error();
    }
    else //Else an error
        syntax_error();
}

//Parses IF statement structures
void parse_if_stmt()
{
    token = lexer.GetToken();
    if(token.token_type == IF) //Check for IF
    {
        token = lexer.GetToken();
        if(token.token_type == LPAREN) //Check for LPAREN
        {
            int temp = parse_expression(); //Check for expression
            if(temp != 3) //Check for matching BOOL
                type_mismatch(token.line_no, 4);

            token = lexer.GetToken();
            if(token.token_type == RPAREN)
                parse_body();
            else //Else an error
                syntax_error();
        }
        else //Else an error
            syntax_error();
    }
    else //Else an error
        syntax_error();
}

//Parse statements for statement function
void parse_stmt()
{
    token = lexer.GetToken();
	lexer.UngetToken(token);
	
    if(token.token_type == ID) //Check for ID in assignment
        parse_assignment_stmt(); //Parse assignment
    else if(token.token_type == IF) //Check for IF statement
        parse_if_stmt(); //Parse if
    else if(token.token_type == WHILE) //Check for WHILE statement
        parse_while_stmt(); //Parse while
    else if(token.token_type == SWITCH) //Check for SWITCH statement
        parse_switch_stmt(); //Parse switch
    else //Else an error
        syntax_error();
}

//Parse the list of statements
void parse_stmt_list()
{
    token = lexer.GetToken();
    if(token.token_type == ID || token.token_type == IF || token.token_type == WHILE || token.token_type == SWITCH) //Check for stmt
    {
        lexer.UngetToken(token);
        parse_stmt(); //Parse the statement
        token = lexer.GetToken();
        if(token.token_type == ID || token.token_type == IF || token.token_type == WHILE || token.token_type == SWITCH) //Check stmt again
        {
            lexer.UngetToken(token);
            parse_stmt_list();
        }
        else if (token.token_type == RBRACE) //Check for a new body
            lexer.UngetToken(token);
    }
    else //Else an error
        syntax_error();
}

//Parses the body of the code
void parse_body()
{
    token = lexer.GetToken();
    if(token.token_type == LBRACE) //Check for LBRACE
    {
        parse_stmt_list(); //Parse the statement list
        token = lexer.GetToken();
        if(token.token_type != RBRACE) //Check for RBRACE
            syntax_error();
    }
    else if(token.token_type == END_OF_FILE) //Check for EOF
        lexer.UngetToken(token);
    else //Else an Error
        syntax_error();
}

//Parses the type name
void parse_type_name()
{
    token = lexer.GetToken();
    if(token.token_type == INT || token.token_type == REAL || token.token_type == BOO) //Check for token type
    {
		scopeTable* iterator = symbolTable; //Compare with the list
		while(iterator->next != NULL)
		{
			if(iterator->item->line_no == token.line_no) //Found
				iterator->item->type = token.token_type;
			iterator = iterator->next; //Next node
		}
		
		if(iterator->item->line_no == token.line_no) //Check last node
			iterator->item->type = token.token_type;
    }
    else //Else an error
        syntax_error();
}

//Parses the declared variable
void parse_var_decl()
{
    token = lexer.GetToken();
	
    if(token.token_type == ID) //Check for ID in var_list
    {
        lexer.UngetToken(token);
        parse_var_list(); //Parse var_list
        token = lexer.GetToken();
        if(token.token_type == COLON)
        {
            parse_type_name(); //Parse the type name
            token = lexer.GetToken();
            if(token.token_type != SEMICOLON) //Else an error
				syntax_error();
        }
        else //Else an error
            syntax_error();
    }
    else //Else an error
        syntax_error();
}

//Parses the declared variable list
void parse_var_decl_list()
{
    token = lexer.GetToken();
    while(token.token_type == ID) //Check for ID in var_list
    {
        lexer.UngetToken(token);
        parse_var_decl(); //Parse declared variables
        token = lexer.GetToken();
    }
    lexer.UngetToken(token);
}

//Parses Global Vars
void parse_global_vars()
{
    token = lexer.GetToken();
    if(token.token_type == ID) //Check for ID in var_list
    {
        lexer.UngetToken(token);
        parse_var_decl_list(); //Parse declared variables list
    }
    else //Else an error
        syntax_error();
}

//Parses the program
void parse_program()
{
    token = lexer.GetToken();
    while (token.token_type != END_OF_FILE)
    {
        if(token.token_type == ID) //Check for ID in var_list
        {
            lexer.UngetToken(token);
            parse_global_vars(); //Parse global variables
            parse_body(); //Parse the body
        }
        else if(token.token_type == LBRACE) //Check for LBRACE in body
        {
            lexer.UngetToken(token);
            parse_body(); //Parse the body
        }
        else //Else an error
            syntax_error();

        token = lexer.GetToken();
    }
}

//Prints the final output by iterating through the list of tables
void print_list()
{
    scopeTable* iterator = symbolTable;
	string output = "";
    string lCase;
    int type;

    while(iterator->next != NULL) //Iterate through the list
    {
       if(iterator->item->type > 3 && iterator->item->printed == false) //For type unknown
        {          
            type = iterator->item->type; //Collect type, name, edit print
            output += iterator->item->name;
            iterator->item->printed = true;
			
            while(iterator->next != NULL) 
            {
                iterator = iterator->next;
                if(iterator->item->type == type) //Found the same type
                {
                    output += ", " + iterator->item->name;
                    iterator->item->printed = true;
                }
            }
			
            output += ": ? #"; //Format the output
			iterator->item->printed = true;
            cout << output <<endl;
            
            output = ""; //Reset output and iterator
            iterator = symbolTable;
        }
        else if(iterator->item->type < 4 && iterator->item->printed == false) //For type known
        {
            lCase = keyword[(iterator->item->type) - 1]; //Collect keyword, type, edit print
            type = iterator->item->type;
			iterator->item->printed = true;
			
            output = iterator->item->name + ": " + lCase + " #"; //Format the output
            cout << output <<endl;
			
            output = ""; //Reset output

            while(iterator->next != NULL  && iterator->next->item->type == type) //Check for same type
            {
                iterator = iterator->next; //Iterate
				
                lCase = keyword[(iterator->item->type) - 1]; //Collect keyword, type, edit print
				iterator->item->printed = true;
				
                output = iterator->item->name + ": " + lCase + " #"; //Format output
                cout << output <<endl;
                
                output = ""; //Reset output
            }
        }
        else //Keep iterating
            iterator = iterator->next;
    }
	
	//Check the last node
    if (iterator->item->type > 3 && iterator->item->printed == false) //For type unknown
    {
        output += iterator->item->name + ":" + " ? " + "#";
        cout << output <<endl;
    }
    else if(iterator->item->type <= 3 && iterator->item->printed == false) //For type known
    {        
        lCase = keyword[(iterator->item->type) - 1];
        output += iterator->item->name + ": " + lCase + " #";
        cout << output <<endl;
    }
}

//Handles type mismatches
void type_mismatch(int line_no, int type)
{
	switch (type)
	{
		case 1: //LHS and RHS have same type
			cout << "TYPE MISMATCH " << line_no << " C1" << endl;
			break;
		case 2: //Binary Operators have same type
			cout << "TYPE MISMATCH " << line_no << " C2" << endl;
			break;
		case 3: //Unary Operator has type bool
			cout << "TYPE MISMATCH " << line_no << " C3" << endl;
			break;
		case 4: //if and while statements should have type bool
			cout << "TYPE MISMATCH " << line_no << " C4" << endl;
			break;
		case 5: //expression following switch is int
			cout << "TYPE MISMATCH " << line_no << " C5" << endl;
			break;
	}
	exit(1);
}

//Handles syntax errors
void syntax_error()
{
    cout << "\nSyntax Error\n";
    exit(1);
}

//*************************************
//END PARSER

//*************************************
//START LIST FUNCTIONS

//Adds items to the scope table
void add_to_list(string name, int type)
{
    if(symbolTable == NULL) //Create a new table
    {
        scopeTable* newTable = new scopeTable();
        scopeTableItem* newItem = new scopeTableItem();

        newItem->name = name; //Add name
        newItem->line_no = token.line_no; //Add line
        newItem->type = type; //Add type
        newItem->printed = false; //Not printed

        newTable->item = newItem; //Setup table
        newTable->next = NULL;
        newTable->prev = NULL;

        symbolTable = newTable;

    }
    else //Add to existing table
    {
        scopeTable* iterator = symbolTable;
        while(iterator->next != NULL) //Locate end of list
        {
            iterator = iterator->next;
        }

        scopeTable* newTable = new scopeTable(); //Create new entries
        scopeTableItem* newItem = new scopeTableItem();

        newItem->name = name; //Add name
        newItem->line_no = token.line_no; //Add line
        newItem->type = type; //Add type
        newItem->printed = false; //Not printed

        newTable->item = newItem; //Add to the end
        newTable->next = NULL;
        newTable->prev = iterator;
        iterator->next = newTable;
    }
}

//Searches the list of tables for the scope
int search_table(string name)
{
    scopeTable* iterator = symbolTable;
	
    if (symbolTable == NULL) //Check if table exists
    {
        add_to_list(name, enumCount);
        enumCount++;
        return 4;
    }
    else //Search through the list for item
    {
        while(iterator->next != NULL) //Check the list
        {
            if(iterator->item->name == name)
                return iterator->item->type;
            else
                iterator = iterator->next;
        }
		
        if(iterator->item->name == name) //Check the final node
            return iterator->item->type;
        else //Else add to list
        {
            add_to_list(name, enumCount);
            enumCount++;
            return (enumCount - 1);
        }
    }
}

//*************************************
//END LIST FUNCTIONS

//Driver Code
int main()
{
    parse_program();
    print_list();
	
    return 0;
}
