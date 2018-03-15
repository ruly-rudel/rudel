
#define DEF_EXTERN
#include <stdio.h>
#include <assert.h>
#include "builtin.h"
#include "reader.h"
#include "printer.h"
#include "env.h"
#include "eval.h"
#include "core.h"

int main(int argc, char* argv[])
{
#ifdef USE_LINENOISE
	init_linenoise();
#endif // USE_LINENOISE

	value_t r, e;
	value_t env = create_root_env();

	eval(read_str(str_to_rstr("(eval (read-string (str \"(do \" (slurp \"init.rud\") \")\")))")), env);

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

	return 0;
}
