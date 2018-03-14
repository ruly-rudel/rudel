#include <assert.h>
#include "builtin.h"
#include "env.h"


/////////////////////////////////////////////////////////////////////
// private: support functions

static value_t make_bind(value_t key, value_t val, value_t alist)
{
	assert(rtypeof(key)   == CONS_T);
	assert(rtypeof(val)   == CONS_T);
	assert(rtypeof(alist) == CONS_T);

	value_t key_car = car(key);
	value_t val_car = car(val);

	if(nilp(key))
	{
		return alist;
	}
	else if(equal(key_car, str_to_sym("&")))	// rest parameter
	{
		key_car = car(cdr(key));
		return acons(key_car, val, alist);
	}
	else
	{
		return make_bind(cdr(key), cdr(val), acons(key_car, val_car, alist));
	}
}


/////////////////////////////////////////////////////////////////////
// public: Environment create, search and modify functions

value_t	create_env	(value_t key, value_t val, value_t outer)
{
	assert(rtypeof(outer) == CONS_T);
	assert(rtypeof(key)   == CONS_T);
	assert(rtypeof(val)   == CONS_T);

	return cons(make_bind(key, val, NIL), outer);
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
		return assoc(key, car(env));
	}
}

// search whole environment
value_t	get_env_value	(value_t key, value_t env)
{
	assert(rtypeof(env) == CONS_T);
	assert(rtypeof(key) == SYM_T);

	if(nilp(env))
	{
		return RERR(ERR_NOTFOUND);
	}
	else
	{
		value_t s = assoc(key, car(env));
		if(nilp(s))
		{
			return get_env_value(key, cdr(env));
		}
		else
		{
			return cdr(s);
		}
	}
}

// End of File
/////////////////////////////////////////////////////////////////////
