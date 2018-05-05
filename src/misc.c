

#define DEF_EXTERN
#include "builtin.h"
#include "misc.h"
#include "allocator.h"
#include "env.h"
#include "package.h"

/////////////////////////////////////////////////////////////////////
// public: initialize well-known symbols

void init_global(void)
{
	lock_gc();

	value_t pkg = in_package(intern(":user", car(last(g_package_list))));

	g_nil		 = intern("nil",		pkg);	push_root(&g_nil);
	g_t		 = intern("t",			pkg);	push_root(&g_t);
	g_env		 = intern("env",		pkg);	push_root(&g_env);
	g_unquote	 = intern("unquote",		pkg);	push_root(&g_unquote);
	g_splice_unquote = intern("splice-unquote",	pkg);	push_root(&g_splice_unquote);
	g_setq		 = intern("setq",		pkg);	push_root(&g_setq);
	g_let		 = intern("let*",		pkg);	push_root(&g_let);
	g_progn		 = intern("progn",		pkg);	push_root(&g_progn);
	g_if		 = intern("if",			pkg);	push_root(&g_if);
	g_lambda	 = intern("lambda",		pkg);	push_root(&g_lambda);
	g_quote		 = intern("quote",		pkg);	push_root(&g_quote);
	g_quasiquote	 = intern("quasiquote",		pkg);	push_root(&g_quasiquote);
	g_macro		 = intern("macro",		pkg);	push_root(&g_macro);
	g_macroexpand	 = intern("macroexpand",	pkg);	push_root(&g_macroexpand);
	g_rest		 = intern("&rest",		pkg);	push_root(&g_rest);
	g_optional	 = intern("&optional",		pkg);	push_root(&g_optional);
	g_key		 = intern("&key",		pkg);	push_root(&g_key);
	g_trace		 = intern("*trace*",		pkg);	push_root(&g_trace);
	g_debug		 = intern("*debug*",		pkg);	push_root(&g_debug);
	g_gensym_counter = intern("*gensym-counter*",	pkg);	push_root(&g_gensym_counter);
	g_exception_stack= intern("*exception-stack*",	pkg);	push_root(&g_exception_stack);
	g_package        = intern("*package*",		pkg);	push_root(&g_package);


	value_t tbl[] = {
		intern("atom",		pkg),		ROP(IS_ATOM),		RINT(1),
		intern("consp",		pkg),		ROP(IS_CONSP),		RINT(1),
		intern("clojurep",	pkg),		ROP(IS_CLOJUREP),	RINT(1),
		intern("macrop",	pkg),		ROP(IS_MACROP),		RINT(1),
		intern("specialp",	pkg),		ROP(IS_SPECIALP),	RINT(1),
		intern("strp",		pkg),		ROP(IS_STRP),		RINT(1),
		intern("errp",		pkg),		ROP(IS_ERRP),		RINT(1),
		intern("cons",		pkg),		ROP(IS_CONS),		RINT(2),
		intern("car",		pkg),		ROP(IS_CAR),		RINT(1),
		intern("cdr",		pkg),		ROP(IS_CDR),		RINT(1),
		intern("eq",		pkg),		ROP(IS_EQ),		RINT(2),
		intern("equal",		pkg),		ROP(IS_EQUAL),		RINT(2),
		intern("rplaca",	pkg),		ROP(IS_RPLACA),		RINT(2),
		intern("rplacd",	pkg),		ROP(IS_RPLACD),		RINT(2),
		intern("gensym",	pkg),		ROP(IS_GENSYM),		RINT(0),
		intern("+",		pkg),		ROP(IS_ADD),		RINT(2),
		intern("-",		pkg),		ROP(IS_SUB),		RINT(2),
		intern("*",		pkg),		ROP(IS_MUL),		RINT(2),
		intern("/",		pkg),		ROP(IS_DIV),		RINT(2),
		intern("<",		pkg),		ROP(IS_LT),		RINT(2),
		intern("<=",		pkg),		ROP(IS_ELT),		RINT(2),
		intern(">",		pkg),		ROP(IS_MT),		RINT(2),
		intern(">=",		pkg),		ROP(IS_EMT),		RINT(2),
		intern("read-string",	pkg),		ROP(IS_READ_STRING),	RINT(1),
		intern("slurp",		pkg),		ROP(IS_SLURP),		RINT(1),
		intern("eval",		pkg),		ROP(IS_EVAL),		RINT(1),
		intern("err",		pkg),		ROP(IS_ERR),		RINT(1),
		intern("nth",		pkg),		ROP(IS_NTH),		RINT(2),
		intern("init",		pkg),		ROP(IS_INIT),		RINT(0),
		intern("save-core",	pkg),		ROP(IS_SAVECORE),	RINT(1),
		intern("make-vector",	pkg),		ROP(IS_MAKE_VECTOR),	RINT(1),
		intern("vref",		pkg),		ROP(IS_VREF),		RINT(2),
		intern("rplacv",	pkg),		ROP(IS_RPLACV),		RINT(3),
		intern("vsize",		pkg),		ROP(IS_VSIZE),		RINT(1),
		intern("veq",		pkg),		ROP(IS_VEQ),		RINT(2),
		intern("vpush",		pkg),		ROP(IS_VPUSH),		RINT(2),
		intern("vpop",		pkg),		ROP(IS_VPOP),		RINT(1),
		intern("copy-vector",	pkg),		ROP(IS_COPY_VECTOR),	RINT(1),
		intern("vconc",		pkg),		ROP(IS_VCONC),		RINT(2),
		intern("vnconc",	pkg),		ROP(IS_VNCONC),		RINT(2),
		intern("compile-vm",	pkg),		ROP(IS_COMPILE_VM),	RINT(1),
		intern("exec-vm",	pkg),		ROP(IS_EXEC_VM),	RINT(1),
		intern("pr_str",	pkg),		ROP(IS_PR_STR),		RINT(2),
		intern("printline",	pkg),		ROP(IS_PRINTLINE),	RINT(1),
		intern("print",		pkg),		ROP(IS_PRINT),		RINT(1),
		intern("callcc",	pkg),		ROP(IS_CALLCC),		RINT(1),
		intern("read",		pkg),		ROP(IS_READ),		RINT(0),
		intern("count",		pkg),		ROP(IS_COUNT),		RINT(1),
		intern("reverse",	pkg),		ROP(IS_REVERSE),	RINT(1),
		intern("nconc",		pkg),		ROP(IS_NCONC),		RINT(2),
	};

	g_istbl_size = sizeof(tbl) / sizeof(tbl[0]) - 1;
	g_istbl      = (value_t*)malloc(sizeof(value_t) * (g_istbl_size + 1));

	for(int i = 0; i <= g_istbl_size; i++)
	{
		g_istbl[i] = tbl[i];
	}
	push_root_raw_vec(g_istbl, &g_istbl_size);
	unlock_gc();
}

void release_global(void)
{
	pop_root(23);
}

// End of File
/////////////////////////////////////////////////////////////////////
