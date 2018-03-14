#include <assert.h>
#include "builtin.h"
#include "printer.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////
// private: printer support functions

static value_t pr_str_int_rec(uint64_t x, value_t s)
{
	if(x == 0)
	{
		return s;
	}
	else
	{
		value_t r   = cons(RCHAR('0' + (x % 10)), s);

		return pr_str_int_rec(x / 10, r);
	}
}

static value_t pr_str_int(int64_t x)
{
	value_t r;
	if(x < 0)
	{
		r = cons(RCHAR('-'), pr_str_int_rec(-x, NIL));
	}
	else if(x == 0)
	{
		r = str_to_rstr("0");
	}
	else
	{
		r = pr_str_int_rec(x, NIL);
	}

	r.type.main = STR_T;
	return r;
}


static value_t pr_str_cons(value_t x, value_t cyclic)
{
	assert(rtypeof(x) == CONS_T || rtypeof(x) == VEC_T);
	value_t r    = NIL;
	bool is_vec = rtypeof(x) == VEC_T;
	x.type.main = CONS_T;

	if(nilp(x))
	{
		r = str_to_rstr("nil");
	}
#ifdef MAL
	else if(nilp(car(x)) && nilp(cdr(x)))
	{
		r = str_to_rstr(is_vec ? "[]" : "()");
	}
#endif // MAL
	else
	{
		do
		{
			if(nilp(r))
			{
				r = str_to_rstr(is_vec ? "[" : "(");
			}
			else
			{
				nconc(r, str_to_rstr(" "));
			}

			if(rtypeof(x) == CONS_T)
			{
				// check cyclic
				if(!nilp(cyclic))
				{
					value_t ex = assoc(x, cdr(cyclic));
					if(nilp(ex))
					{
						nconc(cyclic, cons(cons(x, NIL), NIL));
						// append string
						nconc(r, pr_str(car(x), cyclic, true));
					}
					else
					{
						value_t n = cdr(ex);
						if(nilp(n))
						{
							n = car(cyclic);
							assert(intp(n));
							rplacd(ex, n);
							rplaca(cyclic, RINT(n.rint.val + 1));
						}

						nconc(r, str_to_rstr("#"));
						nconc(r, pr_str_int(n.rint.val));
						nconc(r, str_to_rstr("#"));
					}
				}
				else	// does not chech cyclic
				{
					// append string
					nconc(r, pr_str(car(x), cyclic, true));
				}

				x = cdr(x);
			}
			else	// dotted
			{
				nconc(r, str_to_rstr(". "));
				nconc(r, pr_str(x, cyclic, true));
				x = NIL;
			}
		} while(!nilp(x));

		nconc(r, str_to_rstr(is_vec ? "]" : ")"));
	}

	r.type.main = STR_T;

	return r;
}

static value_t pr_str_str(value_t s, bool print_readably)
{
	assert(rtypeof(s) == STR_T);
	s.type.main = CONS_T;

	value_t r;
	value_t *cur;
	if(print_readably)
	{
		r = cons(RCHAR('"'), NIL);
		cur = &r.cons->cdr;
	}
	else
	{
		r = NIL;
		cur = &r;
	}

	while(!nilp(s))
	{
		value_t tcar = car(s);
		assert(rtypeof(tcar) == INT_T);

		if(print_readably)	// escape
		{
			if(tcar.rint.val == '"')
			{
				cur = cons_and_cdr(RCHAR('\\'), cur);
				cur = cons_and_cdr(tcar,        cur);
			}
			else if(tcar.rint.val == '\n')
			{
				cur = cons_and_cdr(RCHAR('\\'), cur);
				cur = cons_and_cdr(RCHAR('n'),  cur);
			}
			else if(tcar.rint.val == '\\')
			{
				cur = cons_and_cdr(RCHAR('\\'), cur);
				cur = cons_and_cdr(tcar,        cur);
			}
			else
			{
				cur = cons_and_cdr(tcar,        cur);
			}
		}
		else
		{
			cur = cons_and_cdr(tcar, cur);
		}

		s = cdr(s);
		assert(rtypeof(s) == CONS_T);
	}

	if(print_readably)
	{
		*cur = cons(RCHAR('"'), NIL);
	}

	r.type.main = STR_T;
	return r;
}

static value_t pr_str_cfn(value_t s, value_t cyclic)
{
	assert(rtypeof(s) == CFN_T);

#ifdef PRINT_CLOS_ENV
	s.type.main = CONS_T;
	value_t r = str_to_rstr("(#<C-FUNCTION> . ");
	nconc(r, pr_str(cdr(s), cyclic));
	nconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<C-FUNCTION>");
#endif // PRINT_CLOS_ENV
}

static value_t pr_str_cloj(value_t s, value_t cyclic)
{
	assert(rtypeof(s) == CLOJ_T || rtypeof(s) == MACRO_T);

#ifdef PRINT_CLOS_ENV
	s.type.main = CONS_T;
	value_t r = str_to_rstr("(#<CLOJURE> . ");
	nconc(r, pr_str(cdr(s), cyclic));
	nconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<CLOJURE>");
#endif // PRINT_CLOS_ENV
}

static value_t pr_err(value_t s)
{
	assert(rtypeof(s) == ERR_T);
	switch(s.type.val)
	{
	    case ERR_TYPE:
		return str_to_rstr("type error.");

	    case ERR_PARSE:
		return str_to_rstr("parse error.");

	    case ERR_NOTFOUND:
		return str_to_rstr("symbol or variable is not found.");

	    case ERR_ARG:
		return str_to_rstr("too few argument.");

	    case ERR_NOTFN:
		return str_to_rstr("first element of the list is not a function.");

	    case ERR_NOTSYM:
		return str_to_rstr("symbol.");

	    default:
		return str_to_rstr("unknown error.");
	}
}


/////////////////////////////////////////////////////////////////////
// public: Rudel-specific functions

void printline(value_t s, FILE* fp)
{
	assert(rtypeof(s) == STR_T || rtypeof(s) == SYM_T || rtypeof(s) == CONS_T);
	s.type.main = CONS_T;

	for(; !nilp(s); s = cdr(s))
		fputc(car(s).rint.val, fp);

	fputc('\n', fp);
	fflush(fp);

	return ;
}

value_t pr_str(value_t s, value_t cyclic, bool print_readably)
{
	switch(rtypeof(s))
	{
	    case CONS_T:
	    case VEC_T:
		return pr_str_cons(s, cyclic);

	    case SYM_T:
		return copy_list(s);

	    case INT_T:
		return pr_str_int(s.rint.val);

	    case STR_T:
		return pr_str_str(s, print_readably);

	    case CFN_T:
		return pr_str_cfn(s, cyclic);

	    case CLOJ_T:
	    case MACRO_T:
		return pr_str_cloj(s, cyclic);

	    case ERR_T:
		return pr_err(s);

	    default:
		return s;
	}
}

// End of File
/////////////////////////////////////////////////////////////////////
