
#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include "builtin.h"
#include "allocator.h"
#include "reader.h"
#include "printer.h"
#include "package.h"
#include "env.h"
#include "vm.h"
#include "compile_vm.h"

#define SYM(X) str_to_sym(X)
#define STR(X) str_to_rstr(X)

void repl(value_t pkg)
{
#ifdef USE_LINENOISE
	init_linenoise();
#endif // USE_LINENOISE

	lock_gc();

	// build terminate-catcher code
	value_t str = STR("exception caches at root: ");
	value_t catch = make_vector(8);
	vpush(ROP (IS_PUSH),		catch);
	vpush(str,			catch);
	vpush(ROP(IS_VCONC),		catch);
	vpush(ROP(IS_PRINTLINE),	catch);
	vpush(ROP(IS_GOTO),		catch);

	value_t env = get_pkg_env(pkg);
	value_t clojure = cloj(NIL, NIL, cons(catch, catch), env, NIL);
	set_env(g_exception_stack, cons(clojure, NIL), env);

	// build repl code
	value_t code = make_vector(27);

	vpush(ROP (IS_PUSH),		code);
	vpush(cons(clojure, NIL),       code);
	vpush(ROP (IS_PUSHR),		code);
	vpush(g_exception_stack,        code);
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

	// save continuation
	value_t stack = make_vector(0);
	value_t ret   = make_vector(5);

	vpush(code,     ret);
	vpush(code,     ret);
	vpush(RINT(-1), ret);
	vpush(env,      ret);
	vpush(stack,    ret);

	vpush(ret,      catch);	// continuation itself is stored in catch code.

	value_t cd = cons(code, code);

	unlock_gc();

	// exec repl
	exec_vm(cd, pkg);
}

value_t parse_arg(int argc, char* argv[], value_t pkg)
{
	push_root(&pkg);
	value_t  r   = cons(NIL, NIL);
	value_t  cur = r;
	push_root(&r);
	push_root(&cur);

	for(int i = 0; i < argc; i++)
	{
		CONS_AND_CDR(read_str(str_to_rstr(argv[i]), pkg), cur)
	}

	pop_root(3);
	return cdr(r);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
	init_allocator();

	g_package_list = NIL;
	value_t pkg    = NIL;
	push_root(&pkg);

	pkg = load_core("init.rudc");
	if(errp(pkg))
	{
		lock_gc();
		pkg = create_package(str_to_rstr("user"));
		init_global(pkg);
		init(pkg);
		print(pkg, stdout);
		unlock_gc();
	}

	if(argc == 1)
	{
		repl(pkg);
	}
	else
	{
		value_t val = cdr(parse_arg(argc, argv, pkg));
		push_root(&val);
		value_t key = intern("*ARGV*", pkg);
		value_t env = get_pkg_env(pkg);
		set_env(key, val, env);
		pop_root(1);
		rep_file(argv[1], pkg);
	}

	release_global();
	pop_root(1);
	assert(g_lock_cnt == 0);
	assert(check_lock());
	return 0;
}
