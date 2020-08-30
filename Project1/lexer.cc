#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include <cctype>

#include "lexer.h"
#include "inputbuf.h"

using namespace std;

string reserved[] = 
{   "END_OF_FILE",
    "IF", "WHILE", "DO", "THEN", "PRINT",
    "PLUS", "MINUS", "DIV", "MULT",
    "EQUAL", "COLON", "COMMA", "SEMICOLON",
    "LBRAC", "RBRAC", "LPAREN", "RPAREN",
    "NOTEQUAL", "GREATER", "LESS", "LTEQ", "GTEQ",
    "DOT", "NUM", "ID", "ERROR", "REALNUM", "BASE08NUM", "BASE16NUM"
};

#define KEYWORDS_COUNT 5
string keyword[] = { "IF", "WHILE", "DO", "THEN", "PRINT" };

void Token::Print() //Formatted Print
{
    cout << "{" << this->lexeme << " , "
         << reserved[(int) this->token_type] << " , "
         << this->line_no << "}\n";
}

LexicalAnalyzer::LexicalAnalyzer() //Constructor
{
    this->line_no = 1;
    tmp.lexeme = "";
    tmp.line_no = 1;
    tmp.token_type = ERROR;
}

bool LexicalAnalyzer::SkipSpace() //Assists with skipping spaces
{
    char c;
    bool space_encountered = false;

    input.GetChar(c);
    line_no += (c == '\n');

    while (!input.EndOfInput() && isspace(c)) 
	{
        space_encountered = true;
        input.GetChar(c);
        line_no += (c == '\n');
    }

    if (!input.EndOfInput()) 
	{
        input.UngetChar(c);
    }
    return space_encountered;
}

bool LexicalAnalyzer::IsKeyword(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) 
	{
        if (s == keyword[i]) 
		{
            return true;
        }
    }
    return false;
}

TokenType LexicalAnalyzer::FindKeywordIndex(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) 
	{
        if (s == keyword[i]) 
		{
            return (TokenType) (i + 1);
        }
    }
    return ERROR;
}

bool ispdigit(char c) //Assists with NUM
{
    return c >= '1' && c <= '9';
}

bool ispdigit8(char c) //Assists with Base08NUM
{
    return c >= '1' && c <= '7';
}

bool isdigit8(char c) //Assists with Base08NUM
{
    return c == '0' || ispdigit8(c);
}

bool ispdigit16(char c) //Assists with Base16NUM
{
    return (c >= '1' && c <= '9') || (c >= 'A' && c <= 'F');
}

bool isdigit16(char c) //Assists with Base16NUM
{
    return c == '0' || ispdigit16(c);
}

Token LexicalAnalyzer::ScanNumber() //Checks for type of number
{
    char c, b, a;
    bool base8 = true;
    bool base10 = true;
    input.GetChar(c);
    input.GetChar(b);
    
    if (isdigit(c)) 
    {
        if (c == '0' && b != 'x' && b != '.') //Base check for NUM
        {
            input.UngetChar(b);
            tmp.lexeme = "0";
            tmp.token_type = NUM;
            tmp.line_no = line_no;
        }
        else // TODO: Check for REALNUM, BASE08NUM and BASE16NUM here!
        {
            input.UngetChar(b);
            tmp.lexeme = "";
            while (!input.EndOfInput() && isdigit8(c)) //Checks for digit
            {
                tmp.lexeme += c;
                input.GetChar(c);
            }
            while (!input.EndOfInput() && isdigit(c)) //Checks for base8 conditions
            {
                base8 = false;
                tmp.lexeme += c;
                input.GetChar(c);
            }
            while (!input.EndOfInput() && isxdigit(c) && (isdigit(c) || isupper(c))) //Checks for base16 conditions
            {
                base8 = false;
                base10 = false;
                tmp.lexeme += c;
                input.GetChar(c);
            }
			
			if (c == 'x' && base8) //Check for BASE8NUM & Edge conditions
            {
                input.GetChar(b);
                input.GetChar(a);
                if (b == '0' && a == '8') //BASE08NUM Conditions
                {
                    tmp.lexeme = tmp.lexeme + c + b + a;
                    tmp.token_type = BASE08NUM;
                }
                else if (b == '1' && a == '6') //Edge case for BASE16NUM
                {
                    tmp.lexeme = tmp.lexeme + c + b + a;
                    tmp.token_type = BASE16NUM;
                } 
                else //Return NUM and backtrack
                {
                    input.UngetChar(a);
                    input.UngetChar(b);
                    input.UngetChar(c);
                    int front = 0;
                    while(front < tmp.lexeme.length() && isdigit(tmp.lexeme[front]))
                    {
                        front++;
                    }
                    for (int back = tmp.lexeme.length() - 1; back >= front; back--)
                    {
                        input.UngetChar(tmp.lexeme[back]);
                        tmp.lexeme.erase(back);
                    }
                    tmp.token_type = NUM;
                    tmp.line_no = line_no;
                }
            }
			else if (c == 'x' && !base8) //Handle Base16NUMs
            {
                input.GetChar(b);
                input.GetChar(a);
                if (b == '1' && a == '6') //Check for BASE16NUM
                {
                    tmp.lexeme = tmp.lexeme + c + b + a;
                    tmp.token_type = BASE16NUM;
                    tmp.line_no = line_no;
                } 
                else //Return NUM and backtrack
                {
                    input.UngetChar(a);
                    input.UngetChar(b);
                    input.UngetChar(c);
                    int front = 0;
                    while(front < tmp.lexeme.length() && isdigit(tmp.lexeme[front]))
                    {
                        front++;
                    }
                    for (int back = tmp.lexeme.length() - 1; back >= front; back--)
                    {
                        input.UngetChar(tmp.lexeme[back]);
                        tmp.lexeme.erase(back);
                    }
                    tmp.token_type = NUM;
                    tmp.line_no = line_no;
                }
            }
            else if (c == '.' && base10) //Check for REALNUM
            {
                input.GetChar(b);
                if (!isdigit(b)) //Check for NUM
                {
                    input.UngetChar(b);
                    input.UngetChar(c);
                    tmp.token_type = NUM;
                    tmp.line_no = line_no;
                    return tmp;
				}
				
                tmp.lexeme = tmp.lexeme + c + b;
                input.GetChar(c);
				
                while (isdigit(c) && !input.EndOfInput()) //Collect the REALNUM
                {
                    tmp.lexeme += c;
                    input.GetChar(c);
                }
                if (!input.EndOfInput()) //Backtrack end of file
                {
                    input.UngetChar(c);
                }
                tmp.token_type = REALNUM;
                tmp.line_no = line_no;
            }
			else if (c != '.' && base10) //Check for NUM
            {
                input.UngetChar(c);
                tmp.token_type = NUM;
                tmp.line_no = line_no;
            }
            else //Return NUM and backtrack
            {
                input.UngetChar(c);
                int front = 0;
                while(front < tmp.lexeme.length() && isdigit(tmp.lexeme[front]))
                {
                    front++;
                }
                for (int back = tmp.lexeme.length() - 1; back >= front; back--)
                {
                    input.UngetChar(tmp.lexeme[back]);
                    tmp.lexeme.erase(back);
                }
                tmp.token_type = NUM;
                tmp.line_no = line_no;
            }
            return tmp;
        }
		return tmp;
    }
    else //Error Condition
    {
        input.UngetChar(b);
        if (!input.EndOfInput()) 
        {
            input.UngetChar(c);
        }
        tmp.lexeme = "";
        tmp.token_type = ERROR;
        tmp.line_no = line_no;
        return tmp;
    }
}

Token LexicalAnalyzer::ScanIdOrKeyword() //Handles ID format
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
		{
            input.UngetChar(c);
        }
        tmp.line_no = line_no;
        if (IsKeyword(tmp.lexeme))
            tmp.token_type = FindKeywordIndex(tmp.lexeme);
        else
            tmp.token_type = ID;
    }
	else 
	{
        if (!input.EndOfInput()) 
		{
            input.UngetChar(c);
        }
        tmp.lexeme = "";
        tmp.token_type = ERROR;
    }
    return tmp;
}

TokenType LexicalAnalyzer::UngetToken(Token tok)
{
    tokens.push_back(tok);;
    return tok.token_type;
}

Token LexicalAnalyzer::GetToken()
{
    char c;

    if (!tokens.empty()) {
        tmp = tokens.back();
        tokens.pop_back();
        return tmp;
    }

    SkipSpace();
    tmp.lexeme = "";
    tmp.line_no = line_no;
    input.GetChar(c);
    switch (c) {
        case '.':
            tmp.token_type = DOT;
            return tmp;
        case '+':
            tmp.token_type = PLUS;
            return tmp;
        case '-':
            tmp.token_type = MINUS;
            return tmp;
        case '/':
            tmp.token_type = DIV;
            return tmp;
        case '*':
            tmp.token_type = MULT;
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
        case '[':
            tmp.token_type = LBRAC;
            return tmp;
        case ']':
            tmp.token_type = RBRAC;
            return tmp;
        case '(':
            tmp.token_type = LPAREN;
            return tmp;
        case ')':
            tmp.token_type = RPAREN;
            return tmp;
        case '<':
            input.GetChar(c);
            if (c == '=') //Check for LTEQ
            {
                tmp.token_type = LTEQ;
            } 
            else if (c == '>') //Check for !=
            {
                tmp.token_type = NOTEQUAL;
            } 
            else //Check for EOF
            {
                if (!input.EndOfInput()) 
                {
                    input.UngetChar(c);
                }
                tmp.token_type = LESS;
            }
            return tmp;
        case '>':
            input.GetChar(c);
            if (c == '=') //Check for GTEQ
            {
                tmp.token_type = GTEQ;
            } 
            else //Check for EOF
            {
                if (!input.EndOfInput()) 
                {
                    input.UngetChar(c);
                }
                tmp.token_type = GREATER;
            }
            return tmp;
        default:
            if (isdigit(c)) //Check for Number types
            {
                input.UngetChar(c);
                return ScanNumber();
            } 
            else if (isalpha(c)) //Check for ID
            {
                input.UngetChar(c);
                return ScanIdOrKeyword();
            } 
            else if (input.EndOfInput()) //Check for EOF
            {
                tmp.token_type = END_OF_FILE;
            }
            else //Else there is an error
                tmp.token_type = ERROR;

            return tmp;
    }
}

int main()
{
    LexicalAnalyzer lexer;
    Token token;

    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}