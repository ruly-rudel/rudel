#include <assert.h>
#include "builtin.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////
// private: printer support functions

static value_t pr_str_int(int x)
{
	value_t r = make_vector(0);

	bool is_minus = false;
	if(x < 0)
	{
		x = -x;
		is_minus = true;
	}

	if(x == 0)
	{
		vpush(r, RCHAR('0'));
	}
	else
	{
		while(x != 0)
		{
			vpush_front(r, RCHAR('0' + (x % 10)));
			x /= 10;
		}
	}

	if(is_minus)
	{
		vpush_front(r, RCHAR('-'));
	}

	return r;
}

static value_t pr_str_cons(value_t x, value_t annotate, bool print_readably)
{
	assert(rtypeof(x) == CONS_T);
	value_t r    = make_vector(0);

	if(nilp(x))
	{
		r = str_to_rstr("nil");
	}
	else
	{
		do
		{
			vpush(r, RCHAR(INTOF(vsize(r)) == 0 ? '(' : ' '));

			if(rtypeof(x) == CONS_T)
			{
				// append string
				vnconc(r, pr_str(car(x), annotate, print_readably));
				x = cdr(x);
			}
			else	// dotted
			{
				vpush(r, RCHAR('.'));
				vpush(r, RCHAR(' '));
				vnconc(r, pr_str(x, annotate, print_readably));
				x = NIL;
			}
		} while(!nilp(x));

		vpush(r, RCHAR(')'));
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
							cur = nconc_and_last(pr_str_int(cdr(nex).type.val), cur);
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
							rplaca(cyclic, RINT(n.type.val + 1));
						}

						if(!nilp(find(x, cyc, eq)))	// self-cyclic
						{
							cur = cons_and_cdr(RCHAR('.'), cur);
							cur = cons_and_cdr(RCHAR(' '), cur);
						}

						cur = cons_and_cdr(RCHAR('#'), cur);
						cur = nconc_and_last(pr_str_int(n.type.val), cur);
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

	return r;
}
#endif

static value_t pr_str_str(value_t s, bool print_readably)
{
	assert(vectorp(s));

	value_t r = make_vector(0);

	if(print_readably)
	{
		vpush(r, RCHAR('"'));
	}

	for(int i = 0; i < INTOF(vsize(s)); i++)
	{
		value_t c = vref(s, i);
		assert(charp(c));

		if(print_readably)	// escape
		{
			if(INTOF(c) == '\0')	// null string
			{
				break;
			}
			else if(INTOF(c) == '"')
			{
				vpush(r, RCHAR('\\'));
				vpush(r, c);
			}
			else if(INTOF(c) == '\n')
			{
				vpush(r, RCHAR('\\'));
				vpush(r, RCHAR('n'));
			}
			else if(INTOF(c) == '\\')
			{
				vpush(r, RCHAR('\\'));
				vpush(r, c);
			}
			else
			{
				vpush(r, c);
			}
		}
		else
		{
			if(INTOF(c) == '\0')		// null string
			{
				break;
			}
			else
			{
				vpush(r, c);
			}
		}
	}

	if(print_readably)
	{
		vpush(r, RCHAR('"'));
	}

	if(INTOF(vsize(r)) == 0)
	{
		vpush(r, RCHAR('\0'));
	}

	return r;
}

static value_t pr_str_cfn(value_t s, value_t annotate)
{
	assert(cfnp(s));

#ifdef PRINT_CLOS_ENV
	value_t r = str_to_rstr("(#<COMPILED-FUNCTION> . ");
	vnconc(r, pr_str(cdr(s), annotate));
	vnconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<COMPILED-FUNCTION>");
#endif // PRINT_CLOS_ENV
}

static value_t pr_str_cloj(value_t s, value_t annotate)
{
	assert(clojurep(s));

#ifdef PRINT_CLOS_ENV
	value_t r = str_to_rstr("(#<CLOJURE> . ");
	vnconc(r, pr_str(cdr(s), annotate));
	vnconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<CLOJURE>");
#endif // PRINT_CLOS_ENV
}

static value_t pr_str_macro(value_t s, value_t annotate)
{
	assert(macrop(s));

#ifdef PRINT_CLOS_ENV
	value_t r = str_to_rstr("(#<MACRO> . ");
	vnconc(r, pr_str(cdr(s), annotate));
	vnconc(r, str_to_rstr(")"));

	return r;
#else  // PRINT_CLOS_ENV
	return str_to_rstr("#<MACRO>");
#endif // PRINT_CLOS_ENV
}

static value_t pr_err_cause(value_t e)
{
	value_t cause = RERR_CAUSE(e);
	if(vectorp(cause))
	{
		return copy_vector(cause);
	}
	else if(intp(cause))
	{
		switch(INTOF(cause))
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

		    case ERR_NOTIMPL:
			return str_to_rstr("internal error: that special form is not implemented.");

		    case ERR_ALLOC:
			return str_to_rstr("memory allocation fails.");

		    case ERR_EOS:
			return str_to_rstr("end of sequence.");

		    case ERR_INVALID_IS:
			return str_to_rstr("unknown vm instruction.");

		    default:
			return str_to_rstr("unknown error.");
		}
	}
	else
	{
		return str_to_rstr("invalid type of error.");
	}
}
static value_t pr_err_pos(value_t e)
{
	value_t pos  = RERR_POS(e);
	value_t r    = make_vector(0);

	for(; !nilp(pos); pos = cdr(pos))
	{
		vnconc(r, str_to_rstr("\nat "));
		vnconc(r, pr_str(car(pos), NIL, true));
	}

	return r;
}

static value_t pr_err(value_t s)
{
	assert(errp(s));

	value_t r = pr_err_cause(s);
	return vnconc(r, pr_err_pos(s));
}

static value_t pr_sym(value_t s)
{
	s.type.main = VEC_T;
	value_t r = copy_vector(s);
	return r;
}

static value_t pr_str_char(value_t s)
{
	assert(charp(s));
	value_t r    = make_vector(0);

	vpush(r, RCHAR('\''));
	vpush(r, s);
	vpush(r, RCHAR('\''));
	return r;
}

static value_t pr_ref(value_t s)
{
	assert(refp(s));

	value_t r    = str_to_rstr("#REF:");

	vnconc(r, pr_str_int(REF_D(s)));
	vpush (r, RCHAR(','));
	vnconc(r, pr_str_int(REF_W(s)));
	vpush (r, RCHAR('#'));

	return r;
}

static value_t pr_special(value_t s)
{
	assert(specialp(s));
	switch(SPECIAL(s))
	{
		case SP_SETQ:		return symbol_string(g_setq);
		case SP_LET:		return symbol_string(g_let);
		case SP_PROGN:		return symbol_string(g_progn);
		case SP_IF:		return symbol_string(g_if);
		case SP_LAMBDA:		return symbol_string(g_lambda);
		case SP_QUOTE:		return symbol_string(g_quote);
		case SP_QUASIQUOTE:	return symbol_string(g_quasiquote);
		case SP_MACRO:		return symbol_string(g_macro);
		case SP_MACROEXPAND:	return symbol_string(g_macroexpand);
		case SP_UNQUOTE:	return symbol_string(g_unquote);
		case SP_SPLICE_UNQUOTE:	return symbol_string(g_splice_unquote);
		case SP_AMP:		return symbol_string(g_amp);
		default:		return RERR(ERR_NOTIMPL, NIL);
	}
}

static value_t pr_vec(value_t s)
{
	assert(vectorp(s));
	return str_to_rstr("#<VECTOR>");
}

static value_t pr_vmis(value_t s)
{
	switch(INTOF(s))
	{
		case IS_HALT:		return str_to_rstr("IS_HALT");
		case IS_ADD:		return str_to_rstr("IS_ADD");
		case IS_SUB:		return str_to_rstr("IS_SUB");
		case IS_MUL:		return str_to_rstr("IS_MUL");
		case IS_DIV:		return str_to_rstr("IS_DIV");
		case IS_EQ:		return str_to_rstr("IS_EQ");
		case IS_EQUAL:		return str_to_rstr("IS_EQUAL");
		case IS_CONS:		return str_to_rstr("IS_CONS");
		case IS_CAR:		return str_to_rstr("IS_CAR");
		case IS_CDR:		return str_to_rstr("IS_CDR");
		case IS_MKVEC:		return str_to_rstr("IS_MKVEC");
		case IS_VPUSH:		return str_to_rstr("IS_VPUSH");
		case IS_VPOP:		return str_to_rstr("IS_VPOP");
		case IS_VREF:		return str_to_rstr("IS_VREF");
		case IS_VPUSH_ENV:	return str_to_rstr("IS_VPUSH_ENV");
		case IS_VPOP_ENV:	return str_to_rstr("IS_VPOP_ENV");
		default:		return RERR(ERR_NOTIMPL, NIL);
	}
}
/////////////////////////////////////////////////////////////////////
// public: Rudel-specific functions

void printline(value_t s, FILE* fp)
{
	assert(vectorp(s));
	for(int i = 0; i < INTOF(vsize(s)); i++)
	{
		value_t c = vref(s, i);
		assert(charp(c));
		if(INTOF(c) == '\0') break;
		fputc(INTOF(c), fp);
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
		return pr_str_cons(s, annotate, print_readably);

	    case VEC_T:
		return is_str(s) ? pr_str_str(s, print_readably) : pr_vec(s);

	    case SYM_T:
		return pr_sym(s);

	    case SPECIAL_T:
		return pr_special(s);

	    case INT_T:
		return pr_str_int(INTOF(s));

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

	    case REF_T:
		return pr_ref(s);

	    case VMIS_T:
		return pr_vmis(s);

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
