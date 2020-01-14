//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
#include "ant_pch.h"

struct sformatter
{
    char buf[1024*1024] {0};
    size_t pos = 0;

    auto left() const { return sizeof(buf) - pos; }
    auto ptr() { return buf + pos; }
    void begin() { pos=0; buf[0]=0; }

    cstr put(const string_view& sv)
    {
       auto start = ptr();
       check(sv.size());
       for (char c: sv) buf[pos++] = c;
       return start;
    }
   
    bool check(size_t len)
    {
        if (len >= sizeof buf) throw AntError("String length is larger than maximum buffer size");
        if (len >= left())
        {
            cout << "NOTE: sformatter buffer overflow - wrapping...";
            begin();
            return false;
        }
        return true;
    }

    cstr vformat(cstr fmt, va_list args)
    {
        auto start = ptr();
        auto len = vsnprintf(start, left(), fmt, args);
        if (len < 0) throw AntError("Invalid format string: %s", fmt);
        if (!check(len)) return vformat(fmt, args);
        pos += (size_t)len + 1;
        return start;
    }
};

sformatter& sfmt()
{
    static sformatter* sf = new sformatter();
    return *sf;
}

cstr svformat(cstr fmt, va_list args)
{
    return sfmt().vformat(fmt, args);
}

cstr sformat(cstr fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   cstr s = svformat(fmt, args);
   va_end(args);
   return s;
}

cstr sformat_put(const string_view& sv)
{
    return sfmt().put(sv);
}

void Print(cstr msg)
{
    static ofstream output("log.txt");
    cout << msg;
    output << msg;
    //OutputDebugStringA(msg.c_str());
}

string LoadFile(cstr path)
{
    if (!path) throw AntError("Could not open file: null filename");
    ifstream file;
    file.open(path, ios_base::in);
    if (file.fail()) throw AntError("Could not open file: %s", path);

    string source;
    stringstream ss;

    ss << file.rdbuf();
    source = ss.str();
    file.close();
    source.push_back('\0');
    return source;
}

