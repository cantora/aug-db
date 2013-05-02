#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ccan/tap/tap.h>
#include <ccan/array_size/array_size.h>
#include <ccan/str/str.h>
#include <unistd.h>

#include "test.h"
#include "encoding.h"

struct test {
	void (*fn)();
	int amt;
};

void test1() {
	wchar_t wchars[] = L"○◌◜◝◞◟◾◾◾◾◾◾◾◾◾◾";
	uint8_t utf8[512];
	size_t amt_left;

	ok1(encoding_init() == 0);
	
	amt_left = encoding_wchar_to_utf8(utf8, ARRAY_SIZE(utf8), (uint32_t *) wchars, ARRAY_SIZE(wchars) );
	ok1(amt_left > 0);
	ok1(amt_left == 463);
	diag("amt left: %u", amt_left);
	utf8[ARRAY_SIZE(utf8)-amt_left] = '\0';
	diag("utf8 text: %s", utf8);

#define TEST1AMT 3
	diag("----test1----\n#");
	encoding_free();
}


int main()
{
	int i, len, total_tests;
#define TESTN(_num) {test##_num, TEST##_num##AMT}
	struct test tests[] = {
		TESTN(1)
	};

	total_tests = 0;
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

