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

inline static void copy1(value_t** top, value_t* v)
{
	rtype_t type = rtypeof(*v);
	value_t cur  = AVALUE(*v);
	value_t alloc;

	switch(type)
	{
		case CONS_T:
		case ERR_T:
		case CLOJ_T:
		case MACRO_T:
			if(cur.raw == 0) break;	// null value
			if(ptrp(cur.cons->car))	// target cons is already copied
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
				alloc.raw      = (uintptr_t)*top;
				*(*top)++ = UNSAFE_CAR(cur);
				*(*top)++ = UNSAFE_CDR(cur);

				// write to-space address to car of current cons in from-space
				alloc.type.main = PTR_T;
				cur.cons->car   = alloc;

				// replace value itself to copyed to-space address
				alloc.type.main = type;
				*v = alloc;
			}
			break;

		case VEC_T:
		case SYM_T:
			if(ptrp(cur.vector->type))	// target vector is already copied
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
				assert(intp(cur.vector->size));
				assert(intp(cur.vector->alloc));
				assert(nilp(cur.vector->type));
				// allocate memory and copy vector in from-space to to-space
				alloc.raw           = (uintptr_t)*top;
				*top               += 4;
				alloc.vector->size  = cur.vector->size;
				alloc.vector->alloc = cur.vector->alloc;
				alloc.vector->type  = cur.vector->type;
				if(VPTROF(cur.vector->data))
				{
					assert(ptrp(cur.vector->data));
					alloc.vector->data  = RPTR(*top);
					for(int i = 0; i < INTOF(cur.vector->alloc); i++)
						*(*top)++ = VPTROF(cur.vector->data)[i];
				}
				else
				{
					alloc.vector->data  = RPTR(0);
				}

				// write to-space address to car of current cons in from-space
				alloc.type.main     = PTR_T;
				cur.vector->type    = alloc;

				// replace value itself to copyed to-space address
				alloc.type.main = type;
				*v = alloc;
			}
			break;

		default:
			break;
	}

	return ;
}

static void swap_buffer(void)
{
	value_t* tmp       = g_memory_pool;
	g_memory_pool      = g_memory_top = g_memory_pool_from;
	g_memory_max       = g_memory_top + INITIAL_ALLOC_SIZE;
	g_memory_gc        = g_memory_top + INITIAL_ALLOC_SIZE / 2;
	g_memory_pool_from = tmp;
}

static void exec_gc_root(void)
{
	// copy root to memory pool
	for(int i = 0; i < s_root_ptr; i++)
	{
		if(s_root[i].size)
		{
			for(int j = 0; j <= *s_root[i].size; j++)
			{
				copy1(&g_memory_top, s_root[i].data + j);
			}
		}
		else
		{
			copy1(&g_memory_top, s_root[i].data);
		}
	}

	// copy symbol pool to memory pool
	copy1(&g_memory_top, &s_symbol_pool);

	// scan and copy rest
	value_t* scanned = g_memory_pool;
	while(scanned != g_memory_top)
	{
		copy1(&g_memory_top, scanned++);
	}

#ifdef TRACE_GC
	fprintf(stderr, " Replacing symbols...\n");
#endif
	init_global();
}

/////////////////////////////////////////////////////////////////////
// public: Control GC

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

void pop_root(int n)
{
	for(int i = 0; i < n; i++)
	{
		s_root[s_root_ptr  ].data = 0;
		s_root[s_root_ptr--].size = 0;
	}
	return;
}

value_t* exec_gc(void)
{
#ifdef NOGC
	return 0;
#else  // NOGC

#ifdef TRACE_GC
	fprintf(stderr, "Executing GC...\n");
#endif
	swap_buffer();
	exec_gc_root();


#ifdef TRACE_GC
	fprintf(stderr, "Executing GC Done.\n");
#endif
	return g_memory_top;
#endif // NOGC
}

void force_gc(void)
{
	exec_gc();
}

bool check_lock(void)
{
	return s_root_ptr == 0;
}

/////////////////////////////////////////////////////////////////////
// public: symbol pool

void clear_symtbl(void)
{
	s_symbol_pool = NIL;
}

value_t register_sym(value_t s)
{
	value_t sym = find(s, s_symbol_pool, veq);
	if(nilp(sym))
	{
		push_root(&s);
		value_t snew;
		snew.cons      = alloc_cons();
		snew.cons->car = s;
		snew.cons->cdr = s_symbol_pool;
		snew.type.main = CONS_T;
		s_symbol_pool = snew;
		pop_root(1);
		return s;
	}
	else
	{
		return sym;
	}
}

/////////////////////////////////////////////////////////////////////
// public: core image functions

value_t save_core(value_t fn, value_t root)
{
	assert(is_str(fn));

	// exec partial GC for compaction of root.
#ifdef NOGC
	return 0;
#else  // NOGC

#ifdef TRACE_GC
	fprintf(stderr, "Executing partial GC for env-compaction...\n");
#endif

	// swap buffer and gc partial root
	swap_buffer();
	copy1(&g_memory_top, &root);
	copy1(&g_memory_top, &s_symbol_pool);

	// scan and copy rest
	value_t* scanned = g_memory_pool;
	while(scanned != g_memory_top)
	{
		copy1(&g_memory_top, scanned++);
	}

#ifdef TRACE_GC
	fprintf(stderr, "Executing GC Done, saving core image...\n");
#endif
	char* s = rstr_to_str(fn);
	if(s)
	{
		FILE* fp = fopen(s, "wb");
		free(s);
		if(fp)
		{
			for(value_t* p = g_memory_pool; p < g_memory_top; p++)
			{
				value_t w = *p;

				// address translation, g_memory_pool based.
				if(rtypeof(w) < OTH_T && AVALUE(w).raw != 0)
				{
					// shift addres sizeof(cons_t) because avoiding zero pointer (is nil)
					w.raw -= ((uintptr_t)g_memory_pool - sizeof(cons_t));
				}

				if(fwrite(&w, sizeof(value_t), 1, fp) != 1)
				{
					fclose(fp);
					return RERR(ERR_FWRITE, NIL);
				}
			}
			fclose(fp);
		}
		else
		{
			return RERR(ERR_CANTOPEN, NIL);
		}
	}
	else
	{
		return RERR(ERR_TYPE, NIL);
	}

#ifdef TRACE_GC
	fprintf(stderr, "Saving core image done, execute rest GC...\n");
#endif

	// copy root to memory pool
	for(int i = 0; i < s_root_ptr; i++)
	{
		if(s_root[i].size)
		{
			for(int j = 0; j <= *s_root[i].size; j++)
			{
				copy1(&g_memory_top, s_root[i].data + j);
			}
		}
		else
		{
			copy1(&g_memory_top, s_root[i].data);
		}
	}

	// scan and copy rest
	while(scanned != g_memory_top)
	{
		copy1(&g_memory_top, scanned++);
	}

#ifdef TRACE_GC
	fprintf(stderr, " Replacing symbols...\n");
#endif
	init_global();

#ifdef TRACE_GC
	fprintf(stderr, "Executing GC Done.\n");
#endif

	return g_t;
#endif // NOGC
}

value_t load_core(const char* fn)
{
	// load all heap
	FILE* fp = fopen(fn, "rb");
	if(fp)
	{
		fseek(fp, 0, SEEK_END);
		long size = ftell(fp);
		rewind(fp);
		if(size > 0 && size < INITIAL_ALLOC_SIZE * sizeof(value_t))
		{
			// read core file to heap
			size_t cnt = size / sizeof(value_t);
			long r = fread(g_memory_pool, sizeof(value_t), cnt, fp);
			fclose(fp);
			if(r == cnt)
			{
				// fix address
				for(g_memory_top = g_memory_pool; g_memory_top < g_memory_pool + cnt; g_memory_top++)
				{
					// address translation, g_memory_pool based.
					if(rtypeof(*g_memory_top) < OTH_T && AVALUE(*g_memory_top).raw != 0)
					{
						// shift addres sizeof(cons_t) because avoiding zero pointer (is nil)
						g_memory_top->raw += (uintptr_t)g_memory_pool - sizeof(cons_t);
					}
				}

				// restore env
				value_t env;
				env.raw                 = (uintptr_t) g_memory_pool;
				env.type.main           = CONS_T;
				s_symbol_pool.raw       = (uintptr_t)(g_memory_pool + 2);
				s_symbol_pool.type.main = CONS_T;

				init_global();
				return env;
			}
			else
			{
				return RERR(ERR_FREAD, NIL);
			}
		}
		else
		{
			return RERR(ERR_FREAD, NIL);
		}
	}
	else
	{
		return RERR(ERR_CANTOPEN, NIL);
	}
	// not reached
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
	g_lock_cnt         = 0;
}

cons_t* alloc_cons(void)
{
	cons_t* c = (cons_t*)g_memory_top;
	g_memory_top += 2;

	if((g_memory_top >= g_memory_gc || FORCE_GC) && g_lock_cnt == 0)
	{
		if(exec_gc())
		{
			c = (cons_t*)g_memory_top;
			g_memory_top += 2;
			if(g_memory_top >= g_memory_max)
			{
				return 0;
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
	}

#ifdef DUMP_ALLOC_ADDR
#if __WORDSIZE == 32
	fprintf(stderr, "cons: %08x\n", (int)c);
#else
	fprintf(stderr, "cons: %016lx\n", (int64_t)c);
#endif
#endif	// DUMP_ALLOC_ADDR

	return c;
}

vector_t* alloc_vector()
{
	vector_t* v = (vector_t*)g_memory_top;
	g_memory_top += 4;

	if((g_memory_top >= g_memory_gc || FORCE_GC) && g_lock_cnt == 0)
	{
		if(exec_gc())
		{
			v = (vector_t*)g_memory_top;
			g_memory_top += 4;
			if(g_memory_top >= g_memory_max)
			{
				return 0;
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
	}

#ifdef DUMP_ALLOC_ADDR
#if __WORDSIZE == 32
	fprintf(stderr, "vect: %08x\n", (int)v);
#else
	fprintf(stderr, "vect: %016lx\n", (int64_t)v);
#endif
#endif	// DUMP_ALLOC_ADDR

	return v;
}

value_t alloc_vector_data(value_t v, size_t size)
{
	assert(vectorp(v));
	push_root(&v);

	size = size + (size % 2);	// align

	if((g_memory_top + size >= g_memory_gc || FORCE_GC) && g_lock_cnt == 0)
	{
		if(!exec_gc())
		{
			return NIL;
		}
		else if(g_memory_top + size >= g_memory_max)
		{
			return NIL;
		}
	}
	else if(g_memory_top + size >= g_memory_max)
	{
		return NIL;
	}

	value_t av = AVALUE(v);
	if(VPTROF(av.vector->data))
	{
		for(int i = 0; i < INTOF(av.vector->size); i++)
			g_memory_top[i] = VPTROF(av.vector->data)[i];

		// must be nillify for GC
		for(int i = INTOF(av.vector->size); i < INTOF(av.vector->alloc); i++)
			g_memory_top[i] = NIL;
	}
	else
	{
		// must be nillify for GC
		for(int i = 0; i < INTOF(av.vector->alloc); i++)
			g_memory_top[i] = NIL;
	}

	av.vector->data  = RPTR(g_memory_top);
	av.vector->alloc = RINT(size);
	g_memory_top += size;

#ifdef DUMP_ALLOC_ADDR
#if __WORDSIZE == 32
	fprintf(stderr, "vdat: %08x, size %d\n", ALIGN(av.vector->data), size);
#else
	fprintf(stderr, "vdat: %016lx, size %ld\n", ALIGN(av.vector->data), size);
#endif
#endif	// DUMP_ALLOC_ADDR

	pop_root(1);
	return v;
}

// End of File
/////////////////////////////////////////////////////////////////////
