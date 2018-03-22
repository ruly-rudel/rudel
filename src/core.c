#include <assert.h>
#include <stdlib.h>
#include "builtin.h"
#include "core.h"
#include "env.h"
#include "eval.h"
#include "reader.h"
#include "printer.h"


/////////////////////////////////////////////////////////////////////
// private: core functions for rudel environment


#define DEF_FN_VARG(NAME, BODY) \
static value_t NAME(value_t body, value_t env) \
{ \
	return BODY; \
}

#define DEF_FN_1(NAME, BODY) \
static value_t NAME(value_t body, value_t env) \
{ \
	value_t arg1 = car(body); \
	return BODY; \
}

#define DEF_FN_2(NAME, BODY) \
static value_t NAME(value_t body, value_t env) \
{ \
	value_t arg1 = car(body); \
	value_t arg2 = car(cdr(body)); \
	return BODY; \
}

#define DEF_FN_2INT(NAME, BODY) \
static value_t NAME(value_t body, value_t env) \
{ \
	value_t arg1v = car(body); \
	value_t arg2v = car(cdr(body)); \
	if(rtypeof(arg1v) == INT_T && rtypeof(arg2v) == INT_T) \
	{ \
		int arg1 = INTOF(arg1v); \
		int arg2 = INTOF(arg2v); \
		return BODY; \
	} \
	else \
	{ \
		return RERR(ERR_TYPE); \
	} \
}

#define DEF_FN_1_BEGIN(NAME) \
static value_t NAME(value_t body, value_t env) \
{ \
	value_t arg1 = car(body);


#define DEF_FN_END \
}


#define DEF_FN_R(NAME, ZERO, ONE, OTHER) \
static value_t NAME ## _reduce(value_t arg1, value_t arg2) \
{ \
		return OTHER; \
} \
static value_t NAME(value_t body, value_t env) \
{ \
	value_t arg1 = car(body); \
	if(cnilp(body)) \
	{ \
		return ZERO; \
	} \
	else if(cnilp(cdr(body))) \
	{ \
		return ONE; \
	} \
	else \
	{ \
		return reduce(NAME ## _reduce, body); \
	} \
}

////////// basic functions

DEF_FN_1(b_atom,    atom   (arg1)       ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_consp,   consp  (arg1)       ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_seqp,    seqp   (arg1)       ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_strp,    is_str (arg1)       ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_car,     car    (arg1))
DEF_FN_1(b_cdr,     cdr    (arg1))
DEF_FN_2(b_cons,    cons   (arg1, arg2))
DEF_FN_1(b_err,     rerr   (arg1))
DEF_FN_2(b_eq,      eq     (arg1, arg2) ? SYM_TRUE : SYM_FALSE)
DEF_FN_2(b_equal,   equal  (arg1, arg2) ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_eval,    eval   (arg1, last(env)))
DEF_FN_2(b_rplaca,  rplaca (arg1, arg2))
DEF_FN_2(b_rplacd,  rplacd (arg1, arg2))
DEF_FN_2(b_nth,     rtypeof(arg2) != INT_T ? RERR(ERR_TYPE) : nth(INTOF(arg2), arg1))

DEF_FN_VARG(b_gensym, gensym(last(env)))


////////// special functions for rudel (manipulate environment etc...)

DEF_FN_1(b_read_string,  read_str(arg1))

DEF_FN_1_BEGIN(b_slurp)
	if(rtypeof(arg1) != CONS_T)
	{
		return RERR(ERR_TYPE);
	}
	char* fn  = rstr_to_str(arg1);
	value_t r = slurp(fn);
	free(fn);

	return r;
DEF_FN_END

DEF_FN_VARG(b_init,  init(env))


////////// MATH functions

DEF_FN_2INT(b_lt,  arg1 <  arg2 ? SYM_TRUE : SYM_FALSE)
DEF_FN_2INT(b_elt, arg1 <= arg2 ? SYM_TRUE : SYM_FALSE)
DEF_FN_2INT(b_mt,  arg1 >  arg2 ? SYM_TRUE : SYM_FALSE)
DEF_FN_2INT(b_emt, arg1 >= arg2 ? SYM_TRUE : SYM_FALSE)

DEF_FN_R(b_add, RINT(0),       arg1,                 RINT(INTOF(arg1) + INTOF(arg2)))
DEF_FN_R(b_sub, RINT(0),       RINT(-INTOF(arg1)),   RINT(INTOF(arg1) - INTOF(arg2)))
DEF_FN_R(b_mul, RINT(1),       arg1,                 RINT(INTOF(arg1) * INTOF(arg2)))
DEF_FN_R(b_div, RERR(ERR_ARG), arg1,                 RINT(INTOF(arg1) / INTOF(arg2)))

////////// string and print functions
DEF_FN_R(b_str,
		str_to_rstr(""),
		pr_str(arg1, NIL, false),
		nconc(pr_str(arg1, NIL, false), pr_str(arg2, NIL, false)))

DEF_FN_R(b_pr_str,
		str_to_rstr(""),
		pr_str(arg1, NIL, true),
		nconc(pr_str(arg1, NIL, true),
			nconc(str_to_rstr(" "), pr_str(arg2, NIL, true))))

DEF_FN_R(b_println_str,
		str_to_rstr(""),
		pr_str(arg1, NIL, false),
		nconc(pr_str(arg1, NIL, false),
			nconc(str_to_rstr(" "), pr_str(arg2, NIL, false))))

DEF_FN_VARG(b_println, (printline(b_println_str(body, env), stdout), NIL))

DEF_FN_VARG(b_prn,     (printline(b_pr_str     (body, env), stdout), NIL))



/////////////////////////////////////////////////////////////////////
// public: Create root environment

value_t	create_root_env	(void)
{
	value_t key = list(35,
	                      str_to_sym("nil"),
	                      str_to_sym("t"),
	                      str_to_sym("+"),
	                      str_to_sym("-"),
	                      str_to_sym("*"),
	                      str_to_sym("/"),
	                      str_to_sym("pr-str"),
	                      str_to_sym("str"),
	                      str_to_sym("prn"),
	                      str_to_sym("println"),
	                      str_to_sym("consp"),
	                      str_to_sym("seqp"),
	                      str_to_sym("strp"),
	                      str_to_sym("read-string"),
	                      str_to_sym("slurp"),
	                      str_to_sym("eval"),
	                      str_to_sym("cons"),
	                      str_to_sym("err"),
	                      str_to_sym("nth"),
	                      str_to_sym("car"),
	                      str_to_sym("cdr"),
	                      str_to_sym("="),
	                      str_to_sym("eq"),
	                      str_to_sym("atom"),
	                      str_to_sym("init"),
	                      str_to_sym("rplaca"),
	                      str_to_sym("rplacd"),
	                      str_to_sym("gensym"),
	                      str_to_sym("<"),
	                      str_to_sym("<="),
	                      str_to_sym(">"),
	                      str_to_sym(">="),
	                      str_to_sym("*gensym-counter*"),
	                      str_to_sym("*debug*"),
	                      str_to_sym("*trace*")
	                  );

	value_t val = list(35,
			      NIL,
			      str_to_sym("t"),
	                      cfn(RFN(b_add), NIL),
	                      cfn(RFN(b_sub), NIL),
	                      cfn(RFN(b_mul), NIL),
	                      cfn(RFN(b_div), NIL),
	                      cfn(RFN(b_pr_str), NIL),
	                      cfn(RFN(b_str), NIL),
	                      cfn(RFN(b_prn), NIL),
	                      cfn(RFN(b_println), NIL),
	                      cfn(RFN(b_consp), NIL),
	                      cfn(RFN(b_seqp), NIL),
	                      cfn(RFN(b_strp), NIL),
	                      cfn(RFN(b_read_string), NIL),
	                      cfn(RFN(b_slurp), NIL),
	                      cfn(RFN(b_eval), NIL),
	                      cfn(RFN(b_cons), NIL),
	                      cfn(RFN(b_err), NIL),
	                      cfn(RFN(b_nth), NIL),
	                      cfn(RFN(b_car), NIL),
	                      cfn(RFN(b_cdr), NIL),
	                      cfn(RFN(b_equal), NIL),
	                      cfn(RFN(b_eq), NIL),
	                      cfn(RFN(b_atom), NIL),
	                      cfn(RFN(b_init), NIL),
	                      cfn(RFN(b_rplaca), NIL),
	                      cfn(RFN(b_rplacd), NIL),
	                      cfn(RFN(b_gensym), NIL),
	                      cfn(RFN(b_lt), NIL),
	                      cfn(RFN(b_elt), NIL),
	                      cfn(RFN(b_mt), NIL),
	                      cfn(RFN(b_emt), NIL),
			      RINT(0),
	                      NIL,
	                      NIL
			  );

	return create_env(key, val, NIL);
}


// End of File
/////////////////////////////////////////////////////////////////////
