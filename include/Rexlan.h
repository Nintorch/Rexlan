
/*
	Rexlan programming language
	Created by JustMe
*/

#ifndef _REXLAN_H_
#define _REXLAN_H_

/* Main types */

// "rlstate.h" for definition
typedef struct RL_State RL_State;
typedef void* (*RL_Malloc)(size_t size);
typedef void (*RL_Free)(void* block);

typedef char* RL_Stack;
typedef size_t RL_Offset;
typedef unsigned char RL_Opcode;
typedef unsigned char RL_PrimitiveDatatype;
typedef signed char RL_Byte;
typedef char RL_Bool;
#define RL_TRUE		1
#define RL_FALSE	0

/* Structure for holding variables and values for them */

// Do not change the name, type and the type of reference
// in this struct after compiling some code that uses this
// variable or you might get an undefined behavior later on
typedef struct RL_Value
{
	char* name;
	RL_PrimitiveDatatype type;
	union
	{
		int i;
		double d;
		RL_Byte b; // both byte and a boolean
		struct
		{
			void* ptr;
			void* type; // reserved
		} ref;
	} v;

	struct RL_Value* next;
} RL_Value;


/* Functions, both defined in C and in a script */

typedef void (*RL_CFunc)(RL_State* RL, RL_Stack* stack_ptr, char* args, void* userdata);

// Do not change anything in this struct after compiling
// some code or you might get an undefined behavior later on
typedef struct RL_Function
{
	const char* name;
	RL_PrimitiveDatatype return_type;
	int argc;
	RL_PrimitiveDatatype* arg_types;
	RL_Bool is_c; // 1 if this is a C function
	union
	{
		RL_CFunc cf;
	};
	void* userdata;

	struct RL_Function* next;
} RL_Function;


/* All the available built-in datatypes */

enum DATATYPES
{
	RL_TYPE_VOID = 0,
	RL_TYPE_INT,
	RL_TYPE_DOUBLE,
	RL_TYPE_BYTE,
	RL_TYPE_BOOL,
	RL_TYPE_REF,
};


/* Rexlan functions */

// Create a state, a simple and easy way if you don't need anything more specific
// Parameters are: stack size = 512, malloc and free functions from C stdlib
RL_State* rl_init();

// The more advanced way of creating a Rexlan state which allows
// to specify the stack size and other VM parameters
RL_State* rl_init_ext(int stack_size, RL_Malloc rl_malloc, RL_Free rl_free);

// Free the Rexlan state
void rl_quit(RL_State* RL);

// Run a script
void rl_runfile(RL_State* RL, const char* filename);
void rl_runstring(RL_State* RL, const char* s);

// Define and find a variable from C code
RL_Value* rl_definevar(RL_State* RL, const char* name, RL_PrimitiveDatatype type, void* reserved);
RL_Value* rl_findvar(RL_State* RL, const char* name);

// Define and find a function from C code
// "userdata" is then passed as an argument to "cf" when it's called
// TODO: use signature instead of name, return type and arguments here
RL_Function* rl_definecfunc(RL_State* RL, const char* name, RL_CFunc cf, RL_PrimitiveDatatype return_type, int argc, RL_PrimitiveDatatype* arg_types, void* userdata);
RL_Function* rl_findfunc(RL_State* RL, const char* name);

// Functions which push values onto a stack
void rl_pushint(RL_Stack* stack_ptr, int i);
void rl_pushdouble(RL_Stack* stack_ptr, double d);
void rl_pushbyte(RL_Stack* stack_ptr, char b);
void rl_pushbool(RL_Stack* stack_ptr, char b);

// Utils

#define RL_STATIC_ASSERT(name, cond) static int rl_static_assert_ ## name [(cond)*2-1]

#endif