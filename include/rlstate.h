
/*
	Rexlan programming language
	Created by JustMe
*/

// In order to use this file properly
// you need to #include "Rexlan.h" first

#ifndef _RLSTATE_H_
#define _RLSTATE_H_

#include "rlcompile.h"

#include <stdint.h>

#define rlS_savestacksize 32

typedef struct RL_State
{
	RL_Malloc malloc;
	RL_Free free;

	RL_Value* variables;
	RL_Function* functions;

	RL_CompilerState* compiler;

	struct
	{
		// TODO: stack overflow error
		int stack_size;
		RL_Stack main_stack;

		void* savestack[rlS_savestacksize];
		int savestack_position;
	} runtime;
} RL_State;

RL_Stack rlS_allocstack(RL_State* RL);
void rlS_freestack(RL_State* RL, RL_Stack stack);

const char* rl_gettypename(RL_State* RL, RL_PrimitiveDatatype type);
int rl_sizeof(RL_State* RL, RL_PrimitiveDatatype type);
int rl_convertable(RL_State* RL, RL_PrimitiveDatatype from, RL_PrimitiveDatatype to);

void rl_run(RL_State* RL, RL_Opcode* ms_codedata, RL_Stack stack);

#endif