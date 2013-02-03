#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ccan/tap/tap.h>
#include <ccan/array_size/array_size.h>

#include "test.h"
#include "err.h"

struct test {
	void (*fn)();
	int amt;
};

void test1() {
	int passed = 0;

	diag("++++test1++++");	
	diag("dummy test");
	ok1(14 == 14);
	err_warn(0, "test warning blah blah blah");
#define exit(status) passed = 1
	err_panic(12, "test panic omg");
#undef exit
	ok1(passed == 1);


#define TEST1AMT 2
	diag("----test1----\n#");
}

int g_test2_cleanup_ran = 0;
int g_test2_err_dispatch_cleanup_ran = 0;

void test2_err_dispatch_cleanup(int error) {
	ok1(error == 15);
	g_test2_err_dispatch_cleanup_ran = 1;
}

void test2_cleanup(int error) {
	ok1(error == 15);
	g_test2_cleanup_ran = 1;
}

void test2() {
	int passed = 0;

	diag("++++test2++++");	
	ok1(err_init() == 0);
	err_set_cleanup_fn(test2_cleanup);
	ok1(err_dispatch_init(test2_err_dispatch_cleanup) == 0);

#define pthread_exit(status) passed = 1
	err_die(15, "test err_die");
#undef pthread_exit
	ok1(passed == 1);
	util_usleep(10,0);

	ok1(g_test2_cleanup_ran == 1);
	ok1(g_test2_err_dispatch_cleanup_ran == 1);

	ok1(err_dispatch_free() == 0);
	ok1(err_free() == 0);
#define TEST2AMT 1 + 1 + 3 + 2 + 2
	diag("----test2----\n#");
}

int main()
{
	int i, len, total_tests;
#define TESTN(_num) {test##_num, TEST##_num##AMT}
	struct test tests[] = {
		TESTN(1),
		TESTN(2)
	};

	len = ARRAY_SIZE(tests);
	for(i = 0; i < len; i++) {
		total_tests += tests[i].amt;
	}

	plan_tests(total_tests);

	test_init_api();
	for(i = 0; i < len; i++) {
		(*tests[i].fn)();
	}
	test_free_api();

	return exit_status();
}

