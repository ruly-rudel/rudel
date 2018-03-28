
#include <assert.h>
#include "builtin.h"
#include "vm.h"
#include "env.h"

#define OP_1P0P(X) \
{ \
	r0 = vpop(stack); \
	X; \
}

#define OP_1P1P(X) \
{ \
	r0 = vpop(stack); \
	vpush(stack, X); \
}

#define OP_2P1P(X) \
{ \
	r0 = vpop(stack); \
	r1 = vpop(stack); \
	vpush(stack, X); \
}

value_t exec_vm(value_t code, value_t e)
{
	assert(vectorp(code));
	assert(consp(e));

	value_t stack = make_vector(0);
	value_t env   = e;
	value_t ret   = make_vector(0);

	value_t r0;
	value_t r1;

	unsigned int pc = 0;

	while(true)
	{
		value_t op = vref(code, pc);
		switch(rtypeof(op))
		{
			case INT_T:
			case VEC_T:
				vpush(stack, op);
				break;

			case REF_T:
				vpush(stack, get_env_value_ref(op, env));
				break;

			case CONS_T:
				if(nilp(op))
				{
					vpush(stack, NIL);
				}
				else
				{
					return RERR(ERR_INVALID_IS, RINT(pc));
				}
				break;

			case VMIS_T:
				switch(INTOF(op))
				{
					case IS_HALT:
						return vpop(stack);

					case IS_ADD:
						OP_2P1P(RINT(INTOF(r0) + INTOF(r1)));
						break;

					case IS_SUB:
						OP_2P1P(RINT(INTOF(r0) - INTOF(r1)));
						break;

					case IS_MUL:
						OP_2P1P(RINT(INTOF(r0) * INTOF(r1)));
						break;

					case IS_DIV:
						OP_2P1P(RINT(INTOF(r0) / INTOF(r1)));
						break;

					case IS_EQ:
						OP_2P1P(EQ(r0, r1) ? g_t : NIL);
						break;

					case IS_EQUAL:
						OP_2P1P(equal(r0, r1) ? g_t : NIL);
						break;

					case IS_CONS:
						OP_2P1P(cons(r0, r1));
						break;

					case IS_CAR:
						OP_1P1P(car(r0));
						break;

					case IS_CDR:
						OP_1P1P(cdr(r0));
						break;

					case IS_MKVEC:
						OP_1P1P(make_vector(INTOF(r0)));
						break;

					case IS_VPUSH:
						OP_2P1P(vpush(r1, r0));		//****** ad-hock: swap r1 and r0 later.
						break;

					case IS_VPOP:
						OP_1P1P(vpop(r0));
						break;

					case IS_VREF:
						OP_2P1P(vref(r0, INTOF(r1)));
						break;

					case IS_VPUSH_ENV:
						OP_1P0P(env = cons(r0, env));
						break;

					case IS_VPOP_ENV:
						env = cdr(env);
						break;

					default:
						return RERR(ERR_INVALID_IS, RINT(pc));
				}
				break;

			default:
				return RERR(ERR_INVALID_IS, RINT(pc));
		}

		pc++;
	}

	return NIL;	// not reached
}
