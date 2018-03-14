#include <assert.h>
#include "builtin.h"
#include "scanner.h"


/////////////////////////////////////////////////////////////////////
// private: scanner

static void scan_to_lf(value_t *s)
{
	assert(s != NULL);
	assert(rtypeof(*s) == CONS_T);

	while(!nilp(*s))
	{
		assert(rtypeof(*s) == CONS_T);

		value_t c = car(*s);	// current char
		assert(rtypeof(c) == INT_T);

		if(c.rint.val == '\n')
		{
			*s = cdr(*s);
			return ;
		}

		*s = cdr(*s);
	}

	return ;
}

static value_t scan_to_whitespace(value_t *s)
{
	assert(s != NULL);
	assert(rtypeof(*s) == CONS_T);

	value_t r = NIL;
	value_t *cur = &r;

	while(!nilp(*s))
	{
		assert(rtypeof(*s) == CONS_T);

		value_t c = car(*s);	// current char

		assert(rtypeof(c) == INT_T);

		if( c.rint.val == ' '  ||
		    c.rint.val == '\t' ||
		    c.rint.val == '\n' ||
		    c.rint.val == '('  ||
		    c.rint.val == ')'  ||
		    c.rint.val == '['  ||
		    c.rint.val == ']'  ||
		    c.rint.val == '{'  ||
		    c.rint.val == '}'  ||
		    c.rint.val == ';'  ||
		    c.rint.val == '\'' ||
		    c.rint.val == '"'  ||
		    c.rint.val == ','
		  )
		{
			r.type.main = SYM_T;
			return r;
		}
		else
		{
			cur = cons_and_cdr(c, cur);

			// next
			*s = cdr(*s);
		}
	}

	// must be refactored
	r.type.main = SYM_T;
	return r;
}

static value_t scan_to_doublequote(value_t *s)
{
	assert(s != NULL);
	assert(rtypeof(*s) == CONS_T);

	value_t r = NIL;
	value_t *cur = &r;

	int st = 0;
	while(!nilp(*s))
	{
		assert(rtypeof(*s) == CONS_T);

		value_t c = car(*s);	// current char

		assert(rtypeof(c) == INT_T);

		switch(st)
		{
		    case 0:	// not escape
			if(c.rint.val == '\\')
			{
				st = 1;	// enter escape mode
			}
			else if(c.rint.val == '"')
			{
				*s = cdr(*s);
				r.type.main = STR_T;
				return r;
			}
			else
			{
				cur = cons_and_cdr(c, cur);
			}
			break;

		    case 1:	// escape
		    	if(c.rint.val == 'n')	// \n
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

		// next
		*s = cdr(*s);
	}

	return RERR(ERR_PARSE);	// error: end of source string before doublequote
}

static value_t scan1(value_t *s)
{
	assert(s != NULL);
	assert(rtypeof(*s) == CONS_T);

	while(!nilp(*s))
	{
		assert(rtypeof(*s) == CONS_T);

		value_t c = car(*s);	// current char
		assert(rtypeof(c) == INT_T);

		// skip white space
		if( c.rint.val == ' '  ||
		    c.rint.val == '\t' ||
		    c.rint.val == '\n' ||
		    c.rint.val == ','
		  )
		{
			*s = cdr(*s);
		}
		else
		{
			switch(c.rint.val)
			{
			    // special character
			    case '~':
			    case '[':
			    case ']':
			    case '{':
			    case '}':
			    case '(':
			    case ')':
			    case '\'':
			    case '`':
			    case '^':
			    case '@':
				*s           = cdr (*s);
				value_t sr   = cons(c, NIL);
				sr.type.main = SYM_T;
				return sr;

			    // comment
			    case ';':
				*s           = cdr (*s);
				scan_to_lf(s);
			        break;	// return to skip-white-space

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

	return NIL;	// no form or atom
}



/////////////////////////////////////////////////////////////////////
// public: scanner interface
scan_t scan_init(value_t str)
{
	scan_t r = { 0 };
	r.cur    = str;
	if(r.cur.type.main == STR_T)
	{
		r.cur.type.main = CONS_T;
	}
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
