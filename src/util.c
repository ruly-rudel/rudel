#include <assert.h>
#include "builtin.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////
// public: utility functions

value_t search_alist(value_t list, value_t key)
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

			if(equal(car(vcar), key))
				return vcar;
		}

		return NIL;
	}
}

// End of File
/////////////////////////////////////////////////////////////////////
