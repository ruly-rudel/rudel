

#define DEF_EXTERN
#include "builtin.h"
#include "misc.h"

/////////////////////////////////////////////////////////////////////
// public: initialize well-known symbols

void init_global(void)
{
	g_t		= str_to_sym("t");
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


	value_t tbl[] = {
		str_to_sym("atom"),		ROP(IS_ATOM),
		str_to_sym("consp"),		ROP(IS_CONSP),
		str_to_sym("strp"),		ROP(IS_STRP),
		str_to_sym("cons"),		ROP(IS_CONS),
		str_to_sym("car"),		ROP(IS_CAR),
		str_to_sym("cdr"),		ROP(IS_CDR),
		str_to_sym("eq"),		ROP(IS_EQ),
		str_to_sym("equal"),		ROP(IS_EQUAL),
		str_to_sym("rplaca"),		ROP(IS_RPLACA),
		str_to_sym("rplacd"),		ROP(IS_RPLACD),
		str_to_sym("gensym"),		ROP(IS_GENSYM),
		str_to_sym("+"),		ROP(IS_ADD),
		str_to_sym("-"),		ROP(IS_SUB),
		str_to_sym("*"),		ROP(IS_MUL),
		str_to_sym("/"),		ROP(IS_DIV),
		str_to_sym("<"),		ROP(IS_LT),
		str_to_sym("<="),		ROP(IS_ELT),
		str_to_sym(">"),		ROP(IS_MT),
		str_to_sym(">="),		ROP(IS_EMT),
		str_to_sym("read-string"),	ROP(IS_READ_STRING),
		str_to_sym("slurp"),		ROP(IS_SLURP),
		str_to_sym("eval"),		ROP(IS_EVAL),
		str_to_sym("err"),		ROP(IS_ERR),
		str_to_sym("nth"),		ROP(IS_NTH),
		str_to_sym("init"),		ROP(IS_INIT),
		str_to_sym("make-vector"),	ROP(IS_MAKE_VECTOR),
		str_to_sym("vref"),		ROP(IS_VREF),
		str_to_sym("rplacv"),		ROP(IS_RPLACV),
		str_to_sym("vsize"),		ROP(IS_VSIZE),
		str_to_sym("veq"),		ROP(IS_VEQ),
		str_to_sym("vpush"),		ROP(IS_VPUSH),
		str_to_sym("vpop"),		ROP(IS_VPOP),
		str_to_sym("copy-vector"),	ROP(IS_COPY_VECTOR),
		str_to_sym("vconc"),		ROP(IS_VCONC),
		str_to_sym("vnconc"),		ROP(IS_VNCONC),
		str_to_sym("compile-vm"),	ROP(IS_COMPILE_VM),
		str_to_sym("exec-vm"),		ROP(IS_EXEC_VM),
		str_to_sym("pr_str"),		ROP(IS_PR_STR),
		str_to_sym("printline"),	ROP(IS_PRINTLINE),
		str_to_sym("slurp"),		ROP(IS_SLURP),
		str_to_sym("make-vector-from-list"),		ROP(IS_MVFL),
		str_to_sym("make-list-from-vector"),		ROP(IS_MLFV),
	};

	g_istbl_size = sizeof(tbl) / sizeof(tbl[0]);
	g_istbl      = (value_t*)malloc(sizeof(value_t) * g_istbl_size);

	for(int i = 0; i < g_istbl_size; i++)
	{
		g_istbl[i] = tbl[i];
	}
}
