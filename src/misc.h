#ifndef _MISC_H_
#define _MISC_H_

#ifdef DEF_EXTERN
	#define EXTERN
#else  // DEF_EXTERN
	#define EXTERN extern
#endif // DEF_EXTERN

#include "builtin.h"

EXTERN value_t g_env;
EXTERN value_t g_unquote;
EXTERN value_t g_splice_unquote;
EXTERN value_t g_setq;
EXTERN value_t g_let;
EXTERN value_t g_progn;
EXTERN value_t g_if;
EXTERN value_t g_lambda;
EXTERN value_t g_quote;
EXTERN value_t g_quasiquote;
EXTERN value_t g_macro;
EXTERN value_t g_macroexpand;
EXTERN value_t g_trace;
EXTERN value_t g_debug;
EXTERN value_t g_amp;

void init_global(void);

#endif  // _MISC_H_
