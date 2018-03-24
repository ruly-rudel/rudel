#include <assert.h>
#include "builtin.h"
#include "eval.h"
#include "env.h"
#include "printer.h"
#include "reader.h"

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
		return EQ(ast, g_env) ? env : get_env_value(ast, env);

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
	value_t r = eval_ast(vcdr, env);
	if(errp(r))
	{
		return r;
	}
	else
	{
		return car(last(r));
	}
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
	if(EQ(arg1, RSPECIAL(SP_UNQUOTE)))			// (unquote x)
	{
		value_t arg2 = car(cdr(vcdr));
		return eval(arg2, env);				    // -> return evalueated x
	}
	else if(consp(arg1) &&
		EQ(car(arg1), RSPECIAL(SP_SPLICE_UNQUOTE)))	// ((splice-unquote x) ...)
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
		else if(rtypeof(ast_1st) == REF_T)
		{
			value_t ast_1st_eval = get_env_value_ref(ast_1st, env);
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

#ifdef DEBUG_CMD
static bool is_break(value_t v, value_t env)
{
	// debug hook
	value_t debug_cmd = get_env_value(g_debug, env);
	if(!nilp(debug_cmd))	// debug on
	{
		value_t debug_step = find(str_to_sym("#step#"), debug_cmd, equal);
		value_t is_brk   = find(car(v), debug_cmd, equal);
		if(!nilp(debug_step) || !nilp(is_brk))	// debug REPL
		{
			print(str_to_sym("DEBUG break:"), stderr);
			//eval(list(2, str_to_sym("ppln"), list(2, g_quote, v)), env);
			print(v, stderr);

			return true;
		}
	}

	return false;
}
#endif // DEBUG_CMD

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

	if(rtypeof(vcar) == SPECIAL_T)
	{
		switch(SPECIAL(vcar))
		{
			case SP_SETQ:		return eval_setq	(vcdr, env);
			case SP_LET:		return eval_let		(vcdr, env);
			case SP_PROGN:		return eval_progn	(vcdr, env);
			case SP_IF:		return eval_if		(vcdr, env);
			case SP_LAMBDA:		return cloj		(vcdr, env);
			case SP_QUOTE:		return car		(vcdr);
			case SP_QUASIQUOTE:	return car(eval_quasiquote(vcdr, env));
			case SP_MACRO:		return macro		(vcdr, env);
			case SP_MACROEXPAND:	return macroexpand_all	(car(vcdr), env);
			default:		return RERR(ERR_NOTFN, v);
			//case SP_UNQUOTE:
			//case SP_SPLICE_UNQUOTE:
			//case SP_AMP:
		}
	}
	else
	{
		value_t ev = eval_ast(v, env);
		if(errp(ev))
		{
			return rerr_add_pos(v, ev);
		}
		else
		{
			value_t fn = car(ev);

#ifdef DEBUG_CMD
			// trace enter
			value_t trace_fn = get_env_value(g_trace, env);
			value_t trace_on = nilp(trace_fn) ? NIL : find(car(v), trace_fn, eq);
			if(!nilp(trace_on))
			{
				print(cons(str_to_sym("TRACE enter:"), cons(car(v), cdr(ev))), stderr);
			}

			// debug hook
			bool debug_mode = is_break(cons(car(v), cdr(ev)), env);
#else
			bool debug_mode = false;
#endif // DEBUG_CMD


			// apply
			value_t ret;
			if(rtypeof(fn) == CFN_T)		// compiled function
			{
#ifdef DEBUG_CMD
				if(debug_mode) debug_repl(env);
#endif // DEBUG_CMD

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

#ifdef DEBUG_CMD
			// trace exit
			if(!nilp(trace_on))
			{
				print(list(4, str_to_sym("TRACE exit:"), car(v), str_to_sym("with"), ret), stderr);
			}
#endif // DEBUG_CMD

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

value_t macroexpand_all(value_t ast, value_t env)
{
	if(errp(ast))
	{
		return ast;
	}
	else if(is_str(ast))
	{
		return ast;
	}
	else
	{
		value_t m = macroexpand(ast, env);
		if(rtypeof(m) == CONS_T && !is_str(m))
		{
			value_t r    = NIL;
			value_t* cur = &r;
			while(!nilp(m))
			{
				assert(rtypeof(m) == CONS_T);
				value_t mex = macroexpand_all(car(m), env);
				if(errp(mex))
				{
					return mex;
				}
				else
				{
					cur = cons_and_cdr(mex, cur);
					m = cdr(m);
				}
			}

			return r;
		}
		else
		{
			return m;
		}
	}
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

// End of File
/////////////////////////////////////////////////////////////////////
