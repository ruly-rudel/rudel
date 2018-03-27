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

static inline vector_t* aligned_vaddr(value_t v)
{
#if __WORDSIZE == 32
	return (vector_t*) (((uint32_t)v.vector) & 0xfffffff8);
#else
	return (vector_t*) (((uint64_t)v.vector) & 0xfffffffffffffff8);
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

static inline vector_t* alloc_vector()
{
	vector_t* v = (vector_t*)malloc(sizeof(vector_t));
#if 0
#if __WORDSIZE == 32
	fprintf(stderr, "vector: %08x, size %d\n", (int)v, sizeof(vector_t));
#else
	fprintf(stderr, "vector: %016lx, size %ld\n", (int64_t)v, sizeof(vector_t));
#endif
#endif
	return v;
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

value_t car(value_t x)
{
	if(is_cons_pair_or_nil(x))
	{
		cons_t* c = aligned_addr(x);
		return c ? c->car : NIL;
	}
	else
	{
		return RERR(ERR_TYPE, NIL);
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
		return RERR(ERR_TYPE, NIL);
	}
}

value_t	cons(value_t car, value_t cdr)
{
	value_t	r	= { 0 };
	// r.type.main is always 0 (=CONS_T) because malloc returns ptr aligned to 8byte.
	// r.type.main	= CONS_T;
	r.cons		= alloc_cons();
	if(r.cons)
	{
		r.cons->car	= car;
		r.cons->cdr	= cdr;
		return r;
	}
	else
	{
		return RERR(ERR_ALLOC, NIL);
	}
}

bool seqp(value_t x)
{
	return is_seq(x);
}

bool atom(value_t x)
{
	return !is_seq_or_nil(x) || nilp(x);
}

bool eq(value_t x, value_t y)
{
	return EQ(x, y);
}

bool equal(value_t x, value_t y)
{
	if(EQ(x, y))
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
	else if(vectorp(x))
	{
		return veq(x, y);
	}
	else
	{
		return false;
	}
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
		x = RERR(ERR_TYPE, NIL);
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
		x = RERR(ERR_TYPE, NIL);
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
		return RERR(ERR_TYPE, NIL);
	}
}

value_t nth(int n, value_t x)
{
	if(is_seq_or_nil(x))
	{
		x.cons = aligned_addr(x);
		if(nilp(x))
		{
			return RERR(ERR_RANGE, NIL);
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
		return RERR(ERR_TYPE, NIL);
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

value_t copy_list(value_t list)
{
	assert(is_cons_pair_or_nil(list));
	unsigned int type = list.type.main;
	list.cons    = aligned_addr(list);
	value_t    r = NIL;
	value_t* cur = &r;

	for(; !nilp(list); list = cdr(list))
		cur = cons_and_cdr(car(list), cur);

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
	for(; !nilp(key); key = cdr(key), val = cdr(val))
	{
		assert(rtypeof(key) == CONS_T);
		assert(rtypeof(val) == CONS_T);

		r = cons(cons(car(key), car(val)), r);
	}
	return r;
}

value_t concat(int n, ...)
{
	va_list	 arg;
	value_t r = NIL;

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

int	count(value_t x)
{
	if(rtypeof(x) == CONS_T)
	{
		int i;
		for(i = 0; !nilp(x); x = cdr(x), i++)
			assert(consp(x));

		return i;
	}
	else if(rtypeof(x) == VEC_T)
	{
		return INTOF(vsize(x));
	}
	else
	{
		return 1;
	}
}

value_t symbol_string(value_t sym)
{
	assert(symbolp(sym));
	sym.type.main = VEC_T;
	value_t r = copy_vector(sym);

	return r;
}


/////////////////////////////////////////////////////////////////////
// public: rudel-specific functions

value_t rerr(value_t cause, value_t pos)
{
	assert(vectorp(cause) || intp(cause));

	value_t r = cons(cause, pos);
	r.type.main = ERR_T;
#ifdef DEBUG
	abort();
#endif

	return r;
}

value_t rerr_add_pos(value_t pos, value_t e)
{
	value_t r = cons(car(e), cons(pos, cdr(e)));
	r.type.main = ERR_T;
#ifdef DEBUG
	abort();
#endif

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

value_t	macro(value_t car, value_t cdr)
{
	value_t	r	= cons(car, cdr);
	r.type.main	= MACRO_T;

	return r;
}

value_t slurp(char* fn)
{
	FILE* fp = fopen(fn, "r");

	// read all
	if(fp)
	{
		value_t buf = make_vector(0);

		int c;
		while((c = fgetc(fp)) != EOF)
		{
			vpush(buf, RCHAR(c));
		}
		fclose(fp);

		return buf;
	}
	else	// file not found or other error.
	{
		return RERR(ERR_FILENOTFOUND, NIL);
	}
}

value_t init(value_t env)
{
	env = last(env);
	rplaca(env, car(create_root_env()));

	value_t c = vconc(vconc(str_to_rstr("(progn "), slurp("init.rud")), str_to_rstr(")"));
	return eval(read_str(c), env);
	//return NIL;
}

/////////////////////////////////////////////////////////////////////
// public: vector support

value_t make_vector(unsigned n)
{
	value_t v = { 0 };
	v.vector  = alloc_vector();

	v.vector->size  = RINT(0);
	v.vector->alloc = RINT(0);
	v.vector->rptr  = RINT(0);
	v.vector->data  = 0;
	v.type.main     = VEC_T;
	vresize(v, n);
	return v;
}

value_t vref(value_t v, unsigned pos)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = aligned_vaddr(v);

	if(pos < INTOF(v.vector->size))
	{
		return v.vector->data[pos];
	}
	else
	{
		return RERR(ERR_RANGE, NIL);
	}
}

value_t rplacv(value_t v, unsigned pos, value_t data)
{
	assert(vectorp(v));

	if(pos >= INTOF(vsize(v)))
	{
		vresize(v, pos + 1);
	}

	v.vector = aligned_vaddr(v);
	v.vector->data[pos] = data;

	return data;
}

value_t vsize(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = aligned_vaddr(v);
	return v.vector->size;
}

value_t vrptr(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v.vector = aligned_vaddr(v);
	return v.vector->rptr;
}

value_t vset_rptr(value_t v, value_t n)
{
	assert(vectorp(v) || symbolp(v));
	assert(intp(n));

	v.vector = aligned_vaddr(v);
	return v.vector->rptr = n;
}

value_t vresize(value_t v, int n)
{
	assert(vectorp(v));
	vector_t* va = aligned_vaddr(v);
	if(n < 0)
	{
		return RERR(ERR_RANGE, NIL);
	}
	else
	{

		// resize allocated area
		if(INTOF(va->alloc) < n)
		{
			for(va->alloc = RINT(1); INTOF(va->alloc) < n; va->alloc = RINT(INTOF(va->alloc) << 1));
			value_t* np = (value_t*)realloc(va->data, INTOF(va->alloc) * sizeof(value_t));
			if(np)
			{
				va->data = np;
			}
			else
			{
				return RERR(ERR_ALLOC, NIL);
			}
		}

		// change size and NILify new area
		int cur_size = INTOF(va->size);
		va->size = RINT(n);
		for(int i = cur_size; i < n; i++)
		{
			va->data[i] = NIL;
		}
	}

	return v;
}

bool veq(value_t x, value_t y)
{
	assert(vectorp(x) || symbolp(x));
	assert(vectorp(y) || symbolp(y));
	x.type.main = VEC_T;
	y.type.main = VEC_T;

	if(!EQ(vsize(x), vsize(y)))
	{
		return false;
	}
	else
	{
		for(int i = 0; i < INTOF(vsize(x)); i++)
		{
			if(!EQ(vref(x, i), vref(y, i)))
			{
				return false;
			}
		}
	}

	return true;
}

value_t vpush(value_t v, value_t x)
{
	assert(vectorp(v));

	return rplacv(v, INTOF(vsize(v)), x);
}

value_t vpop(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	int s = INTOF(vsize(v));

	if(s <= 0)
	{
		return RERR(ERR_RANGE, NIL);
	}
	else
	{
		value_t r = vref(v, s - 1);
		vresize(v, s - 1);

		return r;
	}
}

value_t vpeek_front(value_t v)
{
	assert(vectorp(v) || symbolp(v));

	int s = INTOF(vsize(v));
	int r = INTOF(vrptr(v));
	if(s <= r)
	{
		return RERR(ERR_EOS, NIL);
	}
	else
	{
		return vref(v, r);
	}
}

value_t vpop_front(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	value_t r = vpeek_front(v);
	if(!errp(r))
	{
		vset_rptr(v, RINT(INTOF(vrptr(v)) + 1));
	}

	return r;
}

value_t vpush_front(value_t v, value_t x)
{
	assert(vectorp(v) || symbolp(v));

	vpush(v, NIL);
	for(int i = INTOF(vsize(v)) - 2; i >= 0; i--)
	{
		rplacv(v, i + 1, vref(v, i));
	}
	rplacv(v, 0, x);

	return x;
}

value_t copy_vector(value_t src)
{
	assert(vectorp(src));
	value_t r = make_vector(INTOF(vsize(src)));

	for(int i = 0; i < INTOF(vsize(src)); i++)
	{
		rplacv(r, i, vref(src, i));
	}

	return r;
}

value_t vconc(value_t x, value_t y)
{
	assert(vectorp(x));
	assert(vectorp(y));

	int sx = INTOF(vsize(x));
	int sy = INTOF(vsize(y));

	value_t r = make_vector(sx + sy);

	// copy x
	for(int i = 0; i < sx; i++)
	{
		rplacv(r, i, vref(x, i));
	}

	// copy y
	for(int i = 0; i < sy; i++)
	{
		rplacv(r, sx + i, vref(y, i));
	}

	return r;

}

value_t vnconc(value_t x, value_t y)
{
	assert(vectorp(x));
	assert(vectorp(y));

	// copy y
	int sy = INTOF(vsize(y));
	for(int i = 0; i < sy; i++)
	{
		vpush(x, vref(y, i));
	}

	return x;
}

value_t make_vector_from_list(value_t x)
{
	assert(is_cons_pair_or_nil(x));

	value_t r = make_vector(0);
	for(; !nilp(x); x = cdr(x))
	{
		vpush(r, car(x));
	}

	return r;
}

value_t make_list_from_vector(value_t x)
{
	assert(vectorp(x));

	value_t r    = NIL;
	value_t* cur = &r;

	for(int i = 0; i < INTOF(vsize(x)); i++)
	{
		cur = cons_and_cdr(vref(x, i), cur);
	}

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
		cur = cons_and_cdr(RINT(c), cur);
	}

	return r;
}

value_t str_to_vec	(const char* s)
{
	assert(s != NULL);
	value_t    r = make_vector(0);

	int c;
	while((c = *s++) != '\0')
	{
		vpush(r, RCHAR(c));
	}

	return r;
}

value_t str_to_rstr	(const char* s)
{
	assert(s != NULL);
	value_t r = str_to_vec(s);
	if(INTOF(vsize(r)) == 0)
	{
		vpush(r, RCHAR('\0'));
	}

	return r;
}

value_t str_to_sym	(const char* s)
{
	value_t r = str_to_rstr(s);
	r.type.main = SYM_T;

	return register_sym(r);
}

char*   rstr_to_str	(value_t s)
{
	assert(vectorp(s));

	char* buf = (char*)malloc(INTOF(vsize(s)) + 1);
	if(buf)
	{
		char *p = buf;
		for(value_t c = vpop_front(s); !errp(c); c = vpop_front(s))
		{
			assert(charp(c));
			*p++ = INTOF(c);
		}
		*p = '\0';
	}

	return buf;
}

/////////////////////////////////////////////////////////////////////
// public: symbol functions

value_t register_sym(value_t s)
{
	value_t sym = find(s, s_symbol_pool, veq);
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
	r         = vnconc(r, pr_str(n, NIL, false));
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
	return vectorp(v) && INTOF(vsize(v)) > 0 && charp(vref(v, 0));
#else  // STR_EASY_CHECK
	if(nilp(v)) return false;	// nil is not strgin

	for(int i = 0; i < INTOF(vsize(v)); i ++)
	{
		if(!charp(vref(v, i)))
		{
			return false;
		}
	}

	return true;
#endif // STR_EASY_CHECK
}

// End of File
/////////////////////////////////////////////////////////////////////
