#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#define INITIAL_ALLOC_SIZE	(32 * 1024 * 1024)
//#define INITIAL_ALLOC_SIZE	(256 * 1024)
//#define INITIAL_ALLOC_SIZE	(5 * 1024)
#define ROOT_SIZE		1024

#ifdef DEBUG_GC
#define FORCE_GC 1
#else // DEBUG_GC
#define FORCE_GC 0
#endif // DEBUG_GC

EXTERN value_t* g_memory_pool;
#ifndef NOGC
EXTERN value_t* g_memory_pool_from;
#endif  // NOGC
EXTERN value_t* g_memory_top;
EXTERN value_t* g_memory_max;
EXTERN value_t* g_memory_gc;
EXTERN int	g_lock_cnt;

void		push_root		(value_t* v);
void		push_root_raw_vec	(value_t *v, int* sp);
void		pop_root		(int n);
value_t*	exec_gc			(void);
void		force_gc		(void);
INLINE(value_t*	check_gc(void),		(g_memory_top >= g_memory_gc || FORCE_GC) && g_lock_cnt == 0 ?  exec_gc() : 0)
INLINE(int	lock_gc(void),		g_lock_cnt++)
INLINE(int	unlock_gc(void),	g_lock_cnt--)
bool		check_lock		(void);
bool		check_sanity		(void);

void		clear_symtbl		(void);
value_t		register_sym		(value_t s);
value_t		register_key		(value_t s);
value_t		save_core		(value_t fn, value_t root);
value_t		load_core		(const char* fn);

void		init_allocator		(void);
cons_t*		alloc_cons		(void);
vector_t*	alloc_vector		(void);
value_t		alloc_vector_data	(value_t v, size_t size);

#endif // _ALLOCATOR_H_
