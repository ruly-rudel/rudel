
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
#include "asm.h"

#define STR(X) str_to_rstr(X)

void repl(value_t env)
{
#ifdef USE_LINENOISE
	init_linenoise();
#endif // USE_LINENOISE

	lock_gc();

	// build terminate-catcher code
	value_t str = STR("exception caches at root: ");
	code_t catch = { 0 };
	asm_init(&catch, 6);
	asm_op_nd(IS_PUSH,		&catch);
	asm_im_nd(str,			&catch);
	asm_op_nd(IS_VCONC,		&catch);
	asm_op_nd(IS_PRINTLINE,		&catch);
	asm_op_nd(IS_GOTO,		&catch);

	value_t clojure = cloj(NIL, NIL, asm_code(&catch), env, NIL);
	set_env(g_exception_stack, cons(clojure, NIL), env);

	// build repl code
	code_t code = { 0 };
	asm_init(&code, 27);

	value_t repl_begin = STR(".repl_begin");
	value_t repl_eval  = STR(".repl_eval");
	value_t repl_print = STR(".repl_print");

	asm_label (repl_begin,		&code);
	asm_op_nd (IS_PUSH,		&code);
	asm_im_nd (cons(clojure, NIL),	&code);
	asm_op_nd (IS_PUSHR,		&code);
	asm_im_nd (g_exception_stack,	&code);
	asm_op_nd (IS_SETENV,		&code);

	asm_op_nd (IS_READ,		&code);
	asm_op_nd (IS_DUP,		&code);
	asm_op_nd (IS_ERRP,		&code);
	asm_opl_nd(IS_BNIL, repl_eval,	&code);
	asm_op_nd (IS_DUP,		&code);
	asm_op_nd (IS_CAR,		&code);
	asm_op_nd (IS_CONSP,		&code);
	asm_opd_nd(IS_BNIL, 2,		&code);
	asm_opl_nd(IS_BR, repl_print,	&code);
	asm_op_nd (IS_DUP,		&code);
	asm_op_nd (IS_CAR,		&code);
	asm_op_nd (IS_PUSH,		&code);
	asm_im_nd (RINT(ERR_EOF),	&code);
	asm_op_nd (IS_EQ,		&code);
	asm_opl_nd(IS_BNIL, repl_print,	&code);
	asm_op_nd (IS_POP,		&code);
	asm_op_nd (IS_PUSH,		&code);
	asm_im_nd (g_t,			&code);
	asm_op_nd (IS_HALT,		&code);

	asm_label (repl_eval,		&code);
	asm_op_nd (IS_EVAL,		&code);
	asm_label (repl_print,		&code);
	asm_op_nd (IS_PRINT,		&code);
	asm_opl_nd(IS_BRB, repl_begin,	&code);

	asm_fix_label(&code);

	// save continuation (IS_GOTO in catch code)
	value_t stack = make_vector(0);
	value_t ret   = make_vector(5);

	vpush(code.code,	ret);
	vpush(code.debug,	ret);
	vpush(RINT(-1), 	ret);
	vpush(env,      	ret);
	vpush(stack,    	ret);

	asm_im_nd(ret,		&catch);

	value_t cd = asm_code(&code);

	asm_close(&code);
	asm_close(&catch);

	unlock_gc();

	// exec repl
	exec_vm(cd, env);
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
	value_t env    = NIL;
	push_root(&env);

	env = load_core("init.rudc");
	if(errp(env))
	{
		lock_gc();
		value_t pkg = init_package_list();
		init_global();
		env = init(pkg);
		unlock_gc();
	}
	else
	{
		print(env, cdr(get_env_pkg(env)), stdout);
	}

	if(argc == 1)
	{
		repl(env);
	}
	else
	{
		lock_gc();
		value_t pkg = cdr(get_env_pkg(env));
		value_t val = cdr(parse_arg(argc, argv, pkg));
		value_t key = intern("*ARGV*", pkg);
		set_env(key, val, env);
		unlock_gc();
		rep_file(argv[1], env);
	}

	release_global();
	pop_root(1);
	assert(g_lock_cnt == 0);
	assert(check_lock());
	return 0;
}
