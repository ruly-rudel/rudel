#include <assert.h>
#include "builtin.h"
#include "package.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: support functions

static bool sym_name_eq(value_t x, value_t y)
{
	assert(symbolp(x));
	assert(symbolp(y));

	return veq(car(x), car(y));
}

/////////////////////////////////////////////////////////////////////
// public: package and symbol

value_t register_sym(value_t s, value_t pkg)
{
	assert(symbolp(s));
	assert(consp(pkg));

	value_t sym_list = UNSAFE_CDR(pkg);
	value_t sym = find(s, sym_list, sym_name_eq);
	if(nilp(sym))
	{
		rplaca(UNSAFE_CDR(s), pkg);	// set symbol package
		push_root(&pkg);
		push_root(&s);
		push_root(&sym_list);

		value_t snew;
		snew.cons      = alloc_cons();
		snew.cons->car = s;
		snew.cons->cdr = sym_list;
		snew.type.main = CONS_T;

		rplacd(pkg, snew);

		pop_root(3);
		return s;
	}
	else
	{
		return sym;
	}
}

value_t	init_package_list	(void)
{
	g_package_list = NIL;
	value_t key = make_package(NIL, NIL);
	value_t usr = make_package(intern("user", key), NIL);

	return usr;
}

value_t	make_package	(value_t name, value_t use)
{
	assert(symbolp(name) || nilp(name));
	assert(symbolp(use)  || nilp(use));
	push_root(&name);

	value_t r = cons(name, nilp(use) ? NIL : copy_list(cdr(find_package(use))));
	push_root(&r);
	g_package_list = cons(r, g_package_list);
	pop_root(2);
	return r;
}

value_t find_package	(value_t name)
{
	assert(symbolp(name) || nilp(name));

	for(value_t pkg = g_package_list; !nilp(pkg); pkg = UNSAFE_CDR(pkg))
	{
		assert(consp(pkg));
		if(EQ(UNSAFE_CAR(UNSAFE_CAR(pkg)), name))
		{
			return UNSAFE_CAR(pkg);
		}
	}
	return NIL;
}

// End of File
/////////////////////////////////////////////////////////////////////
