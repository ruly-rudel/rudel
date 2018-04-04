#include <assert.h>
#include "builtin.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: support functions


/////////////////////////////////////////////////////////////////////
// public: memory allocator

void init_allocator(void)
{
	g_memory_pool = (value_t*)malloc(sizeof(value_t) * INITIAL_ALLOC_SIZE);
	g_memory_top  = g_memory_pool;
	g_memory_max  = g_memory_pool + INITIAL_ALLOC_SIZE;
}

cons_t* alloc_cons(void)
{
	cons_t* c = (cons_t*)g_memory_top;
	g_memory_top += 2;

#ifdef DUMP_ALLOC_ADDR
#if __WORDSIZE == 32
	fprintf(stderr, "cons: %08x\n", (int)c);
#else
	fprintf(stderr, "cons: %016lx\n", (int64_t)c);
#endif
#endif	// DUMP_ALLOC_ADDR

	return g_memory_top >= g_memory_max ? 0 : c;
}

vector_t* alloc_vector()
{
	vector_t* v = (vector_t*)g_memory_top;
	g_memory_top += 4;

#ifdef DUMP_ALLOC_ADDR
#if __WORDSIZE == 32
	fprintf(stderr, "vector: %08x, size %d\n", (int)v, sizeof(vector_t));
#else
	fprintf(stderr, "vector: %016lx, size %ld\n", (int64_t)v, sizeof(vector_t));
#endif
#endif	// DUMP_ALLOC_ADDR

	return g_memory_top >= g_memory_max ? 0 : v;
}

value_t* alloc_vector_data(value_t v, size_t size)
{
	assert(vectorp(v));
	v.type.main = CONS_T;

	size = size + (size % 2);	// align

	if(g_memory_top + size >= g_memory_max)
	{
		return 0;
	}
	else
	{
		if(v.vector->data)
		{
			for(int i = 0; i < v.vector->size; i++)
				g_memory_top[i] = v.vector->data[i];
		}

		v.vector->data  = g_memory_top;
		v.vector->alloc = size;
		g_memory_top += size;

		return v.vector->data;
	}
}

// End of File
/////////////////////////////////////////////////////////////////////
