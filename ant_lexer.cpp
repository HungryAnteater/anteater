#include "ant_pch.h"
#include "ant.h"

dictionary<int> keywords =
{
	{"and",			 'and'},
	{"break",		 'brk'},
	{"do",			 'do'},
	{"else",		 'else'},
	{"false",		 'fals'},
	{"for",			 'for'},
	{"foreach",		 'frch'},
	{"function",	 'func'},
	{"if",			 'if'},
	{"local",		 'locl'},
	{"not",			 '!'},
	{"or",			 'or'},
	{"return",		 'ret'},
	{"true",		 'true'},
	{"while",		 'whle'},
	{"in",			 'in'},
};

#define combine(a,b) ((a<<8) | b)

#define tok2(a,b) case a:\
	switch(next)\
	{\
		case b: Eat(); token=combine(a,b); return;\
		default: token=a; return;\
	}
	
#define tok3(a,b,c) case a:\
	switch(next)\
	{\
		case b: Eat(); token=combine(a,b); return;\
		case c: Eat(); token=combine(a,c); return;\
		default: token=a; return;\
	}
				
void AntLexer::Next()
{
	for(;;)
	{
		rawToken.clear();
		Eat();
	
		switch(cur)
		{
			case 0:
				token = 'eof';
				return;
			
			case ' ':
			case '\t':
			case '\n':
				 break;

			case '\r':
				if (next != '\n')
					break;
				 
			case '\"':
				GetString();
				return;
				
			case '/':
				switch(next)
				{
					case '/': Eat(); GetComment(); break; // single-line comment
					case '=': Eat(); token = '/='; return;
					case '*': GetBlockComment(); break;
					default: token = '/'; return;
				}
				break;
			
			// Simple operators
			tok3('+', '+', '=')
			tok3('-', '-', '=')
			tok2('*', '=')
			tok2('=', '=')
			tok2('!', '=')
			tok2('<', '=')
			tok2('>', '=')
				
			// Simple tokens
			case '(': case ')':
			case '[': case ']':
			case '{': case '}':
			case '%':
			case '^':
			case '$':
			case ';':
			case ':':
			case ',':
				token = cur;
				return;
			
			default:
			{
				if (isalpha(cur) || cur == '_')
				{
					GetIdentifier();
					return;
				}
				else if (isdigit(cur))
				{
					GetNumber();
					return;
				}
				else
				{
					throw AntError("unrecognized token: %s", rawToken.c_str());
				}
			}
		}
	}
}

void AntLexer::GetString()
{
	token = 'str';
	strToken.clear();
	Eat();
	
	while (cur != '\"')
	{
		strToken += cur;
		Eat();
	}
}

void AntLexer::GetComment()
{
	for(;;)
	{
		switch (cur)
		{
			case 0:
			case '\n':
			case 'eof':
				return;
		}
		
		Eat();
	}
}

void AntLexer::GetBlockComment()
{
	Eat();
	Eat();
	
	while (cur != '*' || next != '/')
	{
		if (cur == 0)
		{
			throw AntError("End of file reached before end of comment block.");
		}
		
		if (cur == '\n')
		{
			line++;
		}
		else if (cur == '/' && next == '*')
		{
			GetBlockComment();
		}

		Eat();
	}
	
	Eat();
}

void AntLexer::GetIdentifier()
{
	strToken.clear();
	strToken += cur;

	for(;;)
	{
		if (isalpha(next) || isdigit(next) || next=='_')
		{
			Eat();
			strToken += cur;
		}
		else
		{
			auto i = keywords.find(strToken);
			
			if (i != keywords.end())
			{
				token = i->second;
				return;
			}
			else
			{			
				token = 'id';
			}
			
			return;
		}
	}
}

void AntLexer::GetNumber()
{
	string s;
	s += cur;
	bool flt = false;
	
	for(;;)
	{
		if (next == '.')
		{
			flt = true;
			Eat();	
			s += cur;
		}
		else if (isdigit(next))
		{
			Eat();
			s += cur;
		}
		else
		{
			if (flt)
			{
				token = 'flt';
				fltToken = (float)atof(s.c_str());
			}
			else
			{
				token = 'int';
				intToken = atoi(s.c_str());
			}
			
			return;
		}
	}
}

void AntLexer::Eat()
{
	if (*ptr == 0)
	{
		cur = 0;
		next = 0;
		return;
	}

	if (*ptr == '\n')
	{
		line++;
		context.clear();
	}
	else
		context += *ptr;

	cur = *ptr++;
	next = *ptr;
	rawToken += cur;
}

const char* TokToStr(int token)
{
	static int i[2];

	int b1 = token & 0x000000FF;
	int b2 = token & 0x0000FF00;
	int b3 = token & 0x00FF0000;
	int b4 = token & 0xFF000000;
	int w1 = (b1 << 8) | (b2 >> 8);
	int w2 = (b3 >> 8) | (b4 >> 24);
	int x = (w1 << 16) | w2;

	if ((x & 0x000000FF) == 0) x >>= 8;
	if ((x & 0x000000FF) == 0) x >>= 8;
	if ((x & 0x000000FF) == 0) x >>= 8;

	i[0] = x;
	i[1] = 0;

	return (char*)&(i[0]);
}

