
#define DEF_EXTERN
#include <stdio.h>
#include <assert.h>
#include "builtin.h"
#include "reader.h"
#include "printer.h"
#include "env.h"
#include "eval.h"

value_t read(FILE* fp)
{
	value_t str = readline(fp);
	if(errp(str))
	{
		return str;
	}
	else
	{
		return read_str(str);
	}
}

value_t eval(value_t v, value_t env);

value_t eval_list(value_t v, value_t env)
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
#ifdef MAL
		else if(nilp(car(v)) && nilp(cdr(v)))	// empty list
		{
			return v;
		}
#endif // MAL
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

void print(value_t s, FILE* fp)
{
	printline(pr_str(s, cons(RINT(0), NIL), true), fp);
	return;
}

int main(int argc, char* argv[])
{
	value_t r, e;
	value_t env = create_root_env();

	// implement not
	eval(read_str(str_to_rstr("(def! not (fn* (a) (if a nil t)))")), env);

	// implement load_file
	eval(read_str(str_to_rstr("(def! load-file (fn* (f) (eval (read-string (str \"(do \" (slurp f) \")\")))))")), env);

	// implement cond
	eval(read_str(str_to_rstr("(defmacro! cond (fn* (& xs) (if (> (count xs) 0) (list 'if (first xs) (if (> (count xs) 1) (nth xs 1) (throw \"odd number of forms to cond\")) (cons 'cond (rest (rest xs)))))))")), env);

	// implement or
	eval(read_str(str_to_rstr("(defmacro! or (fn* (& xs) (if (empty? xs) nil (if (= 1 (count xs)) (first xs) `(let* (or_FIXME ~(first xs)) (if or_FIXME or_FIXME (or ~@(rest xs))))))))")), env);

	for(;;)
	{
		fprintf(stdout, "user> ");
		r = read(stdin);
		if(errp(r))
		{
			if(r.type.val == ERR_EOF)
			{
				fprintf(stdout, "\n");
				break;
			}
			else
			{
				print(r, stderr);
			}
		}
		else
		{
			e = eval(r, env);
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

	return 0;
}
