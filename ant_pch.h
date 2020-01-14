//-----------------------------------------------------------------------------
// Copyright (C) Andrew Coggin, 2020
// All rights reserved.
//-----------------------------------------------------------------------------
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

using namespace std;

using cstr = const char*;
using sview = string_view;
using kview = const string_view&;

template <class T, class K=string>
using dictionary = unordered_map<K, T>;

template <class T, class U>
auto AddUnique(unordered_map<T, U>& map, const T& key, const U& val)
{
    auto [i, success] = map.try_emplace(key, val);
    if (!success) throw exception("Tried to add duplicate value to unordered_map");
    return i;
}

cstr sformat(cstr fmt, ...);
cstr sformat_put(const string_view& s);

template <class... Args>
string format(Args&&... args)
{
    return sformat(args...);
}

void Print(cstr s);
inline void Print(const string& msg) { Print(msg.c_str()); }
template <class... Ts>
void Print(cstr fmt, Ts&&... args) { Print(sformat(fmt, args...)); }

struct AntError : public exception
{
    template <class... Args>
    AntError(cstr fmt, Args&&... args): exception(sformat(fmt, args...)) {}
    AntError(cstr s): exception(s) {}
    AntError(const string& s): exception(s.c_str()) {}
};

inline sview Left(kview s, size_t i)    { return s.substr(0, i); }
inline sview Right(kview s, size_t i)   { return s.substr(min(i+1, s.size())); }

inline sview NoFolder(kview s)          { return Right(s, s.find_last_of("/\\")); }
inline sview NoExtension(kview s)       { return Left(s, s.find_first_of('.')); }

string LoadFile(cstr path);

template <class T1, class T2>
bool Contains(const T1& v, const T2& x)
{
   return find(begin(v), end(v), x) != end(v);
}

template <class K, class V>
bool Find(const unordered_map<K, V>& map, const K& key, V& val)
{
    auto i = map.find(key);
    bool ret = i != map.end();
    if (ret) val = i->second;
    return ret;
}

inline constexpr size_t fnv_offset_basis = 14695981039346656037ULL;
inline constexpr size_t fnv_prime        = 1099511628211ULL;

inline size_t fnv1a_append_bytes(size_t val, const unsigned char* const v, const size_t count) noexcept
{
    for (size_t i=0; i<count; i++)
    {
        val ^= (size_t)v[i];
        val *= fnv_prime;
    }

    return val;
}

template <class T>
size_t hash_array(const T* const v, const size_t count) noexcept
{
    static_assert(is_trivial_v<T>, "Only trivial types can be directly hashed.");
    return fnv1a_append_bytes(fnv_offset_basis, (const unsigned char*)v, count * sizeof(T));
}

template <>
struct hash<cstr>
{
    size_t operator()(cstr key) const noexcept { return hash_array(key, strlen(key)); }
};

template <>
struct equal_to<cstr>
{
	bool operator()(cstr x, cstr y) const { return strcmp(x,y) == 0; }
};

template <class K=int, class V=cstr>
class EnumMap
{
    static_assert(is_integral_v<K> | is_enum_v<K>);
    vector<V> vals;
    unordered_map<V, K> keys;

public:
    EnumMap(initializer_list<pair<K, const V>> init): vals(init.size())
    {
        for (const auto& [i, v]: init)
        {
            vals[(int)i] = v;
            auto [i, inserted] = keys.try_emplace(v, i);
            if (!inserted) throw exception("Tried to insert duplicate key");
        }
    }

    const V& operator[](const K& key) const { return vals.at(key); }
    const K& operator[](const V& val) const { return keys.at(val); }
     
    bool FindKey(const V& val, K& key) const { return ::Find(keys, val, key); }
};

template <class K, class V>
class BiMap
{
public:
    unordered_map<K, V> vals;
    unordered_map<V, K> keys;

    BiMap(initializer_list<pair<const K, V>> init): vals(init)
    {
        for (auto& [t, u]: init)
            keys.emplace(u, t);
    }

    const V* find(const K& t) const { return vals.find(t); }
    const K* find(const V& u) const { return keys.find(u); }

    const V& operator[](const K& key) const { return vals.at(key); }
    const K& From(const V& key) const { return keys.at(key); }

    V& operator[](const K& key) { return vals.at(key); }
    K& From(const V& key) { return keys.at(key); }

    auto Add(const K& key, const V& val)
    {
        return pair
        {
            AddUnique(vals, key, val),
            AddUnique(keys, val, key)
        };
    }

    bool Find(const K& key, V& val) const { return ::Find(vals, key, val); }
    bool FindKey(const V& val, K& key) const { return ::Find(keys, val, key); }
};