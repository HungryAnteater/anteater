#include "ant_pch.h"

string vformat(const char* fmt, va_list args)
{
   string s = string(15, 0);
   auto plen = vsnprintf(s.data(), s.size()+1, fmt, args);
   if (plen < 0) throw exception(("Bad format specifier: "s + fmt).c_str());
   size_t newsz = (size_t)plen;
   if (newsz >= s.size())
   {
      s.resize(newsz);
      plen = vsnprintf(s.data(), s.size()+1, fmt, args);
   }
   else
      s.resize(newsz);
   return s;

   //char buf[1024] {0};
   //auto plen = vsnprintf(buf, sizeof buf, fmt, args);
   //if (plen < 0) throw exception(("Bad format specifier: "s + fmt).c_str());
   //return buf;
}

string format(const char* fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   string s = vformat(fmt, args);
   va_end(args);
   return s;
}

const char* sformat(const char* fmt, ...)
{
	static char buf[512] {0};
	static bool locked = false;
	assert(!locked);
	locked = true;
	va_list args;
	va_start(args, fmt);
	auto plen = vsnprintf(buf, sizeof buf, fmt, args);
	if (plen < 0) return "<bad format specifier>";
	va_end(args);
	return buf;
	locked = false;
}

string LoadFile(const char* path)
{
	if (!path) Error("Could not open file: null filename");
	ifstream file;
	file.open(path, ios_base::in);
	if (file.fail()) Error("Could not open file: %s", path);

	string source;
	stringstream ss;

	ss << file.rdbuf();
	source = ss.str();
	file.close();
	source.push_back('\0');
	return source;
}

