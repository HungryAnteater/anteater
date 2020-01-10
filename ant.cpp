#include "ant_pch.h"
#include "ant.h"

// Utility that assigns IDs to strings
class StringTable
{
	vector<string> strings;
	dictionary<int> stringLookup;

public:

	int GetID(const char* str)
	{
		auto i = stringLookup.find(str);
		if (i != stringLookup.end())
			return i->second;
	
		int index = (int)strings.size();
		strings.push_back(str);
		stringLookup[str] = index;
		return index;
	}

	const char* GetString(int i) const
	{
		if (i < 0 || i >= (int)strings.size())
			Error("Invalid string constant");
		
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
				Error("Enter a valid filename.");

			if (!vm.CompileFile(args[i]))
				throw AntError("Compilation failed.  Terminating.");
		}

		vm.Finalize();

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

string AntValue::ToString() const
{
	switch (type)
	{
		case ANT_INVALID:	return "<invalid>";
		case ANT_INT:		return format("%d", AsInt());
		case ANT_FLOAT:		return format("%g", AsFloat());
		case ANT_STRING:	return AsString();
		case ANT_ARRAY:
		{
			const string tab(4, ' ');
			string s = "{\n"s + tab;
			for (const AntValue& x: AsArray())
				s += tab + x.ToString() + ",\n"s;
			s += "}";
			return s;
		}
		default:
			return "<ERROR>";
	}
}

AntCodeGen::AntCodeGen(const AntParser& parser, AntContext& ctx_, vector<OpCode>& code_):
    lines(parser.lines),
	ctx(ctx_),
	code(code_)
{
	CodeGen(parser.root);
}
