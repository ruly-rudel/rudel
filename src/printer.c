#include <assert.h>
#include "builtin.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////
// private: printer support functions

static value_t pr_str_int(int x)
{
	value_t r = NIL;

	if(x < 0)
	{
		r = nconc(cons(RCHAR('-'), NIL), pr_str_int(-x));
	}
	else if(x == 0)
	{
		r = str_to_rstr("0");
	}
	else
	{
		while(x != 0)
		{
			r = cons(RCHAR('0' + (x % 10)), r);
			x /= 10;
		}
	}

	return r;
}


static value_t pr_str_cons(value_t x, value_t annotate, bool print_readably)
{
	assert(rtypeof(x) == CONS_T);
	value_t r    = NIL;
	value_t* cur = &r;

	if(nilp(x))
	{
		r = str_to_rstr("nil");
	}
	else
	{
		do
		{
			cur = cons_and_cdr(RCHAR(nilp(r) ? '(' : ' '), cur);

			if(rtypeof(x) == CONS_T)
			{
				// append string
				cur = nconc_and_last(pr_str(car(x), annotate, print_readably), cur);
				x = cdr(x);
			}
			else	// dotted
			{
				cur = cons_and_cdr(RCHAR('.'), cur);
				cur = cons_and_cdr(RCHAR(' '), cur);
				cur = nconc_and_last(pr_str(x, annotate, print_readably), cur);
				x = NIL;
			}
		} while(!nilp(x));

		cur = cons_and_cdr(RCHAR(')'), cur);
	}

	return r;
}

#if 0
static value_t pr_str_cons(value_t x, value_t cyclic, bool print_readably)
{
	assert(rtypeof(x) == CONS_T);
	value_t r    = NIL;
	value_t* cur = &r;

	value_t  cyc = NIL;	// current list's cyclic tests
	bool     self_c = false;

	if(nilp(x))
	{
		r = str_to_rstr("nil");
	}
	else
	{
		do
		{
			if(nilp(r))
			{
				cur = cons_and_cdr(RCHAR('('), cur);
			}
			else
			{
				cur = cons_and_cdr(RCHAR(' '), cur);
			}

			if(rtypeof(x) == CONS_T)
			{
				// check cyclic
				if(!nilp(cyclic))
				{
					value_t ex = assoceq(x, cdr(cyclic));
					if(nilp(ex))
					{
						//nconc(cyclic, cons(cons(x, NIL), NIL));
						rplacd(cyclic, cons(cons(x, NIL), cdr(cyclic)));

						// check if current cons is used cyclic later
						value_t nc  = pr_str(car(x), cyclic, print_readably);
						value_t nex = assoceq(x, cdr(cyclic));
						if(!nilp(cdr(nex)))
						{
							cur = cons_and_cdr(RCHAR('#'), cur);
							cur = nconc_and_last(pr_str_int(cdr(nex).rint.val), cur);
							cur = cons_and_cdr(RCHAR('='), cur);
						}

						// append string
						cur = nconc_and_last(nc, cur);
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

						if(!nilp(find(x, cyc, eq)))	// self-cyclic
						{
							cur = cons_and_cdr(RCHAR('.'), cur);
							cur = cons_and_cdr(RCHAR(' '), cur);
						}

						cur = cons_and_cdr(RCHAR('#'), cur);
						cur = nconc_and_last(pr_str_int(n.rint.val), cur);
						cur = cons_and_cdr(RCHAR('#'), cur);
					}
				}
				else	// does not chech cyclic
				{
					// append string
					cur = nconc_and_last(pr_str(car(x), cyclic, print_readably), cur);
				}

				// self-cyclic list check
				if(nilp(find(x, cyc, eq)))
				{
					cyc = cons(x, cyc);
					x = cdr(x);
				}
				else
				{
					x = NIL;
					self_c = true;
				}
			}
			else	// dotted
			{
				cur = cons_and_cdr(RCHAR('.'), cur);
				cur = cons_and_cdr(RCHAR(' '), cur);
				cur = nconc_and_last(pr_str(x, cyclic, print_readably), cur);
				x = NIL;
			}
		} while(!nilp(x));

		cur = cons_and_cdr(RCHAR(')'), cur);
	}

	if(self_c && !nilp(cyclic))
	{
		value_t new_r = NIL;
		cur = cons_and_cdr(RCHAR('#'), &new_r);
		cur = cons_and_cdr(RCHAR('='), cur);
		cur = nconc_and_last(r, cur);
		r = new_r;
	}
	r.type.main = STR_T;

	return r;
}
#endif

static value_t pr_str_str(value_t s, bool print_readably)
{
	assert(rtypeof(s) == CONS_T);

	value_t r = cons(RCHAR('\0'), NIL);
	value_t *cur = &r;
	if(print_readably)
	{
		cur = cons_and_cdr(RCHAR('"'), cur);
	}

	while(!nilp(s))
	{
		value_t tcar = car(s);
		assert(rtypeof(tcar) == CHAR_T);

		if(print_readably)	// escape
		{
			if(tcar.rint.val == '\0')	// null string
			{
				break;
			}
			else if(tcar.rint.val == '"')
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
			if(tcar.rint.val == '\0')	// null string
			{
				break;
			}
			else
			{
				cur = cons_and_cdr(tcar, cur);
			}
		}

		s = cdr(s);
		assert(rtypeof(s) == CONS_T);
	}

	if(print_readably)
	{
		cur = cons_and_cdr(RCHAR('"'), cur);
	}

	return r;
}

static value_t pr_str_cfn(value_t s, value_t annotate)
{
	assert(rtypeof(s) == CFN_T);

#ifdef PRINT_CLOS_ENV
	s.type.main = CONS_T;
	value_t r = str_to_rstr("(#<COMPILED-FUNCTION> . ");
	nconc(r, pr_str(cdr(s), annotate));
	nconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<COMPILED-FUNCTION>");
#endif // PRINT_CLOS_ENV
}

static value_t pr_str_cloj(value_t s, value_t annotate)
{
	assert(rtypeof(s) == CLOJ_T);

#ifdef PRINT_CLOS_ENV
	s.type.main = CONS_T;
	value_t r = str_to_rstr("(#<CLOJURE> . ");
	nconc(r, pr_str(cdr(s), annotate));
	nconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<CLOJURE>");
#endif // PRINT_CLOS_ENV
}

static value_t pr_str_macro(value_t s, value_t annotate)
{
	assert(rtypeof(s) == MACRO_T);

#ifdef PRINT_CLOS_ENV
	s.type.main = CONS_T;
	value_t r = str_to_rstr("(#<MACRO> . ");
	nconc(r, pr_str(cdr(s), annotate));
	nconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<MACRO>");
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

	    case ERR_FILENOTFOUND:
		return str_to_rstr("file not found.");

	    case ERR_RANGE:
		return str_to_rstr("range exceeded.");

	    default:
		return str_to_rstr("unknown error.");
	}
}

static value_t pr_sym(value_t s)
{
	value_t r = copy_list(s);
	r.type.main = CONS_T;
	return r;
}

static value_t pr_str_char(value_t s)
{
	assert(rtypeof(s) == CHAR_T);
	value_t r    = NIL;
	value_t* cur = &r;

	cur = cons_and_cdr(RCHAR('\''), cur);
	cur = cons_and_cdr(s, cur);
	cur = cons_and_cdr(RCHAR('\''), cur);
	return r;
}
/////////////////////////////////////////////////////////////////////
// public: Rudel-specific functions

void printline(value_t s, FILE* fp)
{
	assert(rtypeof(s) == CONS_T);
	s.type.main = CONS_T;

	while(!nilp(s) && car(s).rint.val != '\0')
	{
		fputc(car(s).rint.val, fp);
		s = cdr(s);
	}

	fputc('\n', fp);
	fflush(fp);

	return ;
}

value_t pr_str(value_t s, value_t annotate, bool print_readably)
{
	switch(rtypeof(s))
	{
	    case CONS_T:
		return is_str(s) ? pr_str_str(s, print_readably) : pr_str_cons(s, annotate, print_readably);

	    case SYM_T:
		return pr_sym(s);

	    case INT_T:
		return pr_str_int(s.rint.val);

	    case CFN_T:
		return pr_str_cfn(s, annotate);

	    case CLOJ_T:
		return pr_str_cloj(s, annotate);

	    case MACRO_T:
		return pr_str_macro(s, annotate);

	    case CHAR_T:
		return pr_str_char(s);

	    case ERR_T:
		return pr_err(s);

	    default:
		return s;
	}
}

void print(value_t s, FILE* fp)
{
	printline(pr_str(s, cons(RINT(0), NIL), true), fp);
	return;
}


// End of File
/////////////////////////////////////////////////////////////////////
