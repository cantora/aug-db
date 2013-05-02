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
	ok1(memcmp(test_arr+1, output_buf, 6*sizeof(int)) == 0);
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == (ARRAY_SIZE(buf1)));

#define TEST2AMT 2 + 3 + 11
	diag("----test2----\n#");
}

void test3() {
	struct fifo p1;
	int buf1[256];
	int output_buf[128];
	int zero[128];
	int buf_val[128];
	size_t i;

	for(i = 0; i < 128; i++) {
		buf_val[i] = 0xaabbccdd;
	}
	fifo_init(&p1, buf1, sizeof(int), 128);
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == 128);
	ok1(p1.read == 0);

	fifo_write(&p1, buf_val, 128);
	for(i = 0; i < 128; i++) {
		buf1[128+i] = 0x01234567;
	}
	memset(zero, 0, 128*sizeof(int));

	ok1(fifo_amt(&p1) == 128);
	ok1(fifo_avail(&p1) == 0);
	ok1(p1.read == 0);

	memset(output_buf, 0, 128);
	fifo_peek(&p1, output_buf, 128);
	ok1(memcmp(output_buf, buf_val, 128*sizeof(int)) == 0);

	memset(output_buf, 0, 128*sizeof(int));
	fifo_consume(&p1, output_buf, 96);
	ok1(memcmp(output_buf, buf_val, 96*sizeof(int)) == 0);
	ok1(memcmp(&output_buf[96], zero, 32*sizeof(int)) == 0);
	ok1(fifo_amt(&p1) == 32);
	ok1(fifo_avail(&p1) == 96);
	ok1(p1.read == 96);

	fifo_write(&p1, buf_val, 32);
	ok1(fifo_amt(&p1) == 64);
	ok1(fifo_avail(&p1) == 64);
	ok1(p1.read == 96);
	
	memset(output_buf, 0, 128*sizeof(int));
	fifo_peek(&p1, output_buf, 64);
	ok1(memcmp(output_buf, buf_val, 64*sizeof(int)) == 0);

#define TEST3AMT 3 + 3 + 1 + 5 + 3 + 1
	diag("----test3----\n#");
}

void test4() {
	struct fifo p1;
	int buf1[256];
	int output_buf[128];
	int zero[128];
	int buf_val[128];
	int alt_buf_val[128];
	size_t i;

	for(i = 0; i < 128; i++) {
		buf_val[i] = 0xaabbccdd;
		alt_buf_val[i] = 0x5ab8cc84;
	}
	fifo_init(&p1, buf1, sizeof(int), 128);
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == 128);
	ok1(p1.read == 0);

	fifo_write(&p1, buf_val, 128);
	for(i = 0; i < 128; i++) {
		buf1[128+i] = 0x01234567;
	}
	memset(zero, 0, 128*sizeof(int));

	ok1(fifo_amt(&p1) == 128);
	ok1(fifo_avail(&p1) == 0);
	ok1(p1.read == 0);

	memset(output_buf, 0, 128);
	fifo_peek(&p1, output_buf, 128);
	ok1(memcmp(output_buf, buf_val, 128*sizeof(int)) == 0);

	memset(output_buf, 0, 128*sizeof(int));
	fifo_consume(&p1, output_buf, 96);
	ok1(memcmp(output_buf, buf_val, 96*sizeof(int)) == 0);
	ok1(memcmp(&output_buf[96], zero, 32*sizeof(int)) == 0);
	ok1(fifo_amt(&p1) == 32);
	ok1(fifo_avail(&p1) == 96);
	ok1(p1.read == 96);

	fifo_write(&p1, buf_val, 32);
	ok1(fifo_amt(&p1) == 64);
	ok1(fifo_avail(&p1) == 64);
	ok1(p1.read == 96);
	
	memset(output_buf, 0, 128*sizeof(int));
	fifo_write(&p1, alt_buf_val, 64);
	ok1(fifo_amt(&p1) == 128);
	ok1(fifo_avail(&p1) == 0);
	ok1(p1.read == 96);

	ok1(memcmp(buf1, buf_val, 32*sizeof(int)) == 0);
	ok1(memcmp(buf1+32, alt_buf_val, 64*sizeof(int)) == 0);
	ok1(memcmp(buf1+96, buf_val, 32*sizeof(int)) == 0);

#define TEST4AMT 3 + 3 + 1 + 5 + 3 + 3 + 3
	diag("----test4----\n#");
}


void test5() {
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
	diag("++++test5++++");	
	
	fifo_init(&p1, buf1, sizeof(int), ARRAY_SIZE(buf1) );
	ok1(fifo_amt(&p1) == 0);
	ok1(fifo_avail(&p1) == ARRAY_SIZE(buf1));

	fifo_write(&p1, test_arr, 128);
	ok1(fifo_amt(&p1) == 128);
	ok1(fifo_avail(&p1) == 0);
	fifo_peek(&p1, output_buf, 128);
	ok1(memcmp(output_buf, test_arr, 128*sizeof(int)) == 0);
	
	for(i = 0; i < 256; i++) {
		fifo_pop(&p1, &elem);
		ok1(elem == test_arr[i] && fifo_amt(&p1) == 127 \
				&& fifo_avail(&p1) == 1);
		fifo_push(&p1, test_arr+128+i);
		ok1(fifo_amt(&p1) == 128 && fifo_avail(&p1) == 0);
		memset(output_buf, 0, 128*sizeof(int));
		fifo_peek(&p1, output_buf, 128);
		ok1(memcmp(output_buf, test_arr+i, 128*sizeof(int)));
	}
	
#define TEST5AMT 2 + 3 + 256*3
	diag("----test5----\n#");
}

int main()
{
	int i, len, total_tests;
#define TESTN(_num) {test##_num, TEST##_num##AMT}
	struct test tests[] = {
		TESTN(1),
		TESTN(2),
		TESTN(3),
		TESTN(4),
		TESTN(5)
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

