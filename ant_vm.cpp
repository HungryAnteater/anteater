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
//#define local_ab(op) AntValue& a = Stack(2); AntValue& b = Stack(1); PrintOp("%s %s %s", a.ToString().c_str(), #op, b.ToString().c_str())
#define local_ab AntValue& a=Stack(2); AntValue& b=Stack(1)
#define endcase(op) default: Error("%s operator used on invalid types", #op)

#define intint(op)\
	case ANT_INT:\
		a.AsInt() = a.AsInt() op b.AsInt();\
		Pop(); break

#define intfloat(op)\
	case ANT_FLOAT:\
		a.AsFloat() = a.AsInt() op b.AsFloat();\
		a.type = ANT_FLOAT;\
		Pop(); break
		
#define floatint(op)\
	case ANT_INT:\
		a.AsFloat() = a.AsFloat() op b.AsInt();\
		Pop(); break
		
#define floatfloat(op)\
	case ANT_FLOAT:\
		a.AsFloat() = a.AsFloat() op b.AsFloat();\
		Pop(); break
		
#define nastyopmacro(op)\
	switch (a.type) {\
		case ANT_INT: {\
			switch (b.type) {\
				intint(op);\
				intfloat(op);\
				endcase(op);\
			}\
			break;\
		}\
		case ANT_FLOAT: {\
			switch (b.type) {\
				floatint(op);\
				floatfloat(op);\
				endcase(op);\
			}\
			break;\
		}\
		endcase(op);\
	}
	
#define numcompare(op)\
	if (a.IsInt() && b.IsInt()) a = a.AsInt() op b.AsInt();\
	else if (a.IsFloat() && b.IsFloat()) a = a.AsFloat() op b.AsFloat();\
	else if (a.IsInt() && b.IsFloat()) a = a.AsInt() op b.AsFloat();\
	else if (a.IsFloat() && b.IsInt()) a = a.AsFloat() op b.AsInt()

#define logicalop(op)\
{\
	PrintOp("LOGICALOP %s", #op);\
	local_ab;\
	numcompare(op);\
	else if (a.IsString() && b.IsString()) a = a.AsInt() op b.AsInt();\
	else Error("Comparison between unrelated types");\
	Pop();\
	break;\
}

#define logicalnumop(op)\
{\
	PrintOp("LOGICALOP %s", #op);\
	local_ab;\
	numcompare(op);\
	else Error("Comparison between unrelated types");\
	Pop();\
	break;\
}

bool AntVM::CompileString(const char* source)
{
	try
	{
		Print("    Parsing...\n");
		AntParser parser(source);
		if (bPrintTree) parser.PrintTree();

		Print("    Generating code...\n");
		AntCodeGen codegen(parser, ctx, code);
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
	string name = format("__file%d", numFiles++);
	string src = format("function %s() { \n%s\n }; %s();", name.c_str(), LoadFile(path).c_str(), name.c_str());
	return CompileString(src.c_str());
}

AntVM::AntVM()
{
	code.push_back(OP_CALL);
	code.push_back(4);
	code.push_back(0);
	code.push_back(0xF0F0F0F0);
}

void AntVM::Finalize()
{
	assert(!ctx.scopeStack.empty());
	code[3] = (int)ctx.globalScope->locals.size();
	ctx.scopeStack.pop_back();
	code.push_back(OP_DONE);
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

	Error("operator used on invalid types: %s, %s", a.ToString().c_str(), b.ToString().c_str());
	return nullptr;
}

void AntVM::Run()
{
	vector<AntValue> stack;
	int* ip = code.data();
	int fp = 0;
	vector<int> numParams;
	string output;

	// Readability macros
	#define Push(x)		(stack.push_back(x))
	#define PushVars(n)	(stack.resize(stack.size()+n))
	#define Pop()		(stack.pop_back())
	#define PopVars(n)	(stack.resize(stack.size()-n))
	#define Top()		(stack.back())
	#define Stack(i)	(*(stack.end()-i))
	#define Local(i)	(stack[fp+i])

	#define as_int(i)	Stack(i).AsInt() //(Stack(i).type == ANT_INT ? Stack(i) : throw AntException("Parameter type mismatch.  Expected %s, got %s", AntTypeNames[ANT_INT], AntTypeNames[Stack(i).type])
	#define as_array(i) Stack(i).AsArray() //(Stack(i).type == ANT_ARRAY ? Stack(i) : throw AntException("Invalid arg"))

	//#define SetBuf(x, y, z)\
	//	sprintf_s(buffer, x, a.y, b.z);\
	//	a.AsInt() = gStrings.GetID(buffer);\
	//	a.type = ANT_STRING;\
	//	Pop();
	
	try
	{
		while (*ip && *ip < (int)code.size() && *ip!=OP_DONE)
		{
			PrintOp("%4d:\t\tSTACK SIZE =%3zu: \t\t", ip-code.data(), stack.size());
	
			switch (*ip++)
			{
				case OP_CALL:
				{
					PrintOp("CALL %d %d %d", *ip, *(ip+1), *(ip+2));
					int start = *ip++;
					int nparams = *ip++;
					int nlocals = *ip++;
					numParams.push_back(nparams);
					//Push(AntValue((int)ip));
					Push(AntValue((int)(ip - code.data())));
					Push(AntValue(fp));
					fp = (int)stack.size() - 1;
					PushVars(nlocals);
					ip = &code[start];
					break;
				}
			
				case OP_ASSIGN:
				{
					PrintOp("ASSIGN %d", *ip);
					AntValue& a = Local(*ip++);
					AntValue& b = Stack(1);
					a = b;
					Pop();
					break;
				}
			
				case OP_RETURN:
				{
					PrintOp("RETURN: ");
					AntValue ret = Top();
					stack.resize(fp + 1);
					fp = Top().AsInt();
					Pop();
					//ip = (int*)Top().AsInt();
					ip = code.data() + Top().AsInt();
					Pop();
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
						Error("! operator only valid on integers (bools)");
					Top() = !Top().AsInt();
					break;
				}
			
				case OP_PRINT:
				{
					PrintOp("PRINT");
					AntValue& v = Stack(1);
					output += v.ToString() + "\n"s;
					Pop();
					break;
				}
			
				case OP_PUSH_INT:
					PrintOp("PUSH_INT %d", *ip);
					Push(AntValue(*ip++));
					break;
				
				case OP_PUSH_FLOAT:
					PrintOp("PUSH_FLOAT %f", *(float*)&(*ip));
					Push(AntValue(*(float*)&(*ip++)));
					break;
				
				case OP_PUSH_STRING:
					PrintOp("PUSH_STRING %s", GetString(*ip));
					Push(AntValue(*ip++));
					Top().type = ANT_STRING;
					break;
				
				case OP_PUSH_ARRAY:
				{
					PrintOp("PUSH_ARRAY ");
					int num = *ip++;
					//AntArray array;
					//for (int i=0; i<num; i++)
					//{
					//	array.push_back(Top());
					//	Pop();
					//}
					//Push(AntValue(array));
					Push(AntArray(stack.end()-1-num, stack.end()));
					break;
				}
			
				case OP_GET:
				{
					PrintOp("GET ");
					AntValue& v = Stack(2);
					AntValue& index = Stack(1);
					Stack(2) = v[index];
					//Stack(2) = Stack(2)[Stack(1)]; // array[index]; // array.AsArray().at(index.AsInt());
					PopVars(1);
					break;
				}
			
				case OP_SET:
				{
					PrintOp("SET ");
					Stack(3)[Stack(2)] = Stack(1);
					PopVars(2);
					break;
				}
			
				case OP_PUSH_VAR:
				{
					PrintOp("PUSH_VAR %d", *ip);
					stack.push_back(AntValue());
					AntValue& a = Top();
					AntValue& b = Local(*ip++);
					a = b;
					//Push(Local(*ip++));
					break;
				}
			
				case OP_ADD:
				{
					PrintOp("ADD ");
					local_ab;

					if (a.IsString() || b.IsString())
					{
						a = a.ToString() + b.ToString();
						Pop();
					}
					else
					{
						a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a+b; });
						Pop();
					}
					break;
				}
				
				case OP_SUB:
				{
					PrintOp("SUB ");
					local_ab;
					a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a-b; });
					Pop();
					break;
				}
				
				case OP_MUL:
				{
					PrintOp("MUL ");
					local_ab;
					//nastyopmacro(*);
					a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a*b; });
					Pop();
					break;
				}
				
				case OP_DIV:
				{
					PrintOp("DIV ");
					local_ab;
					a = BinaryOp(a, b, [](auto&& a, auto&& b){ return a/b; });
					Pop();
					break;
				}
			
				case OP_MOD:
				{
					PrintOp("MOD ");
					local_ab;

					if (!a.IsInt() || !b.IsInt())
						Error("% can only be used with integer values");

					Stack(2) = as_int(2) % as_int(1);
					Pop();
					break;
				}
			
				case OP_BRA:
				{
					PrintOp("BRA %d", *ip);
					int offset = *ip++;
					ip += offset;
					break;
				}
			
				case OP_BRZ:
				{
					PrintOp("BRZ %d", *ip);
					int offset = *ip++;
					if (Top().AsInt() == 0)
						ip += offset;
					Pop();
					break;
				}
			
				case OP_BNZ:
				{
					PrintOp("BNZ %d", *ip);
					int offset = *ip++;
					if (Top().AsInt() != 0)
						ip += offset;
					Pop();
					break;
				}
			
				case OP_EQUAL:		logicalop(==)
				case OP_NEQUAL:		logicalop(!=)
				case OP_LESS:		logicalnumop(<)
				case OP_GREATER:	logicalnumop(>)
				case OP_LEQUAL:		logicalnumop(<=)
				case OP_GEQUAL:		logicalnumop(>=)
			
				case OP_DONE:
					Error("Shouldn't get here.");
					break;
				
				default:
					Error("Unknown instruction: %d", *(ip-1));
			}

			PrintOp("\n");
		}
	}
	catch (const exception& e)
	{
		string err = format("Script runtime error: %s", e.what());
		output += err + "\n"s;
		Print(err);
	}

	PrintOp("DONE\n\n");
	Print("\n\nOutput:\n");
	Print(output);
	code.clear();
}
