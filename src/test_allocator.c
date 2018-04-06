#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "builtin.h"
#include "allocator.h"
#include "misc.h"


void test_alloc_000(void)
{
	force_gc();
	value_t r  = cons(RINT(1), RINT(2));
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(g_memory_pool      == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool_from == (value_t*)r2.cons);
	CU_ASSERT(EQ(car(r), RINT(1)));
	CU_ASSERT(EQ(cdr(r), RINT(2)));
}

void test_alloc_001(void)
{
	force_gc();
	value_t r  = cons(RINT(1), RINT(2));
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(g_memory_pool      == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool_from == (value_t*)r2.cons);
	CU_ASSERT(EQ(car(r), RINT(1)));
	CU_ASSERT(EQ(cdr(r), RINT(2)));
}

void test_alloc_002(void)
{
	force_gc();
	value_t r  = cons(RINT(1), cons(RINT(2), cons(RINT(3), NIL)));
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(g_memory_pool           == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool      +  2 == (value_t*)r.cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.cons->cdr.cons->cdr.cons);
	CU_ASSERT(g_memory_pool_from +  4 == (value_t*)r2.cons);
	CU_ASSERT(EQ(first(r),  RINT(1)));
	CU_ASSERT(EQ(second(r), RINT(2)));
	CU_ASSERT(EQ(third(r),  RINT(3)));
	CU_ASSERT(EQ(fourth(r), NIL));
}

void test_alloc_003(void)
{
	force_gc();
	value_t r  = cons(cons(RINT(1), cons(RINT(2), NIL)), cons(RINT(3), cons(RINT(4), NIL)));
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(g_memory_pool           == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool      +  2 == (value_t*)r.cons->car.cons);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.cons->car.cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  6 == (value_t*)r.cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  8 == (value_t*)r.cons->cdr.cons->cdr.cons);
	CU_ASSERT(g_memory_pool_from +  8 == (value_t*)r2.cons);
	CU_ASSERT(EQ(car(car(r)),  RINT(1)));
	CU_ASSERT(EQ(car(cdr(car(r))),  RINT(2)));
	CU_ASSERT(EQ(second(r), RINT(3)));
	CU_ASSERT(EQ(third(r),  RINT(4)));
	CU_ASSERT(EQ(fourth(r), NIL));
}

void test_alloc_004(void)
{
	force_gc();
	value_t r  = NIL;
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(EQ(r, r2));
	CU_ASSERT(nilp(r));
	CU_ASSERT(nilp(r2));
}

void test_alloc_005(void)
{
	force_gc();
	value_t r  = cons(RINT(1), cons(RINT(2), cons(RINT(3), NIL)));
	r.type.main = CLOJ_T;
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(clojurep(r));
	CU_ASSERT(clojurep(r2));
	r.type.main = CONS_T;
	r2.type.main = CONS_T;
	CU_ASSERT(g_memory_pool           == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool      +  2 == (value_t*)r.cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.cons->cdr.cons->cdr.cons);
	CU_ASSERT(g_memory_pool_from +  4 == (value_t*)r2.cons);
	CU_ASSERT(EQ(first(r),  RINT(1)));
	CU_ASSERT(EQ(second(r), RINT(2)));
	CU_ASSERT(EQ(third(r),  RINT(3)));
	CU_ASSERT(EQ(fourth(r), NIL));
}

void test_alloc_006(void)
{
	force_gc();
	value_t r  = cons(RINT(1), cons(RINT(2), cons(RINT(3), NIL)));
	r.type.main = MACRO_T;
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(macrop(r));
	CU_ASSERT(macrop(r2));
	r.type.main = CONS_T;
	r2.type.main = CONS_T;
	CU_ASSERT(g_memory_pool           == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool      +  2 == (value_t*)r.cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.cons->cdr.cons->cdr.cons);
	CU_ASSERT(g_memory_pool_from +  4 == (value_t*)r2.cons);
	CU_ASSERT(EQ(first(r),  RINT(1)));
	CU_ASSERT(EQ(second(r), RINT(2)));
	CU_ASSERT(EQ(third(r),  RINT(3)));
	CU_ASSERT(EQ(fourth(r), NIL));
}

void test_alloc_007(void)
{
	force_gc();
	value_t r   = cons(RINT(1), cons(RINT(2), cons(RINT(3), NIL)));
	rplacd(cdr(cdr(r)), r);
	push_root(&r);
	r.type.main = MACRO_T;
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(macrop(r));
	CU_ASSERT(macrop(r2));
	r.type.main = CONS_T;
	r2.type.main = CONS_T;
	CU_ASSERT(g_memory_pool           == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool      +  2 == (value_t*)r.cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.cons->cdr.cons->cdr.cons);
	CU_ASSERT(g_memory_pool           == (value_t*)r.cons->cdr.cons->cdr.cons->cdr.cons);
	CU_ASSERT(g_memory_pool_from +  4 == (value_t*)r2.cons);
	CU_ASSERT(EQ(first(r),  RINT(1)));
	CU_ASSERT(EQ(second(r), RINT(2)));
	CU_ASSERT(EQ(third(r),  RINT(3)));
	CU_ASSERT(EQ(fourth(r), RINT(1)));
}

void test_alloc_008(void)
{
	force_gc();
	value_t rt = cons(RINT(4), cons(RINT(5), cons(RINT(6), NIL)));
	value_t r  = cons(RINT(1), cons(RINT(2), cons(RINT(3), NIL)));
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(g_memory_pool           == (value_t*)r.cons);
	CU_ASSERT(g_memory_pool      +  2 == (value_t*)r.cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.cons->cdr.cons->cdr.cons);
	CU_ASSERT(g_memory_pool_from + 10 == (value_t*)r2.cons);
	CU_ASSERT(EQ(first(r),  RINT(1)));
	CU_ASSERT(EQ(second(r), RINT(2)));
	CU_ASSERT(EQ(third(r),  RINT(3)));
	CU_ASSERT(EQ(fourth(r), NIL));
}

void test_alloc_009(void)
{
	force_gc();
	value_t r = make_vector(0);
	vpush(RINT(0), r);
	vpush(RINT(1), r);
	vpush(RINT(2), r);
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(vectorp(r));
	CU_ASSERT(vectorp(r2));

	r.type.main  = CONS_T;
	r2.type.main = CONS_T;
	CU_ASSERT(g_memory_pool           == (value_t*)r.vector);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.vector->data);
	CU_ASSERT(g_memory_pool_from      == (value_t*)r2.vector);
	CU_ASSERT(r.vector->size  == 3);
	CU_ASSERT(r.vector->alloc == 4);

	r.type.main  = VEC_T;
	r2.type.main = VEC_T;
	CU_ASSERT(EQ(vref(r, 0), RINT(0)));
	CU_ASSERT(EQ(vref(r, 1), RINT(1)));
	CU_ASSERT(EQ(vref(r, 2), RINT(2)));
	CU_ASSERT(equal(vref(r, 3), RERR(ERR_RANGE, NIL)));
}

void test_alloc_010(void)
{
	force_gc();
	value_t r = make_vector(0);
	vpush(cons(RINT(1), cons(RINT(2), NIL)), r);
	vpush(cons(RINT(3), cons(RINT(4), NIL)), r);
	vpush(RINT(5), r);
	vpush(vref(r, 0), r);
	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(vectorp(r));
	CU_ASSERT(vectorp(r2));

	r.type.main  = CONS_T;
	r2.type.main = CONS_T;
	CU_ASSERT(g_memory_pool           == (value_t*)r.vector);
	CU_ASSERT(g_memory_pool      +  4 == (value_t*)r.vector->data);
	CU_ASSERT(g_memory_pool      +  8 == (value_t*)r.vector->data[0].cons);
	CU_ASSERT(g_memory_pool      + 10 == (value_t*)r.vector->data[0].cons->cdr.cons);
	CU_ASSERT(g_memory_pool      + 12 == (value_t*)r.vector->data[1].cons);
	CU_ASSERT(g_memory_pool      + 14 == (value_t*)r.vector->data[1].cons->cdr.cons);
	CU_ASSERT(g_memory_pool      +  8 == (value_t*)r.vector->data[3].cons);
	CU_ASSERT(g_memory_pool_from      == (value_t*)r2.vector);
	CU_ASSERT(r.vector->size  == 4);
	CU_ASSERT(r.vector->alloc == 4);

	r.type.main  = VEC_T;
	r2.type.main = VEC_T;
	CU_ASSERT(EQ(first(vref (r, 0)), RINT(1)));
	CU_ASSERT(EQ(second(vref(r, 0)), RINT(2)));
	CU_ASSERT(EQ(first(vref (r, 1)), RINT(3)));
	CU_ASSERT(EQ(second(vref(r, 1)), RINT(4)));
	CU_ASSERT(EQ(vref(r, 2), RINT(5)));
	CU_ASSERT(EQ(vref(r, 3), vref(r, 0)));
	CU_ASSERT(equal(vref(r, 4), RERR(ERR_RANGE, NIL)));
}

void test_alloc_011(void)
{
	force_gc();
	value_t r = make_vector(0);
	vpush(str_to_sym("nil"), r);
	vpush(str_to_sym("t"),   r);
	vpush(str_to_sym("hogehoge"),   r);

	value_t str_nil = str_to_sym("nil");
	value_t str_t = str_to_sym("t");
	value_t str_hogehoge = str_to_sym("hogehoge");

	push_root(&r);
	value_t r2 = r;
	force_gc();
	pop_root();

	CU_ASSERT(!EQ(r, r2));
	CU_ASSERT(vectorp(r));
	CU_ASSERT(vectorp(r2));

	CU_ASSERT(EQ(str_to_sym("nil"),        vref (r, 0)));
	CU_ASSERT(EQ(str_to_sym("t"),          vref (r, 1)));
	CU_ASSERT(EQ(str_to_sym("hogehoge"),   vref (r, 2)));
	CU_ASSERT(!EQ(str_to_sym("nil"),        str_nil));
	CU_ASSERT(!EQ(str_to_sym("t"),          str_t));
	CU_ASSERT(!EQ(str_to_sym("hogehoge"),   str_hogehoge));
	CU_ASSERT(equal(str_to_sym("nil"),        str_nil));
	CU_ASSERT(equal(str_to_sym("t"),          str_t));
	CU_ASSERT(equal(str_to_sym("hogehoge"),   str_hogehoge));
}

int main(int argc, char* argv[])
{
	init_allocator();
	init_global();

	CU_pSuite alloc_suite;
	
	CU_initialize_registry();
	alloc_suite = CU_add_suite("Allocator", NULL, NULL);
	CU_add_test(alloc_suite, "test_alloc_000", test_alloc_000);
	CU_add_test(alloc_suite, "test_alloc_001", test_alloc_001);
	CU_add_test(alloc_suite, "test_alloc_002", test_alloc_002);
	CU_add_test(alloc_suite, "test_alloc_003", test_alloc_003);
	CU_add_test(alloc_suite, "test_alloc_004", test_alloc_004);
	CU_add_test(alloc_suite, "test_alloc_005", test_alloc_005);
	CU_add_test(alloc_suite, "test_alloc_006", test_alloc_006);
	CU_add_test(alloc_suite, "test_alloc_007", test_alloc_007);
	CU_add_test(alloc_suite, "test_alloc_008", test_alloc_008);
	CU_add_test(alloc_suite, "test_alloc_009", test_alloc_009);
	CU_add_test(alloc_suite, "test_alloc_010", test_alloc_010);
	CU_add_test(alloc_suite, "test_alloc_011", test_alloc_011);
	CU_console_run_tests();
	CU_cleanup_registry();
	return 0;
}
