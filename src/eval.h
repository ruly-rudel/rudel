#ifndef _eval_h_
#define _eval_h_

#include "misc.h"
#include "builtin.h"

value_t apply		(value_t fn, value_t ev);
value_t macroexpand	(value_t ast, value_t env);
value_t eval		(value_t v, value_t env);
void	init_eval	(void);

#endif // _eval_h_
