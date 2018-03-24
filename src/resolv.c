#include <assert.h>
#include "builtin.h"
#include "eval.h"
#include "env.h"
#include "resolv.h"

/////////////////////////////////////////////////////////////////////
// private: resolv functions

static value_t resolv_lambda(value_t vcdr, value_t env);

static value_t resolv_ast	(value_t ast, value_t env)
{
	value_t new_ast = NIL;
	value_t* cur    = &new_ast;

	switch(rtypeof(ast))
	{
	    case SYM_T:
		return EQ(ast, g_env) ? g_env : get_env_ref(ast, env);

	    case CLOJ_T:
	    case MACRO_T:
		new_ast = resolv_lambda(car(ast), cdr(ast));
		if(errp(new_ast))
		{
			return rerr_add_pos(ast, new_ast);
		}
		else
		{
			if(rtypeof(ast) == MACRO_T)
			{
				return macro(new_ast, cdr(ast));
			}
			else
			{
				return cloj(new_ast, cdr(ast));
			}
		}

	    case CONS_T:
		if(is_str(ast)) return ast;

		while(!nilp(ast))
		{
			assert(rtypeof(ast) == CONS_T);

			value_t ast_car = resolv_bind(car(ast), env);
#ifndef DONT_ABORT_EVAL_ON_ERROR
			if(errp(ast_car))
			{
				return rerr_add_pos(ast, ast_car);
			}
#endif
			cur = cons_and_cdr(ast_car, cur);
			ast = cdr(ast);
		}
		return new_ast;

	    default:
		return ast;
	}
}

static value_t resolv_let(value_t vcdr, value_t env)
{
	if(rtypeof(vcdr) != CONS_T)
	{
		return RERR(ERR_ARG, vcdr);
	}

	// allocate new environment
	value_t let_env = create_env(NIL, NIL, env);

	// local symbols
	value_t def = car(vcdr);
	if(rtypeof(def) != CONS_T)
	{
		return RERR(ERR_ARG, vcdr);
	}

	value_t new_def = NIL;
	value_t* cur    = &new_def;
	while(!nilp(def))
	{
		value_t bind = car(def);
		def = cdr(def);

		// symbol
		value_t sym = car(bind);
		if(rtypeof(sym) != SYM_T)
		{
			return RERR(ERR_ARG, bind);
		}

		// add placeholder to let_env for self-recursive function
		set_env(sym, NIL, let_env);

		// body
		value_t body = resolv_bind(car(cdr(bind)), let_env);
		if(errp(body))
		{
			return rerr_add_pos(cdr(bind), body);
		}
		else
		{
			cur = cons_and_cdr(list(2, sym, body), cur);
			set_env(sym, body, let_env);
		}
	}

	value_t rv = resolv_bind(car(cdr(vcdr)), let_env);
	if(errp(rv))
	{
		return rerr_add_pos(cdr(vcdr), rv);
	}
	else
	{
		return list(2, new_def, rv);
	}
}

static value_t resolv_lambda(value_t vcdr, value_t env)
{
	if(rtypeof(vcdr) != CONS_T)
	{
		return RERR(ERR_ARG, vcdr);
	}

	// local symbols
	value_t def = car(vcdr);
	if(rtypeof(def) != CONS_T)
	{
		return RERR(ERR_ARG, vcdr);
	}

	// allocate new environment
	value_t lambda_env = create_env(def, NIL, env);

	//return list(2, def, resolv_bind(car(cdr(vcdr)), lambda_env));
	value_t rv = resolv_bind(car(cdr(vcdr)), lambda_env);
	if(errp(rv))
	{
		return rerr_add_pos(cdr(vcdr), rv);
	}
	else
	{
		return list(2, def, rv);
	}
}

static value_t resolv_apply(value_t v, value_t env)
{
	assert(rtypeof(v) == CONS_T);

	// expand macro
	v = macroexpand(v, env);
	if(rtypeof(v) != CONS_T)
	{
		return resolv_ast(v, env);
	}
	else if(nilp(v) || is_str(v))
	{
		return v;
	}

	// some special evaluations
	value_t vcar = car(v);
	value_t vcdr = cdr(v);

	value_t rv;
	if(rtypeof(vcar) == SPECIAL_T)
	{
		switch(SPECIAL(vcar))
		{
			case SP_SETQ:
			case SP_PROGN:
			case SP_IF:
			case SP_MACROEXPAND:
				rv = resolv_ast(vcdr, env);
				break;

			case SP_QUOTE:
			case SP_QUASIQUOTE:
				return v;	// ***** ad-hock

			case SP_LET:
				rv = resolv_let(vcdr, env);
				break;

			case SP_LAMBDA:
			case SP_MACRO:
				rv = resolv_lambda(vcdr, env);
				break;

			default:
				return RERR(ERR_NOTFN, v);

		}

		if(!errp(rv))
		{
			rv = cons(vcar, rv);
		}

	}
	else						// apply function
	{
		rv = resolv_ast(v, env);
	}

	if(errp(rv))
	{
		return rerr_add_pos(v, rv);
	}
	else
	{
		return rv;
	}
}


/////////////////////////////////////////////////////////////////////
// public: bind resolver

value_t resolv_bind(value_t v, value_t env)
{
	if(errp(v))
	{
		return v;
	}
	else if(rtypeof(v) == CONS_T)
	{
		if(nilp(v))
		{
			return NIL;
		}
		else if(is_str(v))
		{
			return v;
		}
		else
		{
			return resolv_apply(v, env);
		}
	}
	else
	{
		return resolv_ast(v, env);
	}
}

// End of File
/////////////////////////////////////////////////////////////////////
