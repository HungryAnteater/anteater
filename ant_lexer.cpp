//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
#include "ant_pch.h"
#include "ant.h"

BiMap<int, cstr> keywords
{
    {'and',     "and"     },
    {'brk',     "break"   },
    {'do',      "do"      },
    {'else',    "else"    },
    {'fals',    "false"   },
    {'for',     "for"     },
    {'frch',    "foreach" },
    {'func',    "function"},
    {'if',      "if"      },
    {'locl',    "local"   },
    {'!',       "not"     },
    {'or',      "or"      },
    {'ret',     "return"  },
    {'true',    "true"    },
    {'whle',    "while"   },
    {'in',      "in"      },
};

cstr TokToStr(int tok)
{
    cstr s = nullptr;

    if (tok < 128)
        return sformat("%c", tok);
    if (keywords.Find(tok, s))
        return s;

    static int i[2];

    int b1 = tok & 0x000000FF;
    int b2 = tok & 0x0000FF00;
    int b3 = tok & 0x00FF0000;
    int b4 = tok & 0xFF000000;
    int w1 = (b1 << 8) | (b2 >> 8);
    int w2 = (b3 >> 8) | (b4 >> 24);
    int x = (w1 << 16) | w2;

    if ((x & 0x000000FF) == 0) x >>= 8;
    if ((x & 0x000000FF) == 0) x >>= 8;
    if ((x & 0x000000FF) == 0) x >>= 8;

    i[0] = x;
    i[1] = 0;

    return sformat("%s", (char*)&(i[0]));
}

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
        curColumn = colCounter;
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
        if (cur == '\\')
        {
            switch (next)
            {
                case 'n': cur = '\n';  Eat(); Eat(); break;
                case 'r': cur = '\r';  Eat(); Eat(); break;
                case 't': cur = '\t';  Eat(); Eat(); break;
                case '\"': cur = '\"'; Eat(); Eat(); break;
                case '\'': cur = '\''; Eat(); Eat(); break;
            }
        }
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
            throw AntError("End of file reached before end of comment block.");

        if (cur == '\n')
            curLine++;
        else if (cur == '/' && next == '*')
            GetBlockComment();

        Eat();
    }
    
    Eat();
}

void AntLexer::GetIdentifier()
{
    strToken = cur;
    while (isalpha(next) || isdigit(next) || next=='_')
    {
        Eat();
        strToken += cur;
    }

    if (!keywords.FindKey(strToken.c_str(), token))
        token = 'id';
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
        curLine++;
        colCounter = 0;
    }

    cur = *ptr++;
    next = *ptr;
    rawToken += cur;
    colCounter++;
}
