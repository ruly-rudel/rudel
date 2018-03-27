#ifndef _vm_h_
#define _vm_h_

typedef enum 
{
	LD  = 0,	// push data from env[operand] to stack top
	LDI,		// push operand(immediate) to stack top
	ST,		// pop data from stack top and write to env[operand] 
	CALL,		// push pc to RET stack and jump to operand(immediate) address(absolute)
	RET,		// pop from RET stack and write to pc (and increment it)
	B,		// jump to an operand(immediate) address(absolute)
	BN,		// jump to an operand address if stack top is NIL, otherwise pc + 1
	CONS,		// pop topmost 2 data from stack and cons it and push it.
	EQ,
	ADD,
	SUB,
	MUL,
	DIV,
} op_t;

typedef struct
{
	op_t	opcode;
	value_t	operand;
} code_t;

value_t exec_vm(value_t code, value_t e);

#endif  //  _vm_h_
