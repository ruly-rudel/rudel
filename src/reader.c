#include <assert.h>
#include "builtin.h"
#include "reader.h"
#include "scanner.h"
#include "../linenoise/linenoise.h"


/////////////////////////////////////////////////////////////////////
// private: reader support functions(parser)

static value_t read_int(value_t token)
{
	assert(rtypeof(token) == STR_T || rtypeof(token) == SYM_T);
	token.type.main = CONS_T;

	// sign (if exists)
	value_t tcar = car(token);
	assert(rtypeof(tcar) == INT_T);
	int sign = 1;
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
	unsigned int val = 0;
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
					r = NIL;		// () is nil
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
			else if(tcar.rint.val == ',')
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

#ifndef USE_LINENOISE
static value_t read_line(FILE* fp)
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
#endif // USE_LINENOISE

/////////////////////////////////////////////////////////////////////
// public: Rudel-specific functions

void init_linenoise(void)
{
	linenoiseSetMultiLine(1);
	linenoiseHistoryLoad(RUDEL_INPUT_HISTORY);
	return ;
}

value_t read_str(value_t s)
{
	scan_t st = scan_init(s);

	return read_form(&st);
}

value_t READ(const char* prompt, FILE* fp)
{
#ifdef USE_LINENOISE
	char* line = linenoise(prompt);
	if(line)
	{
		linenoiseHistoryAdd(line);
		linenoiseHistorySave(RUDEL_INPUT_HISTORY);

		value_t r = str_to_rstr(line);
		linenoiseFree(line);
		return read_str(r);
	}
	else
	{
		return RERR(ERR_EOF);
	}
#else	// USE_LINENOISE
	fprintf(stdout, prompt);
	value_t str = read_line(fp);
	if(errp(str))
	{
		return str;
	}
	else
	{
		return read_str(str);
	}
#endif  // USE LINENOISE
}


// End of File
/////////////////////////////////////////////////////////////////////
