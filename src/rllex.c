
/*
	Rexlan programming language
	Created by JustMe
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "Rexlan.h"
#include "rllex.h"
#include "rlstate.h"

#define arr_size(arr) (sizeof(arr)/sizeof(arr[0]))

static const char* token_str[] = {
	"(invalid)", "(identifier)", "(integer number)", "(number as double)", "(character)", "(eof)",

	"int", "double", "byte", "bool", "true", "false", "if", "else",
	"while",

	"==", "!=", ">", ">=", "<", "<=",
};

const char* rlL_tokentostr(int token)
{
	if (token >= NUMTOKENS || token < 0)
		return NULL;
	return token_str[token];
}

int rlL_isskipchar(char c)
{
	switch (c)
	{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			return 1;
	}
	return 0;
}

#define rl_src RL->compiler->src

void rlL_skipwhitespaces(RL_State* RL)
{
	while (rlL_isskipchar(*RL->compiler->src))
		rl_src++;

	// Check for comments
	if (*rl_src == '/')
	{
		if (*(rl_src + 1) == '/') // Single-line comment
		{
			while (*rl_src != 0 && *rl_src != '\n' && *rl_src != '\r') rl_src++;
			rlL_skipwhitespaces(RL);
		}
		else if (*(rl_src + 1) == '*') // Multiline comment
		{
			rl_src += 2; // skip '/*'
			while (*rl_src != 0 && (*rl_src != '*' || *(rl_src+1) != '/')) rl_src++;
			if (*rl_src != 0)
				rl_src += 2; // skip '*/'
			rlL_skipwhitespaces(RL);
		}
		// else not a comment
	}
}

int rlL_isreserved(RL_State* RL)
{
	for (int i = TK_RESERVED; i < TK_RESERVED_END; i++)
	{
		if (!strcmp(RL->compiler->current_token, token_str[i]))
			return i;
	}
	return 0;
}

void rlL_readtoken(RL_State* RL)
{
	// TODO: add buffer overflow check;
	char* s = RL->compiler->current_token;
	while (isalpha(*rl_src) || isdigit(*rl_src) || *rl_src == '_')
		*s++ = *rl_src++;
	*s = 0;
}

int rlL_readnumber(RL_State* RL)
{
	char* s = RL->compiler->current_token;
	int number_type = TK_NUMBERINT;
	while (isdigit(*rl_src) || *rl_src == '.')
	{
		if (*rl_src == '.' && number_type == TK_NUMBERINT)
			number_type = TK_NUMBERDOUBLE;
		else if (*rl_src == '.')
			; // TODO: error: 2 dots in a number
		*s++ = *rl_src++;
	}
	*s = 0;
	return number_type;
}

// TODO: make it linked so it's
// possible to add primitive
// type aliases with typedef
static struct
{
	const char* str;
	int token;
} aliases[][2] = {
	{ "char", TK_BYTE },

	// sized integers
	{ "int8", TK_BYTE },
};

static int rlL_readnext(RL_State* RL)
{
	rlL_skipwhitespaces(RL);
	if (*rl_src == 0)
		return TK_EOF;
	if (isalpha(*rl_src))
	{
		rlL_readtoken(RL);
		rlL_skipwhitespaces(RL);

		int reserved_id = rlL_isreserved(RL);
		if (reserved_id)
			return reserved_id;

		for (int i = 0; i < arr_size(aliases); i++)
		{
			if (!strcmp(aliases[i]->str, RL->compiler->current_token))
				return aliases[i]->token;
		}
		
		return TK_IDENTIFIER;
	}
	if (isdigit(*rl_src))
	{
		return rlL_readnumber(RL);
	}
	char character = *rl_src++;
	switch (character)
	{
		case '=':
			if (*rl_src == '=') { rl_src++; return TK_EQ; }
			return '=';
		case '!':
			if (*rl_src == '=') { rl_src++; return TK_NE; }
			return '!';
		case '>':
			if (*rl_src == '=') { rl_src++; return TK_GE; }
			return '>';
		case '<':
			if (*rl_src == '=') { rl_src++; return TK_LE; }
			return '<';
		case '+':
			if (*rl_src == '+') { rl_src++; return TK_PLPL; }
			return '+';
		case '-':
			if (*rl_src == '-') { rl_src++; return TK_MIMI; }
			return '-';
		case 0:
			return TK_EOF;
		default:
			return character;
	}
}

int rlL_next(RL_State* RL)
{
	return (RL->compiler->next_token = rlL_readnext(RL));
}

int rlL_expect(RL_State* RL, int token)
{
	if (RL->compiler->next_token != token)
	{
		printf("'%c' expected, got '%c' %d\n", token, RL->compiler->next_token, RL->compiler->next_token);
		return 0;
	}
	rlL_next(RL);
	return 1;
}