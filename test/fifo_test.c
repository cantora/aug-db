#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ccan/tap/tap.h>
#include <ccan/array_size/array_size.h>
#include <time.h>
#include <string.h>
#include <locale.h>

#include "test.h"
#include "fifo.h"

struct test {
	void (*fn)();
	int amt;
};

void test1() {
	struct fifo p1;
	char buf1[256];
	char elem;
	char output_buf[16];

	diag("++++test1++++");	
	
	fifo_init(&p1, buf1, sizeof(char), ARRAY_SIZE(buf1) );
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == ARRAY_SIZE(buf1));

	fifo_push(&p1, "a");
	ok1(fifo_amt(&p1) == 1);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-1));
	fifo_top(&p1, &elem);
	ok1(elem == 'a');
	
	elem = '\0';
	fifo_write(&p1, "zbcdefg", 7);
	ok1(fifo_amt(&p1) == 1+7);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-1-7));
	fifo_pop(&p1, &elem);
	ok1(elem == 'a');
	ok1(fifo_amt(&p1) == 7);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-7));
	fifo_pop(&p1, &elem);
	ok1(elem == 'z');
	ok1(fifo_amt(&p1) == 6);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-6));
	fifo_consume(&p1, output_buf, 6);
	ok1(memcmp("bcdefg", output_buf, 6) == 0);
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)));
	
	

#define TEST1AMT 2 + 3 + 11
	diag("----test1----\n#");
}

void test2() {
	struct fifo p1;
	int buf1[256];
	int elem;
	int output_buf[16];
	int test_arr[19];
	size_t i;
	int input;

	srand(time(NULL));
	for(i = 0; i < ARRAY_SIZE(test_arr); i++) {
		test_arr[i] = rand();
	}
	diag("++++test2++++");	
	
	fifo_init(&p1, buf1, sizeof(int), ARRAY_SIZE(buf1) );
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == ARRAY_SIZE(buf1));

	input = 345;
	fifo_push(&p1, &input);
	ok1(fifo_amt(&p1) == 1);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-1));
	fifo_top(&p1, &elem);
	ok1(elem == 345);
	
	elem = 0;
	fifo_write(&p1, test_arr, 7);
	ok1(fifo_amt(&p1) == 1+7);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-1-7));
	fifo_pop(&p1, &elem);
	ok1(elem == 345);
	ok1(fifo_amt(&p1) == 7);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-7));
	fifo_pop(&p1, &elem);
	ok1(elem == test_arr[0]);
	ok1(fifo_amt(&p1) == 6);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)-6));
	fifo_consume(&p1, output_buf, 6);
	ok1(memcmp(test_arr+1, output_buf, 6) == 0);
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)));

#define TEST2AMT 2 + 3 + 11
	diag("----test2----\n#");
}

void test3() {
	struct fifo p1;
	int buf1[128];
	int elem;
	int output_buf[128];
	int test_arr[512];
	size_t i;

	srand(time(NULL));
	for(i = 0; i < ARRAY_SIZE(test_arr); i++) {
		test_arr[i] = rand();
	}
	diag("++++test3++++");	
	
	fifo_init(&p1, buf1, sizeof(int), ARRAY_SIZE(buf1) );
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == ARRAY_SIZE(buf1));

	fifo_write(&p1, test_arr, 128);
	ok1(fifo_amt(&p1) == 128);
	ok1(fifo_avail(&p1) == 0);
	fifo_peek(&p1, output_buf, 128);
	ok1(memcmp(output_buf, test_arr, 128) == 0);
	
	for(i = 0; i < 128; i++) {
		fifo_pop(&p1, &elem);
		ok1(elem == test_arr[i] && fifo_amt(&p1) == 127 \
				&& fifo_avail(&p1) == 1);
		fifo_push(&p1, test_arr+128+i);
		ok1(fifo_amt(&p1) == 128 && fifo_avail(&p1) == 0);
		fifo_peek(&p1, output_buf, 128);
		ok1(memcmp(output_buf, test_arr+i, 128));
	}
	
#define TEST3AMT 2 + 3 + 128*3
	diag("----test3----\n#");
}

int main()
{
	int i, len, total_tests;
#define TESTN(_num) {test##_num, TEST##_num##AMT}
	struct test tests[] = {
		TESTN(1),
		TESTN(2),
		TESTN(3)
	};

	setlocale(LC_ALL,"");
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

