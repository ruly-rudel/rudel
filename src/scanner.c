#include <assert.h>
#include "builtin.h"
#include "scanner.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: scanner

// return current pointed char
static int scan_pch(scan_t* s)
{
	assert(s);
	if(s->rptr >= vsize(s->buf))
	{
		return -1;
	}
	else
	{
		value_t r = vref(s->buf, s->rptr);
		assert(charp(r));
		return INTOF(r);
	}
}

// increment pointer and return pointed char
static int scan_nch(scan_t* s)
{
	assert(s);
	if(s->rptr >= vsize(s->buf) - 1)
	{
		s->rptr = vsize(s->buf);
		return -1;
	}
	else
	{
		s->rptr++;
		return scan_pch(s);
	}
}

static void scan_to_lf(scan_t* s)
{
	assert(s);

	for(int c = scan_pch(s); c >= 0; c = scan_nch(s))
	{
		if(c == '\n')
		{
			scan_nch(s);
			return ;
		}
	}

	return ;
}

static value_t scan_to_whitespace(scan_t* s)
{
	assert(s);

	value_t r = make_vector(0);
	push_root(&r);

	for(int c = scan_pch(s); c >= 0; c = scan_nch(s))
	{
		if(c == ' ' || c == '\t' || c == '\n' || c == '('  || c == ')' || c == ';')
		{
			break;
		}
		else
		{
			vpush(RCHAR(c), r);
		}
	}

	if(vsize(r) == 0)
	{
		vpush(RCHAR('\0'), r);
	}
	r.type.main = SYM_T;

	pop_root(1);
	return r;
}

static value_t scan_to_doublequote(scan_t* s)
{
	assert(s);

	value_t r = make_vector(0);
	push_root(&r);

	int st = 0;

	for(int c = scan_pch(s); c >= 0; c = scan_nch(s))
	{
		switch(st)
		{
		    case 0:	// not escape
			if(c == '\\')
			{
				st = 1;	// enter escape mode
			}
			else if(c == '"')
			{
				scan_nch(s);
				if(vsize(r) == 0)
				{
					vpush(RCHAR('\0'), r);	// null string
				}
				pop_root(1);
				return r;
			}
			else
			{
				vpush(RCHAR(c), r);
			}
			break;

		    case 1:	// escape
			if(c == 'n')	// \n
			{
				vpush(RCHAR('\n'), r);
			}
			else
			{
				vpush(RCHAR(c), r);
			}
			st = 0;
			break;
		}
	}

	pop_root(1);
	return RERR(ERR_PARSE, NIL);	// error: end of source string before doublequote
}

static inline bool is_ws(int c)
{
	return c == ' ' || c == '\t' || c == '\n'
		         || c == '\0';	//******* ad-hock
}

static value_t scan1(scan_t* s)
{
	assert(s);

	// skip space
	for(int c = scan_pch(s); c >= 0 && is_ws(c); c = scan_nch(s)) ;

	int c = scan_pch(s);
	if(c < 0)
	{
		return NIL;
	}
	else
	{
		switch(c)
		{
		    // special character(is first char)
		    case ',':
		    case '(':
		    case ')':
		    case '\'':
		    case '`':
		    case '@':
		    case '\\':
			scan_nch(s);
			value_t sr   = make_vector(1);
			vpush(RCHAR(c), sr);
			sr.type.main = SYM_T;
			return sr;

		    // comment
		    case ';':
			scan_nch(s);
			scan_to_lf(s);
			return scan1(s);

		    // string
		    case '"':
			scan_nch(s);
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
	r.buf    = str;
	push_root(&r.buf);
	r.rptr   = 0;
	r.token  = scan1(&r);
	push_root(&r.token);

	return r;
}

value_t scan_next(scan_t *s)
{
	assert(s != NULL);

	s->token = scan1(s);
	return s->token;
}

value_t scan_peek(scan_t *s)
{
	assert(s != NULL);

	return s->token;
}

void scan_close(void)
{
	pop_root(2);
}

// End of File
/////////////////////////////////////////////////////////////////////
