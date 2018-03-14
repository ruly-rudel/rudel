#include <assert.h>
#include "builtin.h"
#include "reader.h"
#include "scanner.h"


/////////////////////////////////////////////////////////////////////
// private: reader support functions(parser)

static value_t read_int(value_t token)
{
	assert(rtypeof(token) == STR_T || rtypeof(token) == SYM_T);
	token.type.main = CONS_T;

	// sign (if exists)
	value_t tcar = car(token);
	assert(rtypeof(tcar) == INT_T);
	int64_t sign = 1;
	if(tcar.rint.val == '-' || tcar.rint.val == '+')
	{
		if(tcar.rint.val == '-')
		{
			sign = -1;
		}

		// next character
		token = cdr(token);
		assert(rtypeof(token) == CONS_T);

		if(nilp(token))	// not int
		{
			return RERR(ERR_PARSE);
		}

		tcar  = car(token);
		assert(rtypeof(tcar) == INT_T);
	}

	// value
	uint64_t val = 0;
	while(!nilp(token))
	{
		tcar = car(token);
		assert(rtypeof(tcar) == INT_T);

		int cv = tcar.rint.val - '0';
		if(cv >= 0 && cv <= 9)
		{
			val *= 10;
			val += cv;
			token = cdr(token);
			assert(rtypeof(token) == CONS_T);
		}
		else
		{
			return RERR(ERR_PARSE);
		}
	}

	return RINT(sign * val);
}

static value_t read_atom(scan_t* st)
{
	value_t token = scan_peek(st);

	if(rtypeof(token) == SYM_T)
	{
		value_t rint = read_int(token);
		if(!errp(rint))
		{
			token = rint;
		}
	}

	scan_next(st);
	return token;
}

static value_t read_form(scan_t* st);

static value_t read_list(scan_t* st)
{
	assert(st != NULL);

	value_t r = NIL;
	value_t *cur = &r;

	value_t token;

	scan_next(st);	// omit '('
	while(!nilp(token = scan_peek(st)))
	{
		assert(rtypeof(token) == STR_T || rtypeof(token) == SYM_T || errp(token));

		if(errp(token))
		{
			return token;	// scan error
		}
		else if(rtypeof(token) == SYM_T)
		{
			token.type.main = CONS_T;

			value_t c = car(token);	// first char of token

			assert(rtypeof(c) == INT_T);
			if(c.rint.val == ')')
			{
				if(nilp(r))
				{
#ifdef MAL
					r = cons(NIL, NIL);	// () is (nil . nil)
#else  // MAL
					r = NIL;		// () is nil
#endif // MAL
				}
				scan_next(st);
				return r;
			}
			else
			{
				cur = cons_and_cdr(read_form(st), cur);
			}
		}
		else
		{
			cur = cons_and_cdr(read_form(st), cur);
		}
	}

	return RERR(ERR_PARSE);	// error: end of token before ')'
}

#ifdef MAL
static value_t read_vector(scan_t* st)
{
	assert(st != NULL);

	value_t r = NIL;
	value_t *cur = &r;

	value_t token;

	scan_next(st);	// omit '('
	while(!nilp(token = scan_peek(st)))
	{
		assert(rtypeof(token) == STR_T || rtypeof(token) == SYM_T || errp(token));

		if(errp(token))
		{
			return token;	// scan error
		}
		else if(rtypeof(token) == SYM_T)
		{
			token.type.main = CONS_T;

			value_t c = car(token);	// first char of token

			assert(rtypeof(c) == INT_T);
			if(c.rint.val == ']')
			{
				if(nilp(r))
				{
#ifdef MAL
					r = cons(NIL, NIL);	// () is (nil . nil)
#else  // MAL
					r = NIL;		// () is nil
#endif // MAL
				}
				scan_next(st);
				r.type.main = VEC_T;
				return r;
			}
			else
			{
				cur = cons_and_cdr(read_form(st), cur);
			}
		}
		else
		{
			cur = cons_and_cdr(read_form(st), cur);
		}
	}

	return RERR(ERR_PARSE);	// error: end of token before ')'
}
#endif // MAL

static value_t read_form(scan_t* st)
{
	assert(st != NULL);

	value_t token = scan_peek(st);
	if(nilp(token))	// no token
	{
		return NIL;
	}
	else if(errp(token))	// scan error
	{
		return token;
	}
	else
	{
		assert(rtypeof(token) == STR_T || rtypeof(token) == SYM_T);
		if(rtypeof(token) == SYM_T)
		{
			value_t tcons   = token;
			tcons.type.main = CONS_T;
			assert(!nilp(tcons));

			value_t tcar = car(tcons);
			assert(rtypeof(tcar) == INT_T);

			if(tcar.rint.val == '(')
			{
				return read_list(st);
			}
#ifdef MAL
			else if(tcar.rint.val == '[')
			{
				return read_vector(st);
			}
#endif
			else if(tcar.rint.val == '\'')
			{
				scan_next(st);
				return cons(str_to_sym("quote"), cons(read_form(st), NIL));
			}
			else if(tcar.rint.val == '`')
			{
				scan_next(st);
				return cons(str_to_sym("quasiquote"), cons(read_form(st), NIL));
			}
			else if(tcar.rint.val == '~')
			{
				token = scan_next(st);
				if(errp(token))	// scan error
				{
					return token;
				}

				assert(rtypeof(token) == STR_T || rtypeof(token) == SYM_T);
				if(rtypeof(token) == SYM_T)
				{
					value_t tcons   = token;
					tcons.type.main = CONS_T;
					assert(!nilp(tcons));

					value_t tcar = car(tcons);
					assert(rtypeof(tcar) == INT_T);
					if(tcar.rint.val == '@')
					{
						scan_next(st);
						return cons(str_to_sym("splice-unquote"), cons(read_form(st), NIL));
					}
					else
					{
						return cons(str_to_sym("unquote"), cons(read_form(st), NIL));
					}
				}
				else
				{
					return cons(str_to_sym("unquote"), cons(read_form(st), NIL));
				}

			}
			else
			{
				return read_atom(st);
			}
		}
		else
		{
			return read_atom(st);
		}
	}
}



/////////////////////////////////////////////////////////////////////
// public: Rudel-specific functions

value_t readline(FILE* fp)
{
	value_t	 r	= NIL;
	value_t* cur	= &r;

	for(;;)
	{
		int c = fgetc(fp);
		if(c == EOF && nilp(r))	// EOF
		{
			return RERR(ERR_EOF);
		}
		else if(c == '\n' || c == EOF)
		{
			r.type.main = STR_T;
			return r;
		}
		else
		{
			cur = cons_and_cdr(RCHAR(c), cur);
		}
	}
}

value_t read_str(value_t s)
{
	scan_t st = scan_init(s);

	return read_form(&st);
}

// End of File
/////////////////////////////////////////////////////////////////////
