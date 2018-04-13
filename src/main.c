
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

void repl(value_t env)
{
#ifdef USE_LINENOISE
	init_linenoise();
#endif // USE_LINENOISE

	value_t r, c, e;
	for(;;)
	{
		lock_gc();
		r = READ(L"user> ", stdin);
		if(errp(r))
		{
			if(intp(car(r)) && INTOF(car(r)) == ERR_EOF)
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
			c = compile_vm(r, env);
			if(errp(c))
			{
				print(c, stderr);
			}
			else
			{
				unlock_gc();
				e = exec_vm(c, env);
				lock_gc();

				print(e, errp(e) ? stderr : stdout);
			}
		}
		unlock_gc();
	}
}

void rep_file(char* fn, value_t env)
{
	lock_gc();
	value_t fstr = slurp(fn);
	if(errp(fstr))
	{
		print(fstr, stderr);
	}
	else
	{
		value_t s = vnconc(vnconc(str_to_rstr("(progn "), fstr), str_to_rstr(")"));

		value_t c = compile_vm(read_str(s), env);
		if(errp(c))
		{
			print(c, stderr);
		}
		else
		{
			unlock_gc();
			value_t e = exec_vm(c, env);
			lock_gc();

			print(e, errp(e) ? stderr : stdout);
		}
	}
	unlock_gc();

	return ;
}

value_t parse_arg(int argc, char* argv[])
{
	value_t  r   = NIL;
	value_t* cur = &r;
	for(int i = 0; i < argc; i++)
	{
		cur = cons_and_cdr(read_str(str_to_rstr(argv[i])), cur);
	}

	return r;
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

	return 0;
}
