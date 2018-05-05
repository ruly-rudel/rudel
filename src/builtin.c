#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "builtin.h"
#include "allocator.h"
#include "vm.h"
#include "compile_vm.h"
#include "reader.h"
#include "env.h"
#include "package.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////
// private: support functions

static inline bool is_cons_pair(value_t x)
{
	return rtypeof(x) >= CONS_T && rtypeof(x) <= ERR_T;
}

static inline bool is_cons_pair_or_nil(value_t x)
{
	return (rtypeof(x) >= CONS_T && rtypeof(x) <= ERR_T) || nilp(x);
}

/////////////////////////////////////////////////////////////////////
// public: typical lisp functions

value_t car(value_t x)
{
	assert(is_cons_pair_or_nil(x));
	x = AVALUE(x);
	return x.raw ? x.cons->car : NIL;
}

value_t cdr(value_t x)
{
	assert(is_cons_pair_or_nil(x));
	x = AVALUE(x);
	return x.raw ? x.cons->cdr : NIL;
}

value_t	cons(value_t car, value_t cdr)
{
	push_root(&car);
	push_root(&cdr);
	value_t	r	= { 0 };
	r.cons		= alloc_cons();
	pop_root(2);
	if(r.cons)
	{
		r.cons->car	= car;
		r.cons->cdr	= cdr;
		r.type.main	= CONS_T;
		return r;
	}
	else
	{
		return rerr_alloc();
	}
}

bool atom(value_t x)
{
	return rtypeof(x) != CONS_T || nilp(x);
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
	assert(is_cons_pair(x));

	AVALUE(x).cons->car = v;
	return x;
}

value_t rplacd(value_t x, value_t v)
{
	assert(is_cons_pair(x));

	AVALUE(x).cons->cdr = v;
	return x;
}

value_t last(value_t x)
{
	assert(is_cons_pair_or_nil(x));

	if(nilp(x))
	{
		return x;
	}
	else
	{
		for(; !nilp(cdr(x)); x = cdr(x))
			assert(is_cons_pair_or_nil(x));

		return x;
	}
}

value_t nth(int n, value_t x)
{
	assert(is_cons_pair_or_nil(x));

	for(int i = 0; i < n; i++, x = cdr(x))
	{
		if(rtypeof(x) != CONS_T)
		{
			return RERR(ERR_TYPE, NIL);
		}
		else if(nilp(x))
		{
			return RERR(ERR_RANGE, NIL);
		}
	}

	return nilp(x) ? RERR(ERR_RANGE, NIL) : car(x);
}

value_t nconc(value_t a, value_t b)
{
	assert(is_cons_pair_or_nil(a));

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
	value_t args[n];
	va_list	 arg;
	va_start(arg, n);
	for(int i = 0; i < n; i++)
	{
		args[i] = va_arg(arg, value_t);
	}
	va_end(arg);
	int num = n - 1;
	push_root_raw_vec(args, &num);

	value_t    r = cons(NIL, NIL);
	value_t  cur = r;
	push_root(&r);
	push_root(&cur);

	for(int i = 0; i < n; i++)
	{
		CONS_AND_CDR(args[i], cur)
	}

	pop_root(3);
	return cdr(r);
}

value_t find(value_t key, value_t list, bool (*test)(value_t, value_t))
{
	assert(is_cons_pair_or_nil(list));

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

int	count(value_t x)
{
	if(is_cons_pair_or_nil(x))
	{
		int i;
		for(i = 0; !nilp(x); x = cdr(x), i++)
			assert(consp(x));

		return i;
	}
	else if(vectorp(x))
	{
		return vsize(x);
	}
	else
	{
		return 1;
	}
}

value_t reverse(value_t x)
{
	assert(is_cons_pair_or_nil(x));

	if(nilp(x))
	{
		return x;
	}
	else
	{
		value_t r = NIL;
		push_root(&x);
		push_root(&r);
		for(; !nilp(x); x = cdr(x))
		{
			if(consp(x))
			{
				r = cons(car(x), r);
			}
			else
			{
				r = rerr(RINT(ERR_TYPE), NIL);
				break;
			}
		}

		pop_root(2);
		return r;
	}
}

value_t symbol_string(value_t sym)
{
	assert(symbolp(sym));

	sym.type.main = VEC_T;
	return copy_vector(sym);
}

value_t copy_list(value_t lst)
{
	push_root(&lst);
	value_t r   = cons(NIL, NIL);
	value_t cur = r;
	push_root(&r);
	push_root(&cur);

	rtype_t t = rtypeof(lst);

	for(; !nilp(lst); lst = cdr(lst))
	{
		CONS_AND_CDR(car(lst), cur);
	}

	r = cdr(r);
	r.type.main = t;

	pop_root(3);
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

value_t rerr_alloc(void)
{
	fprintf(stderr, "memory allocation fails.\n");
	value_t r = { 0 };
	r.type.main = ERR_T;
	abort();		//***** ad-hock

	return r;
}

value_t rerr_add_pos(value_t pos, value_t e)
{
	value_t cause = car(e);
	push_root(&cause);
	value_t newpos = cons(pos, cdr(e));
	pop_root(1);
	value_t r = cons(cause, newpos);
	r.type.main = ERR_T;
#ifdef DEBUG
	abort();
#endif
	return r;
}

value_t rerr_pos(value_t cause, value_t pos)
{
	push_root(&cause);
	value_t newpos = cons(pos, NIL);
	pop_root(1);
	value_t r = cons(cause, newpos);
	r.type.main = ERR_T;
#ifdef DEBUG
	abort();
#endif
	return r;
}

value_t	cloj(value_t ast, value_t ast_env, value_t code, value_t vm_env, value_t debug)
{
	value_t	r	= list(5, ast, ast_env, code, vm_env, debug);
	r.type.main	= CLOJ_T;

	return r;
}

value_t	macro(value_t ast, value_t ast_env, value_t code, value_t vm_env, value_t debug)
{
	value_t	r	= list(5, ast, ast_env, code, vm_env, debug);
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
		push_root(&buf);

		wint_t c;
		while((c = fgetwc(fp)) != WEOF)
		{
			vpush(RCHAR(c), buf);
		}
		fclose(fp);

		pop_root(1);
		return buf;
	}
	else	// file not found or other error.
	{
		return RERR(ERR_FILENOTFOUND, NIL);
	}
}

value_t init(value_t pkg)
{
	push_root(&pkg);
	value_t env = create_root_env(pkg);
	push_root(&env);

#ifndef NOINIT
	rep_file("init.rud", env);
#endif // NOINIT

	pop_root(2);
	return env;
}

void rep_file(char* fn, value_t env)
{
	lock_gc();

	// build terminate-catcher code
	value_t str = str_to_rstr("exception caches at root: ");
	value_t catch = make_vector(7);
	vpush(ROP (IS_PUSH),		catch);
	vpush(str,			catch);
	vpush(ROP(IS_VCONC),		catch);
	vpush(ROP(IS_PRINTLINE),	catch);
	vpush(ROP(IS_HALT),		catch);

	value_t clojure = cloj(NIL, NIL, cons(catch, catch), NIL, NIL);
	set_env(g_exception_stack, cons(clojure, NIL), env);

	// build rep code
	value_t rfn   = str_to_rstr(fn);
	value_t begin = str_to_rstr("(progn ");
	value_t end   = str_to_rstr(")");

	value_t code  = make_vector(21);

	vpush(ROP (IS_PUSHR),		code);
	vpush(end,			code);
	vpush(ROP (IS_PUSHR),		code);
	vpush(rfn,			code);
	vpush(ROP (IS_SLURP),		code);
	vpush(ROP (IS_DUP),		code);
	vpush(ROP (IS_ERRP),		code);
	vpush(ROPD(IS_BNIL, 2),		code);
	vpush(ROPD(IS_BR, 11),		code);
	vpush(ROP (IS_PUSHR),		code);
	vpush(begin,			code);
	vpush(ROP (IS_VCONC),		code);
	vpush(ROP (IS_VCONC),		code);
	vpush(ROP (IS_READ_STRING),	code);
	vpush(ROP (IS_DUP),		code);
	vpush(ROP (IS_ERRP),		code);
	vpush(ROPD(IS_BNIL, 2),		code);
	vpush(ROPD(IS_BR, 2),		code);
	vpush(ROP (IS_EVAL),		code);
	vpush(ROP (IS_PRINT),		code);
	vpush(ROP (IS_HALT),		code);

	value_t cd = cons(code, code);

	unlock_gc();

	// exec rep
	exec_vm(cd, env);
}

/////////////////////////////////////////////////////////////////////
// public: vector support

value_t make_vector(unsigned n)
{
	value_t v = { 0 };
	v.vector  = alloc_vector();

	if(v.vector)
	{
		v.vector->size  = RINT(0);
		v.vector->alloc = RINT(0);
		v.vector->type  = NIL;
		v.vector->data  = RPTR(0);
		v.type.main     = VEC_T;
		v = alloc_vector_data(v, n);
		return v;
	}
	else
	{
		return rerr_alloc();
	}
}

value_t vref(value_t v, unsigned pos)
{
	assert(vectorp(v) || symbolp(v));
	assert(vsize(v) <= vallocsize(v));
	v = AVALUE(v);

	if(pos < INTOF(v.vector->size))
	{
		return VPTROF(v.vector->data)[pos];
	}
	else
	{
		return RERR(ERR_RANGE, NIL);
	}
}

value_t rplacv(value_t v, unsigned pos, value_t data)
{
	assert(vectorp(v));
	assert(vsize(v) <= vallocsize(v));
	push_root(&v);
	push_root(&data);

	if(pos >= vsize(v))
	{
		vresize(v, pos + 1);
	}

	VPTROF(AVALUE(v).vector->data)[pos] = data;

	pop_root(2);
	return data;
}

int vsize(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v = AVALUE(v);
	assert(INTOF(v.vector->size) <= INTOF(v.vector->alloc));
	return INTOF(v.vector->size);
}

int vallocsize(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	v = AVALUE(v);
	assert(INTOF(v.vector->size) <= INTOF(v.vector->alloc));
	return INTOF(v.vector->alloc);
}

value_t vtype(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	assert(vsize(v) <= vallocsize(v));
	return AVALUE(v).vector->type;
}

value_t* vdata(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	assert(vsize(v) <= vallocsize(v));
	return VPTROF(AVALUE(v).vector->data);
}

value_t vresize(value_t v, int n)
{
	assert(vectorp(v));
	assert(vsize(v) <= vallocsize(v));
	value_t va = AVALUE(v);
	if(n < 0)
	{
		return RERR(ERR_RANGE, NIL);
	}
	else
	{
		// resize allocated area
		if(INTOF(va.vector->alloc) < n)
		{
			int new_alloc;
			for(new_alloc = 1; new_alloc < n; new_alloc *= 2);
			v = alloc_vector_data(v, new_alloc);
			if(nilp(v))
			{
				return rerr_alloc();
			}
			va = AVALUE(v);
		}

		// change size
		va.vector->size = RINT(n);
	}

	return v;
}

bool veq(value_t x, value_t y)
{
	assert(vectorp(x) || symbolp(x));
	assert(vectorp(y) || symbolp(y));
	assert(vsize(x) <= vallocsize(x));
	assert(vsize(y) <= vallocsize(y));
	x.type.main = VEC_T;
	y.type.main = VEC_T;

	if(vsize(x) != vsize(y))
	{
		return false;
	}
	else
	{
		for(int i = 0; i < vsize(x); i++)
		{
			if(!EQ(vref(x, i), vref(y, i)))
			{
				return false;
			}
		}
	}

	return true;
}

value_t vpush(value_t x, value_t v)
{
	assert(vectorp(v));
	assert(vsize(v) <= vallocsize(v));

	push_root(&v);
	rplacv(v, vsize(v), x);		//***** assume no error
	pop_root(1);

	return v;
}

value_t vpop(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	assert(vsize(v) <= vallocsize(v));
	int s = vsize(v);

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

value_t vpeek(value_t v)
{
	assert(vectorp(v) || symbolp(v));
	assert(vsize(v) <= vallocsize(v));
	int s = vsize(v);

	if(s <= 0)
	{
		return RERR(ERR_RANGE, NIL);
	}
	else
	{
		return vref(v, s - 1);
	}
}

value_t vpush_front(value_t v, value_t x)
{
	assert(vectorp(v) || symbolp(v));
	assert(vsize(v) <= vallocsize(v));
	push_root(&v);
	push_root(&x);

	vpush(NIL, v);
	for(int i = vsize(v) - 2; i >= 0; i--)
	{
		rplacv(v, i + 1, vref(v, i));
	}
	rplacv(v, 0, x);

	pop_root(2);
	return x;
}

value_t copy_vector(value_t src)
{
	assert(vectorp(src) || symbolp(src));
	assert(vsize(src) <= vallocsize(src));
	push_root(&src);
	value_t r = make_vector(vsize(src));
	push_root(&r);

	if(!errp(r))
	{
		for(int i = 0; i < vsize(src); i++)
		{
			rplacv(r, i, vref(src, i));
		}
		r.type.main = src.type.main;
	}
	pop_root(2);

	return r;
}

value_t vconc(value_t x, value_t y)
{
	assert(vectorp(x));
	assert(vectorp(y));
	assert(vsize(x) <= vallocsize(x));
	assert(vsize(y) <= vallocsize(y));

	push_root(&x);
	push_root(&y);

	int sx = vsize(x);
	int sy = vsize(y);

	value_t r = make_vector(sx + sy);
	push_root(&r);

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

	pop_root(3);
	return r;
}

value_t vnconc(value_t x, value_t y)
{
	assert(vectorp(x));
	assert(vectorp(y));
	assert(vsize(x) <= vallocsize(x));
	assert(vsize(y) <= vallocsize(y));
	push_root(&x);
	push_root(&y);

	// copy y
	int sy = vsize(y);
	for(int i = 0; i < sy; i++)
	{
		vpush(vref(y, i), x);
	}

	pop_root(2);
	return x;
}

value_t make_vector_from_list(value_t x)
{
	assert(is_cons_pair_or_nil(x));

	push_root(&x);
	value_t r = make_vector(0);
	push_root(&r);

	for(; !nilp(x); x = cdr(x))
	{
		vpush(car(x), r);
	}

	pop_root(2);
	return r;
}

value_t make_list_from_vector(value_t x)
{
	assert(vectorp(x));
	assert(vsize(x) <= vallocsize(x));

	push_root(&x);
	value_t r    = cons(NIL, NIL);
	value_t cur  = r;
	push_root(&r);
	push_root(&cur);

	for(int i = 0; i < vsize(x); i++)
	{
		CONS_AND_CDR(vref(x, i), cur);
	}

	pop_root(3);
	return cdr(r);
}


/////////////////////////////////////////////////////////////////////
// public: bridge functions from C to LISP

value_t str_to_cons	(const char* s)
{
	assert(s != NULL);
	value_t    r = cons(NIL, NIL);
	value_t  cur = r;
	push_root(&r);
	push_root(&cur);

	int c;
	while((c = *s++) != '\0')
	{
		CONS_AND_CDR(RINT(c), cur)
	}

	pop_root(2);

	return cdr(r);
}

value_t str_to_vec	(const char* s)
{
	assert(s != NULL);
	value_t    r = make_vector(MAX(strlen(s), 1));

	int c;
	while((c = *s++) != '\0')
	{
		value_t result = vpush(RCHAR(c), r);
		if(errp(result)) return result;
	}

	return r;
}

value_t mbstr_to_vec	(const char* s)
{
	assert(s != NULL);
	int len = mbstowcs(NULL, s, 0) + 1;
	wchar_t* wc = (wchar_t*)malloc(sizeof(wchar_t) * len);
	if(!wc) return rerr_alloc();

	if(mbstowcs(wc, s, len) == len - 1)
	{
		value_t    r = make_vector(MAX(wcslen(wc), 1));

		for(wchar_t* wcp = wc; *wcp != '\0'; wcp++)
		{
			value_t result = vpush(RCHAR(*wcp), r);
			if(errp(result))
			{
				free(wc);
				return result;
			}

		}

		free(wc);
		return r;
	}
	else
	{
		free(wc);
		return RERR(ERR_WCHAR, NIL);
	}
}

value_t wcstr_to_vec	(const wchar_t* s)
{
	assert(s != NULL);
	value_t r = make_vector(MAX(wcslen(s), 1));

	int c;
	while((c = *s++) != '\0')
	{
		value_t result = vpush(RCHAR(c), r);
		if(errp(result)) return result;
	}

	return r;
}

value_t str_to_rstr	(const char* s)
{
	assert(s != NULL);
	value_t r = str_to_vec(s);
	if(!errp(r))
	{
		if(vsize(r) == 0)
		{	// str_to_vec generates vector that size is more than 1.
			vpush(RCHAR('\0'), r);
		}
	}

	return r;
}

value_t mbstr_to_rstr	(const char* s)
{
	assert(s != NULL);
	value_t r = mbstr_to_vec(s);
	if(!errp(r))
	{
		if(vsize(r) == 0)
		{	// mbstr_to_vec generates vector that size is more than 1.
			vpush(RCHAR('\0'), r);
		}
	}

	return r;
}

value_t wcstr_to_rstr	(const wchar_t* s)
{
	assert(s != NULL);
	value_t r = wcstr_to_vec(s);
	if(!errp(r))
	{
		if(vsize(r) == 0)
		{	// wcstr_to_vec generates vector that size is more than 1.
			vpush(RCHAR('\0'), r);
		}
	}

	return r;
}

value_t make_symbol	(const char* s)
{
	value_t r = str_to_rstr(s);
	if(!errp(r))
	{
		r.type.main = SYM_T;

	}
	return r;
}

value_t intern	(const char* s, value_t package)
{
	value_t r = make_symbol(s);
	return errp(r) ? r : register_sym(r, package);
}

char*   rstr_to_str	(value_t s)
{
	assert(vectorp(s));

	char* buf = (char*)malloc(vsize(s) + 1);
	if(buf)
	{
		char *p = buf;
		for(int i = 0; i < vsize(s); i++)
		{
			value_t c = vref(s, i);
			assert(charp(c));
			*p++ = INTOF(c);
		}
		*p = '\0';
	}

	return buf;
}

/////////////////////////////////////////////////////////////////////
// public: symbol functions

value_t gensym	(value_t env)
{
	push_root(&env);
	value_t r = str_to_rstr("#:G");
	push_root(&r);
	value_t n = get_env_ref(g_gensym_counter, env);
	push_root(&n);
	n         = get_env_value_ref(n, env);
	r         = vnconc(r, pr_str(n, NIL, false));
	r.type.main = SYM_T;

	set_env(g_gensym_counter, RINT(INTOF(n) + 1), env);

	pop_root(3);
	return r;	// gensym symbol is unregisterd: so same name doesn't cause eq.
}

/////////////////////////////////////////////////////////////////////
// public: support functions writing LISP on C
#if 0
value_t* nconc_and_last(value_t v, value_t* c)
{
	*c = v;
	return &AVALUE(last(v)).cons->cdr;
}
#endif

bool is_str(value_t v)
{
#ifdef STR_EASY_CHECK
	return vectorp(v) && vsize(v) > 0 && charp(vref(v, 0));
#else  // STR_EASY_CHECK
	if(!vectorp(v)) return false;

	for(int i = 0; i < vsize(v); i ++)
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
