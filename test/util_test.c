#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void test2() {
	const char *s = "tobo";
	char *out;

	diag("++++test2++++");	
	out = util_tal_multiply(NULL, s, "", 3);
	ok1(strcmp("tobotobotobo", out) == 0);
	talloc_free(out);

	out = util_tal_multiply(NULL, s, "X", 3);
	ok1(strcmp("toboXtoboXtobo", out) == 0);
	talloc_free(out);

	out = util_tal_multiply(NULL, s, "303", 3);
	ok1(strcmp("tobo303tobo303tobo", out) == 0);
	talloc_free(out);

	out = util_tal_multiply(NULL, s, "303", 0);
	ok1(strcmp("", out) == 0);
	talloc_free(out);

	out = util_tal_multiply(NULL, "", "303", 1);
	ok1(strcmp("", out) == 0);
	talloc_free(out);
#define TEST2AMT 5
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

