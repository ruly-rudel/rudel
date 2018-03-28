
#include <assert.h>
#include "builtin.h"
#include "vm.h"
#include "env.h"

#define OP_1P0P(X) \
{ \
	r0 = vpop(stack); \
	X; \
}

#define OP_0P1P(X) \
{ \
	vpush(X, stack); \
}

#define OP_1P1P(X) \
{ \
	r0 = vpop(stack); \
	vpush(X, stack); \
}

#define OP_2P1P(X) \
{ \
	r0 = vpop(stack); \
	r1 = vpop(stack); \
	vpush(X, stack); \
}

#ifdef TRACE_VM
#define TRACE(X)        fprintf(stderr, "[rudel-vm:pc=%08d] "  X  "\n", pc)
#define TRACE1(X, Y)    fprintf(stderr, "[rudel-vm:pc=%08d] "  X  "\n", pc, (int)(Y))
#define TRACE2(X, Y, Z) fprintf(stderr, "[rudel-vm:pc=%08d] "  X  "\n", pc, (int)(Y), (int)(Z))
#else  // TRACE_VM
#define TRACE(X)
#define TRACE1(X, Y)
#define TRACE2(X, Y, Z)
#endif // TRACE_VM

value_t exec_vm(value_t code, value_t e)
{
	assert(vectorp(code));
	assert(consp(e));

	value_t stack = make_vector(0);
	value_t env   = e;
	value_t ret   = make_vector(0);

	value_t r0    = NIL;
	value_t r1    = NIL;

	unsigned int pc = 0;

	while(true)
	{
		value_t op = vref(code, pc);
		switch(rtypeof(op))
		{
			case INT_T:
				TRACE1("push INT: %d", INTOF(op));
				OP_0P1P(op);
				break;

			case VEC_T:
				TRACE("push #<VECTOR>");
				OP_0P1P(op);
				break;

			case REF_T:
				TRACE2("push #REF:%d,%d#", REF_D(op), REF_W(op));
				OP_0P1P(get_env_value_ref(op, env));
				break;

			case CONS_T:
				if(nilp(op))
				{
					TRACE("push NIL");
					OP_0P1P(NIL);
				}
				else
				{
					TRACE("error: attempt to push cons.");
					return RERR(ERR_INVALID_IS, RINT(pc));
				}
				break;

			case VMIS_T:
				switch(op.op.mnem)
				{
					case IS_HALT: TRACE("HALT");
						return vpop(stack);

					case IS_ADD: TRACE("ADD");
						OP_2P1P(RINT(INTOF(r0) + INTOF(r1)));
						break;

					case IS_SUB: TRACE("SUB");
						OP_2P1P(RINT(INTOF(r0) - INTOF(r1)));
						break;

					case IS_MUL: TRACE("MUL");
						OP_2P1P(RINT(INTOF(r0) * INTOF(r1)));
						break;

					case IS_DIV: TRACE("DIV");
						OP_2P1P(RINT(INTOF(r0) / INTOF(r1)));
						break;

					case IS_EQ: TRACE("EQ");
						OP_2P1P(EQ(r0, r1)    ? g_t : NIL);
						break;

					case IS_EQUAL: TRACE("EQUAL");
						OP_2P1P(equal(r0, r1) ? g_t : NIL);
						break;

					case IS_CONS: TRACE("CONS");
						OP_2P1P(cons(r0, r1));
						break;

					case IS_CAR: TRACE("CAR");
						OP_1P1P(car(r0));
						break;

					case IS_CDR: TRACE("CDR");
						OP_1P1P(cdr(r0));
						break;

					case IS_MKVEC: TRACE("MKVEC");
						OP_1P1P(make_vector(INTOF(r0)));
						break;

					case IS_VPUSH: TRACE("VPUSH");
						OP_2P1P(vpush(r0, r1));
						break;

					case IS_VPOP: TRACE("VPOP");
						OP_1P1P(vpop(r0));
						break;

					case IS_VREF: TRACE1("VREF %d", INTOF(r1));
						OP_2P1P(vref(r0, INTOF(r1)));
						break;

					case IS_VPUSH_ENV: TRACE("VPUSH_ENV");
						OP_1P0P(env = cons(r0, env));
						break;

					case IS_VPOP_ENV: TRACE("VPOP_ENV");
						env = cdr(env);
						break;

					case IS_BR: TRACE1("BR %d", pc + op.op.operand);
						pc += op.op.operand - 1;
						break;

					case IS_BNIL: TRACE1("BNIL %d", pc + op.op.operand);
						OP_1P0P(pc += nilp(r0) ? op.op.operand - 1: 0);
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
