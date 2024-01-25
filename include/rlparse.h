
/*
	Rexlan programming language
	Created by JustMe
*/

#ifndef _RLCOMPILE_H_
#define _RLCOMPILE_H_

#include "Rexlan.h"

RL_Opcode* rl_compile(RL_State* RL, const char* src);
RL_PrimitiveDatatype rlC_eval(RL_State* RL);

#endif