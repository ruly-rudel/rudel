
#include <assert.h>
#include "builtin.h"
#include "compile_vm.h"
#include "env.h"
#include "allocator.h"
#include "vm.h"
#include "printer.h"

/////////////////////////////////////////////////////////////////////
// private:

static value_t compile_vm1(value_t code, value_t debug, value_t ast, value_t env);

/////////////////////////////////////////////////////////////////////
// special form: setq
static value_t compile_vm_setq(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	// key
	value_t key = car(ast);
	if(!symbolp(key))
	{
		return RERR(ERR_NOTSYM, ast);
	}

	// value
	value_t val_notev = cdr(ast);
	if(rtypeof(val_notev) != CONS_T)
	{
		return RERR(ERR_ARG, cdr(ast));
	}

	value_t key_ref = NIL;
	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);
	push_root(&key);
	push_root(&key_ref);

	code = compile_vm1(code, debug, car(val_notev), env);

	if(!errp(code))
	{
		key_ref = get_env_ref(key, env);
		if(errp(key_ref))
		{
			vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
			vpush(key,           code);	vpush(ast, debug); //**** push symbol to code
		}
		else
		{
			vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
			vpush(key_ref,       code);	vpush(ast, debug); // push ref itself to stack
		}

		vpush(ROP(IS_SETENV), code);		vpush(ast, debug);
	}

	pop_root(6);
	return code;
}

/////////////////////////////////////////////////////////////////////
// special form: progn
static value_t compile_vm_progn(value_t code, value_t debug, value_t ast, value_t env)
{
	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);

	for(; !nilp(ast); ast = cdr(ast))
	{
		assert(consp(ast));
		code = compile_vm1(code, debug, car(ast), env);
		if(errp(code)) goto cleanup;
		vpush(ROP(IS_POP), code);		vpush(ast, debug);

	}
	vpop(code);	// remove last IS_POP

cleanup:
	pop_root(4);
	return code;
}

/////////////////////////////////////////////////////////////////////
// special form: if
static value_t compile_vm_if(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	// cereate environment
	if(!consp(ast))
	{
		return RERR(ERR_ARG, ast);
	}

	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);

	// eval condition
	code = compile_vm1(code, debug, first(ast), env);
	if(errp(code)) goto cleanup;

	int cond_br = vsize(code);
	vpush(ROP(IS_BNIL), code);	vpush(ast, debug);	// dummy operand(0)

	// true situation
	code = compile_vm1(code, debug, second(ast), env);
	if(errp(code)) goto cleanup;

	int true_br = vsize(code);
	vpush(ROP(IS_BR), code);	vpush(ast, debug);	// dummy operand(0)

	// false situation
	code = compile_vm1(code, debug, third(ast), env);
	if(errp(code)) goto cleanup;

	int next_addr = vsize(code);

	// fix branch address: relative to pc
	rplacv(code, cond_br, ROPD(IS_BNIL, true_br + 1 - cond_br));
	rplacv(code, true_br, ROPD(IS_BR,   next_addr - true_br));

cleanup:
	pop_root(4);
	return code;
}

/////////////////////////////////////////////////////////////////////
// special form: let
static value_t compile_vm_let(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	if(!consp(ast))
	{
		return RERR(ERR_ARG, ast);
	}

	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);

	// allocate new environment
	value_t let_env = create_lambda_env(NIL, env);
	push_root(&let_env);

	// local symbols
	value_t def = car(ast);
	if(rtypeof(def) != CONS_T)
	{
		pop_root(5);
		return RERR(ERR_ARG, ast);
	}
	push_root(&def);

	// create env vector
	vpush(ROPD(IS_MKVEC_ENV, count(def)), code);	vpush(ast, debug);

	// push empty env vector to env
	vpush(ROP(IS_VPUSH_ENV), code);			vpush(ast, debug);	// duplicate ENV_VEC on stack top

	// cereate environment

	value_t bind = NIL;
	value_t sym  = NIL;
	push_root(&bind);
	push_root(&sym);
	for(; !nilp(def); def = cdr(def))
	{
		bind = first(def);
		if(!consp(bind))
		{
			code = rerr_pos(RINT(ERR_TYPE), ast);
			goto cleanup;
		}

		// symbol
		sym = car(bind);
		if(!symbolp(sym)) goto cleanup;
		set_env(sym, NIL, let_env);	// for self-reference

		// body
		code = compile_vm1(code, debug, car(cdr(bind)), let_env);
		if(errp(code)) goto cleanup;
		// add let environment.
		vpush(ROP(IS_PUSHR), code);		vpush(ast, debug);
		vpush(sym,           code);		vpush(ast, debug);
		vpush(ROP(IS_CONS_VPUSH), code);	vpush(ast, debug);
	}

	vpush(ROP(IS_POP), code);			vpush(ast, debug);	// pop environment

	// let body
	code = compile_vm1(code, debug, second(ast), let_env);
	if(errp(code)) goto cleanup;

	// pop env
	vpush(ROP(IS_VPOP_ENV), code);			vpush(ast, debug);

cleanup:
	pop_root(8);
	return code;
}

/////////////////////////////////////////////////////////////////////
// special form: lambda
static value_t	compile_vm_arg_lambda(value_t code, value_t debug, value_t key, value_t env)
{
	value_t key_car = NIL;
	assert(vectorp(code));
	assert(consp(key) || nilp(key));

	push_root(&code);
	push_root(&debug);
	push_root(&key);
	push_root(&env);
	push_root(&key_car);

	vpush(ROPD(IS_MKVEC_ENV, count(key)), code);		vpush(key, debug);
	vpush(ROP (IS_VPUSH_ENV),             code);		vpush(key, debug);	// duplicate ENV_VEC on stack top

	int st = 0;
	for(int i = 0; !nilp(key); i++, (key = cdr(key)))
	{
		assert(consp(key));

		key_car = car(key);
		switch(st)
		{
		case 0:	// normal parameter
			if(EQ(key_car, RSPECIAL(SP_OPTIONAL)))	// optional parameter
			{
				st = 1;
			}
			else if(EQ(key_car, RSPECIAL(SP_REST)))	// rest parameter
			{
				st = 2;
			}
			else if(EQ(key_car, RSPECIAL(SP_KEY)))	// keyword parameter
			{
				st = 3;
			}
			else
			{
				vpush(ROP (IS_DEC_ARGNUM),   code);	vpush(key, debug);
				vpush(ROP (IS_SWAP),         code);	vpush(key, debug);
				vpush(ROP (IS_PUSHR),        code);	vpush(key, debug);
				vpush(key_car,               code);	vpush(key, debug);
				vpush(ROP (IS_CONS_VPUSH),   code);	vpush(key, debug);
			}
			break;

		case 1:	// optional parameter
			if(EQ(key_car, RSPECIAL(SP_OPTIONAL)))	// optional parameter
			{
				code = RERR(ERR_ARG, key);
				goto cleanup;
			}
			else if(EQ(key_car, RSPECIAL(SP_REST)))	// rest parameter
			{
				st = 2;
			}
			else if(EQ(key_car, RSPECIAL(SP_KEY)))	// keyword parameter
			{
				st = 3;
			}
			else
			{
				vpush(ROP (IS_DEC_ARGNUM),   code);	vpush(key, debug);
				vpush(ROP (IS_SWAP),         code);	vpush(key, debug);
				vpush(ROP (IS_PUSHR),        code);	vpush(key, debug);
				vpush(key_car,               code);	vpush(key, debug);
				vpush(ROP (IS_CONS_VPUSH),   code);	vpush(key, debug);
			}
			break;

		case 2:	// rest parameter
			vpush(ROP (IS_PUSHR),        code);	vpush(key, debug);
			vpush(key_car,               code);	vpush(key, debug);
			vpush(ROP (IS_VPUSH_REST),   code);	vpush(key, debug);
			goto cleanup;

		case 3:	// keyword parameter 1: push rest to env
			vpush(ROP (IS_CONS_REST),    code);	vpush(key, debug);
			st = 4;
			// fall through

		case 4:	// keyword parameter 2: push each keyword
			vpush(ROP (IS_DUP),          code);	vpush(key, debug);
			vpush(ROP (IS_ROTL),         code);	vpush(key, debug);

			code = compile_vm1(code, debug, car(cdr(key_car)), env);
			if(errp(code)) goto cleanup;

			vpush(ROP (IS_PUSHR),        code);	vpush(key, debug);
			vpush(car(car(key_car)),     code);	vpush(key, debug);
			vpush(ROP (IS_GETF),         code);	vpush(key, debug);
			vpush(ROP (IS_PUSHR),        code);	vpush(key, debug);
			vpush(car(cdr(car(key_car))),code);	vpush(key, debug);
			vpush(ROP (IS_CONS_VPUSH),   code);	vpush(key, debug);
			vpush(ROP (IS_SWAP),         code);	vpush(key, debug);
			break;
		}
	}

	if(st == 4)
	{
		vpush(ROP(IS_POP), code);			vpush(key, debug);	// pop plist
		vpush(ROP(IS_POP), code);			vpush(key, debug);	// pop environment
	}
	else
	{
		vpush(ROP(IS_ISZERO_ARGNUM),   code);		vpush(key, debug);
		vpush(ROP(IS_POP), code);			vpush(key, debug);	// pop environment
	}

cleanup:
	pop_root(5);
	return code;
}

static value_t compile_vm_lambda(value_t ast, value_t env)
{
	assert(consp(ast));
	assert(consp(env));

	if(!consp(ast)) return RERR(ERR_ARG, ast);

	// check if lambda or macro
	value_t fn = first(ast);
	if(!(EQ(fn, RSPECIAL(SP_LAMBDA)) || EQ(fn, RSPECIAL(SP_MACRO)))) return rerr_pos(RINT(ERR_ARG), ast);

	// allocate new environment for compilation
	value_t def = second(ast);
	if(rtypeof(def) != CONS_T) return RERR(ERR_ARG, ast);

	push_root(&ast);
	push_root(&env);
	push_root(&def);

	value_t lambda_env = create_lambda_env(def, env);
	push_root(&lambda_env);

	// compile body
	value_t lambda_code  = make_vector(1);
	push_root(&lambda_code);
	value_t lambda_debug = make_vector(1);
	push_root(&lambda_debug);
	lambda_code = compile_vm_arg_lambda(lambda_code, lambda_debug, def, lambda_env);	// with rest parameter support
	if(errp(lambda_code)) goto cleanup;

	lambda_code = compile_vm1(lambda_code, lambda_debug, third(ast), lambda_env);
	if(errp(lambda_code)) goto cleanup;

	// lambda is clojure call
	vpush(ROPD(IS_RET, count(def)), lambda_code);	vpush(ast, lambda_debug);	// RET
	lambda_code = cons(lambda_code, lambda_debug);

cleanup:
	pop_root(6);
	return lambda_code;
}

/////////////////////////////////////////////////////////////////////
// special form: quasiquote
static value_t compile_vm_quasiquote(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);

	if(!consp(ast))					// atom
	{
		vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
		vpush(ast, code);		vpush(ast, debug);	  // -> push immediate to stack
		pop_root(4);
		return code;
	}
	else
	{
		value_t arg1 = first(ast);
		push_root(&arg1);
		if(EQ(arg1, RSPECIAL(SP_UNQUOTE)))	// (unquote x)
		{
			value_t arg2 = second(ast);
			pop_root(5);
			return compile_vm1(code, debug, arg2, env);	  // -> add evaluation code
		}
		else if(consp(arg1) && EQ(first(arg1), RSPECIAL(SP_SPLICE_UNQUOTE)))	// ((splice-unquote x) ...)
		{
			code = compile_vm_quasiquote(code, debug, cdr(ast), env);
			if(errp(code)) goto cleanup;
			code = compile_vm1(code, debug, second(arg1), env);
			if(errp(code)) goto cleanup;
			vpush(ROP(IS_NCONC), code);	vpush(ast, debug);
		}
		else
		{
			code = compile_vm_quasiquote(code, debug, cdr(ast), env);
			if(errp(code)) goto cleanup;
			code = compile_vm_quasiquote(code, debug, arg1    , env);
			if(errp(code)) goto cleanup;
			vpush(ROP(IS_CONS), code);	vpush(ast, debug);
		}
	}

cleanup:
	pop_root(5);
	return code;
}

/////////////////////////////////////////////////////////////////////
// special form: macroexpand
// applicative order
static value_t compile_vm_arg(bool eval_it, value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast) || nilp(ast));
	assert(consp(env));

	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);

	if(!nilp(ast))
	{
		code = compile_vm_arg(eval_it, code, debug, cdr(ast), env);
		if(errp(code)) goto cleanup;

		if(eval_it)
		{
			code = compile_vm1(code, debug, car(ast), env);
			if(errp(code)) goto cleanup;
			vpush(ROP(IS_SWAP), code);	vpush(ast, debug);
		}
		else
		{
			vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
			vpush(car(ast),      code);	vpush(ast, debug); //**** push symbol to code
			vpush(ROP(IS_SWAP),  code);	vpush(ast, debug);
		}
	}

cleanup:
	pop_root(4);
	return code;
}

static value_t compile_vm_macroexpand(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);

	// PUSH AST
	code = compile_vm1(code, debug, ast, env);
	if(errp(code)) goto cleanup;

	// check macro
	vpush(ROP (IS_DUP),      code);	vpush(ast, debug);
	vpush(ROP (IS_CONSP),    code);	vpush(ast, debug);
	vpush(ROPD(IS_BNIL, 33), code);	vpush(ast, debug);
	vpush(ROP (IS_DUP),      code);	vpush(ast, debug);
	vpush(ROP (IS_CAR),      code);	vpush(ast, debug);

	vpush(ROP (IS_DUP),      code);	vpush(ast, debug);
	vpush(ROP (IS_SPECIALP), code);	vpush(ast, debug);
	vpush(ROPD(IS_BNIL, 3),  code);	vpush(ast, debug);
	vpush(ROP (IS_POP),      code);	vpush(ast, debug);
	vpush(ROPD(IS_BR, 26),   code);	vpush(ast, debug);

	vpush(ROP (IS_EVAL),     code);	vpush(ast, debug);
	vpush(ROP (IS_MACROP),   code);	vpush(ast, debug);
	vpush(ROPD(IS_BNIL, 23), code);	vpush(ast, debug);

	/////////
	// is macro
	// count arguments and reverse AST
	vpush(ROP (IS_DUP),      code);	vpush(ast, debug);
	vpush(ROP (IS_COUNT),    code);	vpush(ast, debug);
	vpush(ROP (IS_SWAP),     code);	vpush(ast, debug);
	vpush(ROP (IS_REVERSE),  code);	vpush(ast, debug);

	// PUSH each elements of ast to stack
	vpush(ROP (IS_DUP),      code);	vpush(ast, debug);
	vpush(ROP (IS_CAR),      code);	vpush(ast, debug);
	vpush(ROP (IS_ROTL),     code);	vpush(ast, debug);
	vpush(ROP (IS_CDR),      code);	vpush(ast, debug);
	vpush(ROP (IS_DUP),      code);	vpush(ast, debug);
	vpush(ROPD(IS_BNIL, 2),  code);	vpush(ast, debug);
	vpush(ROPD(IS_BRB, 6),   code);	vpush(ast, debug);

	// eval (car ast) -> #<MACRO>
	vpush(ROP (IS_POP),      code);	vpush(ast, debug);
	vpush(ROP (IS_SWAP),     code);	vpush(ast, debug);
	vpush(ROP (IS_EVAL),     code);	vpush(ast, debug);

	// set argnum
	vpush(ROP (IS_SWAP),     code);	vpush(ast, debug);
	vpush(ROP (IS_PUSHR),    code);	vpush(ast, debug);
	vpush(RINT(1),           code);	vpush(ast, debug);
	vpush(ROP (IS_SWAP),     code);	vpush(ast, debug);
	vpush(ROP (IS_SUB),      code);	vpush(ast, debug);
	vpush(ROP (IS_ARGNUM),   code);	vpush(ast, debug);

	// expand macro and return to top (macro expantion result may be macro call)
	vpush(ROP (IS_AP),       code);	vpush(ast, debug);
	vpush(ROPD(IS_BRB, 34),  code);	vpush(ast, debug);

cleanup:
	pop_root(4);
	return code;
}

/////////////////////////////////////////////////////////////////////
// apply
static value_t compile_vm_check_builtin(value_t atom)
{
	assert(symbolp(atom));
	for(int i = 0; i <= g_istbl_size; i += 3)
	{
		if(EQ(g_istbl[i], atom))
		{
			return g_istbl[i + 1];
		}
	}

	return NIL;
}

static int compile_vm_get_builtin_argnum(value_t atom)
{
	assert(symbolp(atom));
	for(int i = 0; i <= g_istbl_size; i += 3)
	{
		if(EQ(g_istbl[i], atom))
		{
			return INTOF(g_istbl[i + 2]);
		}
	}

	return -1;
}

// applicative order
static value_t compile_vm_builtin_arg(bool eval_it, value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast) || nilp(ast));
	assert(consp(env));

	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);

	if(!nilp(ast))
	{
		code = compile_vm_builtin_arg(eval_it, code, debug, cdr(ast), env);
		if(errp(code)) goto cleanup;
		if(eval_it)
		{
			code = compile_vm1(code, debug, car(ast), env);
			if(errp(code)) goto cleanup;
		}
		else
		{
			vpush(ROP(IS_PUSHR), code);	vpush(ast, debug);
			vpush(car(ast), code);		vpush(ast, debug);
		}
	}

cleanup:
	pop_root(4);
	return code;
}

static value_t compile_vm_special(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	value_t fn = first(ast);
	assert(specialp(fn));
	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);
	push_root(&fn);

	switch(INTOF(fn))
	{
		case SP_SETQ:
			code = compile_vm_setq      (code, debug, cdr(ast), env);
			break;

		case SP_LET:
			code = compile_vm_let       (code, debug, cdr(ast), env);
			break;

		case SP_PROGN:
			code = compile_vm_progn     (code, debug, cdr(ast), env);
			break;

		case SP_IF:
			code = compile_vm_if        (code, debug, cdr(ast), env);
			break;

		case SP_LAMBDA:		// set only s-expression
			vpush(ROP(IS_PUSH), code);				vpush(ast, debug);
			vpush(cloj (ast, env, NIL, NIL, NIL), code);		vpush(ast, debug);
			break;

		case SP_MACRO:		// set only s-expression
			vpush(ROP(IS_PUSH), code);				vpush(ast, debug);
			vpush(macro(ast, env, NIL, NIL, NIL), code);		vpush(ast, debug);
			break;

		case SP_QUOTE:
			vpush(ROP(IS_PUSHR), code);				vpush(ast, debug);
			vpush(second(ast), code);				vpush(ast, debug);
			break;

		case SP_QUASIQUOTE:
			code = compile_vm_quasiquote (code, debug, second(ast), env);
			break;

		case SP_MACROEXPAND:
			code = compile_vm_macroexpand(code, debug, second(ast), env);
			break;

		case SP_REST:	//******* ad-hock: ignore it
		case SP_OPTIONAL:
		case SP_KEY:
			break;

		default:
			code = RERR(ERR_NOTIMPL, ast);
			break;
	}

	pop_root(5);
	return code;
}

static value_t compile_vm_apply(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(ast));
	assert(consp(env));

	/////////////////////////////////////
	// CASE 1:special form
	value_t fn = first(ast);
	if(specialp(fn))
	{
		return compile_vm_special(code, debug, ast, env);
	}

	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);
	push_root(&fn);

	int argnum = count(cdr(ast));

	/////////////////////////////////////
	// CASE 2:builtin function or some optimizes
	if(symbolp(fn))
	{
		value_t bfn = compile_vm_check_builtin(fn);
		if(!nilp(bfn))			// apply builtin
		{
			push_root(&bfn);
			int n = compile_vm_get_builtin_argnum(fn);
			if(n != argnum)
			{
				pop_root(6);
				return RERR(ERR_ARG, ast);
			}
			code = compile_vm_builtin_arg(true, code, debug, cdr(ast), env);		// arguments
			if(errp(code))
			{
				pop_root(6);
				return code;
			}
			vpush(ROP(IS_PUSH),   code);		vpush(ast, debug);
			vpush(RINT(argnum),   code);		vpush(ast, debug);
			vpush(ROP(IS_ARGNUM), code);		vpush(ast, debug);
			//vpush(ROPD(IS_ARGNUM, argnum), code);		vpush(ast, debug);
			vpush(bfn, code);		vpush(ast, debug);
			pop_root(6);
			return code;
		}
		else				// apply setq-ed lambda/macro(is function or macro)
		{
			value_t ref = get_env_ref(fn, env);
			if(!errp(ref))
			{
				value_t sym = get_env_value_ref(ref, env);
				if(!errp(ref))
				{
					if(clojurep(sym))	// is clojure call
					{
						code = compile_vm_builtin_arg(true, code, debug, cdr(ast), env);
						if(errp(code)) goto cleanup;

						vpush(ROP(IS_PUSH),   code);		vpush(ast, debug);
						vpush(RINT(argnum),   code);		vpush(ast, debug);
						vpush(ROP(IS_ARGNUM), code);		vpush(ast, debug);
						//vpush(ROPD(IS_ARGNUM, argnum), code);	vpush(ast, debug);

						vpush(ROP(IS_PUSH), code);		vpush(ast, debug);
						vpush(ref,          code);		vpush(ast, debug);
						vpush(ROP(IS_AP),   code);		vpush(ast, debug);

						goto cleanup;
					}
					else if(macrop(sym))	// is macro call
					{
						code = compile_vm_builtin_arg(false, code, debug, cdr(ast), env);
						if(errp(code)) goto cleanup;

						vpush(ROP(IS_PUSH),   code);		vpush(ast, debug);
						vpush(RINT(argnum),   code);		vpush(ast, debug);
						vpush(ROP(IS_ARGNUM), code);		vpush(ast, debug);
						//vpush(ROPD(IS_ARGNUM, argnum), code);	vpush(ast, debug);

						vpush(ROP(IS_PUSH),        code);	vpush(ast, debug);
						vpush(ref,                 code);	vpush(ast, debug);
						vpush(ROP(IS_AP),          code);	vpush(ast, debug);
						vpush(ROP(IS_COMPILE_VM),  code);	vpush(ast, debug);
						vpush(ROP(IS_EXEC_VM),     code);	vpush(ast, debug);

						goto cleanup;
					}
				}
			}
		}
	}

	/////////////////////////////////////
	// CASE 3:normal apply

	// evaluate 1st element of apply list
	code = compile_vm1(code, debug, fn, env);
	if(errp(code)) goto cleanup;

	///////////////////
	// check function type and select call type
	vpush(ROP(IS_DUP), code);		vpush(ast, debug);
	vpush(ROP(IS_MACROP), code);		vpush(ast, debug);

	int cond_br = vsize(code);
	vpush(ROP(IS_BNIL), code);		vpush(ast, debug);	// dummy operand(0)

	///////////////////
	// true situation: macro call
	code = compile_vm_arg(false, code, debug, cdr(ast), env);	// arguments
	if(errp(code)) goto cleanup;

	vpush(ROP(IS_PUSH),   code);		vpush(ast, debug);
	vpush(RINT(argnum),   code);		vpush(ast, debug);
	vpush(ROP(IS_ARGNUM), code);		vpush(ast, debug);
	//vpush(ROPD(IS_ARGNUM, argnum), code);	vpush(ast, debug);
	vpush(ROP(IS_AP),          code);	vpush(ast, debug);
	vpush(ROP(IS_COMPILE_VM),  code);	vpush(ast, debug);
	vpush(ROP(IS_EXEC_VM),     code);	vpush(ast, debug);

	int true_br = vsize(code);
	vpush(ROP(IS_BR), code);		vpush(ast, debug);	// dummy operand(0)

	///////////////////
	// false situation: clojure call
	code = compile_vm_arg(true, code, debug, cdr(ast), env);	// arguments
	if(errp(code)) goto cleanup;

	vpush(ROP(IS_PUSH),   code);		vpush(ast, debug);
	vpush(RINT(argnum),   code);		vpush(ast, debug);
	vpush(ROP(IS_ARGNUM), code);		vpush(ast, debug);
	//vpush(ROPD(IS_ARGNUM, argnum), code);	vpush(ast, debug);
	vpush(ROP(IS_AP), code);		vpush(ast, debug);

	int next_addr = vsize(code);

	///////////////////
	// fix branch address: relative to pc
	rplacv(code, cond_br, ROPD(IS_BNIL, true_br + 1 - cond_br));
	rplacv(code, true_br, ROPD(IS_BR,   next_addr - true_br));

cleanup:
	pop_root(5);
	return code;
}

/////////////////////////////////////////////////////////////////////
// compile one term
static value_t compile_vm1(value_t code, value_t debug, value_t ast, value_t env)
{
	assert(vectorp(code));
	assert(consp(env));

	value_t r = NIL;
	push_root(&code);
	push_root(&debug);
	push_root(&ast);
	push_root(&env);
	push_root(&r);

	switch(rtypeof(ast))
	{
		case INT_T:
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(ast,          code);	vpush(ast, debug);
			break;

		case SYM_T:
			r = compile_vm_check_builtin(ast);	// Builtin Function (is one VM Instruction)
			if(nilp(r))
			{
				r = get_env_ref(ast, env);
				if(errp(r))
				{
					r = ast;		// late bind(will be bound while exec VM)
				}
			}
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(r,            code);	vpush(ast, debug);
			break;

		case REF_T:
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(ast,          code);	vpush(ast, debug);
			break;

		case VEC_T:
			vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
			vpush(ast,          code);	vpush(ast, debug);	//****** heap address in code: fix it.
			break;

		case CONS_T:
			if(nilp(ast))
			{
				vpush(ROP(IS_PUSH), code);	vpush(ast, debug);
				vpush(ast,          code);	vpush(ast, debug);
			}
			else
			{
				code = compile_vm_apply(code, debug, ast, env);
			}
			break;

		case PTR_T:
			code = RERR(ERR_TYPE, ast);
			break;

		case CLOJ_T:
		case MACRO_T:
			// compile lambda or macro body
			if(nilp(third(ast)))
			{
				value_t lambda_code = compile_vm_lambda(first(ast), second(ast));
				if(errp(lambda_code))
				{
					pop_root(5);
					return lambda_code;
				}
				else
				{
					rplaca(cdr(cdr(ast)), lambda_code);
				}
			}
			break;

		case SPECIAL_T:
			switch(INTOF(ast))
			{
				//****** ignore '&xxx' for dummy compilation: it is not evaluate.
				case SP_REST:
				case SP_OPTIONAL:
				case SP_KEY:
					break;

				default:
					code = RERR(ERR_NOTIMPL, ast);
					break;
			}
			break;


		default:
			code = RERR(ERR_NOTIMPL, ast);
			break;
	}

	pop_root(5);
	return code;
}

/////////////////////////////////////////////////////////////////////
// public: compile for VM


value_t compile_vm(value_t ast, value_t env)
{
	assert(consp(env));

	push_root(&ast);
	push_root(&env);

	value_t code  = make_vector(1);
	push_root(&code);

	value_t debug = make_vector(1);
	push_root(&debug);

	code = compile_vm1(code, debug, ast, env);
	if(!errp(code))
	{
		vpush(ROP(IS_RET), code);	vpush(ast, debug);
	}
	else
	{
		pop_root(4);
		return code;
	}

	pop_root(4);
	return cons(code, debug);
}

// End of File
/////////////////////////////////////////////////////////////////////
