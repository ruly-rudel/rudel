#include <assert.h>
#include "builtin.h"
#include "env.h"


/////////////////////////////////////////////////////////////////////
// private: support functions


/////////////////////////////////////////////////////////////////////
// public: Environment create, search and modify functions

value_t	create_env	(value_t key, value_t val, value_t outer)
{
	value_t alist = make_vector(0);
	int i = 0;

	for(; !nilp(key); key = UNSAFE_CDR(key), val = cdr(val))
	{
		assert(consp(key));
		assert(rtypeof(val) == CONS_T);	// nil is acceptable

		value_t key_car = UNSAFE_CAR(key);
		value_t val_car = car(val);

		if(EQ(key_car, RSPECIAL(SP_AMP)))	// rest parameter
		{
			key_car = car(cdr(key));
			rplacv(alist, i++, cons(key_car, val));		// for resolv order.
			break;
		}
		else
		{
			assert(symbolp(key_car));
			rplacv(alist, i++, cons(key_car, val_car));		// for resolv order.
		}
	}

	return cons(alist, outer);
}

// search only current environment, set only on current environment
value_t	set_env		(value_t key, value_t val, value_t env)
{
	assert(consp(env));
	assert(symbolp(key));

	value_t r = find_env(key, env);
	if(nilp(r))
	{
		r = cons(key, val);
		if(nilp(UNSAFE_CAR(env)))
		{
			value_t alist = make_vector(1);
			rplacv(alist, 0, r);
			rplaca(env, alist);
		}
		else
		{
			value_t alist = UNSAFE_CAR(env);
			assert(vectorp(alist));
			rplacv(alist, INTOF(vsize(alist)), r);
		}
	}
	else
	{
		rplacd(r, val);
	}

	return r;
}

static value_t vassoceq(value_t key, value_t avector)
{
	assert(symbolp(key));
	assert(vectorp(avector));

	for(int i = 0; i < INTOF(vsize(avector)); i++)
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
value_t	find_env	(value_t key, value_t env)
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
		value_t s = vassoceq(key, UNSAFE_CAR(env));
		if(!nilp(s))
		{
			return UNSAFE_CDR(s);
		}
	}

	value_t r = str_to_rstr("variable ");
	r = nconc(r, copy_list(key));
	r = nconc(r, str_to_rstr(" is not found."));

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

		for(int width = 0; width < INTOF(vsize(alist)); width++)
		{
			value_t pair = vref(alist, width);
			assert(consp(pair));

			if(UNSAFE_CAR(pair).raw == key.raw)
			{
				return RREF(depth, width);
			}
		}
	}

	value_t r = str_to_rstr("variable ");
	r = nconc(r, copy_list(key));
	r = nconc(r, str_to_rstr(" is not found."));

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

// End of File
/////////////////////////////////////////////////////////////////////
