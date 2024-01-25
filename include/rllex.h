
/*
	Rexlan programming language
	Created by JustMe
*/

// In order to use this file properly
// you need to #include "Rexlan.h" first

#ifndef _RLLEX_H_
#define _RLLEX_H_

enum TOKENS
{
	TK_INVALID = 0,
	TK_IDENTIFIER,
	TK_NUMBERINT,
	TK_NUMBERDOUBLE,
	TK_UNKCHAR,
	TK_EOF,

	TK_RESERVED, // reserved words start
	TK_INT = TK_RESERVED,
	TK_DOUBLE,
	TK_BYTE,
	TK_BOOL,
	TK_TRUE,
	TK_FALSE,
	TK_IF,
	TK_ELSE,
	TK_WHILE,
	TK_RESERVED_END,

	TK_EQ = TK_RESERVED_END, // ==
	TK_NE, // !=
	TK_GE, // >=
	TK_LE, // <=

	TK_PLPL, // ++
	TK_MIMI, // --

	NUMTOKENS,
};

int rlL_next(RL_State* RL);
int rlL_expect(RL_State* RL, int token);
void rlL_skipwhitespaces(RL_State* RL);
#define rlL_accept(RL, token) (RL->compiler->next_token == token ? rlL_next(RL) : 0)

#endif