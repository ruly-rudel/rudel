#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#define INITIAL_ALLOC_SIZE	(16 * 1024 * 1024)

EXTERN value_t* g_memory_pool;
#ifndef NOGC
EXTERN value_t* g_memory_pool_back;
#endif  // NOGC
EXTERN value_t* g_memory_top;
EXTERN value_t* g_memory_max;


cons_t*		alloc_cons		(void);
vector_t*	alloc_vector		(void);
value_t*	alloc_vector_data	(value_t v, size_t size);

void init_allocator(void);

#endif // _ALLOCATOR_H_
