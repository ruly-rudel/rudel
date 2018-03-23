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
static value_t s_setq;
static value_t s_let;
static value_t s_progn;
static value_t s_if;
static value_t s_lambda;
static value_t s_quote;
static value_t s_quasiquote;
static value_t s_macro;
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
	    case REF_T:
		return get_env_value_ref(ast, env);

	    case SYM_T:
		return eq(ast, s_env) ? env : get_env_value(ast, env);

	    case CONS_T:
		if(is_str(ast)) return ast;

		while(!nilp(ast))
		{
			assert(rtypeof(ast) == CONS_T);

			value_t ast_car = eval(car(ast), env);
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

static value_t eval_setq(value_t vcdr, value_t env)
{
	// key
	value_t key = car(vcdr);
	if(rtypeof(key) != SYM_T)
	{
		return RERR(ERR_NOTSYM, vcdr);
	}

	// value
	value_t val_notev = cdr(vcdr);
	if(rtypeof(val_notev) != CONS_T)
	{
		return RERR(ERR_ARG, cdr(vcdr));
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
		return val;
	}
	else
	{
		return rerr_add_pos(val_notev, val);
	}
}

static value_t eval_progn(value_t vcdr, value_t env)
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

		// body
		value_t body = eval(car(cdr(bind)), let_env);
		if(errp(body))
		{
			return rerr_add_pos(cdr(bind), body);
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

static void debug_repl(value_t env)
{
	for(;;) {
		value_t r = READ("debug# ", stdin);
		if(errp(r))
		{
			if(rtypeof(car(r)) == INT_T && INTOF(car(r)) == ERR_EOF)
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

static bool is_break(value_t v, value_t env)
{
	// debug hook
	value_t debug_cmd = get_env_value(s_debug, env);
	if(!nilp(debug_cmd))	// debug on
	{
		value_t debug_step = find(str_to_sym("#step#"), debug_cmd, equal);
		value_t is_brk   = find(car(v), debug_cmd, equal);
		if(!nilp(debug_step) || !nilp(is_brk))	// debug REPL
		{
			print(str_to_sym("DEBUG break:"), stderr);
			//eval(list(2, str_to_sym("ppln"), list(2, s_quote, v)), env);
			print(v, stderr);

			return true;
		}
	}

	return false;
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
	else if(nilp(v) || is_str(v))
	{
		return v;
	}

	// some special evaluations
	value_t vcar = car(v);
	value_t vcdr = cdr(v);

	if(eq(vcar, s_setq))		// setq
	{
		return eval_setq(vcdr, env);
	}
	else if(eq(vcar, s_let))	// let*
	{
		return eval_let(vcdr, env);
	}
	else if(eq(vcar, s_progn))	// progn
	{
		return eval_progn(vcdr, env);
	}
	else if(eq(vcar, s_if))		// if
	{
		return eval_if(vcdr, env);
	}
	else if(eq(vcar, s_lambda))	// lambda
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
	else if(eq(vcar, s_macro))	// macro
	{
		return macro(vcdr, env);
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
			return rerr_add_pos(v, ev);
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
			bool debug_mode = is_break(cons(car(v), cdr(ev)), env);

			// apply
			value_t ret;
			if(rtypeof(fn) == CFN_T)		// compiled function
			{
				if(debug_mode) debug_repl(env);

				fn.type.main = CONS_T;
				// apply
				ret = car(fn).rfn(cdr(ev), env);
			}
			else if(rtypeof(fn) == CLOJ_T)		// (ast . env)
			{
				ret = apply(fn, cdr(ev), debug_mode);
			}
			else
			{
				ret = RERR(ERR_NOTFN, ev);
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
		ast = apply(macro, cdr(ast), false);
	}

	return ast;
}

value_t apply(value_t fn, value_t args, bool blk)
{
	fn.type.main = CONS_T;

	value_t fn_ast = car(fn);
	if(rtypeof(fn_ast) != CONS_T)
	{
		return RERR(ERR_ARG, fn);
	}

	// formal arguments from clojure
	value_t fn_args = car(fn_ast);
	if(rtypeof(fn_args) != CONS_T)
	{
		return RERR(ERR_ARG, fn_ast);
	}

	// function body from clojure
	fn_ast = cdr(fn_ast);
	if(rtypeof(fn_ast) != CONS_T)
	{
		return RERR(ERR_ARG, fn_ast);
	}
	value_t fn_body = car(fn_ast);

	// create bindings (is environment)
	value_t fn_env = create_env(fn_args, args, cdr(fn));

	// debug repl
	if(blk) debug_repl(fn_env);

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
		else if(is_str(v))
		{
			return v;
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
	s_setq		= str_to_sym("setq");
	s_let		= str_to_sym("let*");
	s_progn		= str_to_sym("progn");
	s_if		= str_to_sym("if");
	s_lambda	= str_to_sym("lambda");
	s_quote		= str_to_sym("quote");
	s_quasiquote	= str_to_sym("quasiquote");
	s_macro		= str_to_sym("macro");
	s_macroexpand	= str_to_sym("macroexpand");
	s_trace		= str_to_sym("*trace*");
	s_debug		= str_to_sym("*debug*");
}

// End of File
/////////////////////////////////////////////////////////////////////
