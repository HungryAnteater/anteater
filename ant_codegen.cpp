#include "ant_pch.h"
#include "ant.h"

#define node(i)			(n->children[i])
#define numnodes		((int)n->children.size())
#define checknodes(num)	if (numnodes != num) Error("Invalid node children")

AntNode* lastNode = nullptr;

void AntCodeGen::CodeGen(AntNode* n)
{
	auto start = code.size();
	lastNode = n;

	try
	{
		switch (n->type)
		{
			case NODE_INT:
				Emit(OP_PUSH_INT);
				Emit(n->asInt);
				break;
			
			case NODE_FLOAT:
				Emit(OP_PUSH_FLOAT);
				Emit(*(int*)&n->asFloat);
				break;
			
			case NODE_STRING:
				Emit(OP_PUSH_STRING);
				Emit(n->asInt);
				break;
	
			case NODE_ID:
			{
				int offset = ctx.CurScope().GetLocal(n->AsString());
				Emit(OP_PUSH_VAR);
				Emit(offset);
				break;
			}
		
			case NODE_ARRAY:
			{
				for (int i=numnodes-1; i>=0; i--)
					CodeGen(node(i));
				Emit(OP_PUSH_ARRAY);
				Emit(numnodes);
				break;
			}
		
			case NODE_ARRAY_GET:
			{
				CodeGen(node(0));
				CodeGen(node(1));
				Emit(OP_GET);
				break;
			}
		
			case NODE_ARRAY_SET:
			{
				CodeGen(node(0));
				CodeGen(node(1));
				CodeGen(node(2));
				Emit(OP_SET);
				break;
			}
		
			case NODE_ASSIGN:
			{
				int offset = ctx.CurScope().GetLocal(node(0)->AsString());
				CodeGen(node(1));
				Emit(OP_ASSIGN);
				Emit(offset);
				break;
			}

			case NODE_ABSTRACT:
			{
				for (int i=0; i<numnodes; i++)
					CodeGen(node(i));
				break;
			}
		
			case NODE_WHILE:
			{
				int start = (int)code.size() - 1;
				CodeGen(node(0));
				Emit(OP_BRZ);
				int patchback = ForwardJump();
				CodeGen(node(1));
				Emit(OP_BRA);
				Emit(start - (int)code.size());
				PatchForwardJump(patchback);
				break;
			}
		
			case NODE_FUNC:
			{
				AntScope* scope = ctx.CurScope().AddFunction(node(0)->AsString());
				AntNode* params = node(1);
				AntNode* locals = node(2);
				AntNode* block = node(3);
				ctx.scopeStack.push_back(scope);
			
				for (size_t i=0; i<params->children.size(); i++)
				{
					const char* name = params->children[i]->AsString();
					scope->AddParam(name);
				}
			
				for (size_t i=0; i<locals->children.size(); i++)
				{
					const char* name = locals->children[i]->AsString();
					scope->AddLocal(name);
				}
			
				Emit(OP_BRA);
				int patch = ForwardJump();
				scope->begin = (int)code.size();
				ctx.functionMap[scope->begin] = scope;
				CodeGen(block);
				PatchForwardJump(patch);
				ctx.scopeStack.pop_back();
				break;
			}
		
			case NODE_CALL:
			{
				if (strcmp(node(0)->AsString(), "print") == 0)
				{
					checknodes(2);
					CodeGen(node(1));
					Emit(OP_PRINT);
				}
				else
				{
					AntScope* func = ctx.CurScope().FindFunction(node(0)->AsString());
					for (int i=numnodes-1; i>=1; i--)
						CodeGen(node(i));
					Emit(OP_CALL);
					Emit(func->begin);
					Emit((int)func->params.size());
					Emit((int)func->locals.size());
				}
				break;
			}
		
			case NODE_RETURN:
			{
				if (numnodes > 0)
					CodeGen(node(0));
				else
				{
					Emit(OP_PUSH_INT);
					Emit(0);
				}
				Emit(OP_RETURN);
				break;
			}
		
			case NODE_LOCAL:
			{
				checknodes(2);
				int offset = ctx.CurScope().AddLocal(node(0)->AsString());
				CodeGen(node(1));
				Emit(OP_ASSIGN);
				Emit(offset);
				break;
			}
		
			case NODE_IF:
			{
				CodeGen(node(0));
				Emit(OP_BRZ);
				int patch = ForwardJump();
				CodeGen(node(1));
				Emit(OP_BRA);
				int patch2 = ForwardJump();
				PatchForwardJump(patch);
				if (numnodes == 3)
				{
					CodeGen(node(2));
					PatchForwardJump(patch2);
				}
				break;
			}
		
			#define binop(op)\
				checknodes(2);\
				CodeGen(node(0));\
				CodeGen(node(1));\
				Emit(op);\
				break
	
			case NODE_EQUAL:		binop(OP_EQUAL);
			case NODE_NOT_EQUAL:	binop(OP_NEQUAL);
			case NODE_LESS:			binop(OP_LESS);
			case NODE_LEQUAL:		binop(OP_LEQUAL);
			case NODE_GREATER:		binop(OP_GREATER);
			case NODE_GEQUAL:		binop(OP_GEQUAL);
		
			case NODE_AND:			binop(OP_AND);
			case NODE_OR:			binop(OP_OR);
			case NODE_NOT:			binop(OP_NOT);
		
			case NODE_ADD:			binop(OP_ADD);
			case NODE_SUB:			binop(OP_SUB);
			case NODE_MUL:			binop(OP_MUL);
			case NODE_DIV:			binop(OP_DIV);
			case NODE_MOD:			binop(OP_MOD);
		
			case NODE_NEG:
			{
				checknodes(1);
				AntNode* zero = new AntNode(NODE_INT);
				zero->asInt = 0;
				CodeGen(zero);
				CodeGen(node(0));
				Emit(OP_SUB);
				break;
			}
		
			default:
				Error("Unknown node type: %d", n->type);
				assert(false);
		}
	}
	catch (const AntError& e)
	{
        for (int i=0; i<(int)lines.size(); i++)
            Print("    %d: %s\n", i, lines[i].c_str());
		string msg = ReportError(n->line, n->column, e.what());
		throw exception(msg.c_str());
	}
}

void AntCodeGen::PrintCode(const AntContext& ctx, const vector<OpCode>& code)
{
	Print("\n\nCodeGen Output:\n");

	auto i = code.begin();
	while (i!=code.end())
	{
		size_t off = i - code.begin();
		Print("%4zu:\t\t", off);
	
		switch (*i++)
		{
			case OP_PUSH_INT: Print("PUSH_INT             %d", *i++); break;
			case OP_PUSH_FLOAT: Print("PUSH_FLOAT             %f", *(float*)&(*i++)); break;
			case OP_PUSH_STRING: Print("PUSH_STRING             %s", GetString(*i++)); break;
			case OP_PUSH_VAR: Print("PUSH_VAR             %d", *i++); break;
			case OP_PUSH_ARRAY: Print("PUSH_ARRAY             %d", *i++); break; 

			case OP_GET: Print("GET"); break;
			case OP_SET: Print("SET"); break;
			
			case OP_EQUAL: Print("EQUAL"); break;
			case OP_NEQUAL: Print("NEQUAL"); break;
			case OP_LESS: Print("LESS"); break;
			case OP_GREATER: Print("GREATER"); break;
			case OP_LEQUAL: Print("LEQUAL"); break;
			case OP_GEQUAL: Print("GEQUAL"); break;
			case OP_AND: Print("AND"); break;
			case OP_OR: Print("OR"); break;
			case OP_NOT: Print("NOT"); break;
			case OP_ADD: Print("ADD"); break;
			case OP_SUB: Print("SUB"); break;
			case OP_MUL: Print("MUL"); break;
			case OP_DIV: Print("DIV"); break;
			case OP_MOD: Print("MOD"); break;
			case OP_BRA: Print("BRA             %d", *i++); break;
			case OP_BRZ: Print("BRZ				%d", *i++); break;
			case OP_BNZ: Print("BNZ				%d", *i++); break;
			case OP_CALL:
			{
				int func = *i++;
				int nargs = *i++;
				int nlocals = *i++;
				AntScope* pfunc = ctx.functionMap.at(func);
				Print("CALL			%s  %d  %d", pfunc->name.c_str(), nargs, nlocals);
				break;
			}
			case OP_ASSIGN:
			{
				Print("ASSIGN			%d", *i++);
				break;
			}
			case OP_RETURN: Print("RETURN"); break;
			case OP_PRINT: Print("PRINT"); break;
				
			default:
				Print("<INVALID_OP>: %d", *(i-1));
		}

		Print("\n");
	}

	Print("DONE\n");
}
