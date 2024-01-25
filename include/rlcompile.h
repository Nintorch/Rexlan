
/*
	Rexlan programming language
	Created by JustMe
*/

#ifndef _RLOPCODE_H_
#define _RLOPCODE_H_

#include "Rexlan.h"
#include <stdint.h>

#define rlO_primitive(upper) \
	RLOP_GET ## upper, \
	RLOP_PUSH ## upper, \
	RLOP_SET ## upper ## STACK, \
	RLOP_SET ## upper

#define rlO_ops(type) \
	RLOP_ADD ## type, \
	RLOP_SUB ## type, \
	RLOP_MUL ## type, \
	RLOP_DIV ## type, \
	RLOP_MOD ## type, \
	RLOP_NEG ## type, \
	RLOP_EQ ## type, \
	RLOP_NE ## type, \
	RLOP_GR ## type, \
	RLOP_GE ## type, \
	RLOP_LS ## type, \
	RLOP_LE ## type

enum RL_OPCODES
{
	RLOP_RET = 0,

	// Datatypes
	rlO_primitive(INT),
	rlO_primitive(DOUBLE),
	rlO_primitive(BYTE),

	RLOP_PUSHTRUE,
	RLOP_PUSHFALSE,

	RLOP_CAST, // Convert between primitive types

	rlO_ops(I), // integer operations
	rlO_ops(D), // double operations
	rlO_ops(B), // byte operations

	RLOP_SAVESTACK,

	RLOP_CALLC,
	RLOP_CALLCUD, // RLOP_CALLC with UserData

	RLOP_JUMP,
	RLOP_JUMPTRUE,
	RLOP_JUMPFALSE,

	RLOP_ADDSTACK,

	NUMOPCODES,
};

typedef struct RL_InstructionList
{
	RL_Opcode opcode;
	union
	{
		RL_Offset offset;

		int integer;
		RL_Byte byte;
		double dbl;

		struct
		{
			RL_CFunc cfunc;
			void* ud;
		} c_call;

		struct
		{
			RL_PrimitiveDatatype from, to;
		} cast;

		void* addr;
	} args;
	struct RL_InstructionList* next;
} RL_InstructionList;

typedef struct RL_CompilerState
{
	const char* src;
	char current_token[256];
	int next_token;
	RL_InstructionList* codedata_head;
	RL_InstructionList* codedata_tail;
	RL_Opcode* opcode_data;
	RL_Offset opcode_loc;
} RL_CompilerState;

RL_InstructionList* rlC_new_opc(RL_State* RL);
RL_CompilerState* rlC_new_compstate(RL_State* RL);

RL_STATIC_ASSERT(opcode_count, NUMOPCODES < 256);

RL_Offset rlO_currentposition(RL_State* RL);
void rlO_checkarith(RL_State* RL, RL_PrimitiveDatatype type);
RL_Opcode* rlO_tobytecode(RL_State* RL, RL_InstructionList* il);

void rlOP_GETINT(RL_State* RL, int* in);
void rlOP_PUSHINT(RL_State* RL, int i);
void rlOP_SETINTSTACK(RL_State* RL, int* out);
void rlOP_GETDOUBLE(RL_State* RL, double* in);
void rlOP_PUSHDOUBLE(RL_State* RL, double i);
void rlOP_SETDOUBLESTACK(RL_State* RL, double* out);
void rlOP_GETBYTE(RL_State* RL, RL_Byte* in);
void rlOP_PUSHBYTE(RL_State* RL, RL_Byte b);
void rlOP_SETBYTESTACK(RL_State* RL, RL_Byte* out);
void rlOP_PUSHBOOL(RL_State* RL, int b);

void rlOP_SET(RL_State* RL, void* out, RL_PrimitiveDatatype type);
void rlOP_GET(RL_State* RL, void* in, RL_PrimitiveDatatype type);
void rlOP_NEG(RL_State* RL, RL_PrimitiveDatatype type);
void rlOP_CAST(RL_State* RL, RL_PrimitiveDatatype from, RL_PrimitiveDatatype to);
void rlOP_CALLC(RL_State* RL, RL_CFunc f, void* userdata);
RL_Offset* rlOP_JUMP(RL_State* RL);
RL_Offset* rlOP_JUMPTRUE(RL_State* RL);
RL_Offset* rlOP_JUMPFALSE(RL_State* RL);
void rlOP_ADDSTACK(RL_State* RL, RL_Offset o);

void rlOP_Single(RL_State* RL, RL_Opcode op);

#endif