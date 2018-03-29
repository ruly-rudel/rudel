
#include <assert.h>
#include "builtin.h"
#include "vm.h"
#include "env.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////
// macros

#define OP_1P0P(X) \
{ \
	r0 = local_vpop(stack); \
	X; \
}

#define OP_0P1P(X) \
{ \
	local_vpush(X, stack); \
}

#define OP_1P1P(X) \
{ \
	r0 = local_vpop(stack); \
	local_vpush(X, stack); \
}

#define OP_2P1P(X) \
{ \
	r0 = local_vpop(stack); \
	r1 = local_vpop(stack); \
	local_vpush(X, stack); \
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

/////////////////////////////////////////////////////////////////////
// private: unroll inner-loop

static inline vector_t* local_aligned_vaddr(value_t v)
{
#if __WORDSIZE == 32
	return (vector_t*) (((uint32_t)v.vector) & 0xfffffff8);
#else
	return (vector_t*) (((uint64_t)v.vector) & 0xfffffffffffffff8);
#endif
}

inline static value_t local_make_vector(unsigned n)
{
	value_t v;
	v.vector        = (vector_t*)malloc(sizeof(vector_t));
	v.vector->size  = RINT(0);
	v.vector->alloc = RINT(n);
	v.vector->type  = NIL;
	v.vector->data  = (value_t*)malloc(n * sizeof(value_t));
	v.type.main     = VEC_T;
	return v;
}

inline static value_t local_vref(value_t v, unsigned pos)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);
	return v.vector->data[pos];
}

inline static value_t local_vpush(value_t x, value_t v)
{
	assert(vectorp(v));

	v.vector = local_aligned_vaddr(v);
	int s = INTOF(v.vector->size);
	int a = INTOF(v.vector->alloc);
	if(s + 1 >= a)
	{
		a = a * 2;
		v.vector->alloc = RINT(a);
		value_t* np = (value_t*)realloc(v.vector->data, a * sizeof(value_t));
		if(np)
		{
			v.vector->data = np;
		}
		else
		{
			return RERR(ERR_ALLOC, NIL);
		}
	}

	v.vector->data[s] = x;
	v.vector->size = RINT(s + 1);

	return v;
}

inline static value_t local_vpop(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);
	int s = INTOF(v.vector->size) - 1;
	v.vector->size = RINT(s);
	return v.vector->data[s];
}

inline static value_t local_vpeek(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);
	return v.vector->data[INTOF(v.vector->size) - 1];
}

inline static value_t local_get_env_value_ref(value_t ref, value_t env)
{
	assert(consp(env));
	assert(rtypeof(ref) == REF_T);

	for(int d = REF_D(ref); d > 0; d--, env = UNSAFE_CDR(env))
		assert(consp(env));

	assert(consp(env));
	env = UNSAFE_CAR(env);

	return UNSAFE_CDR(local_vref(env, REF_W(ref)));
}


/////////////////////////////////////////////////////////////////////
// public: VM
value_t exec_vm(value_t c, value_t e)
{
	assert(vectorp(c));
	assert(consp(e));

	value_t stack = make_vector(16);
	value_t code  = c;
	value_t env   = e;
	value_t ret   = make_vector(256);

	value_t r0    = NIL;
	value_t r1    = NIL;

	int pc = 0;

	while(true)
	{
		value_t op = local_vref(code, pc);
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
				OP_0P1P(local_get_env_value_ref(op, env));
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

			case CLOJ_T: TRACE("push #<CLOJURE>");
				rplacd(op, env);	// set current environment
				OP_0P1P(op);
				break;

			case VMIS_T:
				switch(op.op.mnem)
				{
					// core functions
					case IS_HALT: TRACE("HALT");
						return vpop(stack);

					case IS_VPUSH_ENV: TRACE("VPUSH_ENV");
						r0 = local_vpeek(stack);
						env = cons(r0, env);
						break;

					case IS_VPOP_ENV: TRACE("VPOP_ENV");
						env = UNSAFE_CDR(env);
						break;

					case IS_BR: TRACE1("BR %d", pc + op.op.operand);
						pc += op.op.operand - 1;
						break;

					case IS_BNIL: TRACE1("BNIL %d", pc + op.op.operand);
						OP_1P0P(pc += nilp(r0) ? op.op.operand - 1: 0);
						break;

					case IS_CONS: TRACE("CONS");
						OP_2P1P(cons(r0, r1));
						break;

					case IS_MKVEC_ENV: TRACE1("MKVEC_ENV %d", op.op.operand);
						OP_0P1P(local_make_vector(op.op.operand));
						break;

					case IS_VPUSH: TRACE("VPUSH");
						OP_2P1P(vpush(r0, r1));
						break;

					case IS_DUP: TRACE("DUP");
						r0 = local_vpeek(stack);
						vpush(r0, stack);
						break;

					case IS_POP: TRACE("POP");
						OP_1P0P();
						break;

					case IS_NIL_CONS_VPUSH: TRACE("NIL_CONS_VPUSH");
						r0 = local_vpop(stack);
						r1 = local_vpeek(stack);
						local_vpush(cons(NIL, r0), r1);
						break;

					case IS_AP:
					{
						r0 = local_vpop(stack);
						if(clojurep(r0))	// compiled function
						{
							TRACE("AP");
							// save contexts
							local_vpush(code,     ret);
							local_vpush(RINT(pc), ret);
							local_vpush(env,      ret);

							// set new execute contexts
							r1 = r0;
							r1.type.main = CONS_T;
							code = UNSAFE_CAR(r1);	// clojure code
							env  = UNSAFE_CDR(r1);	// clojure environment
							pc   = -1;
							OP_1P0P(env = cons(r0, env));	// new environment as arguments

#ifdef REPLACE_STACK
							// save contexts(stack is last)
							local_vpush(stack,    ret);
							stack = make_vector(16);
#endif	// REPLACE_STACK
						}
						else
						{
							return RERR(ERR_INVALID_CLOJ, RINT(pc));
						}

					}
					break;

					case IS_RET: TRACE("RET");
#ifdef REPLACE_STACK
						r0   = local_vpop(stack);	// ret value
						stack = local_vpop(ret);
#endif	// REPLACE_STACK
						env  = local_vpop(ret);
						pc   = INTOF(local_vpop(ret));
						code = local_vpop(ret);
#ifdef REPLACE_STACK
						local_vpush(r0, stack);
#endif	// REPLACE_STACK
						break;

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

					case IS_LT: TRACE("LT");
						OP_2P1P(INTOF(r0) < INTOF(r1) ? g_t : NIL);
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

					case IS_VPOP: TRACE("VPOP");
						OP_1P1P(vpop(r0));
						break;

					case IS_VREF: TRACE1("VREF %d", INTOF(r1));
						OP_2P1P(vref(r0, INTOF(r1)));
						break;
				}
				break;

			default:
				return RERR(ERR_INVALID_IS, RINT(pc));
		}

		pc++;
	}

	return NIL;	// not reached
}


// End of File
/////////////////////////////////////////////////////////////////////
