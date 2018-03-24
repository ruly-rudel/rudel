#ifndef _builtin_h_
#define _builtin_h_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>

typedef enum _rtype_t {
	// main 8types (content is address)
	CONS_T = 0,	// sequence
	VEC_T,		// sequence
	ERR_T,
	SYM_T,
	CFN_T,		// dotted
	CLOJ_T,		// dotted
	MACRO_T,	// dotted
	OTH_T,

	// sub types (content is value)
	INT_T,
	CHAR_T,
	REF_T,
	SPECIAL_T,
} rtype_t;

typedef enum {
	SP_SETQ = 0,
	SP_LET,
	SP_PROGN,
	SP_IF,
	SP_LAMBDA,
	SP_QUOTE,
	SP_QUASIQUOTE,
	SP_MACRO,
	SP_MACROEXPAND,
	SP_UNQUOTE,
	SP_SPLICE_UNQUOTE,
	SP_AMP,
} special_t;

#if __WORDSIZE == 32
typedef struct
{
	uint32_t	main:   3;
	uint32_t	sub:    5;
	int32_t		val:   24;
} type_t;

typedef struct
{
	uint32_t	main:   3;
	uint32_t	sub:    5;
	uint32_t	depth: 12;
	uint32_t	width: 12;
} ref_t;
#else
typedef struct
{
	uint64_t	main:   3;
	uint64_t	sub:    5;
	int64_t		val:   56;
} type_t;

typedef struct
{
	uint64_t	main:   3;
	uint64_t	sub:    5;
	uint64_t	depth: 28;
	uint64_t	width: 28;
} ref_t;
#endif

union _value_t;
typedef union _value_t (*rfn_t)(union _value_t, union _value_t);

struct _cons_t;
typedef union _value_t
{
	type_t		type;
	struct _cons_t*	cons;
	ref_t		ref;
	rfn_t		rfn;
	void*		ptr;
#if __WORDSIZE == 32
	int32_t		raw;
#else
	int64_t		raw;
#endif
} value_t;


typedef struct _cons_t
{
	value_t		car;
	value_t		cdr;
} cons_t;


#define NIL         ((value_t){ .type.main   = CONS_T, .type.sub = 0,         .type.val   = 0   })

#define RCHAR(X)    ((value_t){ .type.main   = OTH_T,  .type.sub = CHAR_T,    .type.val   = (X) })
#define RINT(X)     ((value_t){ .type.main   = OTH_T,  .type.sub = INT_T,     .type.val   = (X) })
#define RSPECIAL(X) ((value_t){ .type.main   = OTH_T,  .type.sub = SPECIAL_T, .type.val   = (X) })
#define RREF(X, Y)  ((value_t){ .ref .main   = OTH_T,  .ref .sub = REF_T,     .ref .depth = (X), .ref.width = (Y) })
#define RFN(X)      ((value_t){ .rfn = (X) })
#define RERR(X, Y)   rerr(RINT(X), (Y))

#define INTOF(X)        ((X).raw >> 8)
#define REF_D(X)        ((X).ref.depth)
#define REF_W(X)        ((X).ref.width)
#define SPECIAL(X)      ((X).type.val)

#define RERR_CAUSE(X)	car(X)
#define RERR_POS(X)	cdr(X)

#define ERR_TYPE		1
#define ERR_EOF			2
#define ERR_PARSE		3
#define ERR_NOTFOUND		4
#define ERR_ARG			5
#define ERR_NOTFN		6
#define ERR_NOTSYM		7
#define ERR_FILENOTFOUND	8
#define ERR_RANGE		9
#define ERR_NOTIMPL		10

rtype_t rtypeof	(value_t v);

value_t car		(value_t x);
value_t cdr		(value_t x);
value_t	cons		(value_t car, value_t cdr);
value_t rerr		(value_t cause, value_t pos);
value_t rerr_add_pos	(value_t pos, value_t e);
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
value_t symbol_string	(value_t sym);
value_t init		(value_t env);

value_t str_to_cons	(const char* s);
value_t str_to_rstr	(const char* s);
value_t str_to_sym	(const char* s);
char*   rstr_to_str	(value_t s);

value_t register_sym	(value_t s);
value_t gensym		(value_t env);

value_t* cons_and_cdr	(value_t v, value_t* c);
value_t* nconc_and_last	(value_t v, value_t* c);
bool	is_str		(value_t v);

#endif // _builtin_h_
