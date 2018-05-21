#ifndef _ASM_H_
#define _ASM_H_

#include "misc.h"
#include "builtin.h"

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

typedef struct
{
	value_t		code;
	value_t		debug;
	value_t		labels;
	value_t		replaces;
} code_t;

code_t* asm_init	(code_t* code, int n);
void	asm_close	(code_t* code);
value_t asm_code	(code_t* code);

code_t* asm_im		(value_t im, value_t dbg, code_t* code);
code_t* asm_im_nd	(value_t im, code_t* code);
code_t* asm_op		(vmis_t is, value_t dbg, code_t* code);
code_t* asm_op_nd	(vmis_t is, code_t* code);
code_t* asm_opd		(vmis_t is, int operand, value_t dbg, code_t* code);
code_t* asm_opd_nd	(vmis_t is, int operand, code_t* code);
code_t* asm_opl		(vmis_t is, value_t label, value_t dbg, code_t* code);
code_t* asm_opl_nd	(vmis_t is, value_t label, code_t* code);
code_t*	asm_label	(value_t label, code_t* code);
void asm_fix_label	(code_t* code);

#endif // _ASM_H_
