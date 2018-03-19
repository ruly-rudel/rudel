
#define DEF_EXTERN
#include <stdio.h>
#include <assert.h>
#include "builtin.h"
#include "reader.h"
#include "printer.h"
#include "env.h"
#include "eval.h"
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
}

void rep_file(char* fn, value_t env)
{
	value_t c = concat(3, str_to_rstr("(do "), slurp(fn), str_to_rstr(")"));
	c.type.main = STR_T;
	print(eval(read_str(c), env), stdout);
}

int main(int argc, char* argv[])
{
	value_t env = create_env(NIL, NIL, NIL);

	print(init(env), stdout);

	if(argc == 1)
	{
		repl(env);
	}
	else
	{
		rep_file(argv[1], env);
	}

	return 0;
}
