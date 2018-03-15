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

static value_t add(value_t body, value_t env)
{
	assert(rtypeof(body) == CONS_T);

	value_t sum = RINT(0);
	if(!nilp(body))
	{
		for(value_t cur = body; !nilp(cur); cur = cdr(cur))
		{
			value_t cura = car(cur);
			sum = RINT(sum.rint.val + cura.rint.val);
		}

	}
	return sum;
}

static value_t sub(value_t body, value_t env)
{
	assert(rtypeof(body) == CONS_T);
	if(nilp(body))
	{
		return RERR(ERR_ARG);
	}
	else
	{
		value_t sum = car(body);
		assert(rtypeof(sum) == INT_T);
		body = cdr(body);
		if(!nilp(body))
		{
			for(value_t cur = body; !nilp(cur); cur = cdr(cur))
			{
				value_t cura = car(cur);
				sum = RINT(sum.rint.val - cura.rint.val);
			}
		}

		return sum;
	}
}

static value_t mul(value_t body, value_t env)
{
	assert(rtypeof(body) == CONS_T);

	value_t sum = RINT(1);
	if(!nilp(body))
	{
		for(value_t cur = body; !nilp(cur); cur = cdr(cur))
		{
			value_t cura = car(cur);
			sum = RINT(sum.rint.val * cura.rint.val);
		}

	}
	return sum;
}

static value_t divide(value_t body, value_t env)
{
	assert(rtypeof(body) == CONS_T);
	if(nilp(body))
	{
		return RERR(ERR_ARG);
	}
	else
	{
		value_t sum = car(body);
		assert(rtypeof(sum) == INT_T);
		body = cdr(body);
		if(!nilp(body))
		{
			for(value_t cur = body; !nilp(cur); cur = cdr(cur))
			{
				value_t cura = car(cur);
				sum = RINT(sum.rint.val / cura.rint.val);
			}
		}

		return sum;
	}
}

static value_t b_pr_str2(value_t body, value_t cyclic, bool print_readably, bool ws_sep)
{
	if(nilp(body))
	{
		return NIL;
	}
	else
	{
		if(ws_sep)  // separate with white space
		{
			return nconc(
				str_to_rstr(" "),
				nconc(
					pr_str(car(body), cyclic, print_readably),
					b_pr_str2(cdr(body), cyclic, print_readably, ws_sep)
				)
			);
		}
		else
		{
			return nconc(
				pr_str(car(body), cyclic, print_readably),
				b_pr_str2(cdr(body), cyclic, print_readably, ws_sep)
			);
		}
	}
}

static value_t b_pr_str1(value_t body, value_t cyclic, bool print_readably, bool ws_sep)
{
	if(nilp(body))
	{
		//return str_to_rstr(print_readably ? "\"\"" : "");
		return str_to_rstr("");
	}
	else
	{
		return nconc(
			pr_str(car(body), cyclic, print_readably),
			b_pr_str2(cdr(body), cyclic, print_readably, ws_sep)
		);
	}
}

static value_t b_pr_str(value_t body, value_t env)
{
	return b_pr_str1(body, cons(RINT(0), NIL), true, true);
}

static value_t str(value_t body, value_t env)
{
	return b_pr_str1(body, cons(RINT(0), NIL), false, false);
}

static value_t prn(value_t body, value_t env)
{
	printline(b_pr_str1(body, cons(RINT(0), NIL), true, true), stdout);
	return NIL;
}

static value_t println(value_t body, value_t env)
{
	printline(b_pr_str1(body, cons(RINT(0), NIL), false, true), stdout);
	return NIL;
}

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

static value_t consp(value_t body, value_t env)
{
	value_t arg = car(body);
	return rtypeof(arg) == CONS_T && !nilp(arg) ? SYM_TRUE : SYM_FALSE;
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

static value_t b_equal(value_t body, value_t env)
{
	return equal(car(body), car(cdr(body))) ? SYM_TRUE : SYM_FALSE;
}

static value_t b_eq(value_t body, value_t env)
{
	return eq(car(body), car(cdr(body))) ? SYM_TRUE : SYM_FALSE;
}

static value_t b_atom(value_t body, value_t env)
{
	return atom(car(body)) ? SYM_TRUE : SYM_FALSE;
}

static value_t comp(value_t body, bool (*comp_fn)(int, int))
{
	value_t a = car(body);
	value_t b = car(cdr(body));
	if(rtypeof(a) == INT_T && rtypeof(b) == INT_T)
	{
		return comp_fn(a.rint.val, b.rint.val) ? SYM_TRUE : SYM_FALSE;
	}
	else
	{
		return RERR(ERR_TYPE);
	}
}

static bool comp_lt(int a, int b)
{
	return a < b;
}

static bool comp_elt(int a, int b)
{
	return a <= b;
}

static bool comp_mt(int a, int b)
{
	return a > b;
}

static bool comp_emt(int a, int b)
{
	return a >= b;
}

static value_t lt(value_t body, value_t env)
{
	return comp(body, comp_lt);
}

static value_t elt(value_t body, value_t env)
{
	return comp(body, comp_elt);
}

static value_t mt(value_t body, value_t env)
{
	return comp(body, comp_mt);
}

static value_t emt(value_t body, value_t env)
{
	return comp(body, comp_emt);
}

static value_t read_string(value_t body, value_t env)
{
	return read_str(car(body));
}

static value_t slurp(value_t body, value_t env)
{
	// filename
	value_t fn = car(body);
	if(rtypeof(fn) != STR_T)
	{
		return RERR(ERR_TYPE);
	}

	// to cstr
	char* fn_cstr = rstr_to_str(fn);
	FILE* fp = fopen(fn_cstr, "r");
	free(fn_cstr);

	// read all
	if(fp)
	{
		value_t buf = NIL;
		value_t* cur = &buf;

		int c;
		while((c = fgetc(fp)) != EOF)
		{
			cur = cons_and_cdr(RCHAR(c), cur);
		}
		fclose(fp);
		buf.type.main = STR_T;

		return buf;
	}
	else	// file not found or other error.
	{
		return RERR(ERR_FILENOTFOUND);
	}
}

static value_t b_eval(value_t body, value_t env)
{
	value_t ast = car(body);
	return eval(ast, last(env));	// use root environment
}

static value_t b_cons(value_t body, value_t env)
{
	value_t a = car(body);
	value_t b = car(cdr(body));
	return cons(a, b);
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

static value_t b_car(value_t body, value_t env)
{
	return car(car(body));
}

static value_t b_cdr(value_t body, value_t env)
{
	return cdr(car(body));
}

static value_t b_init(value_t body, value_t env)
{
	// initialize root environment
	value_t last_env = last(env);
	rplaca(last_env, car(create_root_env()));
	return eval(read_str(str_to_rstr("(eval (read-string (str \"(do \" (slurp \"init.rud\") \")\")))")), last_env);
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
	                      cfn(RFN(add), NIL),
	                      cfn(RFN(sub), NIL),
	                      cfn(RFN(mul), NIL),
	                      cfn(RFN(divide), NIL),
	                      cfn(RFN(b_pr_str), NIL),
	                      cfn(RFN(str), NIL),
	                      cfn(RFN(prn), NIL),
	                      cfn(RFN(println), NIL),
	                      cfn(RFN(b_list), NIL),
	                      cfn(RFN(consp), NIL),
	                      cfn(RFN(count), NIL),
	                      cfn(RFN(read_string), NIL),
	                      cfn(RFN(slurp), NIL),
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
	                      cfn(RFN(lt), NIL),
	                      cfn(RFN(elt), NIL),
	                      cfn(RFN(mt), NIL),
	                      cfn(RFN(emt), NIL)
			  );

	return create_env(key, val, NIL);
}


// End of File
/////////////////////////////////////////////////////////////////////
