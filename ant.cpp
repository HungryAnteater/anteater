//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
#include "ant_pch.h"
#include "ant.h"

// Utility that assigns IDs to strings
class StringTable
{
    vector<string> strings;
    dictionary<int> stringLookup;

public:

    int GetID(cstr str)
    {
        auto i = stringLookup.find(str);
        if (i != stringLookup.end())
            return i->second;
    
        int index = (int)strings.size();
        strings.push_back(str);
        stringLookup[str] = index;
        return index;
    }

    cstr GetString(int i) const
    {
        if (i < 0 || i >= (int)strings.size())
            throw AntError("Invalid string constant");
        
        return strings[i].c_str();
    }
};

StringTable gStrings;

int GetID(cstr str) { return gStrings.GetID(str); }
cstr GetString(int id) { return gStrings.GetString(id); }

//-------------------------------------------------------------------------
int main(int numArgs, char* args[])
{
    vector<const char*> sources;
    const char* outPath = nullptr;
    bool bPause = false;

    AntVM vm;

    for (int i=1; i<numArgs; i++)
    {
        if (args[i] && args[i][0]=='-')
        {
            if (args[i][1] == 't') vm.bPrintTree = true;
            else if (args[i][1] == 'c') vm.bPrintCode = true;
            else if (args[i][1] == 'p') bPause = true;
        }
    }

    try
    {

        for (int i=1; i<numArgs; i++)
        {
            if (args[i] && args[i][0]=='-')
                continue;
        
            if (args[i] == nullptr)
                throw AntError("Enter a valid filename.");

            if (!vm.CompileFile(args[i]))
                throw AntError("Compilation failed.  Terminating.");
        }

        if (vm.bPrintCode)
            AntCodeGen::PrintCode(vm.ctx, vm.code);

        vm.Run();
    }
    catch (const AntError& e)
    {
        Print(e.what());
    }

    Print("\n--------------- Done ---------------\n");
    if (bPause) system("pause");
    return 0;
}

cstr AntValue::ToString() const
{
    switch (type)
    {
        case ANT_INVALID:   return "<invalid>";
        case ANT_INT:       return sformat("%d", AsInt());
        case ANT_FLOAT:     return sformat("%f", AsFloat());
        case ANT_STRING:    return sformat("%s", AsString());
        case ANT_ARRAY:
        {
            string s = "\n{\n"s;
            for (const AntValue& x: AsArray())
                s += sformat("   %s,\n", x.ToString());
            s += "}";
            return sformat("%s", s.c_str());
        }
        default:
            return "<ERROR>";
    }
}
