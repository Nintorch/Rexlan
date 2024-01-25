
/*
	Rexlan programming language
	Created by JustMe

	rlcompile.c - instruction list to bytecode compiler
*/

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "Rexlan.h"
#include "rlcompile.h"
#include "rllex.h"
#include "rlstate.h"
#include "rlparse.h"

#define rl_inst RL->compiler->codedata_tail
#define rl_opcode RL->compiler->opcode_data
#define rl_opcodeloc RL->compiler->opcode_loc

RL_InstructionList* rlC_new_opc(RL_State* RL)
{
	RL_InstructionList* value = (RL_InstructionList*)RL->malloc(sizeof(RL_InstructionList));
	if (!value)
	{
		printf("Error allocating new opcode list entry\n");
		return NULL;
	}
	value->opcode = RLOP_RET;
	value->next = NULL;
	return value;
}

RL_CompilerState* rlC_new_compstate(RL_State* RL)
{
	RL_CompilerState* cs = (RL_CompilerState*)RL->malloc(sizeof(RL_CompilerState));
	cs->src = NULL;
	memset(cs->current_token, 0, sizeof(cs->current_token));
	cs->next_token = 0;
	cs->codedata_head = NULL;
	cs->codedata_tail = NULL;
	cs->opcode_data = NULL;
	cs->opcode_loc = 0;
	return cs;
}

static size_t rlO_instsize(RL_State* RL, RL_Opcode op)
{
	switch (op)
	{
		case RLOP_GETINT:
		case RLOP_SETINTSTACK:
		case RLOP_GETDOUBLE:
		case RLOP_SETDOUBLESTACK:
		case RLOP_GETBYTE:
		case RLOP_SETBYTESTACK:
			return sizeof(op) + sizeof(rl_inst->args.addr);

		case RLOP_PUSHINT:
			return sizeof(op) + sizeof(int);
		case RLOP_PUSHDOUBLE:
			return sizeof(op) + sizeof(double);
		case RLOP_PUSHBYTE:
			return sizeof(op) + sizeof(RL_Byte);

		case RLOP_CAST:
			return sizeof(op) + 2 * sizeof(RL_PrimitiveDatatype);

		case RLOP_CALLC:
			return sizeof(op) + sizeof(rl_inst->args.c_call.cfunc);
		case RLOP_CALLCUD:
			return sizeof(op)
				+ sizeof(rl_inst->args.c_call.cfunc)
				+ sizeof(rl_inst->args.c_call.ud);

		case RLOP_JUMP:
		case RLOP_JUMPTRUE:
		case RLOP_JUMPFALSE:
		case RLOP_ADDSTACK:
			return sizeof(op) + sizeof(rl_inst->args.offset);

		default:
			return sizeof(op);
	}
}

RL_Opcode* rlO_tobytecode(RL_State* RL, RL_InstructionList* il)
{
	size_t bcsize = 0;

	for (RL_InstructionList* i = il; i; i = i->next)
		bcsize += rlO_instsize(RL, i->opcode);

	rl_opcode = (RL_Opcode*)RL->malloc(bcsize);
	if (!rl_opcode)
	{
		printf("Error allocating opcode data in rlopcode.c rlO_tobytecode\n");
		return NULL;
	}

	RL_Opcode* data = rl_opcode;
	for (RL_InstructionList* i = il; i; i = i->next)
	{
		RL_Opcode op = i->opcode;
		*data++ = op;
		switch (op)
		{
			case RLOP_GETINT:
			case RLOP_SETINTSTACK:
			case RLOP_GETDOUBLE:
			case RLOP_SETDOUBLESTACK:
			case RLOP_GETBYTE:
			case RLOP_SETBYTESTACK:
				*(void**)data = i->args.addr;
				data += sizeof(void*);
				break;

			case RLOP_PUSHINT:
				*(int*)data = i->args.integer;
				data += sizeof(int);
				break;
			case RLOP_PUSHDOUBLE:
				*(double*)data = i->args.dbl;
				data += sizeof(double);
				break;
			case RLOP_PUSHBYTE:
				*(RL_Byte*)data = i->args.byte;
				data += sizeof(RL_Byte);
				break;

			case RLOP_CAST:
				*(RL_PrimitiveDatatype*)data = i->args.cast.from;
				data += sizeof(RL_PrimitiveDatatype);
				*(RL_PrimitiveDatatype*)data++ = i->args.cast.to;
				data += sizeof(RL_PrimitiveDatatype);
				break;

			case RLOP_CALLC:
			case RLOP_CALLCUD:
				*(RL_CFunc*)data = i->args.c_call.cfunc;
				data += sizeof(RL_CFunc);
				if (op == RLOP_CALLCUD)
				{
					*(void**)data = i->args.c_call.ud;
					data += sizeof(void*);
				}
				break;

			case RLOP_JUMP:
			case RLOP_JUMPTRUE:
			case RLOP_JUMPFALSE:
			case RLOP_ADDSTACK:
				*(RL_Offset*)data = i->args.offset;
				data += sizeof(RL_Offset);
				break;
		}
	}

	return rl_opcode;
}

static void rlO_nextinst(RL_State* RL)
{
	RL->compiler->opcode_loc += rlO_instsize(RL, rl_inst->opcode);

	RL_InstructionList* il = rlC_new_opc(RL);
	rl_inst->next = il;
	rl_inst = il;
}

RL_Offset rlO_currentposition(RL_State* RL)
{
	return RL->compiler->opcode_loc;
}

void rlO_checkarith(RL_State* RL, RL_PrimitiveDatatype type)
{
	switch (type)
	{
		case RL_TYPE_INT:
		case RL_TYPE_DOUBLE:
		case RL_TYPE_BYTE:
			break;
		default:
			printf("Unsupported type for arithmetic operations: %s\n", ""/*rl_gettypename(type)*/);
			return;
	}
}

void rlOP_GETINT(RL_State* RL, int* in)
{
	rl_inst->opcode = RLOP_GETINT;
	rl_inst->args.addr = (void*)in;
	rlO_nextinst(RL);
}

void rlOP_PUSHINT(RL_State* RL, int i)
{
	rl_inst->opcode = RLOP_PUSHINT;
	rl_inst->args.integer = i;
	rlO_nextinst(RL);
}

void rlOP_SETINTSTACK(RL_State* RL, int* out)
{
	rl_inst->opcode = RLOP_SETINTSTACK;
	rl_inst->args.addr = (void*)out;
	rlO_nextinst(RL);
}

void rlOP_GETDOUBLE(RL_State* RL, double* in)
{
	rl_inst->opcode = RLOP_GETDOUBLE;
	rl_inst->args.addr = (void*)in;
	rlO_nextinst(RL);
}

void rlOP_PUSHDOUBLE(RL_State* RL, double d)
{
	rl_inst->opcode = RLOP_PUSHDOUBLE;
	rl_inst->args.dbl = d;
	rlO_nextinst(RL);
}

void rlOP_SETDOUBLESTACK(RL_State* RL, double* out)
{
	rl_inst->opcode = RLOP_SETDOUBLESTACK;
	rl_inst->args.addr = (void*)out;
	rlO_nextinst(RL);
}

void rlOP_GETBYTE(RL_State* RL, RL_Byte* in)
{
	rl_inst->opcode = RLOP_GETBYTE;
	rl_inst->args.addr = (void*)in;
	rlO_nextinst(RL);
}

void rlOP_PUSHBYTE(RL_State* RL, RL_Byte b)
{
	rl_inst->opcode = RLOP_PUSHBYTE;
	rl_inst->args.byte = b;
	rlO_nextinst(RL);
}

void rlOP_SETBYTESTACK(RL_State* RL, RL_Byte* out)
{
	rl_inst->opcode = RLOP_SETBYTESTACK;
	rl_inst->args.addr = (void*)out;
	rlO_nextinst(RL);
}

void rlOP_PUSHBOOL(RL_State* RL, int b)
{
	if (b)
		rlOP_Single(RL, RLOP_PUSHTRUE);
	else
		rlOP_Single(RL, RLOP_PUSHFALSE);
}

void rlOP_SET(RL_State* RL, void* out, RL_PrimitiveDatatype type)
{
	switch (type)
	{
		case RL_TYPE_INT:
			rlOP_SETINTSTACK(RL, out);
			break;
		case RL_TYPE_DOUBLE:
			rlOP_SETDOUBLESTACK(RL, out);
			break;
		case RL_TYPE_BYTE:
		case RL_TYPE_BOOL:
			rlOP_SETBYTESTACK(RL, out);
			break;
		default:
			printf("Unsupported type for SET operation: %s\n", ""/*rl_gettypename(RL, type)*/);
			return;
	}
}

void rlOP_GET(RL_State* RL, void* in, RL_PrimitiveDatatype type)
{
	switch (type)
	{
		case RL_TYPE_INT:
			rlOP_GETINT(RL, in);
			break;
		case RL_TYPE_DOUBLE:
			rlOP_GETDOUBLE(RL, in);
			break;
		case RL_TYPE_BYTE:
		case RL_TYPE_BOOL:
			rlOP_GETBYTE(RL, in);
			break;
		default:
			printf("Unsupported type for GET operation: %s\n", ""/*rl_gettypename(type)*/);
			return;
	}
}

void rlOP_NEG(RL_State* RL, RL_PrimitiveDatatype type)
{
	switch (type)
	{
		case RL_TYPE_INT:
			rlOP_Single(RL, RLOP_NEGI);
			break;
		case RL_TYPE_DOUBLE:
			rlOP_Single(RL, RLOP_NEGD);
			break;
		case RL_TYPE_BYTE:
			rlOP_Single(RL, RLOP_NEGB);
			break;
		default:
			printf("Unsupported type for arithmetic operation: %d\n", type);
			return;
	}
}

void rlOP_CAST(RL_State* RL, RL_PrimitiveDatatype from, RL_PrimitiveDatatype to)
{
	if (from == RL_TYPE_BOOL)
		from = RL_TYPE_BYTE;

	if (from == to || (from == RL_TYPE_BYTE && to == RL_TYPE_BOOL))
		return;

	rl_inst->opcode = RLOP_CAST;
	rl_inst->args.cast.from = from;
	rl_inst->args.cast.to = to;
}

void rlOP_CALLC(RL_State* RL, RL_CFunc f, void* userdata)
{
	if (!userdata)
	{
		rl_inst->opcode = RLOP_CALLC;
		rl_inst->args.c_call.cfunc = f;
	}
	else
	{
		rl_inst->opcode = RLOP_CALLCUD;
		rl_inst->args.c_call.cfunc = f;
		rl_inst->args.c_call.ud = userdata;
	}
	rlO_nextinst(RL);
}

RL_Offset* rlOP_JUMP(RL_State* RL)
{
	rl_inst->opcode = RLOP_JUMP;
	rl_inst->args.offset = 0;

	RL_Offset* offset_ptr = &rl_inst->args.offset;
	rlO_nextinst(RL);

	return offset_ptr;
}

RL_Offset* rlOP_JUMPTRUE(RL_State* RL)
{
	rl_inst->opcode = RLOP_JUMPTRUE;
	rl_inst->args.offset = 0;

	RL_Offset* offset_ptr = &rl_inst->args.offset;
	rlO_nextinst(RL);

	return offset_ptr;
}

RL_Offset* rlOP_JUMPFALSE(RL_State* RL)
{
	rl_inst->opcode = RLOP_JUMPFALSE;
	rl_inst->args.offset = 0;

	RL_Offset* offset_ptr = &rl_inst->args.offset;
	rlO_nextinst(RL);

	return offset_ptr;
}

void rlOP_ADDSTACK(RL_State* RL, RL_Offset o)
{
	if (o == 0)
		return;

	rl_inst->opcode = RLOP_ADDSTACK;
	rl_inst->args.offset = o;
	rlO_nextinst(RL);
}

void rlOP_Single(RL_State* RL, RL_Opcode op)
{
	rl_inst->opcode = op;
	rlO_nextinst(RL);
}