

#define DEF_EXTERN
#include "builtin.h"
#include "misc.h"

/////////////////////////////////////////////////////////////////////
// public: initialize well-known symbols

void init_global(void)
{
	g_env		= str_to_sym("env");
	g_unquote	= str_to_sym("unquote");
	g_splice_unquote= str_to_sym("splice-unquote");
	g_setq		= str_to_sym("setq");
	g_let		= str_to_sym("let*");
	g_progn		= str_to_sym("progn");
	g_if		= str_to_sym("if");
	g_lambda	= str_to_sym("lambda");
	g_quote		= str_to_sym("quote");
	g_quasiquote	= str_to_sym("quasiquote");
	g_macro		= str_to_sym("macro");
	g_macroexpand	= str_to_sym("macroexpand");
	g_trace		= str_to_sym("*trace*");
	g_debug		= str_to_sym("*debug*");
	g_amp		= str_to_sym("&");
}
