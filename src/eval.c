#include <assert.h>
#include "builtin.h"
#include "eval.h"
#include "env.h"
#include "printer.h"
#include "reader.h"

/////////////////////////////////////////////////////////////////////
// private: well-known symbols

static value_t s_env;
static value_t s_unquote;
static value_t s_splice_unquote;
static value_t s_def;
static value_t s_let;
static value_t s_do;
static value_t s_if;
static value_t s_fn;
static value_t s_quote;
static value_t s_quasiquote;
static value_t s_defmacro;
static value_t s_macroexpand;
static value_t s_trace;
static value_t s_debug;

/////////////////////////////////////////////////////////////////////
// private: eval functions


static value_t eval_ast	(value_t ast, value_t env)
{
	value_t new_ast = NIL;
	value_t* cur    = &new_ast;

	switch(rtypeof(ast))
	{
	    case SYM_T:
		return eq(ast, s_env) ? env : get_env_value(ast, env);

	    case CONS_T:
		while(!nilp(ast))
		{
			assert(rtypeof(ast) == CONS_T);

			value_t ast_car = eval(car(ast), env);
#ifndef DONT_ABORT_EVAL_ON_ERROR
			if(errp(ast_car))
			{
				return ast_car;
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
		value_t target = find_env_all(key, env);
		if(nilp(target))
		{
			set_env(key, val, env);		// set to current
		}
		else
		{
			rplacd(target, val);		// replace
		}
	}
	return val;
}

static value_t eval_do(value_t vcdr, value_t env)
{
	return car(last(eval_ast(vcdr, env)));
}

static value_t eval_if(value_t vcdr, value_t env)
{
	value_t cond = eval(car(vcdr), env);

	if(nilp(cond))	// false
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
	if(!consp(vcdr))					// raw
	{
		return vcdr;					    // -> return as is.
	}

	value_t arg1 = car(vcdr);
	if(eq(arg1, s_unquote))			// (unquote x)
	{
		value_t arg2 = car(cdr(vcdr));
		return eval(arg2, env);				    // -> return evalueated x
	}
	else if(consp(arg1) &&
		eq(car(arg1), s_splice_unquote))	// ((splice-unquote x) ...)
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

	// val
	value_t val = eval(car(cdr(vcdr)), env);
	if(!errp(val) && val.type.main == CLOJ_T)
	{
		val.type.main = MACRO_T;
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
			value_t ast_1st_eval = cdr(find_env_all(ast_1st, env));
			if(rtypeof(ast_1st_eval) == MACRO_T)
			{
				return ast_1st_eval;
			}
		}
	}

	return NIL;
}

static value_t eval_apply(value_t v, value_t env)
{
	assert(rtypeof(v) == CONS_T);

	// expand macro
	v = macroexpand(v, env);
	if(rtypeof(v) != CONS_T)
	{
		return eval_ast(v, env);
	}
	else if(nilp(v))
	{
		return v;
	}

	// some special evaluations
	value_t vcar = car(v);
	value_t vcdr = cdr(v);

	if(eq(vcar, s_def))		// def!
	{
		return eval_def(vcdr, env);
	}
	else if(eq(vcar, s_let))	// let*
	{
		return eval_let(vcdr, env);
	}
	else if(eq(vcar, s_do))		// do
	{
		return eval_do(vcdr, env);
	}
	else if(eq(vcar, s_if))		// if
	{
		return eval_if(vcdr, env);
	}
	else if(eq(vcar, s_fn))		// fn*
	{
		return cloj(vcdr, env);
	}
	else if(eq(vcar, s_quote))	// quote
	{
		return car(vcdr);
	}
	else if(eq(vcar, s_quasiquote))	// quasiquote
	{
		return car(eval_quasiquote(vcdr, env));
	}
	else if(eq(vcar, s_defmacro))	// defmacro!
	{
		return eval_defmacro(vcdr, env);
	}
	else if(eq(vcar, s_macroexpand))	// macroexpand
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

			// trace enter
			value_t trace_fn = get_env_value(s_trace, env);
			value_t trace_on = nilp(trace_fn) ? NIL : find(car(v), trace_fn, eq);
			if(!nilp(trace_on))
			{
				print(cons(str_to_sym("TRACE enter:"), cons(car(v), cdr(ev))), stderr);
			}

			// debug hook
			value_t debug_cmd = get_env_value(s_debug, env);
			if(!nilp(debug_cmd))	// debug on
			{
				value_t debug_step = find(str_to_sym("#step#"), debug_cmd, equal);
				value_t is_break   = find(car(v), debug_cmd, equal);
				if(!nilp(debug_step) || !nilp(is_break))	// debug REPL
				{
					print(cons(str_to_sym("DEBUG break:"), cons(car(v), cdr(ev))), stderr);
					for(;;) {
						value_t r = READ("debug> ", stdin);
						if(errp(r))
						{
							if(r.type.val == ERR_EOF)
							{
								break;
							}
							else
							{
								print(r, stderr);
							}
						}
						else
						{
							value_t e = eval(r, env);
							if(!errp(e))
							{
								print(e, stdout);
							}
							else
							{
								print(e, stderr);
							}
						}
					}
				}

			}

			// apply
			value_t ret;
			if(rtypeof(fn) == CFN_T)		// compiled function
			{
				fn.type.main = CONS_T;
				// apply
				ret = car(fn).rfn(cdr(ev), env);
			}
			else if(rtypeof(fn) == CLOJ_T)		// (ast . env)
			{
				ret = apply(fn, cdr(ev));
			}
			else
			{
				ret = RERR(ERR_NOTFN);
			}

			// trace exit
			if(!nilp(trace_on))
			{
				print(list(4, str_to_sym("TRACE exit:"), car(v), str_to_sym("with"), ret), stderr);
			}

			return ret;
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
	value_t fn_args = car(fn_ast);
	if(rtypeof(fn_args) != CONS_T)
	{
		return RERR(ERR_ARG);
	}

	// function body from clojure
	fn_ast = cdr(fn_ast);
	if(rtypeof(fn_ast) != CONS_T)
	{
		return RERR(ERR_ARG);
	}
	value_t fn_body = car(fn_ast);

	// create bindings (is environment)
	value_t fn_env = create_env(fn_args, args, cdr(fn));

	// apply
	return eval(fn_body, fn_env);
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
			return eval_apply(v, env);
		}
	}
	else
	{
		return eval_ast(v, env);
	}
}

void init_eval(void)
{
	s_env		= str_to_sym("env");
	s_unquote	= str_to_sym("unquote");
	s_splice_unquote= str_to_sym("splice-unquote");
	s_def		= str_to_sym("def!");
	s_let		= str_to_sym("let*");
	s_do		= str_to_sym("do");
	s_if		= str_to_sym("if");
	s_fn		= str_to_sym("fn*");
	s_quote		= str_to_sym("quote");
	s_quasiquote	= str_to_sym("quasiquote");
	s_defmacro	= str_to_sym("defmacro!");
	s_macroexpand	= str_to_sym("macroexpand");
	s_trace		= str_to_sym("*trace*");
	s_debug		= str_to_sym("*debug*");
}

// End of File
/////////////////////////////////////////////////////////////////////
