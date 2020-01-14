//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
// This file contains the virtual machine/interpreter.
// Local macros are relied on heavily to simplify the code and
// maximize performance.
//-----------------------------------------------------------------------------
#include "ant_pch.h"
#include "ant.h"

#define DEBUG_OPCODE

#ifdef DEBUG_OPCODE
    #define PrintOp Print
#else
    #define PrintOp(...)
#endif

// Hairy interpreter macros
#define numcompare(op)\
    if (a.IsInt() && b.IsInt()) a = a.AsInt() op b.AsInt();\
    else if (a.IsFloat() && b.IsFloat()) a = a.AsFloat() op b.AsFloat();\
    else if (a.IsInt() && b.IsFloat()) a = a.AsInt() op b.AsFloat();\
    else if (a.IsFloat() && b.IsInt()) a = a.AsFloat() op b.AsInt()

#define logicalop(op)\
{\
    PrintOp("LOGICALOP %s", #op);\
    AntValue& a = Stack(2);\
    AntValue& b = Stack(1);\
    numcompare(op);\
    else if (a.IsString() && b.IsString()) a = a.AsInt() op b.AsInt();\
    else throw AntError("Comparison between unrelated types");\
    PopVars(1);\
    break;\
}

#define logicalnumop(op)\
{\
    PrintOp("LOGICALOP %s", #op);\
    AntValue& a = Stack(2);\
    AntValue& b = Stack(1);\
    numcompare(op);\
    else throw AntError("Comparison between unrelated types");\
    PopVars(1);\
    break;\
}

constexpr int escapedChars[] {'n', 'r', 't'};

bool AntVM::CompileString(const char* source)
{
    try
    {
        Print("    Parsing...\n");
        AntParser parser(source);
        if (bPrintTree) parser.PrintTree();

        Print("    Generating code...\n");
        AntCodeGen codegen(parser.root, ctx, code);
    }
    catch (const AntError& e)
    {
        Print(e.what());
        return false;
    }
    
    return true;
}

bool AntVM::CompileFile(const char* path)
{
    static int numFiles = 0;

    Print("\nCompiling %s...\n", path);
    string noext(NoExtension(path));
    string name = sformat("__%s", noext.c_str(), numFiles++);
    string src = sformat("function %s() { \n%s\n return; }; %s();", name.c_str(), LoadFile(path).c_str(), name.c_str());
    return CompileString(src.c_str());
}

template <class OP>
AntValue BinaryOp(const AntValue& a, const AntValue& b, OP op)
{
    switch (a.type)
    {
        case ANT_INT:
        {
            switch (b.type)
            {
                case ANT_INT: return op(a.AsInt(), b.AsInt());
                case ANT_FLOAT: return op(a.AsInt(), b.AsFloat());
            }
            break;
        }
        case ANT_FLOAT:
        {
            switch (b.type)
            {
                case ANT_INT: return op(a.AsFloat(), b.AsInt());
                case ANT_FLOAT: return op(a.AsFloat(), b.AsFloat());
            }
            break;
        }
    }

    throw AntError("operator used on invalid types: %s, %s", a.ToString(), b.ToString());
    return nullptr;
}

void AntVM::Run()
{
    code.push_back(OP_DONE);
    vector<AntValue> stack;
    int* ip = code.data();
    int fp = 0;
    vector<int> numParams;
    string output;

    // Readability macros
    #define Push(x)     (stack.push_back(x))
    #define PushVars(n) (stack.resize(stack.size()+n))
    #define PopVars(n)  (stack.resize(stack.size()-n))
    #define Top()       (stack.back())
    #define Stack(i)    (*(stack.end()-i))
    #define Local(i)    (stack[(size_t)fp+i])

    try
    {
        while (*ip && *ip < (int)code.size() && *ip!=OP_DONE)
        {
            PrintOp("%4d:   stack: %-3zu         ", ip-code.data(), stack.size());
    
            switch (*ip++)
            {
                case OP_CALL:
                {
                    PrintOp("CALL               %-3d  %-3d  %-3d", *ip, *(ip+1), *(ip+2));
                    int start = *ip++;
                    int nparams = *ip++;
                    int nlocals = *ip++;
                    numParams.push_back(nparams);
                    Push(AntValue((int)(ip - code.data())));
                    Push(AntValue(fp));
                    fp = (int)stack.size() - 1;
                    PushVars(nlocals);
                    ip = &code[start];
                    break;
                }
            
                case OP_ASSIGN:
                {
                    PrintOp("ASSIGN             %d", *ip);
                    AntValue& a = Local(*ip++);
                    AntValue& b = Stack(1);
                    a = b;
                    PopVars(1);
                    break;
                }
            
                case OP_RETURN:
                {
                    PrintOp("RETURN:            ");
                    AntValue ret = Top();
                    stack.resize((size_t)fp + 1);
                    fp = Top().AsInt();
                    PopVars(1);
                    //ip = (int*)Top().AsInt();
                    ip = code.data() + Top().AsInt();
                    PopVars(1);
                    int numtopop = numParams.back();
                    PopVars(numtopop);
                    numParams.pop_back();
                    Push(ret);
                    break;
                }
            
                case OP_NOT:
                {
                    PrintOp("NOT");
                    if (!Top().IsInt())
                        throw AntError("! operator only valid on integers (bools)");
                    Top() = !Top().AsInt();
                    break;
                }
            
                case OP_PRINT:
                {
                    PrintOp("PRINT");
                    AntValue& v = Stack(1);
                    for (cstr c=v.ToString(); *c; c++)
                    {
                        if (*c == '\\' && Contains(escapedChars, *(c+1)))
                            int escaped = combine('\\', *++c);
                        else
                            output += *c;
                    }
                    output += '\n';
                    PopVars(1);
                    break;
                }
            
                case OP_PUSH_INT:
                    PrintOp("PUSH_INT           %d", *ip);
                    Push(AntValue(*ip++));
                    break;
                
                case OP_PUSH_FLOAT:
                    PrintOp("PUSH_FLOAT         %f", *(float*)&(*ip));
                    Push(AntValue(*(float*)&(*ip++)));
                    break;
                
                case OP_PUSH_STRING:
                    PrintOp("PUSH_STRING        \"%s\"", GetString(*ip));
                    Push(AntValue(*ip++));
                    Top().type = ANT_STRING;
                    break;
                
                case OP_PUSH_ARRAY:
                {
                    PrintOp("PUSH_ARRAY         ");
                    int num = *ip++;
                    Push(AntArray(stack.rbegin(), stack.rbegin()+num));
                    break;
                }
            
                case OP_GET:
                {
                    PrintOp("GET                ");
                    AntValue& v = Stack(2);
                    AntValue& i = Stack(1);
                    v = v[i];
                    PopVars(1);
                    break;
                }
            
                case OP_SET:
                {
                    PrintOp("SET                ");
                    AntValue& v = Stack(3);
                    AntValue& i = Stack(2);
                    AntValue& x = Stack(1);
                    v[i] = x;
                    PopVars(2);
                    break;
                }
            
                case OP_PUSH_VAR:
                {
                    PrintOp("PUSH_VAR           %d", *ip);
                    stack.push_back(AntValue());
                    AntValue& a = Top();
                    AntValue& b = Local(*ip++);
                    a = b;
                    break;
                }
            
                case OP_ADD:
                {
                    PrintOp("ADD                ");
                    AntValue& a = Stack(2);
                    AntValue& b = Stack(1);

                    if (a.IsString() || b.IsString())
                        a = sformat("%s%s", a.ToString(), b.ToString());
                    else
                        a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a+b; });

                    PopVars(1);
                    break;
                }
                
                case OP_SUB:
                {
                    PrintOp("SUB                ");
                    AntValue& a = Stack(2);
                    AntValue& b = Stack(1);
                    a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a-b; });
                    PopVars(1);
                    break;
                }
                
                case OP_MUL:
                {
                    PrintOp("MUL                ");
                    AntValue& a = Stack(2);
                    AntValue& b = Stack(1);
                    a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a*b; });
                    PopVars(1);
                    break;
                }
                
                case OP_DIV:
                {
                    PrintOp("DIV                ");
                    AntValue& a = Stack(2);
                    AntValue& b = Stack(1);
                    a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a/b; });
                    PopVars(1);
                    break;
                }
            
                case OP_MOD:
                {
                    PrintOp("MOD                ");
                    AntValue& a = Stack(2);
                    AntValue& b = Stack(1);
                    if (!a.IsInt() || !b.IsInt())
                        throw AntError("% can only be used with integer values");
                    a = a.AsInt() % b.AsInt();
                    PopVars(1);
                    break;
                }
            
                case OP_BRA:
                {
                    PrintOp("BRA                %d", *ip);
                    int offset = *ip++;
                    ip += offset;
                    break;
                }

                case OP_BRZ:
                {
                    PrintOp("BRZ                %d", *ip);
                    int offset = *ip++;
                    if (Top().AsInt() == 0)
                        ip += offset;
                    PopVars(1);
                    break;
                }
            
                case OP_BNZ:
                {
                    PrintOp("BNZ                %d", *ip);
                    int offset = *ip++;
                    if (Top().AsInt() != 0)
                        ip += offset;
                    PopVars(1);
                    break;
                }
            
                case OP_EQUAL:      logicalop(==)
                case OP_NEQUAL:     logicalop(!=)
                case OP_LESS:       logicalnumop(<)
                case OP_GREATER:    logicalnumop(>)
                case OP_LEQUAL:     logicalnumop(<=)
                case OP_GEQUAL:     logicalnumop(>=)
            
                case OP_DONE:
                    throw AntError("Shouldn't get here.");
                    break;
                
                default:
                    throw AntError("Unknown instruction: %d", *(ip-1));
            }

            PrintOp("\n");
        }
    }
    catch (const exception& e)
    {
        string err = sformat("Script runtime error: %s", e.what());
        output += err + "\n"s;
        Print(err);
    }

    PrintOp("DONE\n\n");
    Print("\n\nOutput:\n");
    Print(output);
    code.clear();
}
