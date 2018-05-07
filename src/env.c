#include <assert.h>
#include "builtin.h"
#include "env.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: support functions

static value_t vassoceq(value_t key, value_t avector)
{
	assert(symbolp(key));
	assert(vectorp(avector));

	for(int i = 0; i < vsize(avector); i++)
	{
		value_t pair = vref(avector, i);
		assert(rtypeof(pair) == CONS_T);
		if(!nilp(pair) && EQ(UNSAFE_CAR(pair), key))
		{
			return pair;
		}
	}
	return NIL;
}

// search only current environment
static value_t	find_env	(value_t key, value_t env)
{
	assert(consp(env));
	assert(symbolp(key));

	if(nilp(env))
	{
		return NIL;
	}
	else
	{
		return vassoceq(key, car(env));
	}
}


/////////////////////////////////////////////////////////////////////
// public: Environment create, search and modify functions

value_t	create_lambda_env	(value_t key, value_t outer)
{
	push_root(&key);
	push_root(&outer);
	value_t alist = make_vector(0);
	push_root(&alist);
	value_t key_car = NIL;
	push_root(&key_car);
	value_t r = cons(alist, outer);
	push_root(&r);

	int st = 0;
	int i = 0;
	for(; !nilp(key); key = UNSAFE_CDR(key))
	{
		assert(consp(key));

		key_car = UNSAFE_CAR(key);
		switch(st)
		{
		case 0:	// normal parameter
			if(EQ(key_car, RSPECIAL(SP_OPTIONAL)))	// optional parameter
			{
				st = 1;
			}
			else if(EQ(key_car, RSPECIAL(SP_REST)))	// rest parameter
			{
				st = 2;
			}
			else if(EQ(key_car, RSPECIAL(SP_KEY)))	// keyword parameter
			{
				st = 3;
			}
			else
			{
				if(!symbolp(key_car)) goto cleanup;
				rplacv(alist, i++, cons(key_car, NIL));
			}
			break;

		case 1:	// optional parameter
			if(EQ(key_car, RSPECIAL(SP_OPTIONAL)))	// optional parameter
			{
				r = RERR(ERR_ARG, key);
				goto cleanup;
			}
			else if(EQ(key_car, RSPECIAL(SP_REST)))	// rest parameter
			{
				st = 2;
			}
			else if(EQ(key_car, RSPECIAL(SP_KEY)))	// keyword parameter
			{
				st = 3;
			}
			else
			{
				if(!symbolp(key_car)) goto cleanup;
				rplacv(alist, i++, cons(car(key_car), NIL));
			}
			break;

		case 2:	// rest parameter
			if(!symbolp(key_car)) goto cleanup;
			rplacv(alist, i++, cons(key_car, NIL));
			goto cleanup;

		case 3:	// keyword parameter
			rplacv(alist, i++, cons(car(cdr(car(key_car))), NIL));
			break;
		}

#if 0
		assert(consp(key));

		value_t key_car = UNSAFE_CAR(key);

		if(EQ(key_car, RSPECIAL(SP_REST)))	// rest parameter
		{
			key_car = second(key);
			rplacv(alist, i, cons(key_car, NIL));
			break;
		}
		else
		{
			assert(symbolp(key_car));
			rplacv(alist, i, cons(key_car, NIL));
		}
#endif
	}

cleanup:
	pop_root(5);
	return r;
}

// search only current environment, set only on current environment
value_t	set_env		(value_t key, value_t val, value_t env)
{
	assert(consp(env));
	assert(symbolp(key));
	push_root(&key);
	push_root(&val);
	push_root(&env);

	value_t r = find_env(key, env);
	push_root(&r);
	if(nilp(r))
	{
		r = cons(key, val);
		value_t alist = UNSAFE_CAR(env);
		assert(vectorp(alist));
		rplacv(alist, vsize(alist), r);
	}
	else
	{
		rplacd(r, val);
	}

	pop_root(4);
	return r;
}

// search whole environment
value_t	find_env_all	(value_t key, value_t env)
{
	assert(consp(env));
	assert(symbolp(key) || rtypeof(key) == REF_T);

	if(rtypeof(key) == SYM_T)
	{
		for(; !nilp(env); env = UNSAFE_CDR(env))
		{
			assert(rtypeof(env) == CONS_T);
			value_t s = vassoceq(key, UNSAFE_CAR(env));
			if(!nilp(s))
			{
				return s;
			}
		}

		return NIL;
	}
	else	// REF_T
	{
		for(int d = REF_D(key); d > 0; d--, env = UNSAFE_CDR(env))
			assert(consp(env));

		env = UNSAFE_CAR(env);

		return vref(env, REF_W(key));
	}
}

// search whole environment
value_t	get_env_value	(value_t key, value_t env)
{
	assert(consp(env));
	assert(symbolp(key));

	for(; !nilp(env); env = UNSAFE_CDR(env))
	{
		assert(consp(env));
		value_t alist = car(env);
		assert(vectorp(alist));

		for(int width = 0; width < vsize(alist); width++)
		{
			value_t pair = vref(alist, width);
			assert(consp(pair));

			if(UNSAFE_CAR(pair).raw == key.raw)
			{
				return UNSAFE_CDR(pair);
			}
		}
	}

	push_root(&key);
	value_t r = str_to_rstr("variable ");
	push_root(&r);
	r = vnconc(r, symbol_string(key));
	r = vnconc(r, str_to_rstr(" is not found."));
	pop_root(2);

	return rerr(r, NIL);
}

value_t	get_env_pkg	(value_t env)
{
	assert(consp(env));
	env = last(env);
	assert(consp(env));
	value_t alist = car(env);
	assert(vectorp(alist));
	push_root(&alist);

	value_t key = str_to_vec("*package*");

	for(int width = 0; width < vsize(alist); width++)
	{
		value_t pair = vref(alist, width);
		assert(consp(pair));

		if(veq(UNSAFE_CAR(UNSAFE_CAR(pair)), key))
		{
			pop_root(1);
			return UNSAFE_CDR(pair);
		}
	}

	value_t r = str_to_rstr("cannot find *package*.");
	pop_root(1);

	return rerr(r, NIL);
}

// search whole environment
value_t	get_env_ref	(value_t key, value_t env)
{
	assert(consp(env));
	assert(symbolp(key));

	for(int depth = 0; !nilp(env); depth++, env = UNSAFE_CDR(env))
	{
		assert(consp(env));
		value_t alist = car(env);
		assert(vectorp(alist));

		for(int width = 0; width < vsize(alist); width++)
		{
			value_t pair = vref(alist, width);
			assert(consp(pair));

			if(UNSAFE_CAR(pair).raw == key.raw)
			{
				return RREF(depth, width);
			}
		}
	}

	push_root(&key);
	value_t r = str_to_rstr("variable ");
	push_root(&r);
	r = vnconc(r, symbol_string(key));
	r = vnconc(r, str_to_rstr(" is not found."));
	pop_root(2);

	return rerr(r, NIL);
}

value_t	get_env_value_ref(value_t ref, value_t env)
{
	assert(consp(env));
	assert(rtypeof(ref) == REF_T);

	for(int d = REF_D(ref); d > 0; d--, env = UNSAFE_CDR(env))
		assert(consp(env));

	assert(consp(env));
	env = UNSAFE_CAR(env);

	return UNSAFE_CDR(vref(env, REF_W(ref)));
}

value_t	create_root_env	(value_t pkg)
{
	push_root(&pkg);
	value_t env = cons(make_vector(7), NIL);
	push_root(&env);

	set_env(g_nil,			NIL, 		env);
	set_env(g_t,			g_t,		env);
	set_env(g_package,		pkg,		env);
	set_env(g_gensym_counter,	RINT(0),	env);
	set_env(g_exception_stack,	NIL, 		env);
	set_env(g_debug,		NIL, 		env);
	set_env(g_trace,		NIL, 		env);

	pop_root(2);
	return env;
}

#if 0
value_t	init_root_env	(value_t pkg)
{
	assert(consp(pkg));

	value_t pkg_val = UNSAFE_CDR(pkg);
	assert(consp(pkg_val));
	push_root(&pkg_val);

	value_t key = list(6, g_nil, g_t, g_gensym_counter, g_exception_stack, g_debug, g_trace);
	push_root(&key);

	value_t val = list(6, NIL,   g_t, RINT(0),          NIL,               NIL,     NIL);
	rplaca(pkg_val, create_env(key, val, NIL));
	pop_root(2);

	return pkg;
}
#endif

// End of File
/////////////////////////////////////////////////////////////////////
