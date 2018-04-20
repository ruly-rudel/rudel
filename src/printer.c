#include <assert.h>
#include <wchar.h>
#include "builtin.h"
#include "printer.h"
#include "reader.h"
#include "allocator.h"

/////////////////////////////////////////////////////////////////////
// private: printer support functions

static value_t pr_str_int(int x)
{
	value_t r = make_vector(0);
	push_root(&r);

	bool is_minus = false;
	if(x < 0)
	{
		x = -x;
		is_minus = true;
	}

	if(x == 0)
	{
		vpush(RCHAR('0'), r);
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

	pop_root(1);
	return r;
}

static value_t pr_str_cons(value_t x, value_t annotate, bool print_readably)
{
	assert(rtypeof(x) == CONS_T);
	push_root(&x);
	push_root(&annotate);
	value_t r    = make_vector(0);
	push_root(&r);

	if(nilp(x))
	{
		pop_root(3);
		return str_to_rstr("nil");
	}
	else
	{
		do
		{
			vpush(RCHAR(vsize(r) == 0 ? '(' : ' '), r);

			if(rtypeof(x) == CONS_T)
			{
				// append string
				vnconc(r, pr_str(car(x), annotate, print_readably));
				x = cdr(x);
			}
			else	// dotted
			{
				vpush(RCHAR('.'), r);
				vpush(RCHAR(' '), r);
				vnconc(r, pr_str(x, annotate, print_readably));
				x = NIL;
			}
		} while(!nilp(x));

		vpush(RCHAR(')'), r);

		pop_root(3);
		return r;
	}
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

	push_root(&s);
	value_t r = make_vector(0);
	push_root(&r);

	if(print_readably)
	{
		vpush(RCHAR('"'), r);
	}

	for(int i = 0; i < vsize(s); i++)
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
				vpush(RCHAR('\\'), r);
				vpush(c, r);
			}
			else if(INTOF(c) == '\n')
			{
				vpush(RCHAR('\\'), r);
				vpush(RCHAR('n'), r);
			}
			else if(INTOF(c) == '\\')
			{
				vpush(RCHAR('\\'), r);
				vpush(c, r);
			}
			else
			{
				vpush(c, r);
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
				vpush(c, r);
			}
		}
	}

	if(print_readably)
	{
		vpush(RCHAR('"'), r);
	}

	if(vsize(r) == 0)
	{
		vpush(RCHAR('\0'), r);
	}

	pop_root(2);

	return r;
}

static value_t pr_str_cloj(value_t s, value_t annotate)
{
	assert(clojurep(s));

	return str_to_rstr("#<CLOJURE>");
}

static value_t pr_str_macro(value_t s, value_t annotate)
{
	assert(macrop(s));

	return str_to_rstr("#<MACRO>");
}

static value_t pr_str_ptr(value_t s, value_t annotate)
{
	assert(ptrp(s));

	return str_to_rstr("#<PTR>");
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
			return str_to_rstr("internal error: that is not implemented yet.");

		    case ERR_ALLOC:
			return str_to_rstr("memory allocation fails.");

		    case ERR_EOS:
			return str_to_rstr("end of sequence.");

		    case ERR_INVALID_IS:
			return str_to_rstr("unknown vm instruction.");

		    case ERR_ARG_BUILTIN:
			return str_to_rstr("built-in functions is not first class now. we will fix it later.");

		    case ERR_INVALID_AP:
			return str_to_rstr("try to AP other than clojure.");

		    case ERR_CANTOPEN:
			return str_to_rstr("cannot open file.");

		    case ERR_FWRITE:
			return str_to_rstr("cannot write data to file.");

		    case ERR_FREAD:
			return str_to_rstr("cannot read data from file.");

		    case ERR_EXCEPTION:
			return str_to_rstr("throw exception to the empty exception stack.");

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
	push_root(&pos);
	value_t r    = make_vector(0);
	push_root(&r);

	for(; !nilp(pos); pos = cdr(pos))
	{
		vnconc(r, str_to_rstr("\nat "));
		vnconc(r, pr_str(car(pos), NIL, true));
	}

	pop_root(2);
	return r;
}

static value_t pr_err(value_t s)
{
	assert(errp(s));

	push_root(&s);
	value_t r = pr_err_cause(s);
	push_root(&r);
	r = vnconc(r, pr_err_pos(s));
	pop_root(2);
	return r;
}

static value_t pr_sym(value_t s)
{
	assert(symbolp(s));

	s.type.main = VEC_T;
	return copy_vector(s);
}

static value_t pr_str_char(value_t s)
{
	assert(charp(s));
	value_t r    = make_vector(3);

	vpush(RCHAR('\''), r);
	vpush(s, r);
	vpush(RCHAR('\''), r);
	return r;
}

static value_t pr_ref(value_t s)
{
	assert(refp(s));

	value_t r    = str_to_rstr("#REF:");
	push_root(&r);

	vnconc(r, pr_str_int(REF_D(s)));
	vpush (RCHAR(','), r);
	vnconc(r, pr_str_int(REF_W(s)));
	vpush (RCHAR('#'), r);

	pop_root(1);
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
		default:		return RERR(ERR_NOTIMPL, str_to_rstr("special"));
	}
}

static value_t pr_vec(value_t s)
{
	assert(vectorp(s));
	return str_to_rstr("#<VECTOR>");
}

static value_t pr_vmis(value_t s)
{
	switch(s.op.mnem)	//***** printing operand is not support yet. fix it.
	{
		case IS_HALT:		return str_to_rstr("IS_HALT");
		case IS_BR:		return str_to_rstr("IS_BR");
		case IS_BNIL:		return str_to_rstr("IS_BNIL");
		case IS_MKVEC_ENV:	return str_to_rstr("IS_MKVEC_ENV");
		case IS_VPUSH_ENV:	return str_to_rstr("IS_VPUSH_ENV");
		case IS_VPOP_ENV:	return str_to_rstr("IS_VPOP_ENV");
		case IS_NIL_CONS_VPUSH:	return str_to_rstr("IS_NIL_CONS_VPUSH");
		case IS_CONS_VPUSH:	return str_to_rstr("IS_CONS_VPUSH");
		case IS_CALLCC:		return str_to_rstr("IS_CALLCC");
		case IS_AP:		return str_to_rstr("IS_AP");
		case IS_GOTO:		return str_to_rstr("IS_GOTO");
		case IS_RET:		return str_to_rstr("IS_RET");
		case IS_DUP:		return str_to_rstr("IS_DUP");
		case IS_PUSH:		return str_to_rstr("IS_PUSH");
		case IS_PUSHR:		return str_to_rstr("IS_PUSHR");
		case IS_POP:		return str_to_rstr("IS_POP");
		case IS_SETENV:		return str_to_rstr("IS_SETENV");
		case IS_NCONC:		return str_to_rstr("IS_NCONC");
		case IS_SETSYM:		return str_to_rstr("IS_SETSYM");
		case IS_RESTPARAM:	return str_to_rstr("IS_RESTPARAM");
		case IS_ATOM:		return str_to_rstr("IS_ATOM");
		case IS_CONSP:		return str_to_rstr("IS_CONSP");
		case IS_CLOJUREP:	return str_to_rstr("IS_CLOJUREP");
		case IS_MACROP:		return str_to_rstr("IS_MACROP");
		case IS_CONS:		return str_to_rstr("IS_CONS");
		case IS_CAR:		return str_to_rstr("IS_CAR");
		case IS_CDR:		return str_to_rstr("IS_CDR");
		case IS_EQ:		return str_to_rstr("IS_EQ");
		case IS_EQUAL:		return str_to_rstr("IS_EQUAL");
		case IS_RPLACA:		return str_to_rstr("IS_RPLACA");
		case IS_RPLACD:		return str_to_rstr("IS_RPLACD");
		case IS_GENSYM:		return str_to_rstr("IS_GENSYM");
		case IS_LT:		return str_to_rstr("IS_LT");
		case IS_ELT:		return str_to_rstr("IS_ELT");
		case IS_MT:		return str_to_rstr("IS_MT");
		case IS_EMT:		return str_to_rstr("IS_EMT");
		case IS_ADD:		return str_to_rstr("IS_ADD");
		case IS_SUB:		return str_to_rstr("IS_SUB");
		case IS_MUL:		return str_to_rstr("IS_MUL");
		case IS_DIV:		return str_to_rstr("IS_DIV");
		case IS_READ_STRING:	return str_to_rstr("IS_READ_STRING");
		case IS_SLURP:		return str_to_rstr("IS_SLURP");
		case IS_EVAL:		return str_to_rstr("IS_EVAL");
		case IS_ERR:		return str_to_rstr("IS_ERR");
		case IS_NTH:		return str_to_rstr("IS_NTH");
		case IS_INIT:		return str_to_rstr("IS_INIT");
		case IS_MAKE_VECTOR:	return str_to_rstr("IS_MAKE_VECTOR");
		case IS_VREF:		return str_to_rstr("IS_VREF");
		case IS_RPLACV:		return str_to_rstr("IS_RPLACV");
		case IS_VSIZE:		return str_to_rstr("IS_VSIZE");
		case IS_VEQ:		return str_to_rstr("IS_VEQ");
		case IS_VPUSH:		return str_to_rstr("IS_VPUSH");
		case IS_VPOP:		return str_to_rstr("IS_VPOP");
		case IS_COPY_VECTOR:	return str_to_rstr("IS_COPY_VECTOR");
		case IS_VCONC:		return str_to_rstr("IS_VCONC");
		case IS_VNCONC:		return str_to_rstr("IS_VNCONC");
		case IS_COMPILE_VM:	return str_to_rstr("IS_COMPILE_VM");
		case IS_EXEC_VM:	return str_to_rstr("IS_EXEC_VM");
		case IS_PR_STR:		return str_to_rstr("IS_PR_STR");
		case IS_PRINTLINE:	return str_to_rstr("IS_PRINTLINE");
		case IS_PRINT:		return str_to_rstr("IS_PRINT");
		case IS_READ:		return str_to_rstr("IS_READ");
		case IS_MVFL:		return str_to_rstr("IS_MVFL");
		case IS_MLFV:		return str_to_rstr("IS_MLFV");
		default:		return RERR(ERR_NOTIMPL, str_to_rstr("VMIS"));
	}
}
/////////////////////////////////////////////////////////////////////
// public: Rudel-specific functions

void printline(value_t s, FILE* fp)
{
	assert(vectorp(s) || nilp(s));
	if(!nilp(s))
	{
		char buf[MB_CUR_MAX + 1];
		for(int i = 0; i < vsize(s); i++)
		{
			value_t c = vref(s, i);
			assert(charp(c));
			if(INTOF(c) == '\0') break;
#ifdef USE_LINENOISE
			int len = wctomb(buf, INTOF(c));
			if(len > 0)
			{
				fwrite(buf, 1, len, fp);
			}
			else
			{
				abort();
			}
#else // USE_LINENOISE
			fputwc(INTOF(c), fp);
#endif // USE_LINENOISE
		}
	}

#ifdef USE_LINENOISE
	fputc('\n', fp);
#else // USE_LINENOISE
	fputwc('\n', fp);
#endif // USE_LINENOISE
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

	    case CLOJ_T:
		return pr_str_cloj(s, annotate);

	    case MACRO_T:
		return pr_str_macro(s, annotate);

	    case PTR_T:
		return pr_str_ptr(s, annotate);

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
	push_root(&s);
	GCCONS(c, RINT(0), NIL);

	value_t r = pr_str(s, c, true);
	if(errp(r))
	{
		push_root(&r);
		printline(pr_str(r, NIL, true), stderr);
		pop_root(1);
	}
	else
	{
		printline(r, fp);
	}

	pop_root(2);
	return;
}


// End of File
/////////////////////////////////////////////////////////////////////
