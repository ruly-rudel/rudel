#include <assert.h>
#include "builtin.h"
#include "reader.h"
#include "scanner.h"
#include "allocator.h"
#include "../linenoise/linenoise.h"


/////////////////////////////////////////////////////////////////////
// private: reader support functions(parser)

static value_t read_int(value_t token)
{
	assert(vectorp(token) || symbolp(token));

	// sign (if exists)
	int idx  = 0;
	value_t c = vref(token, idx);
	assert(charp(c));

	int sign = 1;
	if(INTOF(c) == '-' || INTOF(c) == '+')
	{
		idx++;

		if(INTOF(c) == '-')
		{
			sign = -1;
		}

		// next character
		c = vref(token, idx);

		if(errp(c))	// not int
		{
			return NIL;
		}

		assert(charp(c));
	}

	// value
	int val = 0;
	for(; idx < vsize(token); idx++)
	{
		c = vref(token, idx);
		assert(charp(c));

		int cv = INTOF(c) - '0';
		if(cv >= 0 && cv <= 9)
		{
			val *= 10;
			val += cv;
		}
		else
		{
			return NIL;
		}
	}

	return RINT(sign * val);
}

static value_t read_atom(scan_t* st)
{
	value_t token = scan_peek(st);

	if(symbolp(token))
	{
		value_t rint = read_int(token);
		if(!nilp(rint))
		{
			token = rint;
		}
		else
		{
			token = register_sym(token);
			value_t tbl[] = {
				g_setq,           RSPECIAL(SP_SETQ),
				g_let,            RSPECIAL(SP_LET),
				g_progn,          RSPECIAL(SP_PROGN),
				g_if,             RSPECIAL(SP_IF),
				g_lambda,         RSPECIAL(SP_LAMBDA),
				g_quote,          RSPECIAL(SP_QUOTE),
				g_quasiquote,     RSPECIAL(SP_QUASIQUOTE),
				g_macro,          RSPECIAL(SP_MACRO),
				g_macroexpand,    RSPECIAL(SP_MACROEXPAND),
				g_unquote,        RSPECIAL(SP_UNQUOTE),
				g_splice_unquote, RSPECIAL(SP_SPLICE_UNQUOTE),
				g_amp,            RSPECIAL(SP_AMP),
			};
			for(int i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i += 2)
			{
				if(EQ(tbl[i], token))
				{
					token = tbl[i+1];
					break;
				}
			}
		}
	}
	else if(vectorp(token))	// string
	{
		token = copy_vector(token);
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
		assert(vectorp(token) || symbolp(token) || errp(token));

		if(errp(token))
		{
			return token;	// scan error
		}
		else if(symbolp(token))
		{
			value_t c = vref(token, 0);	// first char of token

			assert(charp(c));
			if(INTOF(c) == ')')
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

	return RERR(ERR_PARSE, NIL);	// error: end of token before ')'
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
		assert(vectorp(token) || symbolp(token));
		if(symbolp(token))
		{
			value_t c  = vref(token, 0);
			assert(charp(c));

			if(INTOF(c) == '(')
			{
				return read_list(st);
			}
			else if(INTOF(c) == '\'')
			{
				scan_next(st);
				return cons(RSPECIAL(SP_QUOTE), cons(read_form(st), NIL));
			}
			else if(INTOF(c) == '`')
			{
				scan_next(st);
				return cons(RSPECIAL(SP_QUASIQUOTE), cons(read_form(st), NIL));
			}
			else if(INTOF(c) == ',')
			{
				token = scan_next(st);
				if(errp(token))	// scan error
				{
					return token;
				}

				assert(vectorp(token) || symbolp(token));
				if(symbolp(token))
				{
					value_t c  = vref(token, 0);
					assert(charp(c));
					if(!errp(c) && INTOF(c) == '@')
					{
						scan_next(st);
						return cons(RSPECIAL(SP_SPLICE_UNQUOTE), cons(read_form(st), NIL));
					}
					else
					{
						return cons(RSPECIAL(SP_UNQUOTE), cons(read_form(st), NIL));
					}
				}
				else
				{
					return cons(RSPECIAL(SP_UNQUOTE), cons(read_form(st), NIL));
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
	value_t	 r	= make_vector(0);

	for(;;)
	{
		int c = fgetc(fp);
		if(c == EOF && vsize(r) == 0)	// EOF
		{
			return RERR(ERR_EOF, NIL);
		}
		else if(c == '\n' || c == EOF)
		{
			return r;
		}
		else
		{
			vpush(RCHAR(c), r);
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

	value_t r = read_form(&st);
	return r;
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
		return RERR(ERR_EOF, NIL);
	}
#else	// USE_LINENOISE
	fputs(prompt, stdout);
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
