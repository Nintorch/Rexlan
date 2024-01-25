
/*
	Rexlan programming language
	Created by JustMe

	rlparse.c - convert code source to bytecode instruction list
	(which is compiled to bytecode in rlopcode.c)
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "Rexlan.h"
#include "rlparse.h"
#include "rllex.h"
#include "rlcompile.h"
#include "rlstate.h"

void rlC_block(RL_State* RL);

#define rlC_typefromtoken(token) (token-TK_INT+RL_TYPE_INT)

// Jumps from current position to pos
#define rlC_forwardjump(pos) (rlO_currentposition(RL) - pos)
#define rlC_backwardjump(pos) (pos - rlO_currentposition(RL) - sizeof(RL_Opcode))

static void incdec(RL_State* RL, int value)
{
	RL_Value* v;
	if (v = rl_findvar(RL, RL->compiler->current_token))
	{
		rlOP_GET(RL, &v->v, v->type);
		switch (v->type)
		{
			case RL_TYPE_INT:
				rlOP_PUSHINT(RL, value);
				rlOP_Single(RL, RLOP_ADDI);
				break;
			case RL_TYPE_DOUBLE:
				rlOP_PUSHDOUBLE(RL, value);
				rlOP_Single(RL, RLOP_ADDD);
				break;
			case RL_TYPE_BYTE:
				rlOP_PUSHBYTE(RL, value);
				rlOP_Single(RL, RLOP_ADDB);
				break;
			default:
				printf("prefix increment unusable on variable '%s'\n", v->name);
				return;
		}
		rlOP_SET(RL, &v->v, v->type);
	}
	else
	{
		printf("Unknown identifier '%s'\n", RL->compiler->current_token);
		return;
	}
}

static void function_call(RL_State* RL, RL_Function* f)
{
	RL_PrimitiveDatatype type;
	rlL_expect(RL, '(');
	rlOP_Single(RL, RLOP_SAVESTACK);
	for (int i = 0; i < f->argc; i++)
	{
		if (rlL_accept(RL, ')'))
		{
			printf("Function '%s' accepts %d arguments but %d were passed\n",
				f->name, f->argc, i);
		}
		type = rlC_eval(RL);
		if (rl_convertable(RL, type, f->arg_types[i]))
		{
			rlOP_CAST(RL, type, f->arg_types[i]);
		}
		else
		{
			printf("Cannot convert the value of type '%d' to '%d' (the type of argument %d of function '%s')\n",
				type, f->arg_types[i], i + 1, f->name);
		}

		if (i < (f->argc - 1))
		{
			if (rlL_accept(RL, ')'))
			{
				printf("Function '%s' accepts %d arguments but %d were passed\n",
					f->name, f->argc, i + 1);
			}
			else
				rlL_expect(RL, ',');
		}
	}
	rlL_expect(RL, ')');

	if (f->is_c)
		rlOP_CALLC(RL, f->cf, f->userdata);
}

void rlC_statement(RL_State* RL);

static void block_or_statement(RL_State* RL)
{
	if (rlL_accept(RL, '{'))
		rlC_block(RL);
	else
		rlC_statement(RL);
}

static void variable_define(RL_State* RL, RL_PrimitiveDatatype type)
{
	RL_PrimitiveDatatype result;
	RL_Value* v;
loop:
	v = rl_definevar(RL, RL->compiler->current_token, type, NULL);
	rlL_next(RL);
	if (rlL_accept(RL, '='))
	{
		result = rlC_eval(RL);
		if (result != type)
		{
			if (rl_convertable(RL, result, type))
			{
				printf("Warning: type converted - %s to %s\n", rl_gettypename(RL, result), rl_gettypename(RL, type));
				rlOP_CAST(RL, result, type);
			}
			else
			{
				printf("Error: expression with type %s not convertable to type %s\n",
					rl_gettypename(RL, result), rl_gettypename(RL, type));
			}
		}
		rlOP_SET(RL, &v->v, type);
		if (rlL_accept(RL, ','))
		{
			goto loop;
		}
	}
	else if (rlL_accept(RL, ','))
	{
		goto loop;
	}
	rlL_expect(RL, ';');
}

void rlC_statement(RL_State* RL)
{
	int token = RL->compiler->next_token;
	RL_PrimitiveDatatype type;
	RL_Value* v;
	RL_Function* f;
	RL_PrimitiveDatatype result;
	RL_Offset* offset, * offset2;
	RL_Offset pos, pos2, pos3;
	const char* savesrc;

	switch (token)
	{
		case TK_IDENTIFIER:
			if (v = rl_findvar(RL, RL->compiler->current_token))
			{
				rlL_next(RL);
				if (rlL_accept(RL, '='))
				{
					type = rlC_eval(RL);
					if (rl_convertable(RL, type, v->type))
					{
						rlOP_CAST(RL, type, v->type);
						rlOP_SET(RL, &v->v, v->type);
					}
					else
					{
						printf("Cannot convert the value of type '%d' to '%d' (the type of variable '%s')\n",
							type, v->type, v->name);
					}
				}
				else if (rlL_accept(RL, TK_PLPL))
				{
					incdec(RL, 1);
				}
				else if (rlL_accept(RL, TK_MIMI))
				{
					incdec(RL, -1);
				}
			}
			else if (f = rl_findfunc(RL, RL->compiler->current_token))
			{
				rlL_next(RL);
				function_call(RL, f);
				// Ignore the return value, we're not in an expression
				rlOP_ADDSTACK(RL, -rl_sizeof(RL, f->return_type));
			}
			else
			{
				printf("Unknown identifier '%s'\n", RL->compiler->current_token);
				return;
			}
			rlL_expect(RL, ';');
			break;
		
		// Declaring a variable with primitive type
		case TK_INT:
		case TK_DOUBLE:
		case TK_BYTE:
		case TK_BOOL:
			type = rlC_typefromtoken(token);
			rlL_next(RL);
			variable_define(RL, type);
			break;
		case TK_IF:
			rlL_next(RL);
			rlL_expect(RL, '(');
			result = rlC_eval(RL);
			rlL_expect(RL, ')');
			if (!rl_convertable(RL, result, RL_TYPE_BOOL))
			{
				printf("Boolean expression expected\n");
				return;
			}
			if (result != RL_TYPE_BOOL)
			{
				rlOP_CAST(RL, result, RL_TYPE_BOOL);
			}

			offset = rlOP_JUMPFALSE(RL);
			pos2 = rlO_currentposition(RL) - sizeof(RL_Offset);
			block_or_statement(RL);
			
			savesrc = RL->compiler->src;
			if (rlL_accept(RL, TK_ELSE))
			{
				offset2 = rlOP_JUMP(RL);
				pos = rlO_currentposition(RL) - sizeof(RL_Offset);

				*offset = rlC_forwardjump(pos2);

				rlL_skipwhitespaces(RL);
				block_or_statement(RL);
				*offset2 = rlC_forwardjump(pos);
			}
			else
			{
				RL->compiler->src = savesrc;
				*offset = rlC_forwardjump(pos2);
			}
			break;
		case TK_WHILE:
			pos = rlO_currentposition(RL);

			rlL_next(RL);
			rlL_expect(RL, '(');
			result = rlC_eval(RL);
			rlL_expect(RL, ')');

			if (!rl_convertable(RL, result, RL_TYPE_BOOL))
			{
				printf("Boolean expression expected\n");
				return;
			}
			if (result != RL_TYPE_BOOL)
			{
				rlOP_CAST(RL, result, RL_TYPE_BOOL);
			}

			offset = rlOP_JUMPFALSE(RL);
			pos2 = rlO_currentposition(RL) - sizeof(RL_Offset);

			block_or_statement(RL);

			pos3 = rlC_backwardjump(pos);
			*rlOP_JUMP(RL) = pos3;
			*offset = rlC_forwardjump(pos2);

			break;
		case TK_PLPL:
			rlL_next(RL);
			incdec(RL, 1);
			rlL_next(RL);
			rlL_expect(RL, ';');
			break;
		case TK_MIMI:
			rlL_next(RL);
			incdec(RL, -1);
			rlL_next(RL);
			rlL_expect(RL, ';');
			break;
		case ';':
			return;
		case TK_EOF:
			return;
		default:
			printf("Unknown token %d '%c'\n", token, token);
			exit(1);
			return;
	}
}

void rlC_block(RL_State* RL)
{
	while (1)
	{
		if (rlL_accept(RL, '}') || rlL_accept(RL, TK_EOF))
			return;
		rlC_statement(RL);
		rlL_skipwhitespaces(RL);
	}
}

RL_Opcode* rl_compile(RL_State* RL, const char* src)
{
	RL->compiler->src = src;

	RL->compiler->codedata_head = rlC_new_opc(RL);
	RL->compiler->codedata_tail = RL->compiler->codedata_head;
	if (!RL->compiler->codedata_head)
	{
		printf("data == null\n");
		return NULL;
	}

	rlL_next(RL);
	while (*RL->compiler->src > 0)
		block_or_statement(RL);
	rlOP_Single(RL, RLOP_RET);
	return rlO_tobytecode(RL, RL->compiler->codedata_head);
}

static RL_PrimitiveDatatype primitive(RL_State* RL)
{
	int used_arith = 0;
	int negate = 1;
	RL_Value* v;
	RL_Function* f;
	const char* s;
	int i;
	double d;

	rlL_skipwhitespaces(RL);

	if (rlL_accept(RL, '+'))
	{
		used_arith = 1;
	}
	else if (rlL_accept(RL, '-'))
	{
		used_arith = 1;
		negate = -1;
	}

	if (rlL_accept(RL, '('))
	{
		RL_PrimitiveDatatype type = rlC_eval(RL);
		rlL_expect(RL, ')');
		if (used_arith)
			rlO_checkarith(RL, type);
		if (negate < 0)
			rlOP_NEG(RL, type);
		return type;
	}

	switch (RL->compiler->next_token)
	{
		case TK_IDENTIFIER:
			if (v = rl_findvar(RL, RL->compiler->current_token))
			{
				rlOP_GET(RL, &v->v, v->type);
				if (used_arith)
					rlO_checkarith(RL, v->type);
				if (negate < 0)
					rlOP_NEG(RL, v->type);
				rlL_next(RL);
				return v->type;
			}
			else if (f = rl_findfunc(RL, RL->compiler->current_token))
			{
				rlL_next(RL);
				function_call(RL, f);
				if (negate < 0)
					rlOP_NEG(RL, f->return_type);
				return f->return_type;
			}
			break;
		case TK_TRUE:
			if (used_arith)
				rlO_checkarith(RL, RL_TYPE_BOOL); // throw an error
			rlOP_PUSHBOOL(RL, RL_TRUE);
			rlL_next(RL);
			return RL_TYPE_BOOL;
		case TK_FALSE:
			if (used_arith)
				rlO_checkarith(RL, RL_TYPE_BOOL); // throw an error
			rlOP_PUSHBOOL(RL, RL_FALSE);
			rlL_next(RL);
			return RL_TYPE_BOOL;
		case TK_NUMBERINT:
			s = RL->compiler->current_token;
			i = strtol(s, &s, 10);
			rlOP_PUSHINT(RL, i * negate);
			rlL_next(RL);
			return RL_TYPE_INT;
		case TK_NUMBERDOUBLE:
			s = RL->compiler->current_token;
			d = strtod(s, &s);
			rlOP_PUSHDOUBLE(RL, d * negate);
			rlL_next(RL);
			return RL_TYPE_DOUBLE;
		default:
			printf("Some error in ms_opcode.c - primitive() '%c' %d\n", (char)RL->compiler->next_token, RL->compiler->next_token);
			return RL_TYPE_VOID;
	}
	printf("Some error 2 in ms_opcode.c - primitive()\n");
	return RL_TYPE_VOID;
}

static int RLOP_eval_do(RL_State* RL, RL_Opcode i, RL_Opcode d, RL_Opcode b, RL_PrimitiveDatatype type1, RL_PrimitiveDatatype type2)
{
	if (type1 == type2 && type1 == RL_TYPE_INT)
	{
		rlOP_Single(RL, i);
	}
	else if (type1 == type2 && type1 == RL_TYPE_DOUBLE)
	{
		rlOP_Single(RL, d);
	}
	else if (type1 == type2 && type1 == RL_TYPE_BYTE || type1 == RL_TYPE_BOOL)
	{
		rlOP_Single(RL, b);
	}
	else if (type1 == RL_TYPE_INT && type2 != RL_TYPE_INT && rl_convertable(RL, type2, RL_TYPE_INT))
	{
		rlOP_CAST(RL, type2, RL_TYPE_INT);
		rlOP_Single(RL, i);
	}
	else if (type1 == RL_TYPE_DOUBLE && type2 != RL_TYPE_DOUBLE && rl_convertable(RL, type2, RL_TYPE_DOUBLE))
	{
		rlOP_CAST(RL, type2, RL_TYPE_DOUBLE);
		rlOP_Single(RL, d);
	}
	else if (type1 == RL_TYPE_BYTE && type2 != RL_TYPE_BYTE && rl_convertable(RL, type2, RL_TYPE_BYTE))
	{
		rlOP_CAST(RL, type2, RL_TYPE_BYTE);
		rlOP_Single(RL, b);
	}
	else
	{
		printf("Some error in RLOP_eval_do\n"); // TODO: better error reporting
		return 1;
	}
	return 0;
}

static RL_PrimitiveDatatype multiply(RL_State* RL)
{
	RL_PrimitiveDatatype type1 = primitive(RL),
		type2 = RL_TYPE_VOID;
	while (RL_TRUE)
	{
		if (rlL_accept(RL, '*'))
		{
			type2 = primitive(RL);
			if (RLOP_eval_do(RL, RLOP_MULI, RLOP_MULD, RLOP_MULB, type1, type2))
				return RL_TYPE_VOID;
		}
		else if rlL_accept(RL, '/')
		{
			type2 = primitive(RL);
			if (RLOP_eval_do(RL, RLOP_DIVI, RLOP_DIVD, RLOP_DIVB, type1, type2))
				return RL_TYPE_VOID;
		}
		else if rlL_accept(RL, '%')
		{
			type2 = primitive(RL);
			if (RLOP_eval_do(RL, RLOP_MODI, RLOP_MODD, RLOP_MODB, type1, type2))
				return RL_TYPE_VOID;
		}
		else break;
	}
	return type1;
}

static RL_PrimitiveDatatype sum(RL_State* RL)
{
	RL_PrimitiveDatatype type1 = multiply(RL),
		type2 = RL_TYPE_VOID;
	while (1)
	{
		if (rlL_accept(RL, '+'))
		{
			type2 = multiply(RL);
			if (RLOP_eval_do(RL, RLOP_ADDI, RLOP_ADDD, RLOP_ADDB, type1, type2))
				return RL_TYPE_VOID;
		}
		else if (rlL_accept(RL, '-'))
		{
			type2 = multiply(RL);
			if (RLOP_eval_do(RL, RLOP_SUBI, RLOP_SUBD, RLOP_SUBB, type1, type2))
				return RL_TYPE_VOID;
		}
		else break;
	}
	return type1;
}

//static int is_comparison()
//{
//	switch (*RL->compiler->src)
//	{
//		case ';':
//			return 0;
//		case '=':
//			if (*(RL->compiler->src + 1) == '=')
//			{
//				RL->compiler->src += 2;
//				msL_skipwhitespaces();
//				return TK_EQ;
//			}
//			break;
//		case '!':
//			if (*(RL->compiler->src + 1) == '=')
//			{
//				RL->compiler->src += 2;
//				msL_skipwhitespaces();
//				return TK_NE;
//			}
//			break;
//		case '>':
//			if (*(RL->compiler->src + 1) == '=')
//			{
//				RL->compiler->src += 2;
//				msL_skipwhitespaces();
//				return TK_GE;
//			}
//			RL->compiler->src++;
//			msL_skipwhitespaces();
//			return TK_GR;
//		case '<':
//			if (*(RL->compiler->src + 1) == '=')
//			{
//				RL->compiler->src += 2;
//				msL_skipwhitespaces();
//				return TK_LE;
//			}
//			RL->compiler->src++;
//			msL_skipwhitespaces();
//			return TK_LS;
//	}
//	return 0;
//}

static int is_comparison(RL_State* RL)
{
	switch (RL->compiler->next_token)
	{
		case TK_EQ:
		case TK_NE:
		case TK_GE:
		case TK_LE:
		case '>':
		case '<':
			return 1;
		default:
			return 0;
	}
}

#define rlO_compcase(op) \
	case TK_ ## op: \
		if (RLOP_eval_do(RL, RLOP_ ## op ## I, RLOP_ ## op ## D, RLOP_ ## op ## B, type1, type2)) \
			return RL_TYPE_VOID; \
		break;

#define TK_GR '>'
#define TK_LS '<'

static RL_PrimitiveDatatype comparison(RL_State* RL)
{
	RL_PrimitiveDatatype type1 = sum(RL),
		type2 = RL_TYPE_VOID;
	int done_comparison = 0;
	int comparison = 0;

	while (is_comparison(RL))
	{
		comparison = RL->compiler->next_token;
		rlL_next(RL);
		type2 = sum(RL);
		switch (comparison)
		{
			rlO_compcase(EQ);
			rlO_compcase(NE);
			rlO_compcase(GR);
			rlO_compcase(GE);
			rlO_compcase(LS);
			rlO_compcase(LE);
		}
		type1 = RL_TYPE_BOOL;
	}
	return type1;
}

RL_PrimitiveDatatype rlC_eval(RL_State* RL)
{
	rlL_skipwhitespaces(RL);
	return comparison(RL);
}