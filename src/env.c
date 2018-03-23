#include <assert.h>
#include "builtin.h"
#include "env.h"


/////////////////////////////////////////////////////////////////////
// private: support functions


/////////////////////////////////////////////////////////////////////
// public: Environment create, search and modify functions

value_t	create_env	(value_t key, value_t val, value_t outer)
{
	value_t amp = str_to_sym("&");
	value_t alist = NIL;

	while(!nilp(key))
	{
		assert(rtypeof(key)   == CONS_T);
		assert(rtypeof(val)   == CONS_T);

		value_t key_car = car(key);
		value_t val_car = car(val);

		if(equal(key_car, amp))	// rest parameter
		{
			key_car = car(cdr(key));
			alist = acons(key_car, val, alist);
			break;
		}
		else
		{
			alist = acons(key_car, val_car, alist);
		}
		key = cdr(key);
		val = cdr(val);
	}

	return cons(alist, outer);
}

// search only current environment, set only on current environment
value_t	set_env		(value_t key, value_t val, value_t env)
{
	assert(rtypeof(env) == CONS_T);
	assert(rtypeof(key) == SYM_T);

	value_t r = find_env(key, env);
	if(nilp(r))
	{
		//r = rplaca(env, acons(key, val, car(env)));
		r = cons(key, val);
		if(nilp(car(env)))
		{
			rplaca(env, list(1, r));
		}
		else
		{
			rplacd(last(car(env)), cons(r, NIL));
		}
	}
	else
	{
		rplacd(r, val);
	}

	return r;
}

// search only current environment
value_t	find_env	(value_t key, value_t env)
{
	assert(rtypeof(env) == CONS_T);
	assert(rtypeof(key) == SYM_T);

	if(nilp(env))
	{
		return NIL;
	}
	else
	{
		return assoceq(key, car(env));
	}
}

// search whole environment
value_t	find_env_all	(value_t key, value_t env)
{
	assert(rtypeof(key) == SYM_T);

	while(!nilp(env))
	{
		assert(rtypeof(env) == CONS_T);
		value_t s = assoceq(key, car(env));
		if(!nilp(s))
		{
			return s;
		}
		env = cdr(env);
	}

	return NIL;
}

// search whole environment
value_t	get_env_value	(value_t key, value_t env)
{
	assert(rtypeof(key) == SYM_T);

	while(!nilp(env))
	{
		assert(rtypeof(env) == CONS_T);
		value_t s = assoceq(key, car(env));
		if(!nilp(s))
		{
			return cdr(s);
		}
		env = cdr(env);
	}

	return RERR(ERR_NOTFOUND, NIL);
}

// search whole environment
value_t	get_env_ref	(value_t key, value_t env)
{
	assert(rtypeof(key) == SYM_T);

	int depth = 0;
	while(!nilp(env))
	{
		assert(rtypeof(env) == CONS_T);
		int width = 0;
		for(value_t v = car(env); !nilp(v); v = cdr(v))
		{
			value_t vcar = car(v);
			assert(rtypeof(vcar) == CONS_T);

			if(car(vcar).raw == key.raw)
			{
				return RREF(depth, width);
			}
			width++;
		}
		env = cdr(env);
		depth++;
	}

	return RERR(ERR_NOTFOUND, NIL);
}

value_t	get_env_value_ref(value_t ref, value_t env)
{
	assert(rtypeof(ref) == REF_T);

	int d = REF_D(ref);
	while(d--)
	{
		env = cdr(env);
	}
	env = car(env);

	int w = REF_W(ref);
	while(w--)
	{
		env = cdr(env);
	}

	return cdr(car(env));
}

// End of File
/////////////////////////////////////////////////////////////////////
