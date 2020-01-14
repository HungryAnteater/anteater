//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
#include "ant_pch.h"
#include "ant.h"

void AntNode::PrintNode() const
{
    static int depth = 0;
    static const bool parens = false;
    static const cstr tab = "  ";

    depth++;
    string indent;
    for (int i=0; i<depth; i++) indent += tab;
    const string cr = "\n"s + indent;
        
    Print(cr);
    if (parens)
        Print("(");
    
    switch (type)
    {
        #define scase(x, ...) case x: Print(__VA_ARGS__); break

        scase(NODE_ABSTRACT,    "node");

        scase(NODE_FUNC,        "function");
        scase(NODE_ASSIGN,      "=");
        scase(NODE_LOCAL,       "local");
        scase(NODE_FUNC_PARAMS, "func_params");
        scase(NODE_FUNC_LOCALS, "func_locals");

        scase(NODE_BREAK,       "break");
        scase(NODE_RETURN,      "return");
        scase(NODE_CALL,        "call");

        scase(NODE_AND,         "and");
        scase(NODE_OR,          "or");
        scase(NODE_NOT,         "!");

        scase(NODE_EQUAL,       "==");
        scase(NODE_NOT_EQUAL,   "!=");
        scase(NODE_LESS,        "<");
        scase(NODE_GREATER,     ">");
        scase(NODE_LEQUAL,      "<=");
        scase(NODE_GEQUAL,      ">=");

        scase(NODE_ADD,         "+");
        scase(NODE_SUB,         "-");
        scase(NODE_MUL,         "*");
        scase(NODE_DIV,         "/");
        scase(NODE_MOD,         "%%");
        scase(NODE_NEG,         "neg");
        scase(NODE_CAT,         "$");
        
        scase(NODE_IF,          "if");
        scase(NODE_WHILE,       "while");
        scase(NODE_DO_WHILE,    "do");
        scase(NODE_FOREACH,     "foreach");

        scase(NODE_TRUE,        "true");
        scase(NODE_FALSE,       "false");

        scase(NODE_ID,          "id: %s", AsString());
        scase(NODE_INT,         "int: %d", asInt);
        scase(NODE_FLOAT,       "float: %gf", asFloat);
        scase(NODE_STRING,      "string: \"%s\"", AsString());
        scase(NODE_ARRAY,       "array: ...");

        #undef scase
    }

    if (!children.empty())
    {
        for (const auto& c: children)
            c->PrintNode();

        if (parens)
            Print(cr);
    }

    if (parens)
        Print(")");

    depth--;
}


