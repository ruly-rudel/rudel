
#include <stdio.h>
#include <assert.h>
#include "builtin.h"
#include "reader.h"
#include "printer.h"
#include "env.h"
#include "eval.h"
#include "resolv.h"
#include "core.h"

void repl(value_t env)
{
#ifdef USE_LINENOISE
	init_linenoise();
#endif // USE_LINENOISE

	value_t r, e;
	for(;;)
	{
		r = READ("user> ", stdin);
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
}

void rep_file(char* fn, value_t env)
{
	value_t fstr = slurp(fn);
	if(errp(fstr))
	{
		print(fstr, stderr);
	}
	else
	{
		value_t c = vnconc(vnconc(str_to_rstr("(progn "), fstr), str_to_rstr(")"));
		print(eval(read_str(c), env), stdout);
	}

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
	init_global();

	value_t env = create_env(NIL, NIL, NIL);
	print(init(env), stdout);

	if(argc == 1)
	{
		repl(env);
	}
	else
	{
		set_env(str_to_sym("*ARGV*"), cdr(parse_arg(argc, argv)), env);
		rep_file(argv[1], env);
	}

	return 0;
}
