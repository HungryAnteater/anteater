//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
#include "ant_pch.h"
#include "ant.h"

int AntScope::AddLocal(const char* name)
{
    if (IsDeclared(name))
        throw AntError("Symbol already declared: %s", name);
    
    locals.push_back(name);
    int index = (int)locals.size();
    symbols[name] = index;
    return index;
}

int AntScope::AddParam(const char* name)
{
    if (IsDeclared(name))
        throw AntError("Symbol already declared: %s", name);
    
    params.push_back(name);
    int index = -(int)params.size() - 1;
    symbols[name] = index;
    return index;
}

int AntScope::GetLocal(const char* name)
{
    int i = FindSymbol(name);
    if (i == 0)
        throw AntError("Undeclared variable: %s", name);
    return i;
}

AntScope* AntScope::AddFunction(const char* name)
{
    if (IsDeclared(name))
        throw AntError("Symbol already declared: %", name);

    AntScope* func = new AntScope();
    func->parent = this;
    func->name = name;
    int index = (int)children.size();
    children.push_back(func);
    symbols[name] = index;
    return func;
}

bool AntScope::IsDeclared(const char* name)
{
    return symbols.find(name) != symbols.end();
}

AntScope* AntScope::FindFunction(const char* name)
{
    if (!IsDeclared(name))
        return parent ? parent->FindFunction(name) : nullptr;

    return children.at(FindSymbol(name));
}

int AntScope::FindSymbol(const char* symbol)
{
    auto i = symbols.find(symbol);
    if (i == symbols.end())
        throw AntError("Could not find symbol: %s", symbol);
    return i->second;
}
