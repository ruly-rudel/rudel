
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
			return unlock_gc(), rerr_alloc(); \
		} \
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
			return unlock_gc(), rerr_alloc(); \
		} \
	} \
	ret_raw[++rsp] = (X); \
}

#define LOCAL_VPOP_RET_RAW    (ret_raw[rsp--])



#define OP_1P0P(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	r1 = (X); \
	if(errp(r1)) return RERR_OVW_PC(r1); \
}

#define OP_1P0PNE(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	X; \
}

#define OP_0P1P(X) \
{ \
	r0 = (X); \
	if(errp(r0)) return RERR_OVW_PC(r0); \
	LOCAL_VPUSH_RAW(r0); \
}

#define OP_1P1P(X) \
{ \
	r0 = LOCAL_VPEEK_RAW; \
	r1 = (X); \
	if(errp(r1)) return RERR_OVW_PC(r1); \
	LOCAL_RPLACV_TOP_RAW(r1); \
}

#define OP_2P1P(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	r1 = LOCAL_VPEEK_RAW; \
	r2 = (X); \
	if(errp(r2)) return RERR_OVW_PC(r2); \
	LOCAL_RPLACV_TOP_RAW(r2); \
}

#define OP_3P1P(X) \
{ \
	r0 = LOCAL_VPOP_RAW; \
	r1 = LOCAL_VPOP_RAW; \
	r2 = LOCAL_VPEEK_RAW; \
	r3 = (X); \
	if(errp(r3)) return RERR_OVW_PC(r3); \
	LOCAL_RPLACV_TOP_RAW(r3); \
}

#define FIRST(X)  (UNSAFE_CAR(X))
#define SECOND(X) (UNSAFE_CAR(UNSAFE_CDr(X)))
#define THIRD(X)  (UNSAFE_CAR(UNSAFE_CDR(UNSAFE_CDR(X))))
#define FOURTH(X) (UNSAFE_CAR(UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(X)))))

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

#define RERR_TYPE_PC	(unlock_gc(), RERR(ERR_TYPE, cons(vref(debug, pc), NIL)))
#define RERR_PC(X)	(unlock_gc(), RERR((X), cons(vref(debug, pc), NIL)))
#define RERR_OVW_PC(X)	(unlock_gc(), rerr(RERR_CAUSE(X), cons(vref(debug, pc), NIL)))

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
	value_t v = { 0 };
	v.vector  = alloc_vector();

	if(v.vector)
	{
		v.vector->size  = 0;
		v.vector->alloc = n;
		v.vector->type  = NIL;
		v.vector->data  = 0;
		v.type.main     = VEC_T;
		alloc_vector_data(v, n);
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
	v.vector = local_aligned_vaddr(v);
	return v.vector->data[pos];
}

inline static value_t local_vref_safe(value_t v, unsigned pos)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);

	if(pos < v.vector->size)
	{
		return v.vector->data[pos];
	}
	else
	{
		return RERR(ERR_RANGE, NIL);
	}
}

inline static value_t local_vpush(value_t x, value_t v)
{
	assert(vectorp(v));

	value_t va;
	va.vector = local_aligned_vaddr(v);
	int s = va.vector->size;
	int a = va.vector->alloc;
	if(s + 1 >= a)
	{
		a = a * 2;
		//va.vector->alloc = a;
		//value_t* np = (value_t*)realloc(v.vector->data, a * sizeof(value_t));
		value_t* np = alloc_vector_data(v, a);
		if(!np)
		{
			return rerr_alloc();
		}
	}

	va.vector->data[s] = x;
	va.vector->size = s + 1;

	return v;
}

inline static value_t local_vpop(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);
	int s = v.vector->size - 1;
	v.vector->size = s;
	return v.vector->data[s];
}

inline static value_t local_vpeek(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);
	return v.vector->data[v.vector->size - 1];
}

inline static value_t local_rplacv_top(value_t x, value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);
	return v.vector->data[v.vector->size - 1] = x;
}

inline static value_t local_rplacv(value_t v, int i, value_t x)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = local_aligned_vaddr(v);
	return v.vector->data[i] = x;
}

inline static value_t local_get_env_value_ref(value_t ref, value_t env)
{
	assert(consp(env));
	assert(rtypeof(ref) == REF_T);

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

	value_t r0raw = NIL;

	// register stack, ret, code, env, reg to root
	push_root_raw_vec(stack_raw, &sp);
	push_root_raw_vec(ret_raw,   &rsp);
	push_root        (&code);
	push_root        (&debug);
	push_root        (&env);
	push_root        (&r0);
	push_root        (&r1);
	push_root        (&r2);
	push_root        (&r3);

	for(int pc = 0; true; pc++)
	{
		value_t op = local_vref(code, pc);
		assert(rtypeof(op) == VMIS_T);

		lock_gc();
		switch(op.op.mnem)
		{
			// core functions
			case IS_HALT: TRACE("HALT");
#ifdef PRINT_STACK_USAGE
				fprintf(stderr, "VM stack usage: stack %d, return %d\n", alloc, ralloc);
#endif // PRINT_STACK_USAGE
				// pop root
				for(int i = 0; i < 9; i++)
					pop_root();
				unlock_gc();
				return LOCAL_VPOP_RAW;

			case IS_BR: TRACE1("BR %x", pc + op.op.operand);
				pc += op.op.operand - 1;
				break;

			case IS_BNIL: TRACE1("BNIL %x", pc + op.op.operand);
				OP_1P0PNE(pc += nilp(r0) ? op.op.operand - 1: 0);
				break;

			case IS_MKVEC_ENV: TRACE1("MKVEC_ENV %d", op.op.operand);
				OP_0P1P(local_make_vector(op.op.operand));
				break;

			case IS_VPUSH_ENV: TRACE("VPUSH_ENV");
				r0 = LOCAL_VPEEK_RAW;
				env = cons(r0, env);
				break;

			case IS_VPOP_ENV: TRACE("VPOP_ENV");
				env = UNSAFE_CDR(env);
				break;

			case IS_NIL_CONS_VPUSH: TRACE("NIL_CONS_VPUSH");
				r0 = LOCAL_VPOP_RAW;
				r1 = LOCAL_VPEEK_RAW;
				local_vpush(cons(NIL, r0), r1);
				break;

			case IS_CONS_VPUSH: TRACE("CONS_VPUSH");
				r0 = LOCAL_VPOP_RAW;
				r1 = LOCAL_VPOP_RAW;
				r2 = LOCAL_VPEEK_RAW;
				local_vpush(cons(r0, r1), r2);
				break;

			case IS_AP:
			{
				r1 = LOCAL_VPOP_RAW;
				r0 = LOCAL_VPOP_RAW;
				if(clojurep(r0))	// compiled function
				{
					TRACE("AP(Clojure)");
					r0raw = r0;
					r0raw.type.main = CONS_T;
					if(nilp(THIRD(r0raw)))
					{
						r2 = compile_vm(r0, env);
						if(errp(r2)) return unlock_gc(), r2;
					}
					// save contexts
					LOCAL_VPUSH_RET_RAW(code);
					LOCAL_VPUSH_RET_RAW(debug);
					LOCAL_VPUSH_RET_RAW(RINT(pc));
					LOCAL_VPUSH_RET_RAW(env);

					// set new execute contexts
					code  = UNSAFE_CAR(THIRD(r0raw));	// clojure code
					debug = UNSAFE_CDR(THIRD(r0raw));	// clojure debug symbols
					env  = cons(r1, FOURTH(r0raw));	// clojure environment + new environment as arguments
					pc   = -1;
				}
				else if(macrop(r0))	// compiled macro
				{
					TRACE("AP(Macro)");
					r0raw = r0;
					r0raw.type.main = CONS_T;
					if(nilp(THIRD(r0raw)))
					{
						r2 = compile_vm(r0, env);
						if(errp(r2)) return unlock_gc(), r2;
					}
					TRACE("  Expand macro, invoking another VM.");
					r2 = THIRD(r0raw);
					r3 = FOURTH(r0raw);
					value_t ext = exec_vm(r2, cons(r1, r3));
					if(errp(ext)) return unlock_gc(), ext;
					TRACE("  Compiling the result on-the-fly.");
					value_t new_code = compile_vm(ext, env);
					if(errp(new_code)) return unlock_gc(), new_code;
					TRACE("  Evaluate it, invoking another VM.");
					value_t res = exec_vm(new_code, env);
					if(errp(res)) return unlock_gc(), res;
					TRACE("AP(Macro) Done.");
					LOCAL_VPUSH_RAW(res);
				}
				else
				{
					return RERR_PC(ERR_INVALID_AP);
				}
			}
			break;

			case IS_RET: TRACE("RET");
				env   = LOCAL_VPOP_RET_RAW;
				pc    = INTOF(LOCAL_VPOP_RET_RAW);
				debug = LOCAL_VPOP_RET_RAW;
				code  = LOCAL_VPOP_RET_RAW;
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
					r0 = get_env_ref(r0, env);
					if(errp(r0))
					{
						return RERR_OVW_PC(r0);
					}
					local_rplacv(code, pc, r0);		// replace symbol to reference
					TRACE2N("-> #REF:%d,%d# ", REF_D(r0), REF_W(r0));
					r0 = local_get_env_value_ref(r0, env);
				}
				else if(clojurep(r0) || macrop(r0))
				{
					r0raw = r0;
					r0raw.type.main = CONS_T;
					UNSAFE_CDR(UNSAFE_CDR(UNSAFE_CDR(r0raw))).cons->car = env;	// set current environment
				}
#ifdef TRACE_VM
				print(pr_str(r0, NIL, true), stderr);
#endif // TRACE_VM
				if(errp(r0))
				{
					return RERR_OVW_PC(r0);
				}
				LOCAL_VPUSH_RAW(r0);
				break;

			case IS_PUSHR: TRACEN("PUSHR: ");
				r0 = local_vref(code, ++pc);	// next code is entity
#ifdef TRACE_VM
				print(pr_str(r0, NIL, true), stderr);
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
					value_t target = find_env_all(r0, env);
					if(nilp(target))
					{
						set_env(r0, r1, last(env));	// set to root
					}
					else
					{
						rplacd(target, r1);		// replace
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

			case IS_SETSYM: TRACE1("SETSYM %d", op.op.operand);
				r0 = UNSAFE_CAR(env);
				r2 = LOCAL_VPOP_RAW;
				r1 = local_vref_safe(r0, op.op.operand);
				if(errp(r1))
				{
					return RERR_PC(ERR_ARG);
				}
				else
				{
					r1.cons->car = r2;
				}
				break;

			case IS_RESTPARAM: TRACE1("RESTPARAM %d", pc + op.op.operand);
			{
				r0 = UNSAFE_CAR(env);
				r2 = LOCAL_VPOP_RAW;
				int size = vsize(r0);
				r1 = NIL;
				value_t* cur = &r1;

				for(int i = op.op.operand; i < size; i++)
				{
					cur = cons_and_cdr(UNSAFE_CDR(local_vref(r0, i)), cur);
				}
				r3 = rplacv(r0, op.op.operand, cons(r2, r1));
				if(errp(r3))
				{
					return RERR_PC(ERR_ARG);
				}
			}
				break;

			case IS_MACROEXPAND:
			{
				r1 = LOCAL_VPOP_RAW;
				r0 = LOCAL_VPOP_RAW;
				if(macrop(r0))	// compiled macro
				{
					TRACE("MACROEXPAND");
					r0raw = r0;
					r0raw.type.main = CONS_T;
					if(nilp(THIRD(r0raw)))
					{
						r2 = compile_vm(r0, env);
						if(errp(r2)) return unlock_gc(), r2;
					}
					TRACE("  Expand macro, invoking another VM.");
					r2 = THIRD(r0raw);
					r3 = FOURTH(r0raw);
					value_t ext = exec_vm(r2, cons(r1, r3));
					if(errp(ext)) return unlock_gc(), ext;
					TRACE("MACROEXPAND Done.");
					LOCAL_VPUSH_RAW(ext);
				}
				else
				{
					return RERR_PC(ERR_INVALID_AP);
				}
			}
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

			case IS_STRP: TRACE("STRP");
				OP_1P1P(is_str(r0) ? g_t : NIL);
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

			case IS_EQ: TRACE("EQ");
				OP_2P1P(EQ(r0, r1)    ? g_t : NIL);
				break;

			case IS_EQUAL: TRACE("EQUAL");
				OP_2P1P(equal(r0, r1) ? g_t : NIL);
				break;

			case IS_RPLACA: TRACE("RPLACA");
				OP_2P1P(consp(r0) ? rplaca(r0, r1) : RERR_TYPE_PC);
				break;

			case IS_RPLACD: TRACE("RPLACD");
				OP_2P1P(consp(r0) ? rplacd(r0, r1) : RERR_TYPE_PC);
				break;

			case IS_GENSYM: TRACE("GENSYM");
				OP_0P1P(gensym(last(env)));
				break;

			case IS_ADD: TRACE("ADD");
				OP_2P1P(intp(r0) && intp(r1) ? (RINT(INTOF(r0) + INTOF(r1))) : RERR_TYPE_PC);
				break;

			case IS_SUB: TRACE("SUB");
				OP_2P1P(intp(r0) && intp(r1) ? (RINT(INTOF(r0) - INTOF(r1))) : RERR_TYPE_PC);
				break;

			case IS_MUL: TRACE("MUL");
				OP_2P1P(intp(r0) && intp(r1) ? (RINT(INTOF(r0) * INTOF(r1))) : RERR_TYPE_PC);
				break;

			case IS_DIV: TRACE("DIV");
				OP_2P1P(intp(r0) && intp(r1) ? (RINT(INTOF(r0) / INTOF(r1))) : RERR_TYPE_PC);
				break;

			case IS_LT: TRACE("LT");
				OP_2P1P(intp(r0) && intp(r1) ? (INTOF(r0) <  INTOF(r1) ? g_t : NIL) : RERR_TYPE_PC);
				break;

			case IS_ELT: TRACE("ELT");
				OP_2P1P(intp(r0) && intp(r1) ? (INTOF(r0) <= INTOF(r1) ? g_t : NIL) : RERR_TYPE_PC);
				break;

			case IS_MT: TRACE("MT");
				OP_2P1P(intp(r0) && intp(r1) ? (INTOF(r0) >  INTOF(r1) ? g_t : NIL) : RERR_TYPE_PC);
				break;

			case IS_EMT: TRACE("EMT");
				OP_2P1P(intp(r0) && intp(r1) ? (INTOF(r0) >= INTOF(r1) ? g_t : NIL) : RERR_TYPE_PC);
				break;

			case IS_READ_STRING: TRACE("READ_STRING");
				OP_1P1P(vectorp(r0) ? read_str(r0) : RERR_TYPE_PC);
				break;

			case IS_EVAL: TRACE("EVAL");
				r0 = LOCAL_VPEEK_RAW;
				r1 = compile_vm(r0, last(env));
				LOCAL_RPLACV_TOP_RAW(exec_vm(r1, last(env)));
				break;

			case IS_ERR: TRACE("ERR");
				OP_1P0P(rerr(r0, RINT(pc)));
				break;

			case IS_NTH: TRACE("NTH");
				OP_2P1P(intp(r1) ? nth(INTOF(r1), r0) : RERR_TYPE_PC);
				break;

			case IS_INIT: TRACE("INIT");
				OP_0P1P(init(env));
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
				OP_1P1P(vectorp(r0) ? RINT(vsize(r0)) : RERR_TYPE_PC);
				break;

			case IS_VEQ: TRACE("VEQ");
				OP_2P1P(vectorp(r0) && vectorp(r1) ? (veq(r0, r1) ? g_t : NIL) : RERR_TYPE_PC);
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
				OP_2P1P(vectorp(r0) && vectorp(r1) ? vnconc(r0, r1) : RERR_TYPE_PC);
				break;

			case IS_COMPILE_VM: TRACE("COMPILE_VM");
				OP_1P1P(compile_vm(r0, last(env)));
				break;

			case IS_EXEC_VM: TRACE("EXEC_VM");
				OP_1P1P(consp(r0) && vectorp(car(r0)) ? exec_vm(r0, last(env)) : RERR_TYPE_PC);
				break;

			case IS_PR_STR: TRACE("PR_STR");
				OP_2P1P(pr_str(r0, NIL, !nilp(r1)));
				break;

			case IS_PRINTLINE: TRACE("PRINTLINE");
				OP_1P1P(vectorp(r0) || nilp(r0) ? (printline(r0, stdout), NIL) : RERR_TYPE_PC);
				break;

			case IS_SLURP: TRACE("SLURP");
			{
				r0 = LOCAL_VPEEK_RAW;
				if(!vectorp(r0))
				{
					return RERR_TYPE_PC;
				}
				char* fn  = rstr_to_str(r0);
				LOCAL_RPLACV_TOP_RAW(slurp(fn));
				free(fn);
			}
			break;

			case IS_MVFL: TRACE("MVFL");
				OP_1P1P(consp(r0) || nilp(r0) ? make_vector_from_list(r0) : RERR_TYPE_PC);
				break;

			case IS_MLFV: TRACE("MLFV");
				OP_1P1P(vectorp(r0) ? make_list_from_vector(r0) : RERR_TYPE_PC);
				break;

			default:
				return RERR_PC(ERR_INVALID_IS);
		}
		unlock_gc();

		// clear temporal registers for safe
		r0 = r1 = r2 = r3 = NIL;
		check_gc();
	}

	return NIL;	// not reached
}


// End of File
/////////////////////////////////////////////////////////////////////
