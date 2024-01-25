
/*
	Rexlan programming language
	Created by JustMe
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "Rexlan.h"
#include "rlstate.h"
#include "rlcompile.h"
#include "rllex.h"
#include "rlparse.h"

static void free_function(RL_State* RL, RL_Function* f);
static void free_value(RL_State* RL, RL_Value* v);

static void* default_malloc(size_t size)
{
	return malloc(size);
}

static void default_free(void* block)
{
	free(block);
}

RL_State* rl_init()
{
	return rl_init_ext(512, default_malloc, default_free);
}

RL_State* rl_init_ext(int stack_size, RL_Malloc rl_malloc, RL_Free rl_free)
{
	RL_State* RL = (RL_State*)rl_malloc(sizeof(RL_State));
	if (!RL)
	{
		printf("Error allocating Rexlan state\n");
		return NULL;
	}

	RL->malloc = rl_malloc;
	RL->free = rl_free;

	RL->runtime.stack_size = stack_size;
	RL->runtime.main_stack = rlS_allocstack(RL);
	if (!RL->runtime.main_stack)
	{
		printf("Error allocating Rexlan stack\n");
		return NULL;
	}

	RL->variables = NULL;
	RL->functions = NULL;
	RL->runtime.savestack_position = 0;

	RL->compiler = rlC_new_compstate(RL);
	return RL;
}

void rl_quit(RL_State* RL)
{
	RL_Function* f = RL->functions, *nextf;
	RL_Value* v = RL->variables, *nextv;

	rlS_freestack(RL, RL->runtime.main_stack);
	RL->free(RL->compiler->codedata_head);

	while (f) {
		nextf = f->next;
		free_function(RL, f);
		f = nextf;
	}

	while (v) {
		nextv = v->next;
		free_value(RL, v);
		v = nextv;
	}

	RL->free(RL);
}

RL_Stack rlS_allocstack(RL_State* RL)
{
	return (RL_Stack)RL->malloc(RL->runtime.stack_size);
}

void rlS_freestack(RL_State* RL, RL_Stack stack)
{
	RL->free(stack);
}

RL_Value* rl_definevar(RL_State* RL, const char* name, RL_PrimitiveDatatype type, void* reserved)
{
	RL_Value* v = rl_findvar(RL, name);
	if (v)
	{
		if (v->type == type)
		{
			printf("Warning: variable '%s' is already defined with the same type\n", name);
			return v;
		}
		else
		{
			printf("Variable '%s' is already defined with different type\n", name);
			return NULL;
		}
	}

	v = RL->variables;
	if (v)
	{
		while (v->next)
			v = v->next;
		v->next = RL->malloc(sizeof(RL_Value));
		if (!v->next)
		{
			printf("Error allocating variable '%s'\n", name);
			return NULL;
		}

		v = v->next;
	}
	else
	{
		v = RL->variables = RL->malloc(sizeof(RL_Value));
		if (!v)
		{
			printf("Error allocating variable '%s'\n", name);
			return NULL;
		}
	}

	v->type = type;
	v->name = RL->malloc(strlen(name) + 1);
	if (!v->name)
	{
		printf("Error allocating variable name buffer for variable '%s'\n", name);
		return NULL;
	}
	strcpy_s(v->name, strlen(name) + 1, name);
	v->next = NULL;

	switch (type)
	{
		case RL_TYPE_INT:
			v->v.i = 0;
			break;
		case RL_TYPE_DOUBLE:
			v->v.d = 0.0;
			break;
		case RL_TYPE_BYTE:
		case RL_TYPE_BOOL:
			v->v.b = 0;
			break;
	}
	return v;
}

RL_Value* rl_findvar(RL_State* RL, const char* name)
{
	RL_Value* v = RL->variables;
	for (;;)
	{
		if (!v)
			return NULL;
		if (!strcmp(v->name, name))
			return v;
		v = v->next;
	}
	return NULL;
}

static void free_value(RL_State* RL, RL_Value* v)
{
	RL->free(v->name);
	RL->free(v);
}

RL_Function* rl_definecfunc(RL_State* RL, const char* name, RL_CFunc cf, RL_PrimitiveDatatype return_type, int argc, RL_PrimitiveDatatype* arg_types, void* userdata)
{
	// TODO: copy name
	// TODO: check if function exists
	RL_Function* func = (RL_Function*)RL->malloc(sizeof(RL_Function)),
		*func2;
	if (!func)
	{
		printf("Error allocating function, %s %d\n", __FILE__, __LINE__);
		return NULL;
	}

	func->name = name;
	func->return_type = return_type;
	func->argc = argc;
	func->is_c = 1;
	func->cf = cf;
	func->userdata = userdata;
	func->next = NULL;

	if (argc)
	{
		func->arg_types = (RL_PrimitiveDatatype*)RL->malloc(sizeof(RL_PrimitiveDatatype) * argc);
		if (!func->arg_types)
		{
			printf("Error allocating space for arg_types copy, %s %d\n", __FILE__, __LINE__);
			return NULL;
		}
		memcpy(func->arg_types, arg_types, sizeof(RL_PrimitiveDatatype) * argc);
	}
	else
		func->arg_types = NULL;

	if (RL->functions)
	{
		func2 = RL->functions;
		while (func2->next)
			func2 = func2->next;
		func2->next = func;
	}
	else
	{
		RL->functions = func;
	}
	return func;
}

RL_Function* rl_findfunc(RL_State* RL, const char* name)
{
	RL_Function* f = RL->functions;
	while (f)
	{
		if (!strcmp(name, f->name))
			return f;
		f = f->next;
	}
	return NULL;
}

static void free_function(RL_State* RL, RL_Function* f)
{
	if (f->arg_types)
		RL->free(f->arg_types);
	RL->free(f);
}

void rl_pushint(RL_Stack* stack_ptr, int i)
{
	*(int*)(*stack_ptr) = i;
	*stack_ptr += sizeof(int);
}

void rl_pushdouble(RL_Stack* stack_ptr, double d)
{
	*(double*)(*stack_ptr) = d;
	*stack_ptr += sizeof(double);
}

void rl_pushbyte(RL_Stack* stack_ptr, char b)
{
	*(char*)(*stack_ptr) = b;
	*stack_ptr += sizeof(char);
}

void rl_pushbool(RL_Stack* stack_ptr, char b)
{
	*(char*)(*stack_ptr) = !!b;
	*stack_ptr += sizeof(char);
}

const char* rl_gettypename(RL_State* RL, RL_PrimitiveDatatype type)
{
	switch (type)
	{
		case RL_TYPE_VOID:
			return "void";
		case RL_TYPE_INT:
			return "int";
		case RL_TYPE_DOUBLE:
			return "double";
		case RL_TYPE_BYTE:
			return "byte";
		case RL_TYPE_BOOL:
			return "bool";
		default:
			printf("Unknown type %d\n", type);
			return NULL;
	}
}

int rl_sizeof(RL_State* RL, RL_PrimitiveDatatype type)
{
	switch (type)
	{
		case RL_TYPE_VOID:
			return 0;
		case RL_TYPE_INT:
			return sizeof(int);
		case RL_TYPE_DOUBLE:
			return sizeof(double);
		case RL_TYPE_BYTE:
		case RL_TYPE_BOOL:
			return sizeof(char);
		default:
			printf("Unknown type %d\n", type);
			return -1;
	}
}

int rl_convertable(RL_State* RL, RL_PrimitiveDatatype from, RL_PrimitiveDatatype to)
{
	if (from >= RL_TYPE_INT && from <= RL_TYPE_BOOL &&
		to >= RL_TYPE_INT && to <= RL_TYPE_BOOL)
		return 1;
	return 0;
}

#include <time.h>

void rl_runfile(RL_State* RL, const char* filename)
{
	FILE* file = fopen(filename, "r");
	if (!file)
	{
		printf("Error opening file '%s'\n", filename);
		return;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* src_read = RL->malloc(size+1);
	if (!src_read)
	{
		printf("Error reading file '%s'\n", filename);
		return;
	}

	size = fread(src_read, 1, size, file);
	fclose(file);

	src_read[size] = 0;

	rl_runstring(RL, src_read);

	RL->free(src_read);
}

void rl_runstring(RL_State* RL, const char* s)
{
	RL_Opcode* data = rl_compile(RL, s);
	//printf("Start\n");
	//float startTime = (float)clock() / CLOCKS_PER_SEC;

	rl_run(RL, data, RL->runtime.main_stack);

	//float endTime = (float)clock() / CLOCKS_PER_SEC;
	//printf("elapsed: %f\n", (double)(endTime - startTime));
}

#define rlS_check_underflow() \
	if (stack < stack_min) \
	{ \
		printf("Rexlan stack underflow\n"); \
		return; \
	}

#define rlS_check_overflow(size) \
	if (stack + size >= stack_max) \
	{ \
		printf("Rexlan stack overflow\n"); \
		return; \
	}

#define rlS_substack(v) \
	rlS_check_underflow() \
	stack -= v;

#define rlS_addstack(v) \
	stack += v;

#define rlS_get(type) \
	rlS_check_overflow(sizeof(type)); \
	*(type*)stack = **(type**)rl_inst; \
	rlS_addstack(sizeof(type)); \
	rl_inst += sizeof(type*);

#define rlS_push(type) \
	rlS_check_overflow(sizeof(type)); \
	*(type*)stack = *(type*)rl_inst; \
	rlS_addstack(sizeof(type)); \
	rl_inst += sizeof(type);

#define rlS_setstack(type) \
	rlS_substack(sizeof(type)); \
	**(type**)rl_inst = *(type*)(stack); \
	rl_inst += sizeof(type*);

#define rlS_arith(type, op, t) \
	d0.t = *(((type*)stack) - 2); \
	d1.t = *(((type*)stack) - 1); \
	*(((type*)stack) - 2) = d0.t op d1.t; \
	rlS_substack(sizeof(type));

#define rlS_arithop(type, T, t) \
	case RLOP_ADD ## T: \
		rlS_arith(type, +, t); \
		break; \
	case RLOP_SUB ## T: \
		rlS_arith(type, -, t); \
		break; \
	case RLOP_MUL ## T: \
		rlS_arith(type, *, t); \
		break; \
	case RLOP_DIV ## T: \
		rlS_arith(type, /, t); \
		break; \
	case RLOP_NEG ## T: \
		*((type*)stack - 1) *= -1; \
		break;

#define rlS_comp(type, op, t) \
	d0.t = *(((type*)stack) - 2); \
	d1.t = *(((type*)stack) - 1); \
	rlS_substack(sizeof(type) * 2); \
	*(((RL_Bool*)stack)) = d0.t op d1.t; \
	rlS_addstack(sizeof(RL_Bool));

#define rlS_compops(type, T, t) \
	case RLOP_EQ ## T: \
		rlS_comp(type, ==, t); \
		break; \
	case RLOP_NE ## T: \
		rlS_comp(type, !=, t); \
		break; \
	case RLOP_GR ## T: \
		rlS_comp(type, >, t); \
		break; \
	case RLOP_GE ## T: \
		rlS_comp(type, >=, t); \
		break; \
	case RLOP_LS ## T: \
		rlS_comp(type, <, t); \
		break; \
	case RLOP_LE ## T: \
		rlS_comp(type, <=, t); \
		break;

#define rlS_primitive(ctype, upper) \
	case RLOP_GET ## upper: \
		rlS_get(ctype); \
		break; \
	case RLOP_PUSH ## upper: \
		rlS_push(ctype); \
		break; \
	case RLOP_SET ## upper ## STACK: \
		rlS_setstack(ctype); \
		break;

typedef union
{
	int i;
	double d;
	int8_t b;
	void* ref;
} RL_Register;

void rl_run(RL_State* RL, RL_Opcode* rl_inst, RL_Stack stack)
{
	void** savestack = RL->runtime.savestack;
	RL_Register d0, d1;
	RL_Opcode opcode;
	RL_PrimitiveDatatype type1, type2;
	RL_CFunc cf;
	void* ud;

	RL_Stack stack_min = stack,
		stack_max = stack + RL->runtime.stack_size;

	while (*rl_inst)
	{
		// printf("opcode %d\n", *rl_inst);
		switch (opcode = *rl_inst++)
		{
			rlS_primitive(int, INT);
			rlS_primitive(double, DOUBLE);
			rlS_primitive(int8_t, BYTE);

			case RLOP_PUSHTRUE:
				*(int8_t*)stack = 1;
				stack += sizeof(int8_t);
				break;
			case RLOP_PUSHFALSE:
				*(int8_t*)stack = 0;
				stack += sizeof(int8_t);
				break;

			case RLOP_CAST:
				type1 = *(RL_PrimitiveDatatype*)rl_inst;
				rl_inst += sizeof(RL_PrimitiveDatatype);
				type2 = *(RL_PrimitiveDatatype*)rl_inst;
				rl_inst += sizeof(RL_PrimitiveDatatype);

				switch (type1)
				{
					case RL_TYPE_INT:
						stack -= sizeof(int);
						d0.i = *(int*)stack;
						switch (type2)
						{
							// prevented by the RLOP_CONVERT function
							/*case TYPE_INT:
								break;*/
							case RL_TYPE_DOUBLE:
								*(double*)stack = (double)d0.i;
								stack += sizeof(double);
								break;
							case RL_TYPE_BYTE:
								*(int8_t*)stack = (int8_t)d0.i;
								stack += sizeof(int8_t);
								break;
							case RL_TYPE_BOOL:
								*(int8_t*)stack = !!d0.i;
								stack += sizeof(int8_t);
								break;
							default:
								printf("Error: cannot convert from %d to %d\n", type1, type2);
								break;
						}
						break;
					case RL_TYPE_DOUBLE:
						stack -= sizeof(double);
						d0.d = *(double*)stack;
						switch (type2)
						{
							case RL_TYPE_INT:
								*(int*)stack = (int)d0.d;
								stack += sizeof(int);
								break;
							// prevented by the RLOP_CONVERT function
							/*case TYPE_DOUBLE:
								*(double*)stack = (double)ai;
								stack += sizeof(double);
								break;*/
							case RL_TYPE_BYTE:
								*(int8_t*)stack = (int8_t)d0.d;
								stack += sizeof(int8_t);
								break;
							case RL_TYPE_BOOL:
								*(int8_t*)stack = !!d0.d;
								stack += sizeof(int8_t);
								break;
							default:
								printf("Error: cannot convert from %d to %d\n", type1, type2);
								break;
						}
						break;
					case RL_TYPE_BYTE:
						stack -= sizeof(int8_t);
						d0.b = *(int8_t*)stack;
						switch (type2)
						{
							case RL_TYPE_INT:
								*(int*)stack = (int)d0.b;
								stack += sizeof(int);
								break;
							case RL_TYPE_DOUBLE:
								*(double*)stack = (double)d0.b;
								stack += sizeof(double);
								break;
							// prevented by the RLOP_CONVERT function
							/*case TYPE_BYTE:
								*(BYTE*)stack = (BYTE)ab;
								stack += sizeof(BYTE);
								break;*/
							case RL_TYPE_BOOL:
								*(int8_t*)stack = !!d0.b;
								stack += sizeof(int8_t);
								break;
							default:
								printf("Error: cannot convert from %d to %d\n", type1, type2);
								break;
						}
						break;
				}
				break;

			rlS_arithop(int, I, i);
			rlS_arithop(double, D, d);
			rlS_arithop(int8_t, B, b);

			case RLOP_MODI:
				rlS_arith(int, %, i);
				break;
			case RLOP_MODD:
				d0.d = *(((double*)stack) - 2);
				d1.d = *(((double*)stack) - 1);
				*(((double*)stack) - 2) = fmod(d0.d, d1.d);
				stack -= sizeof(double);
				break;
			case RLOP_MODB:
				rlS_arith(int8_t, %, b);
				break;

			rlS_compops(int, I, i);
			rlS_compops(double, D, d);
			rlS_compops(int8_t, B, b);

			case RLOP_SAVESTACK:
				savestack[RL->runtime.savestack_position++] = stack;
				if (RL->runtime.savestack_position >= rlS_savestacksize)
				{
					printf("Savestack overflow\n");
					return;
				}
				break;
			case RLOP_CALLC:
				stack = savestack[--RL->runtime.savestack_position];
				(*(RL_CFunc*)rl_inst)(RL, &stack, stack, NULL);
				rl_inst += sizeof(RL_CFunc);
				break;
			case RLOP_CALLCUD:
				stack = savestack[--RL->runtime.savestack_position];
				cf = *(RL_CFunc*)rl_inst;
				rl_inst += sizeof(RL_CFunc);
				ud = *(void**)rl_inst;
				rl_inst += sizeof(void*);
				cf(RL, &stack, stack, ud);
				break;
			case RLOP_JUMP:
				// printf("Jump2 %d\n", *(OFFSET*)data);
				rl_inst += *(RL_Offset*)rl_inst;
				break;
			case RLOP_JUMPTRUE:
				stack -= sizeof(int8_t);
				if (*(int8_t*)stack)
					rl_inst += *(RL_Offset*)rl_inst;
				else
					rl_inst += sizeof(RL_Offset);
				break;
			case RLOP_JUMPFALSE:
				stack -= sizeof(int8_t);
				if (*(int8_t*)stack)
					rl_inst += sizeof(RL_Offset);
				else
					rl_inst += *(RL_Offset*)rl_inst;
				break;
			case RLOP_ADDSTACK:
				stack += *(RL_Offset*)rl_inst;
				rl_inst += sizeof(RL_Offset);
				break;

			default:
				printf("Unknown opcode %d '%c'\n", opcode, opcode);
				return;
		}
	}
}