
#include <assert.h>
#include "builtin.h"
#include "asm.h"
#include "allocator.h"


/////////////////////////////////////////////////////////////////////
// private: vm assemblar

static void	asm_push_replaces(value_t label, code_t* code)
{
	int pos = vsize(code->code);
	value_t pair = cons(RINT(pos), label);
	vpush(pair, code->replaces);
}

static value_t	vassoc(value_t key, value_t avector)
{
	assert(vectorp(avector));

	for(int i = 0; i < vsize(avector); i++)
	{
		value_t pair = vref(avector, i);
		assert(rtypeof(pair) == CONS_T);
		if(!nilp(pair) && equal(UNSAFE_CAR(pair), key))
		{
			return pair;
		}
	}
	return NIL;
}


/////////////////////////////////////////////////////////////////////
// public


code_t* asm_init	(code_t* code, int n)
{
	assert(code != 0);

	code->code     = make_vector(n);
	push_root(&code->code);
	code->debug    = make_vector(n);
	push_root(&code->debug);
	code->labels   = make_vector(0);
	push_root(&code->labels);
	code->replaces = make_vector(0);
	push_root(&code->replaces);

	return code;
}

void	asm_close	(code_t *code)
{
	assert(code != 0);

	code->code     = NIL;
	code->debug    = NIL;
	code->labels   = NIL;
	code->replaces = NIL;
	pop_root(4);
}

value_t asm_code	(code_t* code)
{
	assert(code != 0);

	return cons(code->code, code->debug);
}

code_t* asm_im		(value_t im, value_t dbg, code_t* code)
{
	assert(code != 0);

	push_root(&im);
	push_root(&dbg);
	vpush(im,	code->code );
	vpush(dbg,	code->debug);
	pop_root(2);
	return code;
}

code_t* asm_im_nd	(value_t im, code_t* code)
{
	return asm_im(im, im, code);
}

code_t* asm_op		(vmis_t is, value_t dbg, code_t* code)
{
	return asm_im(ROP(is), dbg, code);
}

code_t* asm_op_nd	(vmis_t is, code_t* code)
{
	return asm_im_nd(ROP(is), code);
}

code_t* asm_opd		(vmis_t is, int operand, value_t dbg, code_t* code)
{
	return asm_im(ROPD(is, operand), dbg, code);
}

code_t* asm_opd_nd	(vmis_t is, int operand, code_t* code)
{
	return asm_im_nd(ROPD(is, operand), code);
}

code_t* asm_opl		(vmis_t is, value_t label, value_t dbg, code_t* code)
{
	push_root(&dbg);
	asm_push_replaces(label, code);

	code_t* c = asm_im(ROPD(is, 0), dbg, code);
	pop_root(1);
	return c;
}

code_t* asm_opl_nd	(vmis_t is, value_t label, code_t* code)
{
	asm_push_replaces(label, code);

	return asm_im_nd(ROPD(is, 0), code);
}

code_t*	asm_label	(value_t label, code_t* code)
{
	int pos = vsize(code->code);
	value_t pair = cons(label, RINT(pos));
	vpush(pair, code->labels);

	return code;
}

void asm_fix_label(code_t* code)
{
	int size = vsize(code->replaces);
	for(int i = 0; i < size; i++)
	{
		value_t replace = vref(code->replaces, i);
		value_t label   = vassoc(cdr(replace), code->labels);
		if(nilp(label))
		{
			abort();	//***** ad-hock
		}
		else
		{
			int  l_pos  = INTOF(cdr(label));
			int  op_pos = INTOF(car(replace));
			value_t op  = vref(code->code, op_pos);
			op.op.operand = abs(l_pos - op_pos);
			rplacv(code->code, op_pos, op);
		}
	}

	// clear labels/replaces
	code->labels   = make_vector(0);
	code->replaces = make_vector(0);
}

// End of File
/////////////////////////////////////////////////////////////////////
