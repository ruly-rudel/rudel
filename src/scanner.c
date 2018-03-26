#include <assert.h>
#include "builtin.h"
#include "scanner.h"


/////////////////////////////////////////////////////////////////////
// private: scanner

static void scan_to_lf(value_t *s)
{
	assert(s != NULL);

	for(; !nilp(*s); *s = cdr(*s))
	{
		assert(consp(*s));

		value_t c = car(*s);	// current char
		assert(rtypeof(c) == CHAR_T);

		if(INTOF(c) == '\n')
		{
			*s = cdr(*s);
			return ;
		}
	}

	return ;
}

static value_t scan_to_whitespace(value_t *s)
{
	assert(s != NULL);

	value_t r = NIL;
	value_t *cur = &r;

	for(; !nilp(*s); *s = cdr(*s))
	{
		assert(consp(*s));

		value_t c = car(*s);	// current char

		assert(charp(c));

		if( INTOF(c) == ' '  ||
		    INTOF(c) == '\t' ||
		    INTOF(c) == '\n' ||
		    INTOF(c) == '('  ||
		    INTOF(c) == ')'  ||
		    INTOF(c) == ';'
		  )
		{
			r.type.main = SYM_T;
			return r;
		}
		else
		{
			cur = cons_and_cdr(c, cur);
		}
	}

	// must be refactored
	r.type.main = SYM_T;
	return r;
}

static value_t scan_to_doublequote(value_t *s)
{
	assert(s != NULL);

	value_t r = NIL;
	value_t *cur = &r;

	int st = 0;
	for(; !nilp(*s); *s = cdr(*s))
	{
		assert(consp(*s));

		value_t c = car(*s);	// current char

		assert(charp(c));

		switch(st)
		{
		    case 0:	// not escape
			if(INTOF(c) == '\\')
			{
				st = 1;	// enter escape mode
			}
			else if(INTOF(c) == '"')
			{
				*s = cdr(*s);
				if(nilp(r))
				{
					r = cons(RCHAR('\0'), NIL);	// null string
				}
				return r;
			}
			else
			{
				cur = cons_and_cdr(c, cur);
			}
			break;

		    case 1:	// escape
			if(INTOF(c) == 'n')	// \n
			{
				cur = cons_and_cdr(RCHAR('\n'), cur);
			}
			else
			{
				cur = cons_and_cdr(c, cur);
			}
			st = 0;
			break;
		}
	}

	return RERR(ERR_PARSE, NIL);	// error: end of source string before doublequote
}

static inline bool is_ws(value_t c)
{
	assert(charp(c));
	int cc = INTOF(c);

	return cc == ' ' || cc == '\t' || cc == '\n';
}

static value_t scan1(value_t *s)
{
	assert(s != NULL);

	// skip space
	for(; !nilp(*s) && is_ws(car(*s)); *s = cdr(*s))
		assert(consp(*s));

	if(nilp(*s))
	{
		return NIL;
	}
	else
	{
		value_t c = car(*s);
		switch(INTOF(c))
		{
		    // special character
		    case ',':
		    case '(':
		    case ')':
		    case '\'':
		    case '`':
		    case '@':
			*s           = cdr (*s);
			value_t sr   = cons(c, NIL);
			sr.type.main = SYM_T;
			return sr;

		    // comment
		    case ';':
			*s           = cdr (*s);
			scan_to_lf(s);
			return scan1(s);

		    // string
		    case '"':
			*s           = cdr (*s);
			return scan_to_doublequote(s);

		    // atom
		    default:
			return scan_to_whitespace(s);
		}
	}
}



/////////////////////////////////////////////////////////////////////
// public: scanner interface
scan_t scan_init(value_t str)
{
	scan_t r = { 0 };
	r.cur    = str;
	r.token  = scan1(&r.cur);

	return r;
}

value_t scan_next(scan_t *s)
{
	assert(s != NULL);

	s->token = scan1(&s->cur);
	return s->token;
}

value_t scan_peek(scan_t *s)
{
	assert(s != NULL);

	return s->token;
}

// End of File
/////////////////////////////////////////////////////////////////////
