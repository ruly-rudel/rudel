
#include <assert.h>
#include "builtin.h"
#include "compile_vm.h"
#include "env.h"

/////////////////////////////////////////////////////////////////////
// private:

static value_t compile_vm1(value_t code, value_t ast, value_t env);

static value_t compile_vm_check_builtin(value_t atom, value_t env)
{
	assert(consp(env));

	if(symbolp(atom))
	{
		value_t tbl[] = {
			str_to_sym("atom"),		ROP(IS_ATOM),
			str_to_sym("consp"),		ROP(IS_CONSP),
			str_to_sym("cons"),		ROP(IS_CONS),
			str_to_sym("car"),		ROP(IS_CAR),
			str_to_sym("cdr"),		ROP(IS_CDR),
			str_to_sym("eq"),		ROP(IS_EQ),
			str_to_sym("equal"),		ROP(IS_EQUAL),
			str_to_sym("rplaca"),		ROP(IS_RPLACA),
			str_to_sym("rplacd"),		ROP(IS_RPLACD),
			str_to_sym("gensym"),		ROP(IS_GENSYM),
			str_to_sym("+"),		ROP(IS_ADD),
			str_to_sym("-"),		ROP(IS_SUB),
			str_to_sym("*"),		ROP(IS_MUL),
			str_to_sym("/"),		ROP(IS_DIV),
			str_to_sym("<"),		ROP(IS_LT),
			str_to_sym("<="),		ROP(IS_ELT),
			str_to_sym(">"),		ROP(IS_MT),
			str_to_sym(">="),		ROP(IS_EMT),
			str_to_sym("read-string"),	ROP(IS_READ_STRING),
			str_to_sym("slurp"),		ROP(IS_SLURP),
			str_to_sym("eval"),		ROP(IS_EVAL),
			str_to_sym("err"),		ROP(IS_ERR),
			str_to_sym("nth"),		ROP(IS_NTH),
			str_to_sym("init"),		ROP(IS_INIT),
			str_to_sym("make-vector"),	ROP(IS_MAKE_VECTOR),
			str_to_sym("vref"),		ROP(IS_VREF),
			str_to_sym("rplacv"),		ROP(IS_RPLACV),
			str_to_sym("vsize"),		ROP(IS_VSIZE),
			str_to_sym("veq"),		ROP(IS_VEQ),
			str_to_sym("vpush"),		ROP(IS_VPUSH),
			str_to_sym("vpop"),		ROP(IS_VPOP),
			str_to_sym("copy-vector"),	ROP(IS_COPY_VECTOR),
			str_to_sym("vconc"),		ROP(IS_VCONC),
			str_to_sym("vnconc"),		ROP(IS_VNCONC),
			str_to_sym("compile-vm"),	ROP(IS_COMPILE_VM),
			str_to_sym("exec-vm"),		ROP(IS_EXEC_VM),

			/*
			str_to_sym("pr-str"),		ROP(IS_PR_STR),
			str_to_sym("str"),		ROP(IS_STR),
			str_to_sym("prn"),		ROP(IS_PRN),
			str_to_sym("println"),		ROP(IS_PRINTLN),
			str_to_sym("strp"),		ROP(IS_STRP),
			str_to_sym("make-vector-from-list"),		ROP(IS_make-vector-from-list),
			str_to_sym("make-list-from-vector"),		ROP(IS_make-list-from-vector),
			*/
		};

		for(int i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i += 2)
		{
			if(EQ(tbl[i], atom))
			{
				return tbl[i + 1];
			}
		}
	}

	return NIL;
}

static value_t compile_vm_setq(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	// key
	value_t key = car(ast);
	if(!symbolp(key))
	{
		return RERR(ERR_NOTSYM, ast);
	}

	// value
	value_t val_notev = cdr(ast);
	if(rtypeof(val_notev) != CONS_T)
	{
		return RERR(ERR_ARG, cdr(ast));
	}

	code = compile_vm1(code, car(val_notev), env);

	if(!errp(code))
	{
		value_t key_ref = get_env_ref(key, env);
		if(errp(key_ref))
		{
			vpush(key, code);	//**** push symbol to code
		}
		else
		{
			key_ref.ref.sub = VMREF_T;	// push ref itself to stack
			vpush(key_ref, code);
		}

		vpush(ROP(IS_SETENV), code);
	}
	return code;
}

static value_t compile_vm_progn(value_t code, value_t ast, value_t env)
{
	for(; !nilp(ast); ast = cdr(ast))
	{
		assert(consp(ast));
		code = compile_vm1(code, car(ast), env);
		if(errp(code))
		{
			return code;
		}
		vpush(ROP(IS_POP), code);

	}
	vpop(code);

	return code;
}


// applicative order
static value_t compile_vm_list(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(rtypeof(ast) == CONS_T);
	assert(consp(env));

	if(!nilp(ast))
	{
		code = compile_vm_list(code, cdr(ast), env);
		if(errp(code))
		{
			return code;
		}
		code = compile_vm1(code, car(ast), env);
		if(errp(code))
		{
			return code;
		}
	}

	return code;
}

// ****** not applicative order: fix it with eval
static value_t compile_vm_apply_arg(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(rtypeof(ast) == CONS_T);
	assert(consp(env));

	vpush(ROPD(IS_MKVEC_ENV, count(ast)), code);

	for(; !nilp(ast); ast = cdr(ast))
	{
		code = compile_vm1(code, car(ast), env);
		if(errp(code))
		{
			return code;
		}
		else
		{
			// add apply environment.
			vpush(ROP(IS_NIL_CONS_VPUSH), code);	// env key is NIL
		}
	}

	// push env is in IS_AP

	return code;
}

static value_t compile_vm_if(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	// cereate environment
	if(!consp(ast))
	{
		return RERR(ERR_ARG, ast);
	}

	// eval condition
	code = compile_vm1(code, car(ast), env);
	if(errp(code))
	{
		return code;
	}

	int cond_br = INTOF(vsize(code));
	vpush(ROP(IS_BNIL), code);	// dummy operand(0)

	// true situation
	code = compile_vm1(code, car(cdr(ast)), env);
	if(errp(code))
	{
		return code;
	}

	int true_br = INTOF(vsize(code));
	vpush(ROP(IS_BR), code);	// dummy operand(0)

	// false situation
	code = compile_vm1(code, car(cdr(cdr(ast))), env);
	if(errp(code))
	{
		return code;
	}

	int next_addr = INTOF(vsize(code));

	// fix branch address: relative to pc
	rplacv(code, cond_br, ROPD(IS_BNIL, true_br + 1 - cond_br));
	rplacv(code, true_br, ROPD(IS_BR,   next_addr - true_br));

	return code;
}

static value_t compile_vm_let(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	if(!consp(ast))
	{
		return RERR(ERR_ARG, ast);
	}

	// allocate new environment
	value_t let_env = create_env(NIL, NIL, env);

	// local symbols
	value_t def = car(ast);
	if(rtypeof(def) != CONS_T)
	{
		return RERR(ERR_ARG, ast);
	}

	// create env vector
	vpush(ROPD(IS_MKVEC_ENV, count(def)), code);

	// push empty env vector to env
	vpush(ROP(IS_VPUSH_ENV), code);		// duplicate ENV_VEC on stack top

	// cereate environment

	for(; !nilp(def); def = cdr(def))
	{
		value_t bind = car(def);

		// symbol
		value_t sym = car(bind);
		if(!symbolp(sym))
		{
			return RERR(ERR_ARG, bind);
		}
		set_env(sym, NIL, let_env);	// for self-reference

		// body
		code = compile_vm1(code, car(cdr(bind)), let_env);
		if(errp(code))
		{
			return code;
		}
		else
		{
			// add let environment.
			vpush(ROP(IS_NIL_CONS_VPUSH), code);
		}
	}

	vpush(ROP(IS_POP), code);	// pop environment

	// let body
	code = compile_vm1(code, car(cdr(ast)), let_env);
	if(errp(code))
	{
		return code;
	}
	else
	{
		// pop env
		vpush(ROP(IS_VPOP_ENV), code);
		return code;
	}
}

static value_t compile_vm_lambda1(value_t ast, value_t env)
{
	assert(consp(ast));
	assert(consp(env));

	if(!consp(ast))
	{
		return RERR(ERR_ARG, ast);
	}

	// allocate new environment for compilation
	value_t def = car(ast);
	if(rtypeof(def) != CONS_T)
	{
		return RERR(ERR_ARG, ast);
	}
	value_t lambda_env = create_env(def, NIL, env);

	// compile body
	value_t lambda_code = make_vector(0);
	lambda_code = compile_vm1(lambda_code, car(cdr(ast)), lambda_env);
	if(errp(lambda_code))
	{
		return lambda_code;
	}
	vpush(ROP(IS_RET), lambda_code);	// RET

	return lambda_code;
}

static value_t compile_vm_macro(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	value_t macro_code = compile_vm_lambda1(ast, env);
	if(!errp(macro_code))
	{
		// macro is clojure call
		vpush(macro(macro_code, env), code);	//**** macro environment?
	}

	return code;
}

static value_t compile_vm_lambda(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	value_t lambda_code = compile_vm_lambda1(ast, env);
	if(!errp(lambda_code))
	{
		// lambda is clojure call
		vpush(cloj(lambda_code, NIL), code);	// clojure environment must be set while exec
	}

	return code;
}

static value_t compile_vm_quasiquote(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	if(!consp(ast))					// atom
	{
		vpush(ast, code);			  // -> push immediate to stack
	}
	else
	{
		value_t arg1 = car(ast);
		if(EQ(arg1, RSPECIAL(SP_UNQUOTE)))	// (unquote x)
		{
			value_t arg2 = car(cdr(ast));
			return compile_vm1(code, arg2, env);	  // -> add evaluation code
		}
		else if(consp(arg1) && EQ(car(arg1), RSPECIAL(SP_SPLICE_UNQUOTE)))	// ((splice-unquote x) ...)
		{
			code = compile_vm_quasiquote(code, cdr(ast), env);
			if(errp(code))
			{
				return code;
			}
			code = compile_vm1(code, car(cdr(arg1)), env);
			if(errp(code))
			{
				return code;
			}
			vpush(ROP(IS_CONCAT), code);
		}
		else
		{
			code = compile_vm_quasiquote(code, cdr(ast), env);
			if(errp(code))
			{
				return code;
			}
			code = compile_vm_quasiquote(code, arg1    , env);
			if(errp(code))
			{
				return code;
			}
			vpush(ROP(IS_CONS), code);
		}
	}

	return code;
}

static value_t compile_vm_apply(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	value_t fn = car(ast);

	if(specialp(fn))
	{
		switch(INTOF(fn))
		{
			case SP_SETQ:
				return compile_vm_setq      (code, cdr(ast), env);

			case SP_LET:
				return compile_vm_let       (code, cdr(ast), env);

			case SP_PROGN:
				return compile_vm_progn     (code, cdr(ast), env);

			case SP_IF:
				return compile_vm_if        (code, cdr(ast), env);

			case SP_LAMBDA:
				return compile_vm_lambda    (code, cdr(ast), env);

			case SP_MACRO:
				return compile_vm_macro     (code, cdr(ast), env);

			case SP_QUOTE:
				return vpush(car(cdr(ast)), code);

			case SP_QUASIQUOTE:
				return compile_vm_quasiquote(code, car(cdr(ast)), env);

			default:
				return RERR(ERR_NOTIMPL, ast);
		}
	}
	else
	{
		value_t fn = compile_vm_check_builtin(car(ast), env);
		if(nilp(fn))	// apply clojure
		{
			code = compile_vm_apply_arg(code, cdr(ast), env);	// arguments
			if(errp(code))
			{
				return code;
			}
			code = compile_vm1(code, car(ast), env);		// maybe clojure
			if(errp(code))
			{
				return code;
			}
			vpush(ROP(IS_AP), code);
		}
		else		// apply builtin
		{
			// evaluate each list items
			code = compile_vm_list(code, cdr(ast), env);
			if(errp(code))
			{
				return code;
			}
			vpush(fn, code);
		}
		return code;
	}
}

static value_t compile_vm1(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	value_t r = NIL;
	switch(rtypeof(ast))
	{
		case INT_T:
			vpush(ast, code);
			break;

		case SYM_T:
			r = compile_vm_check_builtin(ast, env); // Builtin Function (is one VM Instruction)
			if(nilp(r))
			{
				r = get_env_ref(ast, env);
				if(errp(r))
				{
					return r;
				}
			}
			else
			{
				return RERR(ERR_ARG_BUILTIN, ast);	//***** buit-in function is not first class: FIX IT!
			}
			vpush(r, code);
			break;

		case REF_T:
			vpush(ast, code);
			break;

		case VEC_T:
			vpush(ast, code);	//****** heap address in code: fix it.
			break;

		case CONS_T:
			if(nilp(ast))
			{
				vpush(ast, code);
			}
			else
			{
				code = compile_vm_apply(code, ast, env);
			}
			break;

		default:
			return RERR(ERR_NOTIMPL, ast);
	}

	return code;
}

/////////////////////////////////////////////////////////////////////
// public: compile for VM


value_t compile_vm(value_t ast, value_t env)
{
	assert(consp(env));

	value_t code = make_vector(0);

	code = compile_vm1(code, ast, env);
	if(!errp(code))
	{
		vpush(ROP(IS_HALT), code);
	}

	return code;
}

// End of File
/////////////////////////////////////////////////////////////////////
