#include <assert.h>
#include "builtin.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: memory and symbol pool


typedef struct
{
	value_t*	data;
	int*		size;
} root_t;

static value_t	s_symbol_pool		= NIL;
static root_t	s_root[ROOT_SIZE]	= { 0 };
static int	s_root_ptr		=   0;
static int	s_lock_cnt		=   0;

/////////////////////////////////////////////////////////////////////
// private: support functions

#ifndef NDEBUG
inline static bool is_from(value_t v)
{
	return ((value_t*)v.cons >= g_memory_pool_from && (value_t*)v.cons < g_memory_pool_from + INITIAL_ALLOC_SIZE);
}

inline static bool is_to(value_t v)
{
	return ((value_t*)v.cons >= g_memory_pool && (value_t*)v.cons < g_memory_max);
}
#endif // NDEBUG

inline static value_t* copy1(value_t* v)
{
	rtype_t type = rtypeof(*v);
	value_t cur  = *v;
	cur.type.main = CONS_T;
	value_t alloc;

	switch(type)
	{
		case CONS_T:
		case ERR_T:
		case CLOJ_T:
		case MACRO_T:
			if(nilp(cur)) return 0;	// nil is not cons cell

			if(gcp(cur.cons->car))	// target cons is already copied
			{
				// replace value itself to copyed to-space address
				alloc = cur.cons->car;
				assert(is_to(alloc));
				alloc.type.main = type;
				*v = alloc;
			}
			else
			{
				assert(is_from(cur));
				// allocate memory and copy car/cdr of current cons in from-space to to-space
				alloc.cons      = (cons_t*)g_memory_top;
				*g_memory_top++ = UNSAFE_CAR(cur);
				*g_memory_top++ = UNSAFE_CDR(cur);

				// write to-space address to car of current cons in from-space
				alloc.type.main = GC_T;
				cur.cons->car   = alloc;

				// replace value itself to copyed to-space address
				alloc.type.main = type;
				*v = alloc;

				// invoke copy1 to car/cdr of current cons
				alloc.type.main = CONS_T;
				copy1(&alloc.cons->car);
				copy1(&alloc.cons->cdr);
			}
			return 0;

		case VEC_T:
		case SYM_T:
			//if(nilp(cur)) return 0;	//****** nil is not vector(but it is not ocuured?)

			if(gcp(cur.vector->type))	// target vector is already copied
			{
				// replace value itself to copyed to-space address
				alloc = cur.vector->type;
				assert(is_to(alloc));
				alloc.type.main = type;
				*v = alloc;
			}
			else
			{
				assert(is_from(cur));
				// allocate memory and copy vector in from-space to to-space
				alloc.vector        = (vector_t*)g_memory_top;
				g_memory_top       += 4;
				alloc.vector->size  = cur.vector->size;
				alloc.vector->alloc = cur.vector->alloc;
				alloc.vector->type  = cur.vector->type;
				alloc.vector->data  = g_memory_top;
				for(int i = 0; i < cur.vector->alloc; i++)
					*g_memory_top++ = cur.vector->data[i];

				// write to-space address to car of current cons in from-space
				alloc.type.main     = GC_T;
				cur.vector->type    = alloc;

				// replace value itself to copyed to-space address
				alloc.type.main = type;
				*v = alloc;

				// invoke copy1 to current vector data
				alloc.type.main = CONS_T;
				for(int i = 0; i < alloc.vector->size; i++)
					copy1(alloc.vector->data + i);
			}
			return 0;

		case GC_T:
			abort();

		default:
			break;
	}

	return v + 1;
}

static value_t* exec_gc(void)
{
#ifdef TRACE_VM
	fprintf(stderr, "Executing GC...\n");
#endif
#ifdef NOGC
	return 0;
#else  // NOGC

	// swap buffer
	value_t* tmp       = g_memory_pool;
	g_memory_pool      = g_memory_top = g_memory_pool_from;
	g_memory_max       = g_memory_top + INITIAL_ALLOC_SIZE;
	g_memory_gc        = g_memory_top + INITIAL_ALLOC_SIZE / 2;
	g_memory_pool_from = tmp;

	// copy root to memory pool
	for(int i = 0; i < s_root_ptr; i++)
	{
		if(s_root[i].size)
		{
			for(int j = 0; j <= *s_root[i].size; j++)
			{
				copy1(s_root[i].data + j);
			}
		}
		else
		{
			copy1(s_root[i].data);
		}
	}

	// copy symbol pool to memory pool
	copy1(&s_symbol_pool);
#ifdef TRACE_VM
	fprintf(stderr, " Replacing symbols...\n");
#endif
	init_global();

#ifdef TRACE_VM
	fprintf(stderr, "Executing GC Done.\n");
#endif
	return g_memory_top;
#endif // NOGC
}

/////////////////////////////////////////////////////////////////////
// public: memory allocator

void init_allocator(void)
{
	g_memory_pool      = (value_t*)malloc(sizeof(value_t) * INITIAL_ALLOC_SIZE);
#ifndef NOGC
	g_memory_pool_from = (value_t*)malloc(sizeof(value_t) * INITIAL_ALLOC_SIZE);
#endif  // NOGC
	g_memory_top       = g_memory_pool;
	g_memory_max       = g_memory_pool + INITIAL_ALLOC_SIZE;
	g_memory_gc        = g_memory_pool + INITIAL_ALLOC_SIZE / 2;
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

	if(g_memory_top >= g_memory_gc && s_lock_cnt == 0)
	{
		if(exec_gc())
		{
			c = (cons_t*)g_memory_top;
			g_memory_top += 2;
			if(g_memory_top >= g_memory_max)
			{
				return 0;
			}
			else
			{
				return c;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if(g_memory_top >= g_memory_max)
		{
			return 0;
		}
		else
		{
			return c;
		}
	}
}

vector_t* alloc_vector()
{
	vector_t* v = (vector_t*)g_memory_top;
	g_memory_top += 4;

#ifdef DUMP_ALLOC_ADDR
#if __WORDSIZE == 32
	fprintf(stderr, "vect: %08x\n", (int)v);
#else
	fprintf(stderr, "vect: %016lx\n", (int64_t)v);
#endif
#endif	// DUMP_ALLOC_ADDR

	if(g_memory_top >= g_memory_gc && s_lock_cnt == 0)
	{
		if(exec_gc())
		{
			v = (vector_t*)g_memory_top;
			g_memory_top += 4;
			if(g_memory_top >= g_memory_max)
			{
				return 0;
			}
			else
			{
				return v;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if(g_memory_top >= g_memory_max)
		{
			return 0;
		}
		else
		{
			return v;
		}
	}
}

value_t* alloc_vector_data(value_t v, size_t size)
{
	assert(vectorp(v));
	v.type.main = CONS_T;

	size = size + (size % 2);	// align

	if(g_memory_top + size >= g_memory_gc && s_lock_cnt == 0)
	{
		if(!exec_gc())
		{
			return 0;
		}
		else if(g_memory_top + size >= g_memory_max)
		{
			return 0;
		}
	}
	else if(g_memory_top + size >= g_memory_max)
	{
		return 0;
	}

	if(v.vector->data)
	{
		for(int i = 0; i < v.vector->size; i++)
			g_memory_top[i] = v.vector->data[i];
	}

	v.vector->data  = g_memory_top;
	v.vector->alloc = size;
	g_memory_top += size;

#ifdef DUMP_ALLOC_ADDR
#if __WORDSIZE == 32
	fprintf(stderr, "vdat: %08x, size %d\n", (int)v.vector->data, size);
#else
	fprintf(stderr, "vdat: %016lx, size %ld\n", (int64_t)v.vector->data, size);
#endif
#endif	// DUMP_ALLOC_ADDR

	return v.vector->data;
}

void clear_symtbl(void)
{
	s_symbol_pool = NIL;
}

value_t register_sym(value_t s)
{
	value_t sym = find(s, s_symbol_pool, veq);
	if(nilp(sym))
	{
		s_symbol_pool = cons(s, s_symbol_pool);
		return s;
	}
	else
	{
		return sym;
	}
}

void check_gc(void)
{
	if(g_memory_top >= g_memory_gc && s_lock_cnt == 0)
	{
#ifdef TRACE_VM
		fprintf(stderr, "Checking memory pool that requires GC.\n");
#endif // TRACE_VM
		exec_gc();
	}
}

void force_gc(void)
{
	exec_gc();
}

// End of File
/////////////////////////////////////////////////////////////////////
