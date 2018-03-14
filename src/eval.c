#include <assert.h>
#include "builtin.h"
#include "eval.h"
#include "env.h"

/////////////////////////////////////////////////////////////////////
// private: eval functions

static value_t eval_ast_list(value_t list, value_t env)
{
	assert(rtypeof(list) == CONS_T);
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
		ret.type.main = CONS_T;
		return ret;
	}
}

static value_t eval_ast	(value_t ast, value_t env)
{
	switch(rtypeof(ast))
	{
	    case SYM_T:
		return get_env_value(ast, env);

	    case CONS_T:
		return eval_ast_list(ast, env);

	    default:
		return ast;
	}
}

static value_t eval_def(value_t vcdr, value_t env)
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

static value_t eval_do(value_t vcdr, value_t env)
{
	return car(last(eval_ast_list(vcdr, env)));
}

static value_t eval_if(value_t vcdr, value_t env)
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

static value_t eval_let(value_t vcdr, value_t env)
{
	if(rtypeof(vcdr) != CONS_T)
	{
		return RERR(ERR_ARG);
	}

	// allocate new environment
	value_t let_env = create_env(NIL, NIL, env);

	// local symbols
	value_t def = car(vcdr);
	if(rtypeof(def) != CONS_T)
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

static value_t eval_quasiquote	(value_t vcdr, value_t env)
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

static value_t eval_defmacro(value_t vcdr, value_t env)
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

static value_t is_macro_call(value_t ast, value_t env)
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

static value_t eval_list(value_t v, value_t env)
{
	assert(rtypeof(v) == CONS_T);

	// expand macro
	v = macroexpand(v, env);
	if(rtypeof(v) != CONS_T)
	{
		return eval_ast(v, env);
	}

	// some special evaluations
	value_t vcar = car(v);
	value_t vcdr = cdr(v);

	if(equal(vcar, str_to_sym("def!")))		// def!
	{
		return eval_def(vcdr, env);
	}
	else if(equal(vcar, str_to_sym("let*")))	// let*
	{
		return eval_let(vcdr, env);
	}
	else if(equal(vcar, str_to_sym("do")))		// do
	{
		return eval_do(vcdr, env);
	}
	else if(equal(vcar, str_to_sym("if")))		// if
	{
		return eval_if(vcdr, env);
	}
	else if(equal(vcar, str_to_sym("fn*")))		// fn*
	{
		return cloj(vcdr, env);
	}
	else if(equal(vcar, str_to_sym("quote")))	// quote
	{
		return car(vcdr);
	}
	else if(equal(vcar, str_to_sym("quasiquote")))	// quasiquote
	{
		return car(eval_quasiquote(vcdr, env));
	}
	else if(equal(vcar, str_to_sym("defmacro!")))	// defmacro!
	{
		return eval_defmacro(vcdr, env);
	}
	else if(equal(vcar, str_to_sym("macroexpand")))	// macroexpand
	{
		return macroexpand(car(vcdr), env);
	}
	else						// apply function
	{
		value_t ev = eval_ast(v, env);
		if(errp(ev))
		{
			return ev;
		}
		else
		{
			value_t fn = car(ev);
			if(rtypeof(fn) == CFN_T)
			{
				fn.type.main = CONS_T;
				// apply
				return car(fn).rfn(cdr(ev), env);
			}
			else if(rtypeof(fn) == CLOJ_T)
			{
				return apply(fn, cdr(ev));
			}
			else
			{
				return RERR(ERR_NOTFN);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////
// public: eval


value_t macroexpand(value_t ast, value_t env)
{
	value_t macro;
	while(!nilp(macro = is_macro_call(ast, env)))
	{
		ast = apply(macro, cdr(ast));
	}

	return ast;
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
	if(rtypeof(fargs) != CONS_T)
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

value_t eval(value_t v, value_t env)
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
		else
		{
			return eval_list(v, env);
		}
	}
	else
	{
		return eval_ast(v, env);
	}
}


// End of File
/////////////////////////////////////////////////////////////////////
