#include <assert.h>
#include "builtin.h"
#include "package.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: symbol pool

//static value_t	s_symbol_pool		= NIL;

/////////////////////////////////////////////////////////////////////
// private: support functions

/////////////////////////////////////////////////////////////////////
// public: package and symbol

#if 0
void clear_symtbl(void)
{
	s_symbol_pool = NIL;
}
#endif

value_t register_sym(value_t s, value_t pkg)
{
	assert(symbolp(s));
	assert(consp(pkg));

	value_t sym_list = UNSAFE_CDR(UNSAFE_CDR(pkg));
	value_t sym = find(s, sym_list, veq);
	if(nilp(sym))
	{
		push_root(&pkg);
		push_root(&s);
		push_root(&sym_list);

		value_t snew;
		snew.cons      = alloc_cons();
		snew.cons->car = s;
		snew.cons->cdr = sym_list;
		snew.type.main = CONS_T;

		rplacd(UNSAFE_CDR(pkg), snew);

		pop_root(3);
		return s;
	}
	else
	{
		return sym;
	}
}

value_t	create_package	(value_t name)
{
	assert(is_str(name));
	push_root(&name);
	
	value_t r = cons(name, cons(NIL, NIL));
	push_root(&r);
	g_package_list = cons(r, g_package_list);
	pop_root(2);
	return r;
}

value_t in_package	(value_t name)
{
	assert(is_str(name));

	for(value_t pkg = g_package_list; !nilp(pkg); pkg = UNSAFE_CDR(pkg))
	{
		assert(consp(pkg));
		if(equal(UNSAFE_CAR(pkg), name))
		{
			return pkg;
		}
	}
	return NIL;
}

// End of File
/////////////////////////////////////////////////////////////////////
