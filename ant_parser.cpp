#include "ant_pch.h"
#include "ant.h"

AntParser::AntParser(const char* src): source(src), lex(src)
{
	// Split source code into lines
	string_view tail = src;

	while (!tail.empty())
	{
		size_t eol = tail.find('\n');
		if (eol == tail.npos) break;
		lines.push_back(string(tail.substr(0, eol)));
		tail = tail.substr(eol+1, source.size());
	}

	try
	{
		root = new AntNode();
		lex.Next();

		do
		{
			root->Add(Statement());
			Expect(';');
		}
		while (lex.token != 'eof');
	}
	catch (const AntError& e)
	{
		string msg = ReportError(lex.line, lex.context.c_str(), e.what());
		throw exception(msg.c_str());
	}
}

AntParser::~AntParser()
{
	delete root;
}

void AntParser::PrintTree()
{
	if (!root)
		return;
	
	root->PrintNode();
	Print("\n");
}

AntNode* AntParser::Function()
{
	Expect('func');
	AntNode* func = new AntNode(NODE_FUNC);
	AntNode* name = new AntNode(NODE_ID);
	AntNode* params = new AntNode(NODE_FUNC_PARAMS);
	AntNode* locals = new AntNode(NODE_FUNC_LOCALS);
	func->Add(name);
	func->Add(params);
	func->Add(locals);
	
	if (lex.token == 'id')
	{
		name->SetString(lex.strToken.c_str());
		lex.Next();
	}
	else
		name->SetString("anonymous");
		
	Expect('(');
	
	while (lex.token != ')')
	{
		params->Add(Identifier());
		
		if (lex.token != ')')
			Expect(',');
	}
	
	Expect(')');
	func->Add(Block());
	return func;
}

AntNode* AntParser::Statement()
{
	AntNode* ret;
	
	switch (lex.token)
	{
		case 'func':
			ret = Function();
			break;
			
		case 'if':
			ret = new AntNode(NODE_IF);
			Expect('if');
			Expect('(');
			ret->Add(Expression());
			Expect(')');
			ret->Add(Statement());
			
			if (lex.token == 'else')
			{
				lex.Next();
				ret->Add(Statement());
			}
			
			break;
		
		case 'whle':
			ret = new AntNode(NODE_WHILE);
			lex.Next();
			Expect('(');
			ret->Add(Expression());
			Expect(')');
			ret->Add(Statement());
			break;
			
		case 'do':
			ret = new AntNode(NODE_DO_WHILE);
			lex.Next();
			ret->Add(Statement());
			Expect('whle');
			ret->Add(Expression());
			break;
			
		case 'frch':
			ret = new AntNode(NODE_FOREACH);
			lex.Next();
			Expect('(');
			ret->Add(Identifier());
			Expect('in');
			ret->Add(Expression());
			Expect(')');
			ret->Add(Statement());
			break;
			
		case 'brk':
			lex.Next();
			ret = new AntNode(NODE_BREAK);
			break;
			
		case '{':
			ret = Block();
			break;
		
		case 'locl':
			lex.Next();
			ret = new AntNode(NODE_LOCAL);
			ret->Add(Identifier());
			if (lex.token == '=')
			{
				lex.Next();
				ret->Add(Expression());
			}
			else
			{
				AntNode* node = new AntNode(NODE_INT);
				node->asInt = 0;
				ret->Add(node);
			}
			break;
			
		case 'ret':
			lex.Next();
			ret = new AntNode(NODE_RETURN);
			if (lex.token != ';') ret->Add(Expression());
			break;
			
		default:
		{
			ret = Expression();
					
			if (lex.token == '=')
			{
				if (ret->type != NODE_ID)
					Error("expected identifier");
				
				AntNode* assignment = new AntNode(NODE_ASSIGN);
				assignment->Add(ret);
				lex.Next();
				assignment->Add(Expression());
				return assignment;
			}
		}
	}
	
	return ret;
}

AntNode* AntParser::Block()
{
	Expect('{');
	AntNode* block = new AntNode(NODE_ABSTRACT);
	
	while (lex.token != '}')
	{
		block->Add(Statement());
		Expect(';');
	}
	
	lex.Next();
	return block;
}

AntNode* AntParser::Expression()
{
	AntNode* a = Expression2();
	
	switch (lex.token)
	{
		case 'and': lex.Next(); return BinaryOp(NODE_AND, a, Expression());
		case 'or': lex.Next(); return BinaryOp(NODE_OR, a, Expression());
		default: return a;
	}
}

AntNode* AntParser::Expression2()
{
	AntNode* a = Expression3();
	
	switch (lex.token)
	{
		case '==': lex.Next(); return BinaryOp(NODE_EQUAL, a, Expression2());
		case '!=': lex.Next(); return BinaryOp(NODE_NOT_EQUAL, a, Expression2());
		case '<': lex.Next(); return BinaryOp(NODE_LESS, a, Expression2());
		case '>': lex.Next(); return BinaryOp(NODE_GREATER, a, Expression2());
		case '<=': lex.Next(); return BinaryOp(NODE_LEQUAL, a, Expression2());
		case '>=': lex.Next(); return BinaryOp(NODE_GEQUAL, a, Expression2());
		default: return a;
	}
}

AntNode* AntParser::Expression3()
{
	AntNode* a = Expression4();
	
	switch (lex.token)
	{
		case '+': lex.Next(); return BinaryOp(NODE_ADD, a, Expression3());
		case '-': lex.Next(); return BinaryOp(NODE_SUB, a, Expression3());
		case '$': lex.Next(); return BinaryOp(NODE_CAT, a, Expression3());
		default: return a;
	}
}

AntNode* AntParser::Expression4()
{
	AntNode* a = Factor();
	
	switch (lex.token)
	{
		case '*': lex.Next(); return BinaryOp(NODE_MUL, a, Expression4());
		case '/': lex.Next(); return BinaryOp(NODE_DIV, a, Expression4());
		case '%': lex.Next(); return BinaryOp(NODE_MOD, a, Expression4());
		default: return a;
	}
}

AntNode* IntNode(int x)
{
	AntNode* n = new AntNode(NODE_INT);
	n->asInt = x;
	return n;
}

AntNode* FloatNode(float x)
{
	AntNode* n = new AntNode(NODE_FLOAT);
	n->asFloat = x;
	return n;
}

AntNode* StringNode(const char* x)
{
	AntNode* n = new AntNode(NODE_STRING);
	n->SetString(x);
	return n;
}

AntNode* IdNode(const char* x)
{
	AntNode* n = new AntNode(NODE_ID);
	n->SetString(x);
	return n;
}

AntNode* AntParser::Factor()
{
	AntNode* factor = nullptr;
	
	switch (lex.token)
	{
		case '(':
			lex.Next();
			factor = Expression();
			Expect(')');
			break;
			
		case 'true': factor=new AntNode(NODE_TRUE); factor->asInt=1; lex.Next(); break;
		case 'fals': factor=new AntNode(NODE_TRUE); factor->asInt=0; lex.Next(); break;
		case 'int': factor=new AntNode(NODE_INT); factor->asInt=lex.intToken; lex.Next(); break;
		case 'flt': factor=new AntNode(NODE_FLOAT); factor->asFloat=lex.fltToken; lex.Next(); break;
		case 'str': factor=new AntNode(NODE_STRING); factor->asInt=GetID(lex.strToken.c_str()); lex.Next(); break;

		case 'id':
			factor = new AntNode(NODE_ID);
			factor->SetString(lex.strToken.c_str());
			lex.Next();
			
			if (lex.token == '(')
			{
				AntNode* call = new AntNode(NODE_CALL);
				call->Add(factor);
				lex.Next();
				
				if (lex.token != ')')
				{
					call->Add(Expression());
					
					while (lex.token == ',')
					{
						lex.Next();
						call->Add(Expression());
					}
				}
				
				Expect(')');
				return call;
			}
			else if (lex.token == '[')
			{
				AntNode* index = new AntNode;
				index->Add(factor);
				lex.Next();
				
				index->Add(Expression());
				Expect(']');
				
				if (lex.token == '=')
				{
					lex.Next();
					index->Add(Expression());
					index->type = NODE_ARRAY_SET;
				}
				else
					index->type = NODE_ARRAY_GET;
				
				return index;
			}
			
			break;
			
		//case 'true':
		//	lex.Next();
		//	factor=new AntNode(NODE_INT);
		//	factor->asInt=1;
		//	break;

		case '-':
			factor = new AntNode(NODE_NEG);
			lex.Next();
			factor->Add(Factor());
			break;
			
		case 'not':
			factor = new AntNode(NODE_NOT);
			lex.Next();
			factor->Add(Expression());
			break;
			
		case '[':
			factor = new AntNode(NODE_ARRAY);
			lex.Next();
			while (lex.token != ']')
			{
				factor->Add(Factor());
				if (lex.token != ']')
				{
					Expect(',');
				}
			}
			lex.Next();
			break;
		
		default:
			throw AntError("Invalid factor");
			break;
	}
	
	return factor;
}

AntNode* AntParser::Identifier()
{
	if (lex.token != 'id')
		throw AntError("Expected identifier, found '%s'", TokToStr(lex.token));
		
	AntNode* n = new AntNode(NODE_ID);
	n->SetString(lex.strToken.c_str());
	lex.Next();
	return n;
}

AntNode* AntParser::BinaryOp(AntNodeType type, AntNode* a, AntNode* b)
{
	AntNode* op = new AntNode(type);
	op->Add(a);
	op->Add(b);
	return op;
}

void AntParser::Expect(int token)
{
	if (lex.token != token)
	{
		string expected = TokToStr(token);
		string got = TokToStr(lex.token);
		throw AntError("Expected '%s', found '%s'", expected.c_str(), got.c_str());
	}
	
	lex.Next();
}
