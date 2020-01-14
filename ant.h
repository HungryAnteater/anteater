//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
// The main interface to the AntEater scripting language is AntVM.
// All other classes are exposed and can be used individually.
//
// To execute script code, you must:
// 1. Load a file as a string (LoadFile function provided).
// 2. Create an AntVM object.
// 3. Call Compile() on the AntVM object one or more times.
// 4. Call Run() on on the AntVM object.
//-----------------------------------------------------------------------------
#pragma once

enum AntNodeType
{
    NODE_ABSTRACT,

    NODE_FUNC,
    NODE_ASSIGN,
    NODE_LOCAL,
    NODE_FUNC_PARAMS,
    NODE_FUNC_LOCALS,
    
    NODE_BREAK,
    NODE_RETURN,
    NODE_CALL,
    
    NODE_AND,
    NODE_OR,
    NODE_NOT,
    
    NODE_EQUAL,
    NODE_NOT_EQUAL,
    NODE_LESS,
    NODE_GREATER,
    NODE_LEQUAL,
    NODE_GEQUAL,
    
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,
    NODE_NEG,
    NODE_CAT,
    
    NODE_IF,
    NODE_WHILE,
    NODE_DO_WHILE,
    NODE_FOREACH,
    
    NODE_TRUE,
    NODE_FALSE,
    NODE_NULL,
    NODE_ARRAY_GET,
    NODE_ARRAY_SET,

    NODE_ID,
    NODE_INT,
    NODE_FLOAT,
    NODE_STRING,
    NODE_ARRAY,

    NUM_NODE_TYPES
};

enum AntType
{
    ANT_INVALID,
    ANT_INT,
    ANT_FLOAT,
    ANT_STRING,
    ANT_ARRAY,
};

inline const EnumMap AntTypeNames
{
    {ANT_INVALID,   "<invalid>"},
    {ANT_INT,       "int"},
    {ANT_FLOAT,     "float"},
    {ANT_STRING,    "string"},
    {ANT_ARRAY,     "array"},
};

typedef int OpCode; // this could be changed to byte as an optimization

enum AntCode : OpCode
{
    OP_DONE,
    OP_PUSH_INT,
    OP_PUSH_FLOAT,
    OP_PUSH_STRING,
    OP_PUSH_VAR,
    OP_EQUAL,
    OP_NEQUAL,
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_BRA,
    OP_BNE,
    OP_BEQ,
    OP_BRZ,
    OP_BNZ,
    OP_CALL,
    OP_ASSIGN,
    OP_RETURN,
    OP_PRINT,
    OP_LESS,
    OP_GREATER,
    OP_LEQUAL,
    OP_GEQUAL,
    OP_MOD,
    OP_PUSH_ARRAY,
    OP_GET,
    OP_SET,

    NUM_OPS
};

inline int curLine = 1;
inline int curColumn = 0;
inline int colCounter = 0;
inline vector<string> lines;

cstr TokToStr(int tok);
int GetID(cstr str);
cstr GetString(int id);

inline cstr ReportError(int line, int col, cstr msg)
{
    return sformat(
        "ERROR: %s\n"
        "    line %d, column %d\n"
        "    ... %s\n"
        "        %s^\n",
        msg, line, col, lines.at(line).c_str(), string(col, ' ').c_str());
}

class AntLexer
{
public:
    AntLexer(const char* source): ptr(source) { curLine = 0; instance = this; }
    ~AntLexer() { instance = nullptr; }

    void Next(); // advance one token
    
    // These are exposed publicly for simplicity
    int token = 0; // cur token (enum)
    string rawToken; // cur token as raw string literal
    string strToken; // cur token as string stripped of ""
    int intToken = 0;
    float fltToken = 0;
    string context;

    static inline AntLexer* instance = nullptr; // TEMP HACK

private:

    void GetString();
    void GetComment();
    void GetBlockComment();
    void GetIdentifier();
    void GetNumber();
    void Eat(); // advance one character

    int cur = 0;
    int next = 0;
    const char* ptr = nullptr;
};

// Node struct used by parser
struct AntNode
{
    AntNode(AntNodeType t=NODE_ABSTRACT): type(t), line(curLine), column(curColumn) {}
    ~AntNode() { for (auto c : children) delete c; }
    
    void Add(AntNode* child) { children.push_back(child); }
    cstr AsString() const { return ::GetString(asInt); }
    void PrintNode() const;

    void Set(int i) { asInt = i; }
    void Set(float f) { asFloat = f; }
    void Set(cstr s) { asInt = GetID(s); }
    void Set(const string& s) { Set(s.c_str()); }
    
    union
    {
        int asInt = 0;
        float asFloat;
    };
    
    AntNodeType type;
    vector<AntNode*> children;
    int line = 0;
    int column = 0;
};

class AntValue;
typedef vector<AntValue> AntArray;

// Similar to AntNode but simplified for use with VM at runtime
class AntValue
{
public:
    AntType type;
    variant<nullptr_t, int, float, AntArray> data;
    
    AntValue(): type(ANT_INVALID), data(nullptr) {}
    AntValue(int i): type(ANT_INT), data(i) {}
    AntValue(float f): type(ANT_FLOAT), data(f) {}
    AntValue(cstr s): type(ANT_STRING), data(GetID(s)) {}
    AntValue(const string& s): AntValue(s.c_str()) {}
    AntValue(AntArray&& v): type(ANT_ARRAY), data(v) {}

    bool IsInt() const { return type==ANT_INT; }
    bool IsFloat() const { return type==ANT_FLOAT; }
    bool IsString() const { return type==ANT_STRING; }
    bool IsArray() const { return type==ANT_ARRAY; }
    bool IsNumber() const { return type==ANT_INT || type==ANT_FLOAT; }

    void SetInt(int i) { type=ANT_INT; data=i; }
    void SetFloat(float f) { type=ANT_FLOAT; data=f; }

    int AsInt() const { CheckType(ANT_INT); return get<int>(data); }
    float AsFloat() const { CheckType(ANT_FLOAT); return get<float>(data); }
    cstr AsString() const { CheckType(ANT_STRING); return GetString(get<int>(data)); }
    AntArray& AsArray() { CheckType(ANT_ARRAY); return get<AntArray>(data); }
    const AntArray& AsArray() const { return ((AntValue*)this)->AsArray(); }

    void CheckType(AntType t) const
    {
        if (type != t)
            throw AntError("Tried to access %s as %s", AntTypeNames[type], AntTypeNames[t]);
    }

    void CheckIndex(const AntValue& i)
    {
        if (type != ANT_ARRAY) throw AntError("Indexer cannot be used on %s", AntTypeNames[type]);
        if (i.type != ANT_INT) throw AntError("Type %s cannot be used to index into arrays", AntTypeNames[i.type]);
        int idx = i.AsInt();
        if (idx < 0 || idx >= AsArray().size())
            throw AntError("Array access out of bounds: %d", idx);
    }

    AntValue operator[](int i) { CheckType(ANT_ARRAY); return AsArray().at(i); } 
    AntValue operator[](const AntValue& i) { CheckIndex(i); int idx = i.AsInt(); return AsArray().at(i.AsInt()); }

    cstr ToString() const;
};

// Parser runs on construction and sets the public root field
// to the resulting parse tree.
class AntParser
{
public:
    AntParser(const char* src);
    ~AntParser() { delete root; }
    
    void PrintTree();

    // Parser output.  Pass these to AntCodeGen
    const string source;
    AntNode* root = nullptr;
    
private:

    AntNode* Statement();
    AntNode* Block();
    AntNode* Expression();
    AntNode* Expression2();
    AntNode* Expression3();
    AntNode* Expression4();
    AntNode* Factor();
    AntNode* Function();
    AntNode* Identifier();
    AntNode* BinaryOp(AntNodeType type, AntNode* a, AntNode* b);

    void Expect(int token); // Throw exception if cur token does not match expectation
    void ExpectNext(int token) { Expect(token); lex.Next(); }

    template <class T>
    AntNode* NewNode(AntNodeType type, const T& value)
    {
        AntNode* n = new AntNode(type);
        n->Set(value);
        lex.Next();
        return n;
    }

    AntLexer lex;
};

// Function object used during code generation only
class AntScope
{
public:
    ~AntScope() { for (auto f: children) delete f; }
    
    int AddLocal(const char* name);
    int GetLocal(const char* name);
    int AddParam(const char* name);
        
    AntScope* AddFunction(const char* name);
    AntScope* FindFunction(const char* name);
    
    bool IsDeclared(const char* name);
    int FindSymbol(const char* symbol);
    
    string name = "anonymous";
    AntScope* parent = nullptr;
    vector<AntScope*> children;
    dictionary<AntScope*> functionLookup;
    vector<string> params;
    vector<string> locals;
    dictionary<int> symbols;
    vector<AntCode> code;
    int begin = 0;
};

struct AntContext
{
    vector<AntScope*> scopeStack;
    unordered_map<int, AntScope*> functionMap;
    AntScope* globalScope = nullptr;

    cstr FuncName(int f) const { return functionMap.at(f)->name.c_str(); }

    AntContext()
    {
        globalScope = new AntScope();
        scopeStack.push_back(globalScope);
        globalScope->name = "main";
        globalScope->begin = 4;
        functionMap[4] = globalScope;
    }

    AntScope& CurScope() { return !scopeStack.empty() ? *scopeStack.back() : *globalScope; }
};

class AntCodeGen
{
public:
    AntCodeGen(AntNode* root, AntContext& ctx_, vector<OpCode>& code_):
        ctx(ctx_),
        code(code_)
    {
        CodeGen(root);
    }

    static void PrintCode(const AntContext& ctx, const vector<OpCode>& range);

private:
    void CodeGen(AntNode* node);

    void Emit(int i) { code.push_back(i); }
    int ForwardJump() { Emit(0); return (int)code.size()-1; }
    void PatchForwardJump(int p) { code[p] = ((int)code.size() - p) - 1; }

    AntContext& ctx;
    vector<OpCode>& code;
};

// This is the main interface that client code will use.
// A single AntVM object stores its currently compiled byte code.
// Compile may be called multiple times and will append newly compiled
// code to existing byte code.
class AntVM
{
public:
    bool CompileString(const char* src);
    bool CompileFile(const char* path);
    void Run();

    bool bPrintTree = false;
    bool bPrintCode = false;

    AntContext ctx;
    vector<OpCode> code;
};
