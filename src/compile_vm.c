
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
			str_to_sym("+"),             ROP(IS_ADD),
			str_to_sym("-"),             ROP(IS_SUB),
			str_to_sym("*"),             ROP(IS_MUL),
			str_to_sym("/"),             ROP(IS_DIV),
			str_to_sym("eq"),            ROP(IS_EQ),
			str_to_sym("equal"),         ROP(IS_EQUAL),
			str_to_sym("<"),             ROP(IS_LT),
			str_to_sym("cons"),          ROP(IS_CONS),
			str_to_sym("car"),           ROP(IS_CAR),
			str_to_sym("cdr"),           ROP(IS_CDR),
			str_to_sym("make-vector"),   ROP(IS_MKVEC),
			str_to_sym("vpush"),         ROP(IS_VPUSH),
			str_to_sym("vpop"),          ROP(IS_VPOP),
			str_to_sym("vref"),          ROP(IS_VREF),
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

	vpush(RINT(0), code);
	vpush(ROP(IS_MKVEC), code);

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
			vpush(NIL, code);	// env key is NIL
			vpush(ROP(IS_CONS), code);
			vpush(ROP(IS_VPUSH), code);
		}
	}

	// push env is in IS_AP
	//vpush(ROP(IS_VPUSH_ENV), code);

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

	// create env vector
	vpush(RINT(0), code);
	vpush(ROP(IS_MKVEC), code);

	// push empty env vector to env
	vpush(ROP(IS_DUP), code);
	vpush(ROP(IS_VPUSH_ENV), code);

	// cereate environment
	// local symbols
	value_t def = car(ast);
	if(rtypeof(def) != CONS_T)
	{
		return RERR(ERR_ARG, ast);
	}

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
			vpush(NIL, code);	// env key is NIL
			vpush(ROP(IS_CONS), code);
			vpush(ROP(IS_VPUSH), code);
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

static value_t compile_vm_lambda(value_t code, value_t ast, value_t env)
{
	//assert(vectorp(code));
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

	// lambda is clojure call
	vpush(cloj(lambda_code, NIL), code);	// clojure environment must be set while exec

	return code;
}

static value_t compile_vm_apply(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	value_t fn = car(ast);

	if(rtypeof(fn) == SPECIAL_T)
	{
		switch(INTOF(fn))
		{
			case SP_LET:
				return compile_vm_let(code, cdr(ast), env);

			case SP_IF:
				return compile_vm_if(code, cdr(ast), env);

			case SP_LAMBDA:
				return compile_vm_lambda(code, cdr(ast), env);

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
