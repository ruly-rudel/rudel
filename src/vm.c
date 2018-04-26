
#include <assert.h>
#include "builtin.h"
#include "vm.h"
#include "allocator.h"
#include "env.h"
#include "reader.h"
#include "printer.h"
#include "compile_vm.h"

/////////////////////////////////////////////////////////////////////
// macros

#define LOCAL_VPUSH_RAW(X) \
{ \
	if(sp + 2 >= alloc) { \
		alloc *= 2; \
		stack_raw = (value_t*)realloc(stack_raw, alloc * sizeof(value_t)); \
		if(!stack_raw) { \
			return rerr_alloc(); \
		} \
		pop_root(2); \
		push_root_raw_vec(stack_raw, &sp); \
		push_root_raw_vec(ret_raw,   &rsp); \
	} \
	stack_raw[++sp] = (X); \
}

#define LOCAL_VPOP_RAW    (stack_raw[sp--])

#define LOCAL_VPEEK_RAW   (stack_raw[sp])

#define LOCAL_RPLACV_TOP_RAW(X)  stack_raw[sp] = (X)


#define LOCAL_VPUSH_RET_RAW(X) \
{ \
	if(rsp + 2 >= ralloc) { \
		ralloc *= 2; \
		ret_raw = (value_t*)realloc(ret_raw, ralloc * sizeof(value_t)); \
		if(!ret_raw) { \
			return rerr_alloc(); \
		} \
		pop_root(1); \
		push_root_raw_vec(ret_raw,   &rsp); \
	} \
	ret_raw[++rsp] = (X); \
}

#define LOCAL_VPOP_RET_RAW    (ret_raw[rsp--])



#define OP_1P0P(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	r1 = (X); \
	if(errp(r1))  THROW(pr_str(RERR_OVW_PC(r1), NIL, false)); \
}

#define OP_1P0PNE(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	X; \
}

#define OP_0P1P(X) \
{ \
	r0 = (X); \
	if(errp(r0))  THROW(pr_str(RERR_OVW_PC(r0), NIL, false)); \
	LOCAL_VPUSH_RAW(r0); \
}

#define OP_1P1P(X) \
{ \
	r0 = LOCAL_VPEEK_RAW; \
	r1 = (X); \
	if(errp(r1))  THROW(pr_str(RERR_OVW_PC(r1), NIL, false)); \
	LOCAL_RPLACV_TOP_RAW(r1); \
}

#define OP_1P1PT(T, X) \
{ \
	r0 = LOCAL_VPEEK_RAW; \
	if(T) \
	{ \
		r1 = (X); \
		LOCAL_RPLACV_TOP_RAW(r1); \
	} \
	else \
	{ \
		THROW(pr_str(RERR_TYPE_PC, NIL, false)); \
	} \
}

#define OP_2P1P(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	r1 = LOCAL_VPEEK_RAW; \
	r2 = (X); \
	if(errp(r2))  THROW(pr_str(RERR_OVW_PC(r2), NIL, false)); \
	LOCAL_RPLACV_TOP_RAW(r2); \
}

#define OP_2P1PT(T, X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	r1 = LOCAL_VPEEK_RAW; \
	if(T) \
	{ \
		r2 = (X); \
		LOCAL_RPLACV_TOP_RAW(r2); \
	} \
	else \
	{ \
		THROW(pr_str(RERR_TYPE_PC, NIL, false)); \
	} \
}

#define OP_3P1P(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	r1 = LOCAL_VPOP_RAW; \
	r2 = LOCAL_VPEEK_RAW; \
	r3 = (X); \
	if(errp(r3))  THROW(pr_str(RERR_OVW_PC(r3), NIL, false)); \
	LOCAL_RPLACV_TOP_RAW(r3); \
}

#define THROW(X) \
{ \
	LOCAL_VPUSH_RAW(X); \
	goto throw; \
}

#define FIRST(X)  (UNSAFE_CAR(X))
#define SECOND(X) (UNSAFE_CAR(UNSAFE_CDR(X)))
#define THIRD(X)  (UNSAFE_CAR(UNSAFE_CDR(UNSAFE_CDR(X))))
#define FOURTH(X) (UNSAFE_CAR(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(X)))))
#define FIFTH(X)  (UNSAFE_CAR(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(X))))))
#define SIXTH(X)  (UNSAFE_CAR(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(X)))))))

#define CONS(X, CAR, CDR) \
	(X).cons = alloc_cons(); \
	(X).cons->car = (CAR); \
	(X).cons->cdr = (CDR); \
	(X).type.main = CONS_T;

#ifdef TRACE_VM
#define TRACEN(X)        fprintf(stderr, "[rudel-vm:pc=%06x] "  X      , pc)
#define TRACE(X)         fprintf(stderr, "[rudel-vm:pc=%06x] "  X  "\n", pc)
#define TRACE1(X, Y)     fprintf(stderr, "[rudel-vm:pc=%06x] "  X  "\n", pc, (int)(Y))
#define TRACE2(X, Y, Z)  fprintf(stderr, "[rudel-vm:pc=%06x] "  X  "\n", pc, (int)(Y), (int)(Z))
#define TRACE2N(X, Y, Z) fprintf(stderr,                        X,           (int)(Y), (int)(Z))
#else  // TRACE_VM
#define TRACEN(X)
#define TRACE(X)
#define TRACE1(X, Y)
#define TRACE2(X, Y, Z)
#define TRACE2N(X, Y, Z)
#endif // TRACE_VM

#define RERR_TYPE_PC	RERR(ERR_TYPE,      cons(local_vref(debug, pc), NIL))
#define RERR_ARG_PC	RERR(ERR_ARG,       cons(local_vref(debug, pc), NIL))
#define RERR_PC(X)	RERR((X),           cons(local_vref(debug, pc), NIL))
#define RERR_OVW_PC(X)	rerr(RERR_CAUSE(X), cons(local_vref(debug, pc), NIL))

/////////////////////////////////////////////////////////////////////
// private: unroll inner-loop

inline static value_t local_make_vector(unsigned n)
{
	value_t v = { 0 };
	v.vector  = alloc_vector();

	if(v.vector)
	{
		v.vector->size  = RINT(0);
		v.vector->alloc = RINT(0);
		v.vector->type  = NIL;
		v.vector->data  = RPTR(0);
		v.type.main     = VEC_T;
		v = alloc_vector_data(v, n);
		return v;
	}
	else
	{
		return rerr_alloc();
	}
}

inline static value_t local_vref(value_t v, unsigned pos)
{
	assert(vectorp(v) || symbolp(v));
	return VPTROF(AVALUE(v).vector->data)[pos];
}

inline static value_t local_vref_safe(value_t v, unsigned pos)
{
	assert(vectorp(v) || symbolp(v));
	v = AVALUE(v);

	if(pos < INTOF(v.vector->size))
	{
		return VPTROF(v.vector->data)[pos];
	}
	else
	{
		return RERR(ERR_RANGE, NIL);
	}
}

inline static value_t local_vpush(value_t x, value_t v)
{
	assert(vectorp(v));

	value_t va = AVALUE(v);
	int s = INTOF(va.vector->size);
	int a = INTOF(va.vector->alloc);
	if(s + 1 >= a)
	{
		push_root(&x);
		a = a ? a * 2 : 2;
		v = alloc_vector_data(v, a);
		if(nilp(v))
		{
			return rerr_alloc();
		}
		va = AVALUE(v);
		pop_root(1);
	}

	VPTROF(va.vector->data)[s] = x;
	va.vector->size = RINT(s + 1);

	return v;
}

inline static value_t local_vpop(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v = AVALUE(v);
	int s = INTOF(v.vector->size) - 1;
	v.vector->size = RINT(s);
	return VPTROF(v.vector->data)[s];
}

inline static value_t local_vpeek(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v = AVALUE(v);
	return VPTROF(v.vector->data)[INTOF(v.vector->size) - 1];
}

inline static value_t local_rplacv_top(value_t x, value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v = AVALUE(v);
	return VPTROF(v.vector->data)[INTOF(v.vector->size) - 1] = x;
}

inline static value_t local_rplacv(value_t v, int i, value_t x)
{
	assert(vectorp(v) || symbolp(v));
	return VPTROF(AVALUE(v).vector->data)[i] = x;
}

inline static value_t local_get_env_value_ref(value_t ref, value_t env)
{
	assert(consp(env));
	assert(refp(ref));

	for(int d = REF_D(ref); d > 0; d--, env = UNSAFE_CDR(env))
		assert(consp(env));

	assert(consp(env));
	env = UNSAFE_CAR(env);

	value_t pair = local_vref_safe(env, REF_W(ref));
	return errp(pair) ? pair : UNSAFE_CDR(pair);
}

static value_t local_set_env_ref(value_t ref, value_t val, value_t env)
{
	assert(consp(env));
	assert(refp(ref));

	for(int d = REF_D(ref); d > 0; d--, env = UNSAFE_CDR(env))
		assert(consp(env));

	assert(consp(env));
	env = UNSAFE_CAR(env);

	value_t pair = local_vref_safe(env, REF_W(ref));
	return errp(pair) ? pair : rplacd(pair, val);
}


/////////////////////////////////////////////////////////////////////
// public: VM
value_t exec_vm(value_t c, value_t e)
{
	assert(consp(c));
	assert(vectorp(car(c)));
	assert(consp(e));

#ifdef CHECK_GC_SANITY
	check_sanity();
#endif // CHECK_GC_SANITY

	int alloc          = 8;
	int sp             = -1;
	value_t* stack_raw = (value_t*)malloc(sizeof(value_t) * alloc);

	int ralloc         = 8;
	int rsp            = -1;
	value_t* ret_raw   = (value_t*)malloc(sizeof(value_t) * ralloc);

	value_t code       = car(c);

	value_t debug      = cdr(c);

	value_t env	   = e;

	value_t r0    = NIL;
	value_t r1    = NIL;
	value_t r2    = NIL;
	value_t r3    = NIL;

	int argnum    = 0;

	// register stack, ret, code, env, reg to root
	push_root        (&code);
	push_root        (&debug);
	push_root        (&env);
	push_root        (&r0);
	push_root        (&r1);
	push_root        (&r2);
	push_root        (&r3);
	push_root_raw_vec(stack_raw, &sp);
	push_root_raw_vec(ret_raw,   &rsp);

	for(int pc = 0; true; pc++)
	{
		value_t op = local_vref(code, pc);
		assert(rtypeof(op) == VMIS_T);

again:
		switch(op.op.mnem)
		{
			// core functions
			case IS_HALT: TRACE("HALT");
#ifdef PRINT_STACK_USAGE
				fprintf(stderr, "VM stack usage: stack %d, return %d\n", alloc, ralloc);
#endif // PRINT_STACK_USAGE
				// pop root
				pop_root(9);
				return LOCAL_VPOP_RAW;

			case IS_BR: TRACE1("BR %x", pc + op.op.operand);
				pc += op.op.operand - 1;
				break;

			case IS_BRB: TRACE1("BRB %x", pc - op.op.operand);
				pc -= op.op.operand + 1;
				break;

			case IS_BNIL: TRACE1("BNIL %x", pc + op.op.operand);
				OP_1P0PNE(pc += nilp(r0) ? op.op.operand - 1: 0);
				break;

			case IS_MKVEC_ENV: TRACE1("MKVEC_ENV %d", op.op.operand);
				OP_0P1P(local_make_vector(op.op.operand));
				break;

			case IS_VPUSH_ENV: TRACE("VPUSH_ENV");
				r0 = LOCAL_VPEEK_RAW;
				CONS(r1, r0, env);
				env = r1;
				break;

			case IS_VPOP_ENV: TRACE("VPOP_ENV");
				env = UNSAFE_CDR(env);
				break;

			case IS_NIL_CONS_VPUSH: TRACE("NIL_CONS_VPUSH");
				r0 = LOCAL_VPOP_RAW;
				r1 = LOCAL_VPEEK_RAW;
				CONS(r2, NIL, r0);
				local_vpush(r2, r1);
				break;

			case IS_CONS_VPUSH: TRACE("CONS_VPUSH");
				r0 = LOCAL_VPOP_RAW;
				r1 = LOCAL_VPOP_RAW;
				r2 = LOCAL_VPEEK_RAW;
				CONS(r3, r0, r1);
				local_vpush(r3, r2);
				break;

			case IS_CALLCC: TRACE("CALLCC");
				// save continuation
				r0 = make_vector(sp  < 0 ? 0 :  sp);	// stack copy, top of stack is not copied bucase it is clojure that called in following IS_AP, and it is not containd in continuation
				for(int i = 0; i < sp; i++)
					local_vpush(stack_raw[i], r0);

				r1 = make_vector(rsp < 0 ? 0 : rsp + 1);	// ret copy
				for(int i = 0; i <= rsp; i++)
					local_vpush(ret_raw[i], r1);

				local_vpush(code,         r1);
				local_vpush(debug,        r1);
				local_vpush(RINT(pc),     r1);
				local_vpush(env,          r1);
				local_vpush(r0,           r1);

				r2 = make_vector(2);				// code invoking continuation
				local_vpush(ROP(IS_GOTO), r2);
				local_vpush(r1,           r2);	// continuation itself(is ret stack including stack)

				// make clojure that invokes continuation and make it argument
				r3 = make_vector(1);
				CONS(r0, r2, r2);
				r0 = cloj(NIL, NIL, r0, NIL, NIL);

				// fix stack and argnum for AP
				r1 = LOCAL_VPOP_RAW;
				LOCAL_VPUSH_RAW(r0);
				LOCAL_VPUSH_RAW(r1);

				argnum = 1;
				// fall-through to AP

			case IS_AP: TRACE("AP");
apply:
				r1 = LOCAL_VPOP_RAW;
				if(clojurep(r1) || macrop(r1))	// compiled function
				{
					if(nilp(THIRD(r1)))
					{
						r2 = compile_vm(r1, env);
						if(errp(r2)) THROW(pr_str(r2, NIL, false));
					}
					// save contexts
					LOCAL_VPUSH_RET_RAW(code);
					LOCAL_VPUSH_RET_RAW(debug);
					LOCAL_VPUSH_RET_RAW(RINT(pc));
					LOCAL_VPUSH_RET_RAW(env);

					// set new execute contexts
					code  = UNSAFE_CAR(THIRD(r1));	// clojure code
					debug = UNSAFE_CDR(THIRD(r1));	// clojure debug symbols
					env   = FOURTH(r1);		// clojure environment
					pc    = -1;
				}
				else if(rtypeof(r1) == VMIS_T)	// VM IS
				{
					op = r1;
					goto again;
				}
				else
				{
					THROW(pr_str(RERR_PC(ERR_INVALID_AP), NIL, false));
				}
				break;

			case IS_GOTO: TRACE("GOTO");
				// fetch first argument as result
				r0 = LOCAL_VPOP_RAW;

				r1 = local_vref(code, ++pc);	// next code is entity(continuation itself)

				// restore ret stack
				rsp = -1;		// clear ret stack
				for(int i = 0; i < vsize(r1); i++)
				{
					LOCAL_VPUSH_RET_RAW(local_vref(r1, i));
				}

				// restore stack
				r2 = LOCAL_VPOP_RET_RAW;
				sp = -1;	// clear stack
				for(int i = 0; i < vsize(r2); i++)
				{
					LOCAL_VPUSH_RAW(local_vref(r2, i));
				}

				// push result to stack
				LOCAL_VPUSH_RAW(r0);
				// fall through to RET

			case IS_RET: TRACE("RET");
				env    = LOCAL_VPOP_RET_RAW;
				pc     = INTOF(LOCAL_VPOP_RET_RAW);
				debug  = LOCAL_VPOP_RET_RAW;
				code   = LOCAL_VPOP_RET_RAW;
				break;

			case IS_DUP: TRACE("DUP");
				r0 = LOCAL_VPEEK_RAW;
				LOCAL_VPUSH_RAW(r0);
				break;

			case IS_PUSH: TRACEN("PUSH: ");
				r0 = local_vref(code, ++pc);	// next code is entity
				if(refp(r0))
				{
					TRACE2N("#REF:%d,%d# ", REF_D(r0), REF_W(r0));
					r0 = local_get_env_value_ref(r0, env);
				}
				else if(symbolp(r0))
				{
#ifdef TRACE_VM
					char* sn = rstr_to_str(symbol_string(r0));
					fprintf(stderr, "%s ", sn);
					free(sn);
#endif // TRACE_VM
					r1 = get_env_ref(r0, env);
					if(errp(r1))
					{
						if(EQ(r0, g_env))
						{
							r0 = env;
						}
						else
						{
							r0 = r1;
						}
					}
					else
					{
						local_rplacv(code, pc, r1);		// replace symbol to reference
						TRACE2N("-> #REF:%d,%d# ", REF_D(r1), REF_W(r1));
						r0 = local_get_env_value_ref(r1, env);
					}
				}
				else if(clojurep(r0) || macrop(r0))
				{
					r0 = copy_list(r0);
					AVALUE(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(r0)))).cons->car = env;	// set current environment
				}
#ifdef TRACE_VM
				print(r0, stderr);
#endif // TRACE_VM
				if(errp(r0))
				{
					r0 = RERR_OVW_PC(r0);
					THROW(pr_str(r0, NIL, false));
				}
				LOCAL_VPUSH_RAW(r0);
				break;

			case IS_PUSHR: TRACEN("PUSHR: ");
				r0 = local_vref(code, ++pc);	// next code is entity
#ifdef TRACE_VM
				print(r0, stderr);
#endif // TRACE_VM
				LOCAL_VPUSH_RAW(r0);
				break;

			case IS_POP: TRACE("POP");
				OP_1P0PNE();
				break;

			case IS_SETENV: TRACE("SETENV");
				r0 = LOCAL_VPOP_RAW;
				r1 = LOCAL_VPEEK_RAW;
				if(symbolp(r0))
				{
					r2 = find_env_all(r0, env);
					if(nilp(r2))
					{
						set_env(r0, r1, last(env));	// set to root
					}
					else
					{
						rplacd(r2, r1);			// replace
					}
				}
				else if(rtypeof(r0) == REF_T)
				{
					local_set_env_ref(r0, r1, env);
				}
				LOCAL_RPLACV_TOP_RAW(r1);
				break;

			case IS_NCONC: TRACE("NCONC");
				OP_2P1P(nconc(r0, r1));
				break;

			case IS_SWAP: TRACE("SWAP");
				r0 = LOCAL_VPOP_RAW;
				r1 = LOCAL_VPOP_RAW;
				LOCAL_VPUSH_RAW(r0);
				LOCAL_VPUSH_RAW(r1);
				break;

			case IS_ARGNUM: TRACE("ARGNUM");
				r0 = LOCAL_VPOP_RAW;
				argnum = INTOF(r0);
				break;

			case IS_DEC_ARGNUM: TRACE("DEC_ARGNUM");
				argnum -= 1;
				break;

			case IS_VPUSH_REST: TRACE("VPUSH_REST");
				r0 = LOCAL_VPOP_RAW;	// KEY
				r1 = LOCAL_VPOP_RAW;	// VEC
				CONS(r2, r0, NIL);
				r3 = r2;
				while(argnum-- > 0)
				{
					CONS_AND_CDR(LOCAL_VPOP_RAW, r3);
				}
				local_vpush(r2, r1);
				break;

			case IS_ISZERO_ARGNUM: TRACE1("ISZERO_ARGNUM %d", argnum);
				if(argnum != 0) THROW(pr_str(RERR_ARG_PC, NIL, false));
				break;

			case IS_ROTL: TRACE("ROTL");
				r0 = LOCAL_VPOP_RAW;
				r1 = LOCAL_VPOP_RAW;
				r2 = LOCAL_VPOP_RAW;
				LOCAL_VPUSH_RAW(r0);
				LOCAL_VPUSH_RAW(r2);
				LOCAL_VPUSH_RAW(r1);
				break;



			case IS_ATOM: TRACE("ATOM");
				OP_1P1P(atom(r0) ? g_t : NIL);
				break;

			case IS_CONSP: TRACE("CONSP");
				OP_1P1P(consp(r0) ? g_t : NIL);
				break;

			case IS_CLOJUREP: TRACE("CLOJUREP");
				OP_1P1P(clojurep(r0) ? g_t : NIL);
				break;

			case IS_MACROP: TRACE("MACROP");
				OP_1P1P(macrop(r0) ? g_t : NIL);
				break;

			case IS_SPECIALP: TRACE("SPECIALP");
				OP_1P1P(specialp(r0) ? g_t : NIL);
				break;

			case IS_STRP: TRACE("STRP");
				OP_1P1P(is_str(r0) ? g_t : NIL);
				break;

			case IS_ERRP: TRACE("ERRP");
				OP_1P1P(errp(r0) ? g_t : NIL);
				break;

			case IS_CONS: TRACE("CONS");
				OP_2P1P(cons(r0, r1));
				break;

			case IS_CAR: TRACE("CAR");
				OP_1P1PT(rtypeof(r0) >= CONS_T && rtypeof(r0) <= ERR_T, car(r0));
				break;

			case IS_CDR: TRACE("CDR");
				OP_1P1PT(rtypeof(r0) >= CONS_T && rtypeof(r0) <= ERR_T, cdr(r0));
				break;

			case IS_EQ: TRACE("EQ");
				OP_2P1P(EQ(r0, r1)    ? g_t : NIL);
				break;

			case IS_EQUAL: TRACE("EQUAL");
				OP_2P1P(equal(r0, r1) ? g_t : NIL);
				break;

			case IS_RPLACA: TRACE("RPLACA");
				OP_2P1P(consp(r0) || macrop(r0) || clojurep(r0) || errp(r0) ? rplaca(r0, r1) : RERR_TYPE_PC);
				break;

			case IS_RPLACD: TRACE("RPLACD");
				OP_2P1P(consp(r0) || macrop(r0) || clojurep(r0) || errp(r0) ? rplacd(r0, r1) : RERR_TYPE_PC);
				break;

			case IS_GENSYM: TRACE("GENSYM");
				OP_0P1P(gensym(last(env)));
				break;

			case IS_ADD: TRACE("ADD");
				OP_2P1PT(intp(r0) && intp(r1), RINT(INTOF(r0) + INTOF(r1)));
				break;

			case IS_SUB: TRACE("SUB");
				OP_2P1PT(intp(r0) && intp(r1), RINT(INTOF(r0) - INTOF(r1)));
				break;

			case IS_MUL: TRACE("MUL");
				OP_2P1PT(intp(r0) && intp(r1), RINT(INTOF(r0) * INTOF(r1)));
				break;

			case IS_DIV: TRACE("DIV");
				OP_2P1PT(intp(r0) && intp(r1), RINT(INTOF(r0) / INTOF(r1)));
				break;

			case IS_LT: TRACE("LT");
				OP_2P1PT(intp(r0) && intp(r1), r0.raw <  r1.raw ? g_t : NIL);
				break;

			case IS_ELT: TRACE("ELT");
				OP_2P1PT(intp(r0) && intp(r1), r0.raw <= r1.raw ? g_t : NIL);
				break;

			case IS_MT: TRACE("MT");
				OP_2P1PT(intp(r0) && intp(r1), r0.raw >  r1.raw ? g_t : NIL);
				break;

			case IS_EMT: TRACE("EMT");
				OP_2P1PT(intp(r0) && intp(r1), r0.raw >= r1.raw ? g_t : NIL);
				break;

			case IS_READ_STRING: TRACE("READ_STRING");
				OP_1P1P(vectorp(r0) ? read_str(r0) : RERR_TYPE_PC);
				break;

			case IS_EVAL: TRACE("EVAL");
				r0 = LOCAL_VPOP_RAW;
				r1 = compile_vm(r0, env);
				if(errp(r1)) THROW(pr_str(r1, NIL, false));

				// save contexts
				LOCAL_VPUSH_RET_RAW(code);
				LOCAL_VPUSH_RET_RAW(debug);
				LOCAL_VPUSH_RET_RAW(RINT(pc));
				LOCAL_VPUSH_RET_RAW(env);

				// set new execute contexts
				code  = UNSAFE_CAR(r1);	// clojure code
				debug = UNSAFE_CDR(r1);	// clojure debug symbols
				pc    = -1;
				break;

			case IS_ERR: TRACE("ERR");
				OP_1P1P(rerr(r0, RINT(pc)));
				break;

			case IS_NTH: TRACE("NTH");
				OP_2P1P(intp(r1) ? nth(INTOF(r1), r0) : RERR_TYPE_PC);
				break;

			case IS_INIT: TRACE("INIT");
				OP_0P1P(init(env));
				break;

			case IS_SAVECORE: TRACE("SAVECORE");
				OP_1P1P(is_str(r0) ? save_core(r0, last(env)) : RERR_TYPE_PC);
				break;

			case IS_MAKE_VECTOR: TRACE("MAKE_VECTOR");
				OP_1P1P(intp(r0) ? make_vector(INTOF(r0)) : RERR_TYPE_PC);
				break;

			case IS_VREF: TRACE("VREF");
				OP_2P1P(vectorp(r0) ? vref(r0, INTOF(r1)) : RERR_TYPE_PC);
				break;

			case IS_RPLACV: TRACE("RPLACV");
				OP_3P1P(vectorp(r0) && intp(r1) ? rplacv(r0, INTOF(r1), r2) : RERR_TYPE_PC);
				break;

			case IS_VSIZE: TRACE("VSIZE");
				OP_1P1PT(vectorp(r0), RINT(vsize(r0)));
				break;

			case IS_VEQ: TRACE("VEQ");
				OP_2P1PT(vectorp(r0) && vectorp(r1), veq(r0, r1) ? g_t : NIL);
				break;

			case IS_VPUSH: TRACE("VPUSH");
				OP_2P1P(vectorp(r1) ? vpush(r0, r1) : RERR_TYPE_PC);
				break;

			case IS_VPOP: TRACE("VPOP");
				OP_1P1P(vectorp(r0) ? vpop(r0) : RERR_TYPE_PC);
				break;

			case IS_COPY_VECTOR: TRACE("COPY_VECTOR");
				OP_1P1P(vectorp(r0) ? copy_vector(r0) : RERR_TYPE_PC);
				break;

			case IS_VCONC: TRACE("VCONC");
				OP_2P1P(vectorp(r0) && vectorp(r1) ? vconc(r0, r1) : RERR_TYPE_PC);
				break;

			case IS_VNCONC: TRACE("VNCONC");
				OP_2P1PT(vectorp(r0) && vectorp(r1), vnconc(r0, r1));
				break;

			case IS_COMPILE_VM: TRACE("COMPILE_VM");
				OP_1P1P(compile_vm(r0, env));
				break;

			case IS_EXEC_VM: TRACE("EXEC_VM");
				r0 = LOCAL_VPOP_RAW;
				if(consp(r0) && vectorp(car(r0)))
				{
					// save contexts
					LOCAL_VPUSH_RET_RAW(code);
					LOCAL_VPUSH_RET_RAW(debug);
					LOCAL_VPUSH_RET_RAW(RINT(pc));
					LOCAL_VPUSH_RET_RAW(env);

					// set new execute contexts
					code  = UNSAFE_CAR(r0);	// clojure code
					debug = UNSAFE_CDR(r0);	// clojure debug symbols
					pc    = -1;
				}
				else
				{
					THROW(pr_str(RERR_TYPE_PC, NIL, false));
				}
				break;

			case IS_PR_STR: TRACE("PR_STR");
				OP_2P1P(pr_str(r0, NIL, !nilp(r1)));
				break;

			case IS_PRINTLINE: TRACE("PRINTLINE");
				OP_1P1P(vectorp(r0) || nilp(r0) ? (printline(r0, stdout), NIL) : RERR_TYPE_PC);
				break;

			case IS_PRINT: TRACE("PRINT");
				OP_1P1P((print(r0, stdout), NIL));
				break;

			case IS_SLURP: TRACE("SLURP");
			{
				r0 = LOCAL_VPEEK_RAW;
				if(!vectorp(r0))
				{
					THROW(pr_str(RERR_TYPE_PC, NIL, false));
				}
				char* fn  = rstr_to_str(r0);
				LOCAL_RPLACV_TOP_RAW(slurp(fn));
				free(fn);
			}
			break;

			case IS_READ: TRACE("READ");
				LOCAL_VPUSH_RAW(READ(PROMPT, stdin));
				break;

			case IS_MVFL: TRACE("MVFL");
				OP_1P1P(consp(r0) || nilp(r0) ? make_vector_from_list(r0) : RERR_TYPE_PC);
				break;

			case IS_MLFV: TRACE("MLFV");
				OP_1P1P(vectorp(r0) ? make_list_from_vector(r0) : RERR_TYPE_PC);
				break;

			case IS_COUNT: TRACE("COUNT");
				OP_1P1P(RINT(count(r0)));
				break;

			case IS_REVERSE: TRACE("REVERSE");
				OP_1P1P(consp(r0) ? reverse(r0) : RERR_TYPE_PC);
				break;

			default:
				THROW(pr_str(RERR_PC(ERR_INVALID_IS), NIL, false));
throw:
				// push exception handler
				r1 = get_env_ref(str_to_sym("*exception-stack*"), env);
				r2 = local_get_env_value_ref(r1, env);
				r3 = car(r2);
				if(!clojurep(r3))
				{
					pop_root(9);
					return RERR_PC(ERR_EXCEPTION);
				}
				LOCAL_VPUSH_RAW(r3);

				// pop *exception-stack*
				r3 = cdr(r2);
				local_set_env_ref(r1, r3, env);
				goto apply;
		}

		// clear temporal registers for safe
		r0 = r1 = r2 = r3 = NIL;
		//check_gc();
#ifdef CHECK_GC_SANITY
		check_sanity();
#endif // CHECK_GC_SANITY
	}

	return NIL;	// not reached
}


// End of File
/////////////////////////////////////////////////////////////////////
