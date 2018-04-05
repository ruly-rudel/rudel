#include <assert.h>
#include "builtin.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: support functions

typedef struct
{
	value_t*	data;
	int*		size;
} root_t;

static root_t s_root[ROOT_SIZE]   = { 0 };
static int s_root_ptr             =   0;
static int s_lock_cnt             =   0;

static value_t* exec_gc(void)
{
#ifdef NOGC
	return 0;
#else  // NOGC

	// swap buffer
	value_t* tmp       = g_memory_pool;
	g_memory_pool      = g_memory_top = g_memory_pool_back;
	g_memory_max       = g_memory_top + INITIAL_ALLOC_SIZE;
	g_memory_pool_back = tmp;

	return g_memory_pool;
#endif // NOGC
}

/////////////////////////////////////////////////////////////////////
// public: memory allocator

void init_allocator(void)
{
	g_memory_pool      = (value_t*)malloc(sizeof(value_t) * INITIAL_ALLOC_SIZE);
#ifndef NOGC
	g_memory_pool_back = (value_t*)malloc(sizeof(value_t) * INITIAL_ALLOC_SIZE);
#endif  // NOGC
	g_memory_top       = g_memory_pool;
	g_memory_max       = g_memory_pool + INITIAL_ALLOC_SIZE;
}

void push_root(value_t* v)
{
	s_root[s_root_ptr  ].data = v;
	s_root[s_root_ptr++].size = 0;
	assert(s_root_ptr < ROOT_SIZE);
	return;
}

void push_root_raw_vec(value_t *v, int* sp)
{
	s_root[s_root_ptr  ].data = v;
	s_root[s_root_ptr++].size = sp;
	assert(s_root_ptr < ROOT_SIZE);
	return;
}

void pop_root(void)
{
	s_root[s_root_ptr  ].data = 0;
	s_root[s_root_ptr--].size = 0;
	return;
}

void lock_gc(void)
{
	s_lock_cnt++;
	assert(s_lock_cnt >= 0);
}

void unlock_gc(void)
{
	s_lock_cnt--;
	assert(s_lock_cnt >= 0);
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

	if(g_memory_top >= g_memory_max)
	{
		if(exec_gc())
		{
			c = (cons_t*)g_memory_top;
			g_memory_top += 2;
			return c;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return c;
	}
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

	if(g_memory_top >= g_memory_max)
	{
		if(exec_gc())
		{
			v = (vector_t*)g_memory_top;
			g_memory_top += 4;
			return v;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return v;
	}
}

value_t* alloc_vector_data(value_t v, size_t size)
{
	assert(vectorp(v));
	v.type.main = CONS_T;

	size = size + (size % 2);	// align

	if(g_memory_top + size >= g_memory_max)
	{
		if(!exec_gc())
		{
			return 0;
		}
	}

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

// End of File
/////////////////////////////////////////////////////////////////////
