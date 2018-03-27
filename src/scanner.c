#include <assert.h>
#include "builtin.h"
#include "scanner.h"


/////////////////////////////////////////////////////////////////////
// private: scanner

static void scan_to_lf(value_t s)
{
	assert(is_str(s));

	for(value_t c = vpeek_front(s); !errp(c); c = (vpop_front(s), vpeek_front(s)))
	{
		assert(charp(c));

		if(INTOF(c) == '\n')
		{
			vpop_front(s);
			return ;
		}
	}

	return ;
}

static value_t scan_to_whitespace(value_t s)
{
	assert(is_str(s));

	value_t r = make_vector(0);

	for(value_t c = vpeek_front(s); !errp(c); c = (vpop_front(s), vpeek_front(s)))
	{
		assert(charp(c));

		if( INTOF(c) == ' '  ||
		    INTOF(c) == '\t' ||
		    INTOF(c) == '\n' ||
		    INTOF(c) == '('  ||
		    INTOF(c) == ')'  ||
		    INTOF(c) == ';'
		  )
		{
			if(INTOF(vsize(r)) == 0)
			{
				vpush(r, RCHAR('\0'));
			}
			r.type.main = SYM_T;
			return r;
		}
		else
		{
			vpush(r, c);
		}
	}

	// must be refactored
	r.type.main = SYM_T;
	return r;
}

static value_t scan_to_doublequote(value_t s)
{
	assert(is_str(s));

	value_t r = make_vector(0);

	int st = 0;

	for(value_t c = vpeek_front(s); !errp(c); c = (vpop_front(s), vpeek_front(s)))
	{
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
				vpop_front(s);
				if(INTOF(vsize(r)) == 0)
				{
					vpush(r, RCHAR('\0'));	// null string
				}
				return r;
			}
			else
			{
				vpush(r, c);
			}
			break;

		    case 1:	// escape
			if(INTOF(c) == 'n')	// \n
			{
				vpush(r, RCHAR('\n'));
			}
			else
			{
				vpush(r, c);
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

	return cc == ' ' || cc == '\t' || cc == '\n'
		         || cc == '\0';	//******* ad-hock
}

static value_t scan1(value_t s)
{
	assert(is_str(s));

	// skip space
	for(value_t c = vpeek_front(s); !errp(c) && is_ws(c); c = (vpop_front(s), vpeek_front(s)))
		assert(charp(c));

	value_t c = vpeek_front(s);
	if(errp(c))
	{
		return NIL;
	}
	else
	{
		switch(INTOF(c))
		{
		    // special character
		    case ',':
		    case '(':
		    case ')':
		    case '\'':
		    case '`':
		    case '@':
			vpop_front(s);
			value_t sr   = make_vector(0);
			vpush(sr, c);
			sr.type.main = SYM_T;
			return sr;

		    // comment
		    case ';':
			vpop_front(s);
			scan_to_lf(s);
			return scan1(s);

		    // string
		    case '"':
			vpop_front(s);
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
	r.token  = scan1(r.cur);

	return r;
}

value_t scan_next(scan_t *s)
{
	assert(s != NULL);

	s->token = scan1(s->cur);
	return s->token;
}

value_t scan_peek(scan_t *s)
{
	assert(s != NULL);

	return s->token;
}

// End of File
/////////////////////////////////////////////////////////////////////
