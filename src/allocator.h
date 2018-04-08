#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#define INITIAL_ALLOC_SIZE	(32 * 1024 * 1024)
//#define INITIAL_ALLOC_SIZE	(256 * 1024)
//#define INITIAL_ALLOC_SIZE	(5 * 1024)
#define ROOT_SIZE		512

EXTERN value_t* g_memory_pool;
#ifndef NOGC
EXTERN value_t* g_memory_pool_from;
#endif  // NOGC
EXTERN value_t* g_memory_top;
EXTERN value_t* g_memory_max;
EXTERN value_t* g_memory_gc;

void		push_root		(value_t* v);
void		push_root_raw_vec	(value_t *v, int* sp);
void		pop_root		(void);
void		lock_gc			(void);
void		unlock_gc		(void);
void		check_gc		(void);
void		force_gc		(void);

void		clear_symtbl		(void);
value_t		register_sym		(value_t s);
value_t		save_core		(value_t fn, value_t root);

void		init_allocator		(void);
cons_t*		alloc_cons		(void);
vector_t*	alloc_vector		(void);
value_t*	alloc_vector_data	(value_t v, size_t size);

#endif // _ALLOCATOR_H_
