#ifndef _eval_h_
#define _eval_h_

#include "misc.h"
#include "builtin.h"

value_t eval_ast	(value_t ast, value_t env);
value_t eval_ast_list	(value_t list, value_t env);
value_t eval_def	(value_t vcdr, value_t env);
value_t eval_let	(value_t vcdr, value_t env);
value_t eval_do		(value_t vcdr, value_t env);
value_t eval_if		(value_t vcdr, value_t env);
value_t eval_quasiquote	(value_t vcdr, value_t env);
value_t eval_defmacro	(value_t vcdr, value_t env);
value_t apply		(value_t fn, value_t ev);
value_t macroexpand	(value_t ast, value_t env);

#endif // _eval_h_
