#include <assert.h>
#include <stdlib.h>
#include "builtin.h"
#include "eval.h"
#include "core.h"
#include "reader.h"
#include "env.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////
// private: symbol pool

static value_t s_symbol_pool = NIL;

/////////////////////////////////////////////////////////////////////
// private: cons allocator

static inline cons_t* aligned_addr(value_t v)
{
#if __WORDSIZE == 32
	return (cons_t*) (((uint32_t)v.cons) & 0xfffffff8);
#else
	return (cons_t*) (((uint64_t)v.cons) & 0xfffffffffffffff8);
#endif
}

static inline cons_t* alloc_cons(void)
{
	cons_t* c = (cons_t*)malloc(sizeof(cons_t));
#if 0
#if __WORDSIZE == 32
	fprintf(stderr, "cons: %08x\n", (int)c);
#else
	fprintf(stderr, "cons: %016lx\n", (int64_t)c);
#endif
#endif

	return c;
}

static inline bool is_cons_pair(value_t x)
{
	return rtypeof(x) < OTH_T && (x.type.sub != 0 || x.type.val != 0);
}

static inline bool is_cons_pair_or_nil(value_t x)
{
	return rtypeof(x) < OTH_T;
}

static inline bool is_seq(value_t x)
{
	return rtypeof(x) <= VEC_T && (x.type.sub != 0 || x.type.val != 0);
}

static inline bool is_seq_or_nil(value_t x)
{
	return rtypeof(x) <= VEC_T;
}


/////////////////////////////////////////////////////////////////////
// public: typical lisp functions

rtype_t rtypeof(value_t v)
{
	return v.type.main == OTH_T ? v.type.sub : v.type.main;
}

value_t car(value_t x)
{
	if(is_cons_pair_or_nil(x))
	{
		cons_t* c = aligned_addr(x);
		return c ? c->car : NIL;
	}
	else
	{
		return RERR(ERR_TYPE);
	}
}

value_t cdr(value_t x)
{
	if(is_cons_pair_or_nil(x))
	{
		cons_t* c = aligned_addr(x);
		return c ? c->cdr : NIL;
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

value_t rerr(value_t data)
{
	assert(rtypeof(data) == CONS_T || rtypeof(data) == INT_T);

	if(rtypeof(data) == INT_T)
	{
		data = cons(data, NIL);
	}
	data.type.main = ERR_T;

	return data;
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

value_t	macro(value_t car, value_t cdr)
{
	value_t	r	= cons(car, cdr);
	r.type.main	= MACRO_T;

	return r;
}

bool errp(value_t x)
{
	return x.type.main == ERR_T;
}

// only for reader and core
bool cnilp(value_t x)
{
	return x.type.main == CONS_T && x.type.sub == 0 && x.type.val == 0;
}

bool nilp(value_t x)
{
	return is_seq_or_nil(x) && x.type.sub == 0 && x.type.val == 0;
}

bool intp(value_t x)
{
	return x.type.main == OTH_T && x.type.sub == INT_T;
}

value_t rplaca(value_t x, value_t v)
{
	if(is_cons_pair(x))
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
	if(is_cons_pair(x))
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
	if(is_seq_or_nil(x))
	{
		value_t i;
		i.cons = aligned_addr(x);
		if(nilp(i))
		{
			return x;
		}
		else
		{
			for(; !nilp(cdr(i)); i = cdr(i))
				assert(is_seq_or_nil(i));

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
	if(is_seq_or_nil(x))
	{
		x.cons = aligned_addr(x);
		if(nilp(x))
		{
			return RERR(ERR_RANGE);
		}
		else
		{
			if(n == 0)
			{
				return car(x);
			}
			else
			{
				return nth(n - 1, cdr(x));
			}
		}

	}
	else
	{
		return RERR(ERR_TYPE);
	}
}

value_t nconc(value_t a, value_t b)
{
	assert(is_seq_or_nil(a));

	value_t l = last(a);
	if(nilp(l))
	{
		int type = a.type.main;
		a = b;
		a.type.main = type;
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
	if(eq(x, y))
	{
		return true;
	}
	else if(rtypeof(x) != rtypeof(y))
	{
		return false;
	}
	else if(is_cons_pair_or_nil(x))
	{
		x.cons = aligned_addr(x);
		y.cons = aligned_addr(y);
		if(nilp(x))
		{
			return nilp(y);
		}
		else if(nilp(y))
		{
			return false;
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

bool atom(value_t x)
{
	return rtypeof(x) != CONS_T || nilp(x);
}

value_t copy_list(value_t list)
{
	assert(is_cons_pair_or_nil(list));
	unsigned int type = list.type.main;
	list.cons    = aligned_addr(list);
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

value_t assoc(value_t key, value_t list, bool (*test)(value_t, value_t))
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

			if(test(car(vcar), key))
				return vcar;
		}

		return NIL;
	}
}

value_t assoceq(value_t key, value_t list)
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

			if(car(vcar).raw == key.raw)
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

value_t pairlis		(value_t key, value_t val)
{
	value_t r = NIL;
	while(!nilp(key))
	{
		assert(rtypeof(key) == CONS_T);
		assert(rtypeof(val) == CONS_T);

		r = cons(cons(car(key), car(val)), r);
		key = cdr(key);
		val = cdr(val);
	}
	return r;
}

bool consp(value_t list)
{
	return rtypeof(list) == CONS_T && !nilp(list);
}

bool seqp(value_t list)
{
	return is_seq(list);
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
		copy.cons    = aligned_addr(copy);

		r = nconc(r, copy);
	}

	va_end(arg);

	return r;
}

value_t find(value_t key, value_t list, bool (*test)(value_t, value_t))
{
	assert(is_seq_or_nil(list));

	if(nilp(list))
	{
		return NIL;
	}
	else
	{
		for(value_t v = list; !nilp(v); v = cdr(v))
		{
			if(test(car(v), key))
				return car(v);
		}

		return NIL;
	}
}

value_t slurp(char* fn)
{
	FILE* fp = fopen(fn, "r");

	// read all
	if(fp)
	{
		value_t buf = NIL;
		value_t* cur = &buf;

		int c;
		while((c = fgetc(fp)) != EOF)
		{
			cur = cons_and_cdr(RCHAR(c), cur);
		}
		fclose(fp);

		return buf;
	}
	else	// file not found or other error.
	{
		return RERR(ERR_FILENOTFOUND);
	}
}

value_t init(value_t env)
{
	env = last(env);
	rplaca(env, car(create_root_env()));

	value_t c = concat(3, str_to_rstr("(progn "), slurp("init.rud"), str_to_rstr(")"));
	return eval(read_str(c), env);
}

value_t reduce(value_t (*fn)(value_t, value_t), value_t args)
{
	value_t arg1 = car(args);
	value_t arg2 = car(cdr(args));
	args = cdr(args);

	do {
		args = cdr(args);
		arg1 = fn(arg1, arg2);
		arg2 = car(args);
	} while(!nilp(args));

	return arg1;
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
		cur = cons_and_cdr(RINT(c), cur);
	}

	return r;
}

value_t str_to_sym	(const char* s)
{
	value_t r = str_to_rstr(s);
	r.type.main = SYM_T;

	return register_sym(r);
}

value_t str_to_rstr	(const char* s)
{
	assert(s != NULL);
	value_t    r = NIL;
	value_t* cur = &r;

	if(*s == '\0')	// null string
	{
		*cur = cons(RCHAR('\0'), NIL);
	}
	else
	{
		int c;
		while((c = *s++) != '\0')
		{
			cur = cons_and_cdr(RCHAR(c), cur);
		}
	}

	return r;
}

char*   rstr_to_str	(value_t s)
{
	assert(rtypeof(s) == CONS_T);

	char* buf = (char*)malloc(512);	// fix it
	if(buf)
	{
		char *p = buf;
		while(!nilp(s))
		{
			assert(rtypeof(car(s)) == CHAR_T);
			*p++ = INTOF(car(s));
			s = cdr(s);
		}
		*p = '\0';
	}

	return buf;
}

/////////////////////////////////////////////////////////////////////
// public: symbol functions

value_t register_sym(value_t s)
{
	value_t sym = find(s, s_symbol_pool, equal);
	if(nilp(sym))
	{
		s_symbol_pool = cons(s, s_symbol_pool);
		return s;
	}
	else
	{
		return sym;
	}
}

value_t gensym	(value_t env)
{
	value_t r = str_to_rstr("#:G");
	value_t n = get_env_value(str_to_sym("*gensym-counter*"), env);
	r         = nconc(r, pr_str(n, NIL, false));
	r.type.main = SYM_T;

	set_env(str_to_sym("*gensym-counter*"), RINT(INTOF(n) + 1), env);

	return r;	// gensym symbol is unregisterd: so same name doesn't cause eq.
}

/////////////////////////////////////////////////////////////////////
// public: support functions writing LISP on C
value_t* cons_and_cdr(value_t v, value_t* c)
{
	*c = cons(v, NIL);
	return &c->cons->cdr;
}

value_t* nconc_and_last(value_t v, value_t* c)
{
	*c = v;
	value_t l = last(v);
	c->type.main = CONS_T;
	return &l.cons->cdr;
}

bool is_str(value_t v)
{
#ifdef STR_EASY_CHECK
	return rtypeof(v) == CONS_T && rtypeof(car(v)) == CHAR_T;
#else  // STR_EASY_CHECK
	if(nilp(v)) return false;	// nil is not strgin

	while(!nilp(v))
	{
		if(rtypeof(v) != CONS_T)
		{
			return false;
		}
		else
		{
			if(rtypeof(car(v)) != CHAR_T)
			{
				return false;
			}
		}
		v = cdr(v);
	}
	return true;
#endif // STR_EASY_CHECK
}

// End of File
/////////////////////////////////////////////////////////////////////
