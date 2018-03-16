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
		int arg1 = arg1v.rint.val; \
		int arg2 = arg2v.rint.val; \
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
	value_t arg2 = car(cdr(body)); \
	if(nilp(body)) \
	{ \
		return ZERO; \
	} \
	else if(nilp(cdr(body))) \
	{ \
		return ONE; \
	} \
	else \
	{ \
		return reduce(NAME ## _reduce, body); \
	} \
}

////////// basic functions

DEF_FN_1(b_atom,  atom(arg1)  ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_consp, consp(arg1) ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_car,   car(arg1))
DEF_FN_1(b_cdr,   cdr(arg1))
DEF_FN_2(b_cons,  cons(arg1, arg2))
DEF_FN_2(b_eq,    eq(arg1, arg2)    ? SYM_TRUE : SYM_FALSE)
DEF_FN_2(b_equal, equal(arg1, arg2) ? SYM_TRUE : SYM_FALSE)
DEF_FN_1(b_eval,  eval(arg1, last(env)))


////////// special functions for rudel (manipulate environment etc...)

DEF_FN_1(b_read_string,  read_str(arg1))

DEF_FN_1_BEGIN(b_slurp)
	if(rtypeof(arg1) != STR_T)
	{
		return RERR(ERR_TYPE);
	}
	char* fn  = rstr_to_str(arg1);
	value_t r = slurp(fn);
	free(fn);

	return r;
DEF_FN_END

static value_t b_init(value_t body, value_t env)
{
	// initialize root environment
	value_t last_env = last(env);
	rplaca(last_env, car(create_root_env()));
	return eval(read_str(str_to_rstr("(eval (read-string (str \"(do \" (slurp \"init.rud\") \")\")))")), last_env);
}


////////// MATH functions

DEF_FN_2INT(b_lt,  arg1 <  arg2 ? SYM_TRUE : SYM_FALSE)
DEF_FN_2INT(b_elt, arg1 <= arg2 ? SYM_TRUE : SYM_FALSE)
DEF_FN_2INT(b_mt,  arg1 >  arg2 ? SYM_TRUE : SYM_FALSE)
DEF_FN_2INT(b_emt, arg1 >= arg2 ? SYM_TRUE : SYM_FALSE)

DEF_FN_R(b_add, RINT(0),       arg1,                 RINT(arg1.rint.val + arg2.rint.val))
DEF_FN_R(b_sub, RINT(0),       RINT(-arg1.rint.val), RINT(arg1.rint.val - arg2.rint.val))
DEF_FN_R(b_mul, RINT(1),       arg1,                 RINT(arg1.rint.val * arg2.rint.val))
DEF_FN_R(b_div, RERR(ERR_ARG), arg1,                 RINT(arg1.rint.val / arg2.rint.val))

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

////////// other functions
static value_t b_list(value_t body, value_t env)
{
	if(nilp(body))
	{
		return NIL;
	}
	else
	{
		return cons(car(body), b_list(cdr(body), env));
	}
}

static value_t count1(value_t body)
{
	if(nilp(body))
	{
		return RINT(0);
	}
	else if(rtypeof(body) != CONS_T)
	{
		return RERR(ERR_TYPE);
	}
	else
	{
		value_t val = count1(cdr(body));
		if(errp(val))
		{
			return val;
		}
		else
		{
			return RINT(val.rint.val + 1);
		}
	}
}

static value_t count(value_t body, value_t env)
{
	value_t arg = car(body);
	if(rtypeof(arg) != CONS_T)
	{
		return RERR(ERR_TYPE);
	}
	arg.type.main = CONS_T;

	return count1(arg);
}


static value_t b_concat(value_t body, value_t env)
{
	value_t r = EMPTY_LIST;
	while(!nilp(body))
	{
		r = concat(2, r, car(body));
		body = cdr(body);
	}

	return r;
}

static value_t b_nth(value_t body, value_t env)
{
	value_t arg1 = car(body);
	value_t arg2 = car(cdr(body));
	if(rtypeof(arg2) != INT_T)
	{
		return RERR(ERR_TYPE);
	}
	else
	{
		return nth(arg2.rint.val, arg1);
	}
}


/////////////////////////////////////////////////////////////////////
// public: Create root environment

value_t	create_root_env	(void)
{
	value_t key = list(29,
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
	                      str_to_sym("list"),
	                      str_to_sym("consp"),
	                      str_to_sym("count"),
	                      str_to_sym("read-string"),
	                      str_to_sym("slurp"),
	                      str_to_sym("eval"),
	                      str_to_sym("cons"),
	                      str_to_sym("concat"),
	                      str_to_sym("nth"),
	                      str_to_sym("car"),
	                      str_to_sym("cdr"),
	                      str_to_sym("="),
	                      str_to_sym("eq"),
	                      str_to_sym("atom"),
	                      str_to_sym("init"),
	                      str_to_sym("<"),
	                      str_to_sym("<="),
	                      str_to_sym(">"),
	                      str_to_sym(">=")
	                  );

	value_t val = list(29,
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
	                      cfn(RFN(b_list), NIL),
	                      cfn(RFN(b_consp), NIL),
	                      cfn(RFN(count), NIL),
	                      cfn(RFN(b_read_string), NIL),
	                      cfn(RFN(b_slurp), NIL),
	                      cfn(RFN(b_eval), NIL),
	                      cfn(RFN(b_cons), NIL),
	                      cfn(RFN(b_concat), NIL),
	                      cfn(RFN(b_nth), NIL),
	                      cfn(RFN(b_car), NIL),
	                      cfn(RFN(b_cdr), NIL),
	                      cfn(RFN(b_equal), NIL),
	                      cfn(RFN(b_eq), NIL),
	                      cfn(RFN(b_atom), NIL),
	                      cfn(RFN(b_init), NIL),
	                      cfn(RFN(b_lt), NIL),
	                      cfn(RFN(b_elt), NIL),
	                      cfn(RFN(b_mt), NIL),
	                      cfn(RFN(b_emt), NIL)
			  );

	return create_env(key, val, NIL);
}


// End of File
/////////////////////////////////////////////////////////////////////
