
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
