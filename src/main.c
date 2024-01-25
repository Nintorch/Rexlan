#include <stdlib.h>
#include <stdio.h>

#include "Rexlan.h"
#include "rlstate.h"

// Rexlan test program

// TODO:
// better organization

void Print(RL_State* RL, RL_Stack* stack_ptr, char* args, void* userdata)
{
	int i = *(int*)args;
	printf("Print: %d\n", i);
}

void GetValue(RL_State* RL, RL_Stack* stack_ptr, char* args, void* userdata)
{
	rl_pushint(stack_ptr, 42);
}

int main()
{
	RL_State* RL = rl_init();

	RL_PrimitiveDatatype types[] = { RL_TYPE_INT };
	int some_test = 6969;
	rl_definecfunc(RL, "print", Print, RL_TYPE_VOID, 1, types, (void*)&some_test);
	rl_definecfunc(RL, "get_value", GetValue, RL_TYPE_INT, 0, NULL, NULL);

	RL_Stack stack_init = RL->runtime.main_stack;

	rl_runfile(RL, "file.rl");

	printf("%d\n", (size_t)RL->runtime.main_stack - (size_t)stack_init);

	for (RL_Value* v = RL->variables; v; v = v->next)
	{
		printf("Name: %s, type: %s, ", v->name, rl_gettypename(RL, v->type));
		switch (v->type)
		{
			case RL_TYPE_INT:
				printf("value: %d\n", v->v.i);
				break;
			case RL_TYPE_DOUBLE:
				printf("value: %f\n", v->v.d);
				break;
			case RL_TYPE_BYTE:
				printf("value: %d\n", v->v.b);
				break;
			case RL_TYPE_BOOL:
				printf("value: %s\n", v->v.b ? "true" : "false");
				break;
		}
	}

	for (RL_Function* f = RL->functions; f; f = f->next)
	{
		printf("Function name: %s, accepts %d args\n", f->name, f->argc);
	}

	rl_quit(RL);
	return 0;
}