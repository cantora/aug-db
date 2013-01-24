#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ccan/tap/tap.h>
#include <ccan/array_size/array_size.h>
#include <time.h>

#include "test.h"
#include "util.h"

struct test {
	void (*fn)();
	int amt;
};

void test1() {
	time_t t1, t2;

	diag("++++test1++++");	
	t1 = time(NULL);
	ok1(util_usleep(4, 500000) == 0);
	t2 = time(NULL);
	/*diag("t1 = %d, t2 = %d", t1, t2);*/
	ok1( (t2 - t1) > 3 );

#define TEST1AMT 2
	diag("----test1----\n#");
}

int main()
{
	int i, len, total_tests;
#define TESTN(_num) {test##_num, TEST##_num##AMT}
	struct test tests[] = {
		TESTN(1)
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

