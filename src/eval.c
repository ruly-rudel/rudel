#include <assert.h>
#include "builtin.h"
#include "eval.h"
#include "env.h"

value_t eval(value_t v, value_t env);

/////////////////////////////////////////////////////////////////////
// private: eval functions


/////////////////////////////////////////////////////////////////////
// public: eval

value_t eval_ast	(value_t ast, value_t env)
{
	switch(rtypeof(ast))
	{
	    case SYM_T:
		return get_env_value(ast, env);

	    case CONS_T:
	    case VEC_T:
		return eval_ast_list(ast, env);

	    default:
		return ast;
	}
}

value_t eval_ast_list(value_t list, value_t env)
{
	assert(rtypeof(list) == CONS_T || rtypeof(list) == VEC_T);
	bool is_vec = rtypeof(list) == VEC_T;
	list.type.main = CONS_T;

	if(nilp(list))
	{
		return NIL;
	}
	else
	{
		value_t lcar = eval(car(list), env);
		if(errp(lcar))
		{
			return lcar;
		}

		value_t lcdr = eval_ast_list(cdr(list), env);
		if(errp(lcdr))
		{
			return lcdr;
		}

		value_t ret = cons(lcar, lcdr);
		ret.type.main = is_vec ? VEC_T : CONS_T;
		return ret;
	}
}

value_t eval_def(value_t vcdr, value_t env)
{
	// key
	value_t key = car(vcdr);
	if(rtypeof(key) != SYM_T)
	{
		return RERR(ERR_NOTSYM);
	}

	// value
	value_t val_notev = cdr(vcdr);
	if(rtypeof(val_notev) != CONS_T)
	{
		return RERR(ERR_ARG);
	}

	value_t val = eval(car(val_notev), env);

	if(!errp(val))
	{
		set_env(key, val, env);
	}
	return val;
}

value_t eval_do(value_t vcdr, value_t env)
{
	return car(last(eval_ast_list(vcdr, env)));
}

value_t eval_if(value_t vcdr, value_t env)
{
	value_t cond = eval(car(vcdr), env);

	if(nilp(cond) || equal(cond, SYM_FALSE))	// false
	{
		return eval(car(cdr(cdr(vcdr))), env);
	}
	else		// true
	{
		return eval(car(cdr(vcdr)), env);
	}
}

value_t eval_let(value_t vcdr, value_t env)
{
	if(rtypeof(vcdr) != CONS_T)
	{
		return RERR(ERR_ARG);
	}

	// allocate new environment
	value_t let_env = create_env(NIL, NIL, env);

	// local symbols
	value_t def = car(vcdr);
	if(rtypeof(def) != CONS_T && rtypeof(def) != VEC_T)
	{
		return RERR(ERR_ARG);
	}
	def.type.main = CONS_T;

	while(!nilp(def))
	{
		// symbol
		value_t sym = car(def);
		def = cdr(def);
		if(rtypeof(sym) != SYM_T)
		{
			return RERR(ERR_ARG);
		}

		// body
		value_t body = eval(car(def), let_env);
		def = cdr(def);
		if(errp(body))
		{
			return body;
		}
		else
		{
			set_env(sym, body, let_env);
		}
	}

	return eval(car(cdr(vcdr)), let_env);
}

value_t eval_quasiquote	(value_t vcdr, value_t env)
{
	if(!is_pair(vcdr))					// raw
	{
		return vcdr;					    // -> return as is.
	}

	value_t arg1 = car(vcdr);
	if(equal(arg1, str_to_sym("unquote")))			// (unquote x)
	{
		value_t arg2 = car(cdr(vcdr));
		return eval(arg2, env);				    // -> return evalueated x
	}
	else if(is_pair(arg1) &&
		equal(car(arg1), str_to_sym("splice-unquote")))	// ((splice-unquote x) ...)
	{
		return concat(2,
		              eval(car(cdr(arg1)), env),
		              eval_quasiquote(cdr(vcdr), env));	    // -> return evaluated x ...
	}
	else							// other
	{
		return cons(eval_quasiquote(arg1,      env),
		            eval_quasiquote(cdr(vcdr), env));	    // -> return cons quasiquote is applied
	}
}

value_t eval_defmacro(value_t vcdr, value_t env)
{
	// key
	value_t key = car(vcdr);
	if(rtypeof(key) != SYM_T)
	{
		return RERR(ERR_NOTSYM);
	}

	// value
	value_t val_notev = cdr(vcdr);
	if(rtypeof(val_notev) != CONS_T)
	{
		return RERR(ERR_ARG);
	}

	value_t val = eval(car(val_notev), env);
	assert(rtypeof(val) == CLOJ_T);
	val.type.main = MACRO_T;

	if(!errp(val))
	{
		set_env(key, val, env);
	}
	return val;
}

value_t is_macro_call(value_t ast, value_t env)
{
	if(rtypeof(ast) == CONS_T)
	{
		value_t ast_1st = car(ast);
		if(rtypeof(ast_1st) == SYM_T)
		{
			value_t ast_1st_eval = get_env_value(ast_1st, env);
			if(rtypeof(ast_1st_eval) == MACRO_T)
			{
				return ast_1st_eval;
			}
		}
	}

	return NIL;
}

value_t apply(value_t fn, value_t args)
{
	fn.type.main = CONS_T;

	value_t fn_ast = car(fn);
	if(rtypeof(fn_ast) != CONS_T)
	{
		return RERR(ERR_ARG);
	}

	// formal arguments from clojure
	value_t fargs = car(fn_ast);
	if(rtypeof(fargs) != CONS_T && rtypeof(fargs) != VEC_T)
	{
		return RERR(ERR_ARG);
	}
	fargs.type.main = CONS_T;

	// function body from clojure
	fn_ast = cdr(fn_ast);
	if(rtypeof(fn_ast) != CONS_T)
	{
		return RERR(ERR_ARG);
	}
	value_t body = car(fn_ast);

	// create bindings (is environment)
	value_t fn_env = create_env(fargs, args, cdr(fn));

	// apply
	return eval(body, fn_env);
}

value_t macroexpand(value_t ast, value_t env)
{
	value_t macro;
	while(!nilp(macro = is_macro_call(ast, env)))
	{
		ast = apply(macro, cdr(ast));
	}

	return ast;
}

// End of File
/////////////////////////////////////////////////////////////////////
