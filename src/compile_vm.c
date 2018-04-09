
#include <assert.h>
#include "builtin.h"
#include "compile_vm.h"
#include "env.h"

/////////////////////////////////////////////////////////////////////
// private:

static value_t compile_vm1(value_t code, value_t debug, value_t ast, value_t env);
static value_t compile_vm1_fn(value_t code, value_t debug, value_t ast, value_t env);

static value_t compile_vm_check_builtin(value_t atom, value_t env)
{
	assert(consp(env));

	if(symbolp(atom))
	{
		for(int i = 0; i < g_istbl_size; i += 3)
		{
			if(EQ(g_istbl[i], atom))
			{
				return g_istbl[i + 1];
			}
		}
	}

	return NIL;
}

static int compile_vm_get_builtin_argnum(value_t atom, value_t env)
{
	assert(consp(env));

	if(symbolp(atom))
	{
		for(int i = 0; i < g_istbl_size; i += 3)
		{
			if(EQ(g_istbl[i], atom))
			{
				return INTOF(g_istbl[i + 2]);
			}
		}
	}

	return -1;
}

static value_t compile_vm_setq(value_t code, value_t debug, value_t ast, value_t env)
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

	code = compile_vm1(code, debug, car(val_notev), env);

	if(!errp(code))
	{
		value_t key_ref = get_env_ref(key, env);
		if(errp(key_ref))
		{
			vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
			vpush(key,           code);	vpush(ast, debug); //**** push symbol to code
		}
		else
		{
			vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
			vpush(key_ref,       code);	vpush(ast, debug); // push ref itself to stack
		}

		vpush(ROP(IS_SETENV), code);		vpush(ast, debug);
	}
	return code;
}

static value_t compile_vm_progn(value_t code, value_t debug, value_t ast, value_t env)
{
	for(; !nilp(ast); ast = cdr(ast))
	{
		assert(consp(ast));
		code = compile_vm1(code, debug, car(ast), env);
		if(errp(code))
		{
			return code;
		}
		vpush(ROP(IS_POP), code);		vpush(ast, debug);

	}
	vpop(code);

	return code;
}


// applicative order
static value_t compile_vm_list(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast) || nilp(ast));
	assert(consp(env));

	if(!nilp(ast))
	{
		code = compile_vm_list(code, debug, cdr(ast), env);
		if(errp(code))
		{
			return code;
		}
		code = compile_vm1(code, debug, car(ast), env);
		if(errp(code))
		{
			return code;
		}
	}

	return code;
}

// ****** not applicative order: fix it with eval
static value_t compile_vm_apply_arg(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast) || nilp(ast));
	assert(consp(env));

	vpush(ROPD(IS_MKVEC_ENV, count(ast)), code);		vpush(ast, debug);

	for(; !nilp(ast); ast = cdr(ast))
	{
		code = compile_vm1(code, debug, car(ast), env);
		if(errp(code))
		{
			return code;
		}
		else
		{
			// add apply environment.
			vpush(ROP(IS_NIL_CONS_VPUSH), code);	vpush(ast, debug);	// env key is NIL
		}
	}

	// push env is in IS_AP

	return code;
}

// ****** not applicative order: fix it with eval
static value_t compile_vm_macro_arg(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast) || nilp(ast));
	assert(consp(env));

	vpush(ROPD(IS_MKVEC_ENV, count(ast)), code);	vpush(ast, debug);

	for(; !nilp(ast); ast = cdr(ast))
	{
		assert(consp(ast));
		vpush(ROP(IS_PUSHR), code);		vpush(ast, debug);		// push unevaluated args
		vpush(car(ast), code);			vpush(ast, debug);
		vpush(ROP(IS_NIL_CONS_VPUSH), code);	vpush(ast, debug);
	}

	// push env is in IS_AP

	return code;
}

static value_t compile_vm_if(value_t code, value_t debug, value_t ast, value_t env)
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
	code = compile_vm1(code, debug, first(ast), env);
	if(errp(code))
	{
		return code;
	}

	int cond_br = vsize(code);
	vpush(ROP(IS_BNIL), code);	vpush(ast, debug);	// dummy operand(0)

	// true situation
	code = compile_vm1(code, debug, second(ast), env);
	if(errp(code))
	{
		return code;
	}

	int true_br = vsize(code);
	vpush(ROP(IS_BR), code);	vpush(ast, debug);	// dummy operand(0)

	// false situation
	code = compile_vm1(code, debug, third(ast), env);
	if(errp(code))
	{
		return code;
	}

	int next_addr = vsize(code);

	// fix branch address: relative to pc
	rplacv(code, cond_br, ROPD(IS_BNIL, true_br + 1 - cond_br));
	rplacv(code, true_br, ROPD(IS_BR,   next_addr - true_br));

	return code;
}

static value_t compile_vm_let(value_t code, value_t debug, value_t ast, value_t env)
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
	vpush(ROPD(IS_MKVEC_ENV, count(def)), code);	vpush(ast, debug);

	// push empty env vector to env
	vpush(ROP(IS_VPUSH_ENV), code);			vpush(ast, debug);	// duplicate ENV_VEC on stack top

	// cereate environment

	for(; !nilp(def); def = cdr(def))
	{
		value_t bind = first(def);

		// symbol
		value_t sym = car(bind);
		if(!symbolp(sym))
		{
			return RERR(ERR_ARG, bind);
		}
		set_env(sym, NIL, let_env);	// for self-reference

		// body
		code = compile_vm1(code, debug, car(cdr(bind)), let_env);
		if(errp(code))
		{
			return code;
		}
		else
		{
			// add let environment.
			vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
			vpush(sym,           code);	vpush(ast, debug);
			vpush(ROP(IS_CONS_VPUSH), code);vpush(ast, debug);
		}
	}

	vpush(ROP(IS_POP), code);			vpush(ast, debug);	// pop environment

	// let body
	code = compile_vm1(code, debug, second(ast), let_env);
	if(errp(code))
	{
		return code;
	}
	else
	{
		// pop env
		vpush(ROP(IS_VPOP_ENV), code);		vpush(ast, debug);
		return code;
	}
}

static value_t	compile_vm_arg_symbol(value_t code, value_t debug, value_t key)
{
	for(int i = 0; !nilp(key); i++, (key = cdr(key)))
	{
		assert(consp(key));

		value_t key_car = car(key);

		if(EQ(key_car, RSPECIAL(SP_AMP)))	// rest parameter
		{
			vpush(ROP (IS_PUSHR),        code);	vpush(key, debug);
			vpush(second(key),           code);	vpush(key, debug);
			vpush(ROPD(IS_RESTPARAM, i), code);	vpush(key, debug);
			break;
		}
		else
		{
			vpush(ROP (IS_PUSHR),        code);	vpush(key, debug);
			vpush(key_car,               code);	vpush(key, debug);
			vpush(ROPD(IS_SETSYM, i),    code);	vpush(key, debug);
		}
	}

	return code;
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
	value_t def = first(ast);
	if(rtypeof(def) != CONS_T)
	{
		return RERR(ERR_ARG, ast);
	}
	value_t lambda_env = create_env(def, NIL, env);


	// compile body
	value_t lambda_code  = make_vector(0);
	value_t lambda_debug = make_vector(0);
	lambda_code = compile_vm_arg_symbol(lambda_code, lambda_debug, def);	// with rest parameter support
	if(errp(lambda_code))
	{
		return lambda_code;
	}

	lambda_code = compile_vm1(lambda_code, lambda_debug, second(ast), lambda_env);
	if(errp(lambda_code))
	{
		return lambda_code;
	}

	return cons(lambda_code, lambda_debug);
}

static value_t compile_vm_macro(value_t ast, value_t env)
{
	assert(consp(ast));
	assert(consp(env));

	value_t macro_code = compile_vm_lambda1(ast, env);
	if(!errp(macro_code))
	{
		// macro invokes another VM.
		vpush(ROP(IS_HALT), car(macro_code));	vpush(ast, cdr(macro_code));	// HALT
	}

	return macro_code;
}

static value_t compile_vm_lambda(value_t ast, value_t env)
{
	assert(consp(ast));
	assert(consp(env));

	value_t lambda_code = compile_vm_lambda1(ast, env);
	if(!errp(lambda_code))
	{
		// lambda is clojure call
		vpush(ROP(IS_RET), car(lambda_code));	vpush(ast, cdr(lambda_code));	// RET
	}

	return lambda_code;
}

static value_t compile_vm_quasiquote(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	if(!consp(ast))					// atom
	{
		vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
		vpush(ast, code);		vpush(ast, debug);	  // -> push immediate to stack
	}
	else
	{
		value_t arg1 = first(ast);
		if(EQ(arg1, RSPECIAL(SP_UNQUOTE)))	// (unquote x)
		{
			value_t arg2 = second(ast);
			return compile_vm1(code, debug, arg2, env);	  // -> add evaluation code
		}
		else if(consp(arg1) && EQ(first(arg1), RSPECIAL(SP_SPLICE_UNQUOTE)))	// ((splice-unquote x) ...)
		{
			code = compile_vm_quasiquote(code, debug, cdr(ast), env);
			if(errp(code))
			{
				return code;
			}
			code = compile_vm1(code, debug, second(arg1), env);
			if(errp(code))
			{
				return code;
			}
			vpush(ROP(IS_NCONC), code);	vpush(ast, debug);
		}
		else
		{
			code = compile_vm_quasiquote(code, debug, cdr(ast), env);
			if(errp(code))
			{
				return code;
			}
			code = compile_vm_quasiquote(code, debug, arg1    , env);
			if(errp(code))
			{
				return code;
			}
			vpush(ROP(IS_CONS), code);	vpush(ast, debug);
		}
	}

	return code;
}

static value_t compile_vm_macroexpand(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	if(consp(ast))
	{
		code = compile_vm1(code, debug, first(ast), env);
		if(errp(code))
		{
			return code;
		}

		code = compile_vm_macro_arg(code, debug, cdr(ast), env);	// arguments
		if(errp(code))
		{
			return code;
		}
		vpush(ROP(IS_MACROEXPAND),    code);	vpush(ast, debug);
	}
	else
	{
		return RERR(ERR_TYPE, NIL);
	}

	return code;
}

static value_t compile_vm_apply(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	value_t fn = first(ast);

	if(specialp(fn))
	{
		switch(INTOF(fn))
		{
			case SP_SETQ:
				return compile_vm_setq      (code, debug, cdr(ast), env);

			case SP_LET:
				return compile_vm_let       (code, debug, cdr(ast), env);

			case SP_PROGN:
				return compile_vm_progn     (code, debug, cdr(ast), env);

			case SP_IF:
				return compile_vm_if        (code, debug, cdr(ast), env);

			case SP_LAMBDA:		// set only s-expression
				vpush(ROP(IS_PUSH), code);				vpush(ast, debug);
				vpush(cloj (cdr(ast), env, NIL, NIL, NIL), code);	vpush(ast, debug);
				return code;

			case SP_MACRO:		// set only s-expression
				vpush(ROP(IS_PUSH), code);				vpush(ast, debug);
				vpush(macro(cdr(ast), env, NIL, NIL, NIL), code);	vpush(ast, debug);
				return code;

			case SP_QUOTE:
				vpush(ROP(IS_PUSHR), code);				vpush(ast, debug);
				vpush(second(ast), code);				vpush(ast, debug);
				return code;

			case SP_QUASIQUOTE:
				return compile_vm_quasiquote (code, debug, second(ast), env);

			case SP_MACROEXPAND:
				return compile_vm_macroexpand(code, debug, second(ast), env);

			case SP_AMP:	//******* ad-hock: ignore it
				return code;

			default:
				return RERR(ERR_NOTIMPL, ast);
		}
	}
	else if(symbolp(fn))
	{
		value_t bfn = compile_vm_check_builtin(first(ast), env);
		if(!nilp(bfn))			// apply builtin
		{
			code = compile_vm_list(code, debug, cdr(ast), env);		// arguments
			if(errp(code))
			{
				return code;
			}
			vpush(bfn, code);		vpush(ast, debug);
			return code;
		}
	}

	// evaluate 1st element of apply list
	code = compile_vm1_fn(code, debug, first(ast), env);
	if(errp(code))
	{
		return code;
	}

	// check function type and select call type
	vpush(ROP(IS_DUP), code);		vpush(ast, debug);
	vpush(ROP(IS_CLOJUREP), code);		vpush(ast, debug);

	int cond_br = vsize(code);
	vpush(ROP(IS_BNIL), code);		vpush(ast, debug);	// dummy operand(0)

	// true situation: clojure call
	code = compile_vm_apply_arg(code, debug, cdr(ast), env);	// arguments
	if(errp(code))
	{
		return code;
	}

	int true_br = vsize(code);
	vpush(ROP(IS_BR), code);		vpush(ast, debug);	// dummy operand(0)

	// false situation: macro call
	code = compile_vm_macro_arg(code, debug, cdr(ast), env);	// arguments
	if(errp(code))
	{
		return code;
	}

	int next_addr = vsize(code);

	// fix branch address: relative to pc
	rplacv(code, cond_br, ROPD(IS_BNIL, true_br + 1 - cond_br));
	rplacv(code, true_br, ROPD(IS_BR,   next_addr - true_br));


	// Join: apply
	vpush(ROP(IS_AP), code);		vpush(ast, debug);
	return code;
}

static value_t compile_vm1_fn(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	value_t r = NIL;
	switch(rtypeof(ast))
	{
		case INT_T:
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(ast,          code);	vpush(ast, debug);
			break;

		case SYM_T:
			r = compile_vm_check_builtin(ast, env); // Builtin Function (is one VM Instruction)
			if(nilp(r))
			{
				r = get_env_ref(ast, env);
				if(errp(r))
				{
					r = ast;		// late bind(will be bound while exec VM)
				}
			}
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(r,            code);	vpush(ast, debug);
			break;

		case REF_T:
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(ast,          code);	vpush(ast, debug);
			break;

		case VEC_T:
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(ast,          code);	vpush(ast, debug);	//****** heap address in code: fix it.
			break;

		case CONS_T:
			if(nilp(ast))
			{
				vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
				vpush(ast,          code);	vpush(ast, debug);
			}
			else
			{
				code = compile_vm_apply(code, debug, ast, env);
			}
			break;

		case PTR_T:
			return RERR(ERR_TYPE, ast);
			break;

		case CLOJ_T:
		case MACRO_T:
			// compile lambda or macro body
			if(nilp(third(ast)))
			{
				value_t lambda_code = clojurep(ast) ?
							compile_vm_lambda(first(ast), second(ast)):
							compile_vm_macro (first(ast), second(ast));
				if(errp(lambda_code))
				{
					return lambda_code;
				}
				else
				{
					rplaca(cdr(cdr(ast)), lambda_code);
				}
			}
			break;

		case SPECIAL_T:
			if(INTOF(ast) != SP_AMP)	//****** ignore '&' for dummy compilation: it is not evaluate.
			{
				return RERR(ERR_NOTIMPL, ast);
			}
			break;


		default:
			return RERR(ERR_NOTIMPL, ast);
	}

	return code;
}

static value_t compile_vm1(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));
	if(symbolp(ast))
	{
		value_t op = compile_vm_check_builtin(ast, env); // Builtin Function (is one VM Instruction)
		if(!nilp(op))
		{
			// create clojure calling built-in function
			int n = compile_vm_get_builtin_argnum(ast, env);
			assert(n >= 0);
			value_t b_code  = make_vector(0);
			value_t b_debug = make_vector(0);
			for(int i = n - 1; i >= 0; i--)
			{
				vpush(ROP(IS_PUSH), b_code);	vpush(ast, b_debug);
				vpush(RREF(0, i),   b_code);	vpush(ast, b_debug);
			}
			vpush(op,          b_code);		vpush(ast, b_debug);
			vpush(ROP(IS_RET), b_code);		vpush(ast, b_debug);

			// generate push-clojure code
			vpush(ROP(IS_PUSH),                  code);			vpush(ast, debug);
			vpush(cloj(NIL, env, cons(b_code, b_debug), NIL, NIL), code);	vpush(ast, debug);
			return code;
		}
	}

	return compile_vm1_fn(code, debug, ast, env);
}

/////////////////////////////////////////////////////////////////////
// public: compile for VM


value_t compile_vm(value_t ast, value_t env)
{
	assert(consp(env));

	value_t code  = make_vector(0);
	value_t debug = make_vector(0);

	code = compile_vm1(code, debug, ast, env);
	if(!errp(code))
	{
		vpush(ROP(IS_HALT), code);	vpush(ast, debug);
	}

	return cons(code, debug);
}

// End of File
/////////////////////////////////////////////////////////////////////
