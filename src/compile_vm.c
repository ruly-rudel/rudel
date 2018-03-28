
#include <assert.h>
#include "builtin.h"
#include "compile_vm.h"
#include "env.h"

/////////////////////////////////////////////////////////////////////
// private:

value_t compile_vm1(value_t code, value_t ast, value_t env);

// applicative order
value_t compile_vm_list(value_t code, value_t ast, value_t env)
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

value_t compile_vm_let(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	// cereate environment
	if(!consp(ast))
	{
		return RERR(ERR_ARG, ast);
	}

	// allocate new environment
	value_t let_env = create_env(NIL, NIL, env);

	vpush(code, RINT(0));
	vpush(code, RIS(IS_MKVEC));

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

		// body
		code = compile_vm1(code, car(cdr(bind)), let_env);
		if(errp(code))
		{
			return code;
		}
		else
		{
			// add let environment.
			vpush(code, NIL);	// env key is NIL
			set_env(sym, NIL, let_env);
			vpush(code, RIS(IS_CONS));
			vpush(code, RIS(IS_VPUSH));
		}
	}

	vpush(code, RIS(IS_VPUSH_ENV));
	code = compile_vm1(code, car(cdr(ast)), let_env);
	if(errp(code))
	{
		return code;
	}
	else
	{
		vpush(code, RIS(IS_VPOP_ENV));
		return code;
	}
}

value_t compile_vm_apply_builtin(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	// evaluate each list items
	code = compile_vm_list(code, cdr(ast), env);
	if(errp(code))
	{
		return code;
	}

	value_t tbl[] = {
		str_to_sym("+"),             RIS(IS_ADD),
		str_to_sym("-"),             RIS(IS_SUB),
		str_to_sym("*"),             RIS(IS_MUL),
		str_to_sym("/"),             RIS(IS_DIV),
		str_to_sym("equal"),         RIS(IS_EQUAL),
		str_to_sym("eq"),            RIS(IS_EQ),
		str_to_sym("cons"),          RIS(IS_CONS),
		str_to_sym("car"),           RIS(IS_CAR),
		str_to_sym("cdr"),           RIS(IS_CDR),
		str_to_sym("make-vector"),   RIS(IS_MKVEC),
		str_to_sym("vpush"),         RIS(IS_VPUSH),
		str_to_sym("vpop"),          RIS(IS_VPOP),
		str_to_sym("vref"),          RIS(IS_VREF),
	};

	value_t fn = car(ast);
	switch(rtypeof(fn))
	{
		case SYM_T:
			for(int i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i += 2)
			{
				if(EQ(tbl[i], fn))
				{
					vpush(code, tbl[i+1]);
					return code;
				}
			}
			return RERR(ERR_NOTIMPL, ast);

		default:
			return RERR(ERR_NOTIMPL, ast);
	}

	return code;
}

value_t compile_vm_apply(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	value_t fn = car(ast);
	switch(rtypeof(fn))
	{
		case SPECIAL_T:
			switch(INTOF(fn))
			{
				case SP_LET:
					return compile_vm_let(code, cdr(ast), env);

				default:
					return RERR(ERR_NOTIMPL, ast);
			}

		case SYM_T:
			fn = get_env_value(fn, env);
			if(errp(fn))
			{
				return fn;
			}
			else if(cfnp(fn))
			{
				return compile_vm_apply_builtin(code, ast, env);
			}
			else
			{
				return RERR(ERR_NOTIMPL, ast);
			}

		default:
			return RERR(ERR_NOTIMPL, ast);
	}
}

value_t compile_vm1(value_t code, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	value_t r = NIL;
	switch(rtypeof(ast))
	{
		case INT_T:
			vpush(code, ast);
			break;

		case SYM_T:
			r = get_env_ref(ast, env);
			vpush(code, r);
			if(errp(r))
			{
				return r;
			}
			break;

		case REF_T:
			vpush(code, ast);
			break;

		case VEC_T:
			vpush(code, ast);	//****** heap address in code: fix it.
			break;

		case CONS_T:
			if(nilp(ast))
			{
				vpush(code, ast);
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
		vpush(code, RIS(IS_HALT));
	}

	return code;
}

// End of File
/////////////////////////////////////////////////////////////////////
