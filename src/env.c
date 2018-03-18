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
		r = rplaca(env, acons(key, val, car(env)));
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
		return assoc(key, car(env), equal);
	}
}

// search whole environment
value_t	find_env_all	(value_t key, value_t env)
{
	assert(rtypeof(key) == SYM_T);

	while(!nilp(env))
	{
		assert(rtypeof(env) == CONS_T);
		value_t s = assoc(key, car(env), equal);
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
		value_t s = assoc(key, car(env), equal);
		if(!nilp(s))
		{
			return cdr(s);
		}
		env = cdr(env);
	}

	return RERR(ERR_NOTFOUND);
}

// End of File
/////////////////////////////////////////////////////////////////////
