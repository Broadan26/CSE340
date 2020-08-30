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
    "PUBLIC", "PRIVATE", "EQUAL",
    "COLON", "COMMA", "SEMICOLON",
    "LBRACE", "RBRACE", "ID", "ERROR"
};

#define KEYWORDS_COUNT 2
string keyword[] = { "public", "private" };

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

Token LexicalAnalyzer::SkipComment() //Assists with skipping over comments
{
	char c;
	input.GetChar(c);
	
	while(!input.EndOfInput() && c != '\n') //Skip to end of line
		input.GetChar(c);
	line_no++; //Increment line number
	
	if(!input.EndOfInput())
		input.UngetChar(c);
	
	return GetToken(); //Begin again
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

TokenType LexicalAnalyzer::UngetToken(Token tok) //Pushes back the token
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
        case '/': //Check for comment
            char b;
			input.GetChar(b);
			if(b == '/')
				return SkipComment();
			else
				input.UngetChar(b);
        case '=': //Check for equal
            tmp.token_type = EQUAL;
            return tmp;
        case ':': //Check for colon
            tmp.token_type = COLON;
            return tmp;
        case ',': //Check for comma
            tmp.token_type = COMMA;
            return tmp;
        case ';': //Check for semicolon
            tmp.token_type = SEMICOLON;
            return tmp;
        case '{': //Check for Left Brace
            tmp.token_type = LBRACE;
            return tmp;
        case '}': //Check for Right Brace
            tmp.token_type = RBRACE;
            return tmp;
		case 'p': //Check for public or private
			char d, e, f, g, h, i; //Create temp storage
			input.GetChar(d); input.GetChar(e); input.GetChar(f); input.GetChar(g); input.GetChar(h); //Collect chars
			
			if(d == 'u' && e == 'b' && f == 'l' && g == 'i' && h == 'c') //Check for public
			{
				tmp.token_type = PUBLIC;
				return tmp;
			}
			input.GetChar(i); //Collect e
			if(d == 'r' && e == 'i' && f == 'v' && g == 'a' && h == 't' && i == 'e') //Check for private
			{
				tmp.token_type = PRIVATE;
				return tmp;
			}
			input.UngetChar(i); input.UngetChar(h); input.UngetChar(g); //Return letters except c
			input.UngetChar(f); input.UngetChar(e); input.UngetChar(d);
        default:
			if (isalpha(c)) //Check for ID
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