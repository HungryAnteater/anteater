#pragma once

#pragma warning (disable : 4311)
#pragma warning (disable : 4312)

#include <cassert>
#include <cstdarg>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <variant>

//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#undef min
//#undef max

#define TCT  template <class T>
#define TCTU template <class T, class U>
#define TCTS template <class... Ts>

using cstr = const char*;
using ccstr = const char* const;

using namespace std;

template <typename V>
using dictionary = unordered_map<string, V>;

inline void Output(const string& msg)
{
	static ofstream output("log.txt");
	cout << msg;
	output << msg;
    //OutputDebugStringA(msg.c_str());
}

string format(const char* fmt, ...);
const char* sformat(const char* fmt, ...);

template <class... Args>
void Print(const char* fmt, Args&&... args)
{
    Output(format(fmt, args...));
}

inline void Print(const string& msg) { Output(msg); }

struct AntError : public exception
{
    template <class... Args>
    AntError(ccstr fmt, Args&&... args): exception(format(fmt, args...).c_str()) {}

    AntError(ccstr s): exception(s) {}
    AntError(const string& s): exception(s.c_str()) {}
};

template <class... Args>
void Error(Args&&... args) { throw AntError(args...); }

//#define AntError(type, line, msg, ...) Error("%s error, line %d: %s\n", type, line, format(msg, __VA_ARGS__).c_str())
//#define LexError(msg, ...) throw AntError("Lex Error: ", format(msg, __VA_ARGS__))  //Error("Lex", line, msg, __VA_ARGS__)
//#define ParseError(msg, ...) throw AntError("Parse Error: ", format(msg, __VA_ARGS__))
#define AssertThrow(exp) if (!(exp)) Error("Assertion failed: " #exp)

string LoadFile(const char* path);

class EnumMap
{
    cstr* entries;
    size_t size;

public:
    EnumMap(initializer_list<pair<int, cstr>> init):
        entries(new cstr[init.size()]),
        size(init.size())
    {
        for (const auto& [idx, val]: init)
            entries[idx] = val;
    }

    const cstr& operator[](size_t i) const { assert(i<size); return entries[i]; }
};
