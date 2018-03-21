#ifndef _builtin_h_
#define _builtin_h_

#include "misc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

typedef enum _rtype_t {
	// main 8types (content is address)
	CONS_T = 0,	// maybe sequence
	VEC_T,
	STR_T,
	SYM_T,
	CFN_T,		// dotted list
	CLOJ_T,
	MACRO_T,
	OTH_T,

	// sub types (content is value)
	INT_T,
	FLOAT_T,
	ERR_T
} rtype_t;

#if __WORDSIZE == 32

typedef struct
{
	uint32_t	main:   3;
	uint32_t	sub:    5;
	uint32_t	val:   24;
} type_t;

typedef struct
{
	int32_t		type:   8;
	int32_t		val:   24;
} rint_t;

typedef struct
{
	float		val;
} rfloat_t;

#else
typedef struct
{
	uint64_t	main:   3;
	uint64_t	sub:   29;
	uint64_t	val:   32;
} type_t;

typedef struct
{
	int64_t		type:  32;
	int64_t		val:   32;
} rint_t;

typedef struct
{
	uint32_t	type;
	float		val;
} rfloat_t;
#endif

union _value_t;
typedef union _value_t (*rfn_t)(union _value_t, union _value_t);

struct _cons_t;
typedef union _value_t
{
	type_t		type;
	rint_t		rint;
	rfloat_t	rfloat;
	struct _cons_t*	cons;
#if __WORDSIZE == 32
	uint32_t	raw;
#else
	uint64_t	raw;
#endif
	rfn_t		rfn;
	void*		ptr;
} value_t;


typedef struct _cons_t
{
	value_t		car;
	value_t		cdr;
} cons_t;


#define NIL       ((value_t){ .type.main   = CONS_T,  .type.sub   = 0,     .type.val = 0   })
#ifdef DEBUG
	#define RERR(X)   (abort(), (value_t){ .type.main   = OTH_T,   .type.sub   = ERR_T, .type.val = (X) })
#else
	#define RERR(X)   ((value_t){ .type.main   = OTH_T,   .type.sub   = ERR_T, .type.val = (X) })
#endif

#define RCHAR(X)  ((value_t){ .rint.type   = INT_T   << 3 | OTH_T, .rint.val   = (X) })
#define RINT(X)   ((value_t){ .rint.type   = INT_T   << 3 | OTH_T, .rint.val   = (X) })
#define RFLOAT(X) ((value_t){ .rfloat.type = FLOAT_T << 3 | OTH_T, .rfloat.val = (X) })
#define RFN(X)    ((value_t){ .rfn = (X) })

#define SYM_TRUE   str_to_sym("t")
#define SYM_FALSE  NIL
#define EMPTY_LIST NIL

#define ERR_TYPE	1
#define ERR_EOF		2
#define ERR_PARSE	3
#define ERR_NOTFOUND	4
#define ERR_ARG		5
#define ERR_NOTFN	6
#define ERR_NOTSYM	7
#define ERR_FILENOTFOUND	8
#define ERR_RANGE	9

rtype_t rtypeof	(value_t v);

value_t car		(value_t x);
value_t cdr		(value_t x);
value_t	cons		(value_t car, value_t cdr);
bool    errp		(value_t x);
bool	cnilp		(value_t x);
bool    nilp		(value_t x);
bool    intp		(value_t x);
value_t rplaca		(value_t x, value_t v);
value_t rplacd		(value_t x, value_t v);
value_t last		(value_t x);
value_t nth		(int n, value_t x);
value_t nconc		(value_t a, value_t b);
bool	eq		(value_t x, value_t y);
bool	equal		(value_t x, value_t y);
bool	atom		(value_t x);
value_t list		(int n, ...);
value_t	cfn		(value_t fn, value_t env);
value_t	cloj		(value_t fn, value_t env);
value_t	macro		(value_t fn, value_t env);
value_t copy_list	(value_t list);
value_t	assoc		(value_t key, value_t list, bool (*test)(value_t, value_t));
value_t assoceq		(value_t key, value_t list);
value_t acons		(value_t key, value_t val, value_t list);
value_t pairlis		(value_t key, value_t val);

bool    consp		(value_t list);
bool    seqp		(value_t list);
value_t concat		(int n, ...);
value_t find		(value_t key, value_t list, bool (*test)(value_t, value_t));
value_t slurp		(char* fn);
value_t reduce		(value_t (*fn)(value_t, value_t), value_t args);
value_t init		(value_t env);

value_t str_to_cons	(const char* s);
value_t str_to_rstr	(const char* s);
value_t str_to_sym	(const char* s);
char*   rstr_to_str	(value_t s);

value_t register_sym	(value_t s);

value_t* cons_and_cdr	(value_t v, value_t* c);
value_t* nconc_and_last	(value_t v, value_t* c);

#endif // _builtin_h_
