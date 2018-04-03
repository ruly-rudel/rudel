

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
		str_to_sym("atom"),		ROP(IS_ATOM),		RINT(1),
		str_to_sym("consp"),		ROP(IS_CONSP),		RINT(1),
		str_to_sym("strp"),		ROP(IS_STRP),		RINT(1),
		str_to_sym("cons"),		ROP(IS_CONS),		RINT(2),
		str_to_sym("car"),		ROP(IS_CAR),		RINT(1),
		str_to_sym("cdr"),		ROP(IS_CDR),		RINT(1),
		str_to_sym("eq"),		ROP(IS_EQ),		RINT(2),
		str_to_sym("equal"),		ROP(IS_EQUAL),		RINT(2),
		str_to_sym("rplaca"),		ROP(IS_RPLACA),		RINT(2),
		str_to_sym("rplacd"),		ROP(IS_RPLACD),		RINT(2),
		str_to_sym("gensym"),		ROP(IS_GENSYM),		RINT(0),
		str_to_sym("+"),		ROP(IS_ADD),		RINT(2),
		str_to_sym("-"),		ROP(IS_SUB),		RINT(2),
		str_to_sym("*"),		ROP(IS_MUL),		RINT(2),
		str_to_sym("/"),		ROP(IS_DIV),		RINT(2),
		str_to_sym("<"),		ROP(IS_LT),		RINT(2),
		str_to_sym("<="),		ROP(IS_ELT),		RINT(2),
		str_to_sym(">"),		ROP(IS_MT),		RINT(2),
		str_to_sym(">="),		ROP(IS_EMT),		RINT(2),
		str_to_sym("read-string"),	ROP(IS_READ_STRING),	RINT(1),
		str_to_sym("slurp"),		ROP(IS_SLURP),		RINT(1),
		str_to_sym("eval"),		ROP(IS_EVAL),		RINT(1),
		str_to_sym("err"),		ROP(IS_ERR),		RINT(1),
		str_to_sym("nth"),		ROP(IS_NTH),		RINT(2),
		str_to_sym("init"),		ROP(IS_INIT),		RINT(0),
		str_to_sym("make-vector"),	ROP(IS_MAKE_VECTOR),	RINT(1),
		str_to_sym("vref"),		ROP(IS_VREF),		RINT(2),
		str_to_sym("rplacv"),		ROP(IS_RPLACV),		RINT(3),
		str_to_sym("vsize"),		ROP(IS_VSIZE),		RINT(1),
		str_to_sym("veq"),		ROP(IS_VEQ),		RINT(2),
		str_to_sym("vpush"),		ROP(IS_VPUSH),		RINT(2),
		str_to_sym("vpop"),		ROP(IS_VPOP),		RINT(1),
		str_to_sym("copy-vector"),	ROP(IS_COPY_VECTOR),	RINT(1),
		str_to_sym("vconc"),		ROP(IS_VCONC),		RINT(2),
		str_to_sym("vnconc"),		ROP(IS_VNCONC),		RINT(2),
		str_to_sym("compile-vm"),	ROP(IS_COMPILE_VM),	RINT(1),
		str_to_sym("exec-vm"),		ROP(IS_EXEC_VM),	RINT(1),
		str_to_sym("pr_str"),		ROP(IS_PR_STR),		RINT(2),
		str_to_sym("printline"),	ROP(IS_PRINTLINE),	RINT(1),
		str_to_sym("slurp"),		ROP(IS_SLURP),		RINT(1),
		str_to_sym("make-vector-from-list"),		ROP(IS_MVFL),		RINT(1),
		str_to_sym("make-list-from-vector"),		ROP(IS_MLFV),		RINT(1),
	};

	g_istbl_size = sizeof(tbl) / sizeof(tbl[0]);
	g_istbl      = (value_t*)malloc(sizeof(value_t) * g_istbl_size);

	for(int i = 0; i < g_istbl_size; i++)
	{
		g_istbl[i] = tbl[i];
	}
}
