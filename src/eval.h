#ifndef _eval_h_
#define _eval_h_

#include "misc.h"
#include "builtin.h"

value_t apply		(value_t fn, value_t args, bool blk);
value_t macroexpand	(value_t ast, value_t env);
value_t macroexpand_all	(value_t ast, value_t env);
value_t eval		(value_t v, value_t env);

#endif // _eval_h_
