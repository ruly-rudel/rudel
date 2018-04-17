
#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include "builtin.h"
#include "allocator.h"
#include "reader.h"
#include "printer.h"
#include "env.h"
#include "vm.h"
#include "compile_vm.h"

#define SYM(X) str_to_sym(X)
#define STR(X) str_to_rstr(X)

void repl(value_t env)
{
#ifdef USE_LINENOISE
	init_linenoise();
#endif // USE_LINENOISE

	lock_gc();

	// build terminate-catcher code
	value_t str = STR("exception caches at root: ");
	value_t catch = make_vector(7);
	vpush(ROP (IS_PUSH),		catch);
	vpush(RREF(0, 0),		catch);
	vpush(ROP (IS_PUSH),		catch);
	vpush(str,			catch);
	vpush(ROP(IS_VCONC),		catch);
	vpush(ROP(IS_PRINTLINE),	catch);
	vpush(ROP(IS_HALT),		catch);

	value_t clojure = cloj(NIL, NIL, cons(catch, catch), NIL, NIL);
	set_env(str_to_sym("*exception-stack*"), cons(clojure, NIL), last(env));

	// build repl code
	value_t code = make_vector(27);

	vpush(ROP (IS_PUSH),		code);
	vpush(cons(clojure, NIL),       code);
	vpush(ROP (IS_PUSHR),		code);
	vpush(str_to_sym("*exception-stack*"), code);
	vpush(ROP (IS_SETENV),		code);

	vpush(ROP (IS_READ),		code);
	vpush(ROP (IS_DUP),		code);
	vpush(ROP (IS_ERRP),		code);
	vpush(ROPD(IS_BNIL, 16),	code);
	vpush(ROP (IS_DUP),		code);
	vpush(ROP (IS_CAR),		code);
	vpush(ROP (IS_CONSP),		code);
	vpush(ROPD(IS_BNIL, 2),		code);
	vpush(ROPD(IS_BR, 12),		code);
	vpush(ROP (IS_DUP),		code);
	vpush(ROP (IS_CAR),		code);
	vpush(ROP (IS_PUSH),		code);
	vpush(RINT(ERR_EOF),		code);
	vpush(ROP (IS_EQ),		code);
	vpush(ROPD(IS_BNIL, 6),	code);
	vpush(ROP (IS_POP),		code);
	vpush(ROP (IS_PUSH),		code);
	vpush(g_t,			code);
	vpush(ROP (IS_HALT),		code);

	vpush(ROP (IS_EVAL),		code);
	vpush(ROP (IS_PRINT),		code);
	vpush(ROPD(IS_BRB, 26),		code);

	value_t cd = cons(code, code);

	unlock_gc();

	// exec repl
	exec_vm(cd, env);
}

void rep_file(char* fn, value_t env)
{
	lock_gc();

	// build terminate-catcher code
	value_t str = STR("exception caches at root: ");
	value_t catch = make_vector(7);
	vpush(ROP (IS_PUSH),		catch);
	vpush(RREF(0, 0),		catch);
	vpush(ROP (IS_PUSH),		catch);
	vpush(str,			catch);
	vpush(ROP(IS_VCONC),		catch);
	vpush(ROP(IS_PRINTLINE),	catch);
	vpush(ROP(IS_HALT),		catch);

	value_t clojure = cloj(NIL, NIL, cons(catch, catch), NIL, NIL);
	set_env(str_to_sym("*exception-stack*"), cons(clojure, NIL), last(env));

	// build rep code
	value_t rfn   = str_to_rstr(fn);
	value_t begin = str_to_rstr("(progn ");
	value_t end   = str_to_rstr(")");

	value_t code  = make_vector(21);

	vpush(ROP (IS_PUSHR),		code);
	vpush(end,			code);
	vpush(ROP (IS_PUSHR),		code);
	vpush(rfn,			code);
	vpush(ROP (IS_SLURP),		code);
	vpush(ROP (IS_DUP),		code);
	vpush(ROP (IS_ERRP),		code);
	vpush(ROPD(IS_BNIL, 2),		code);
	vpush(ROPD(IS_BR, 11),		code);
	vpush(ROP (IS_PUSHR),		code);
	vpush(begin,			code);
	vpush(ROP (IS_VCONC),		code);
	vpush(ROP (IS_VCONC),		code);
	vpush(ROP (IS_READ_STRING),	code);
	vpush(ROP (IS_DUP),		code);
	vpush(ROP (IS_ERRP),		code);
	vpush(ROPD(IS_BNIL, 2),		code);
	vpush(ROPD(IS_BR, 2),		code);
	vpush(ROP (IS_EVAL),		code);
	vpush(ROP (IS_PRINT),		code);
	vpush(ROP (IS_HALT),		code);

	value_t cd = cons(code, code);

	unlock_gc();

	// exec rep
	exec_vm(cd, env);
}

value_t parse_arg(int argc, char* argv[])
{
	value_t  r   = cons(NIL, NIL);
	value_t  cur = r;
	push_root(&r);
	push_root(&cur);

	for(int i = 0; i < argc; i++)
	{
		CONS_AND_CDR(read_str(str_to_rstr(argv[i])), cur)
	}

	pop_root(2);
	return cdr(r);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
	init_allocator();
	value_t env = load_core("init.rudc");
	if(errp(env))
	{
		init_global();
		env = create_env(NIL, NIL, NIL);
		value_t ini = init(env);

		lock_gc();
		print(ini, stdout);
		unlock_gc();
	}
	else
	{
		lock_gc();
		print(env, stdout);
		unlock_gc();
	}

	if(argc == 1)
	{
		repl(env);
	}
	else
	{
		lock_gc();
		set_env(str_to_sym("*ARGV*"), cdr(parse_arg(argc, argv)), env);
		unlock_gc();
		rep_file(argv[1], env);
	}

	assert(g_lock_cnt == 0);
	assert(check_lock());
	return 0;
}
