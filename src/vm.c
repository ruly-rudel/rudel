
#include <assert.h>
#include "builtin.h"
#include "vm.h"
#include "env.h"

#define PUSHS(X)	stack = cons((X), stack)
#define POPS()		stack = cdr(stack)
#define GETS()		car(stack)
#define SETS(X)		stack.cons->car = (X)

#define PUSHR(X)	ret   = cons((X), ret)
#define POPR()		ret   = cdr(ret)
#define GETR()		car(ret)
#define SETR(X)		ret.cons->car = (X)

#define PUSHE(X)	env   = cons((X), env)
#define POPE()		env   = cdr(env)
#define GETE()		car(env)
#define SETE(X)		env.cons->car = (X)

#define OP_2P1P(X) \
{ \
	r0 = GETS(); \
	POPS(); \
	r1 = GETS(); \
	SETS(X); \
}

value_t exec_vm(code_t* code, value_t e)
{
	value_t t     = str_to_sym("t");

	value_t stack = NIL;
	value_t env   = e;
	value_t ret   = NIL;

	value_t r0;
	value_t r1;

	unsigned int pc = 0;

	while(true)
	{
		switch(code[pc].opcode)
		{
			case LD:
				PUSHS(get_env_value(code[pc].operand, env));
				break;

			case LDI:
				PUSHS(code[pc].operand);
				break;

			case ST:
				r0 = GETS();
				r1 = find_env_all(code[pc].operand, env);
				if(nilp(r1))
				{
					set_env(code[pc].operand, r0, env);	// set to current
				}
				else
				{
					rplacd(r1, r0);		// replace
				}
				POPS();
				break;

			case CALL:	// not good.
				r0 = GETS();
				POPS();
				PUSHE(r0);
				PUSHR(RINT(pc));			// +1 when ret
				pc = INTOF(code[pc].operand) - 1;	// +1 after
				break;

			case RET:	// not good.
				POPE();
				pc = INTOF(GETR());
				POPR();
				break;

			case B:
				pc = INTOF(code[pc].operand) - 1;	// +a after
				break;

			case BN:
				r0 = GETS();
				POPS();
				pc = nilp(r0) ? INTOF(code[pc].operand) - 1 : pc;	// +a after
				break;

			case CONS:
				OP_2P1P(cons(r0, r1));
				break;

			case EQ:
				OP_2P1P(eq(r0, r1) ? t : NIL);
				break;

			case ADD:
				OP_2P1P(RINT(INTOF(r0) + INTOF(r1)));
				break;

			case SUB:
				OP_2P1P(RINT(INTOF(r0) - INTOF(r1)));
				break;

			case MUL:
				OP_2P1P(RINT(INTOF(r0) * INTOF(r1)));
				break;

			case DIV:
				OP_2P1P(RINT(INTOF(r0) / INTOF(r1)));
				break;

			default:
				return RERR(ERR_NOTIMPL, NIL);
		}

		pc++;
	}

	return NIL;	// not reached
}
