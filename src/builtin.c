#include <assert.h>
#include <stdlib.h>
#include "builtin.h"

/////////////////////////////////////////////////////////////////////
// private: cons allocator

static cons_t* aligned_addr(value_t v)
{
	return (cons_t*) (((uint64_t)v.cons) & 0xfffffffffffffff8);
}

static cons_t* alloc_cons(void)
{
	cons_t* c = (cons_t*)malloc(sizeof(cons_t));

	return c;
}



/////////////////////////////////////////////////////////////////////
// public: typical lisp functions

rtype_t rtypeof(value_t v)
{
	return v.type.main == OTH_T ? v.type.sub : v.type.main;
}

value_t car(value_t x)
{
	if(rtypeof(x) == VEC_T) x.type.main = CONS_T;

	if(rtypeof(x) == CONS_T)
	{
		return x.type.sub != 0 ? x.cons->car : NIL;
	}
	else
	{
		return RERR(ERR_TYPE);
	}
}

value_t cdr(value_t x)
{
	if(rtypeof(x) == VEC_T) x.type.main = CONS_T;

	if(rtypeof(x) == CONS_T)
	{
		return x.type.sub != 0 ? x.cons->cdr : NIL;
	}
	else
	{
		return RERR(ERR_TYPE);
	}
}

value_t	cons(value_t car, value_t cdr)
{
	value_t	r	= { 0 };
	// r.type.main is always 0 (=CONS_T) because malloc returns ptr aligned to 8byte.
	// r.type.main	= CONS_T;
	r.cons		= alloc_cons();
	r.cons->car	= car;
	r.cons->cdr	= cdr;

	return r;
}

value_t	cfn(value_t car, value_t cdr)
{
	value_t	r	= cons(car, cdr);
	r.type.main	= CFN_T;

	return r;
}

value_t	cloj(value_t car, value_t cdr)
{
	value_t	r	= cons(car, cdr);
	r.type.main	= CLOJ_T;

	return r;
}

bool errp(value_t x)
{
	return x.type.main == OTH_T && x.type.sub == ERR_T;
}

bool nilp(value_t x)
{
	return x.type.main == CONS_T && x.type.sub == 0 && x.type.val == 0;
}

bool intp(value_t x)
{
	return x.type.main == OTH_T && x.type.sub == INT_T;
}

value_t rplaca(value_t x, value_t v)
{
	if(rtypeof(x) == CONS_T || rtypeof(x) == STR_T)
	{
		cons_t* c = aligned_addr(x);
		c->car = v;
	}
	else
	{
		x = RERR(ERR_TYPE);
	}

	return x;
}

value_t rplacd(value_t x, value_t v)
{
	if(rtypeof(x) == CONS_T || rtypeof(x) == STR_T)
	{
		cons_t* c = aligned_addr(x);
		c->cdr = v;
	}
	else
	{
		x = RERR(ERR_TYPE);
	}

	return x;
}

value_t last(value_t x)
{
	if(rtypeof(x) == CONS_T || rtypeof(x) == STR_T || rtypeof(x) == SYM_T)
	{
		if(nilp(x))
		{
			return NIL;
		}
		else
		{
			value_t i;
			for(i.cons = aligned_addr(x); !nilp(cdr(i)); i = cdr(i))
				;

			return i;
		}
	}
	else
	{
		return RERR(ERR_TYPE);
	}
}

value_t nth(int n, value_t x)
{
	if(rtypeof(x) != CONS_T && rtypeof(x) != VEC_T && rtypeof(x) != STR_T && rtypeof(x) != SYM_T)
	{
		return RERR(ERR_TYPE);
	}
	x.type.main = CONS_T;

	if(nilp(x))
	{
		return RERR(ERR_RANGE);
	}

	if(n == 0)
	{
		return car(x);
	}
	else
	{
		return nth(n - 1, cdr(x));
	}
}

value_t nconc(value_t a, value_t b)
{
	assert(rtypeof(a) == CONS_T || rtypeof(a) == STR_T || rtypeof(a) == SYM_T || rtypeof(a) == VEC_T);


	value_t l = last(a);
	if(nilp(l))
	{
		a = b;
	}
	else
	{
		b.type.main = CONS_T;
		rplacd(l, b);
	}

	return a;
}

value_t list(int n, ...)
{
	va_list	 arg;
	value_t    r = NIL;
	value_t* cur = &r;

	va_start(arg, n);
	for(int i = 0; i < n; i++)
	{
		cur = cons_and_cdr(va_arg(arg, value_t), cur);
	}

	va_end(arg);

	return r;
}

bool eq(value_t x, value_t y)
{
	return x.raw == y.raw;
}

bool equal(value_t x, value_t y)
{
	if(rtypeof(x) == VEC_T) x.type.main = CONS_T;
	if(rtypeof(y) == VEC_T) y.type.main = CONS_T;

	if(eq(x, y))
	{
		return true;
	}
	else if(rtypeof(x) != rtypeof(y))
	{
		return false;
	}
	else if(rtypeof(x) == CONS_T || rtypeof(x) == SYM_T || rtypeof(x) == STR_T)
	{
		x.type.main = CONS_T;
		y.type.main = CONS_T;
		if(nilp(x))
		{
			return nilp(y);
		}
		else if(nilp(y))
		{
			return nilp(x);
		}
		else
		{
			return equal(car(x), car(y)) && equal(cdr(x), cdr(y));
		}
	}
	else
	{
		return false;
	}
}

value_t copy_list(value_t list)
{
	assert(rtypeof(list) == CONS_T || rtypeof(list) == SYM_T || rtypeof(list) == STR_T || rtypeof(list) == VEC_T);
	unsigned int type = list.type.main;
	list.type.main = CONS_T;
	value_t    r = NIL;
	value_t* cur = &r;

	while(!nilp(list))
	{
		cur = cons_and_cdr(car(list), cur);
		list = cdr(list);
	}

	r.type.main = type;
	return r;
}

value_t assoc(value_t key, value_t list)
{
	assert(rtypeof(list) == CONS_T);

	if(nilp(list))
	{
		return NIL;
	}
	else
	{
		for(value_t v = list; !nilp(v); v = cdr(v))
		{
			value_t vcar = car(v);
			assert(rtypeof(vcar) == CONS_T);

			if(equal(car(vcar), key))
				return vcar;
		}

		return NIL;
	}
}

value_t acons(value_t key, value_t val, value_t list)
{
	assert(rtypeof(list) == CONS_T);
	return cons(cons(key, val), list);
}

value_t pairlis1(value_t key, value_t val, value_t alist)
{
	assert(rtypeof(key)   == CONS_T);
	assert(rtypeof(val)   == CONS_T);
	assert(rtypeof(alist) == CONS_T);

	if(nilp(key))
	{
		return alist;
	}
	else
	{
		return pairlis1(cdr(key), cdr(val), acons(car(key), car(val), alist));
	}
}

value_t pairlis		(value_t key, value_t val)
{
	assert(rtypeof(key) == CONS_T);
	assert(rtypeof(val) == CONS_T);

	return pairlis1(key, val, NIL);
}

bool is_pair		(value_t list)
{
#ifdef MAL
	return rtypeof(list) == CONS_T && !nilp(car(list));
#else  // MAL
	return rtypeof(list) == CONS_T && !nilp(list);
#endif // MAL
}

bool is_empty(value_t list)
{
#ifdef MAL
	if(rtypeof(list) == VEC_T) list.type.main = CONS_T;
	return rtypeof(list) == CONS_T && nilp(car(list)) && nilp(cdr(list));
#else  // MAL
	return nilp(list);
#endif // MAL
}

value_t concat(int n, ...)
{
	va_list	 arg;
	value_t r = EMPTY_LIST;

	va_start(arg, n);
	for(int i = 0; i < n; i++)
	{
		value_t arg1 = va_arg(arg, value_t);
		value_t copy = copy_list(arg1);
		copy.type.main = CONS_T;

#ifdef MAL
		if(is_empty(r))
		{
			r = copy;
		}
		else
		{
			r = nconc(r, copy);
		}
#else  // MAL
		r = nconc(r, copy);
#endif // MAL
	}

	va_end(arg);

	return r;
}

/////////////////////////////////////////////////////////////////////
// public: bridge functions from C to LISP

value_t str_to_cons	(const char* s)
{
	assert(s != NULL);
	value_t    r = NIL;
	value_t* cur = &r;

	int c;
	while((c = *s++) != '\0')
	{
		cur = cons_and_cdr(RCHAR(c), cur);
	}

	return r;
}

value_t str_to_sym	(const char* s)
{
	value_t r = str_to_cons(s);
	r.type.main = SYM_T;

	return r;
}

value_t str_to_rstr	(const char* s)
{
	value_t r = str_to_cons(s);
	r.type.main = STR_T;

	return r;
}

char*   rstr_to_str	(value_t s)
{
	assert(rtypeof(s) == STR_T);
	s.type.main = CONS_T;

	char* buf = (char*)malloc(256);	// fix it
	if(buf)
	{
		buf[0] = '\0';
		char *p = buf;
		while(!nilp(s))
		{
			*p++ = car(s).rint.val;
			s = cdr(s);
		}
	}

	return buf;
}


/////////////////////////////////////////////////////////////////////
// public: support functions writing LISP on C
value_t* cons_and_cdr(value_t v, value_t* c)
{
	*c = cons(v, NIL);
	return &c->cons->cdr;
}

// End of File
/////////////////////////////////////////////////////////////////////
