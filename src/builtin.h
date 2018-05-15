#ifndef _builtin_h_
#define _builtin_h_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include "misc.h"

typedef enum _rtype_t {
	// main 8types (content is address)
	PTR_T = 0,
	CONS_T,
	SYM_T,
	CLOJ_T,
	MACRO_T,
	ERR_T,
	VEC_T,
	OTH_T,

	// sub types (content is value)
	INT_T,
	CHAR_T,
	REF_T,
	SPECIAL_T,
	VMIS_T,
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
	SP_REST,
	SP_OPTIONAL,
	SP_KEY,
} special_t;

typedef enum {
	IS_HALT = 0,
	IS_BR,		// Branch to pc + operand (relative to pc)
	IS_BRB,		// Branch to pc - operand (relative to pc)
	IS_BNIL,	// Branch to pc + operand when NIL. (relative to pc)
	IS_MKVEC_ENV,
	IS_VPUSH_ENV,
	IS_VPOP_ENV,
	IS_NIL_CONS_VPUSH,
	IS_CONS_VPUSH,
	IS_CALLCC,
	IS_AP,
	IS_GOTO,
	IS_RET,
	IS_DUP,
	IS_PUSH,
	IS_PUSHR,
	IS_POP,
	IS_SETENV,
	IS_NCONC,
	IS_SWAP,
	IS_VPUSH_REST,
	IS_CONS_REST,
	IS_ROTL,
	IS_GETF,
	IS_THROW,

	IS_ATOM,
	IS_CONSP,
	IS_CLOJUREP,
	IS_MACROP,
	IS_SPECIALP,
	IS_STRP,
	IS_ERRP,
	IS_CONS,
	IS_CAR,
	IS_CDR,
	IS_EQ,
	IS_EQUAL,
	IS_RPLACA,
	IS_RPLACD,
	IS_GENSYM,
	IS_LT,
	IS_ELT,
	IS_MT,
	IS_EMT,
	IS_ADD,
	IS_SUB,
	IS_MUL,
	IS_DIV,
	IS_READ_STRING,
	IS_SLURP,
	IS_EVAL,
	IS_ERR,
	IS_NTH,
	IS_INIT,
	IS_SAVECORE,
	IS_MAKE_VECTOR,
	IS_VREF,
	IS_RPLACV,
	IS_VSIZE,
	IS_VEQ,
	IS_VPUSH,
	IS_VPOP,
	IS_COPY_VECTOR,
	IS_VCONC,
	IS_VNCONC,
	IS_COMPILE_VM,
	IS_EXEC_VM,
	IS_PR_STR,
	IS_PRINTLINE,
	IS_PRINT,
	IS_READ,
	IS_COUNT,
	IS_REVERSE,
	IS_MAKE_PACKAGE,
	IS_FIND_PACKAGE,
} vmis_t;

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

typedef struct
{
	uint32_t	main:     3;
	uint32_t	sub:      5;
	uint32_t	mnem:     8;
	uint32_t	operand: 16;
} opcode_t;
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

typedef struct
{
	uint64_t	main:     3;
	uint64_t	sub:      5;
	uint64_t	mnem:     8;
	uint64_t	operand: 48;
} opcode_t;
#endif

struct _cons_t;
struct _vector_t;
typedef union _value_t
{
	type_t			type;
	struct _cons_t*		cons;
	struct _vector_t*	vector;
	ref_t			ref;
	opcode_t		op;
	uintptr_t		raw;
} value_t;


typedef struct _cons_t
{
	value_t		car;
	value_t		cdr;
} cons_t;

typedef struct _vector_t
{
	value_t		size;
	value_t		alloc;
	value_t		type;
	value_t		data;
} vector_t;

#define NIL          ((value_t){ .type.main   = CONS_T, .type.sub = 0,         .type.val   = 0 })
#define RPTR(X)      ((value_t){ .raw = PTR_T | (uintptr_t)(X) })

#if __WORDSIZE == 32
	#define ALIGN(X)    ((X).raw & 0xfffffff8)
#else
	#define ALIGN(X)    ((X).raw & 0xfffffffffffffff8)
#endif

#define AVALUE(X)    ((value_t){ .raw = ALIGN(X) })
#define VPTROF(X)    ((value_t*)ALIGN(X))

#define RCHAR(X)     ((value_t){ .type.main   = OTH_T,  .type.sub = CHAR_T,    .type.val   = (X) })
#define RINT(X)      ((value_t){ .type.main   = OTH_T,  .type.sub = INT_T,     .type.val   = (X) })
#define RSPECIAL(X)  ((value_t){ .type.main   = OTH_T,  .type.sub = SPECIAL_T, .type.val   = (X) })
#define ROP(X)       ((value_t){ .op.main     = OTH_T,  .op.sub   = VMIS_T,    .op.mnem    = (X), .op.operand =  0  })
#define ROPD(X, Y)   ((value_t){ .op.main     = OTH_T,  .op.sub   = VMIS_T,    .op.mnem    = (X), .op.operand = (Y) })
#define RREF(X, Y)   ((value_t){ .ref .main   = OTH_T,  .ref .sub = REF_T,     .ref .depth = (X), .ref.width  = (Y) })
#define RERR(X, Y)   rerr_pos(RINT(X), (Y))

#define INTOF(X)        (((intptr_t)(X).raw) >> 8)
#define REF_D(X)        ((X).ref.depth)
#define REF_W(X)        ((X).ref.width)
#define SPECIAL(X)      ((X).type.val)

#define RERR_CAUSE(X)	car(X)
#define RERR_POS(X)	cdr(X)

#define EQ(X, Y)	((X).raw == (Y).raw)

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
#define ERR_ALLOC		11
#define ERR_EOS			12
#define ERR_INVALID_IS		13
#define ERR_ARG_BUILTIN		14
#define ERR_INVALID_AP		15
#define ERR_CANTOPEN		16
#define ERR_FWRITE		17
#define ERR_FREAD		18
#define ERR_WCHAR		18
#define ERR_EXCEPTION		19

#define UNSAFE_CAR(X)	AVALUE(X).cons->car
#define UNSAFE_CDR(X)	AVALUE(X).cons->cdr

#define GCCONS(X, CAR, CDR) \
	value_t X; \
	X.cons      = alloc_cons(); \
	X.cons->car = (CAR); \
	X.cons->cdr = (CDR); \
	X.type.main = CONS_T; \
	push_root(&X);

#define CONS_AND_CDR(V, C) \
{ \
	value_t R = cons((V), NIL); \
	rplacd((C), R); \
	(C) = R; \
}

INLINE(rtype_t rtypeof(value_t v),  v.type.main == OTH_T ? v.type.sub : v.type.main)
INLINE(bool    nilp(value_t x),     x.type.main == CONS_T &&  x.type.sub == 0 && x.type.val == 0)
INLINE(bool    intp(value_t x),     x.type.main == OTH_T  &&  x.type.sub == INT_T)
INLINE(bool    charp(value_t x),    x.type.main == OTH_T  &&  x.type.sub == CHAR_T)
INLINE(bool    consp(value_t x),    x.type.main == CONS_T && (x.type.sub != 0 || x.type.val != 0))
INLINE(bool    vectorp(value_t x),  x.type.main == VEC_T  && (x.type.sub != 0 || x.type.val != 0))
INLINE(bool    symbolp(value_t x),  x.type.main == SYM_T  && (x.type.sub != 0 || x.type.val != 0))
INLINE(bool    clojurep(value_t x), x.type.main == CLOJ_T && (x.type.sub != 0 || x.type.val != 0))
INLINE(bool    macrop(value_t x),   x.type.main == MACRO_T&& (x.type.sub != 0 || x.type.val != 0))
INLINE(bool    errp(value_t x),     x.type.main == ERR_T)
INLINE(bool    refp(value_t x),     x.type.main == OTH_T  &&  x.type.sub == REF_T)
INLINE(bool    specialp(value_t x), x.type.main == OTH_T  &&  x.type.sub == SPECIAL_T)
INLINE(bool    ptrp(value_t x),     x.type.main == PTR_T)

value_t car		(value_t x);
value_t cdr		(value_t x);
INLINE(value_t	first	(value_t x), car(x));
INLINE(value_t	second	(value_t x), car(cdr(x)));
INLINE(value_t	third	(value_t x), car(cdr(cdr(x))));
INLINE(value_t	fourth	(value_t x), car(cdr(cdr(cdr(x)))));
value_t	cons		(value_t car, value_t cdr);
bool	atom		(value_t x);
bool	eq		(value_t x, value_t y);
bool	equal		(value_t x, value_t y);
value_t rplaca		(value_t x, value_t v);
value_t rplacd		(value_t x, value_t v);
value_t last		(value_t x);
value_t nth		(int n, value_t x);
value_t nconc		(value_t a, value_t b);
value_t list		(int n, ...);
value_t find		(value_t key, value_t list, bool (*test)(value_t, value_t));
int	count		(value_t x);
value_t reverse		(value_t x);
value_t copy_list	(value_t lst);
value_t getf		(value_t key, value_t def, value_t plist);

value_t rerr		(value_t cause, value_t pos);
value_t rerr_alloc	(void);
value_t rerr_pos	(value_t cause, value_t pos);
value_t rerr_add_pos	(value_t pos, value_t e);
value_t	cfn		(value_t fn, value_t env);
value_t	cloj		(value_t ast, value_t ast_env, value_t code, value_t vm_env, value_t debug);
value_t	macro		(value_t ast, value_t ast_env, value_t code, value_t vm_env, value_t debug);
value_t slurp		(char* fn);
value_t init		(value_t pkg);
void	rep_file	(char* fn, value_t env);

value_t make_vector	(unsigned n);
value_t vref		(value_t v, unsigned pos);
value_t rplacv		(value_t v, unsigned pos, value_t data);
int	vsize		(value_t v);
int	vallocsize	(value_t v);
value_t vtype		(value_t v);
value_t*	vdata	(value_t v);
value_t vresize		(value_t v, int n);
bool	veq		(value_t x, value_t y);
value_t vpush		(value_t x, value_t v);
value_t vpop		(value_t v);
value_t vpeek		(value_t v);
value_t vpush_front	(value_t v, value_t x);
value_t copy_vector	(value_t src);
value_t vconc		(value_t x, value_t y);
value_t vnconc		(value_t x, value_t y);
value_t subvec		(value_t v, int begin, int end);

value_t str_to_cons	(const char* s);
value_t str_to_vec	(const char* s);
value_t mbstr_to_vec	(const char* s);
value_t wcstr_to_vec	(const wchar_t* s);
value_t str_to_rstr	(const char* s);
value_t mbstr_to_rstr	(const char* s);
value_t wcstr_to_rstr	(const wchar_t* s);
char*   rstr_to_str	(value_t s);

value_t make_symbol_r	(value_t name);
value_t make_symbol	(const char* s);
value_t symbol_string	(value_t sym);
value_t symbol_string_c	(value_t sym);
value_t symbol_package	(value_t sym);
value_t intern		(const char* s, value_t package);
value_t intern_r	(value_t s, value_t package);

value_t gensym		(value_t env);

bool	is_str		(value_t v);

EXTERN value_t g_nil;
EXTERN value_t g_t;
EXTERN value_t g_env;
EXTERN value_t g_unquote;
EXTERN value_t g_splice_unquote;
EXTERN value_t g_setq;
EXTERN value_t g_let;
EXTERN value_t g_progn;
EXTERN value_t g_if;
EXTERN value_t g_lambda;
EXTERN value_t g_quote;
EXTERN value_t g_quasiquote;
EXTERN value_t g_macro;
EXTERN value_t g_macroexpand;
EXTERN value_t g_rest;
EXTERN value_t g_optional;
EXTERN value_t g_key;
EXTERN value_t g_trace;
EXTERN value_t g_debug;
EXTERN value_t g_gensym_counter;
EXTERN value_t g_exception_stack;
EXTERN value_t g_package;

EXTERN value_t g_package_list;

EXTERN value_t* g_istbl;
EXTERN int      g_istbl_size;

#endif // _builtin_h_
