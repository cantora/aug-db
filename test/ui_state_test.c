#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ccan/tap/tap.h>
#include <ccan/array_size/array_size.h>
#include <time.h>
#include <string.h>

#include "test.h"
#include "ui_state.h"

struct test {
	void (*fn)();
	int amt;
};

void test1() {
	struct fifo p1;
	int buf1[256];
	int outbuf1[256];
	const int *query;
	size_t i, query_len;
	const char queries[] = "ssh\nfind / -name\n\recho 'stuff'\r\nman junk\r";
	int iqueries[ARRAY_SIZE(queries)-1];

	for(i = 0; i < ARRAY_SIZE(queries)-1; i++) {
		iqueries[i] = queries[i];
	}
	fifo_init(&p1, buf1, sizeof(int), ARRAY_SIZE(buf1));
	fifo_write(&p1, iqueries, ARRAY_SIZE(iqueries));

	diag("++++test1++++");	
	
	ui_state_init();
	ok1(ui_state_current() == UI_STATE_QUERY);
	ok1(ui_state_query_run(0) == 0);
	ui_state_query_value(&query, &query_len);
	ok1(query_len == 0);

	ok1(ui_state_consume(&p1) == 4);
	ok1(ui_state_current() == UI_STATE_QUERY);
	ok1(ui_state_query_run(1) == 1);
	ok1(ui_state_query_run(0) == 0);
	ui_state_query_value(&query, &query_len);
	ok1(query_len == 3);
	ok1(memcmp(query, iqueries, 3) == 0);
	ui_state_query_value_reset();
	fifo_peek_all(&p1, outbuf1);
	ok1(memcmp(outbuf1, iqueries + query_len+1, 
			ARRAY_SIZE(iqueries) - query_len-1) == 0);
	
	ok1(ui_state_consume(&p1) == 13);
	ok1(ui_state_current() == UI_STATE_QUERY);
	ok1(ui_state_query_run(1) == 1);
	ui_state_query_value(&query, &query_len);
	ok1(query_len == 12);
	ok1(memcmp(query, iqueries+4, 12) == 0);
	ui_state_query_value_reset();
	fifo_peek_all(&p1, outbuf1);
	ok1(memcmp(outbuf1, iqueries + 4 + query_len+1, 
			ARRAY_SIZE(iqueries) - query_len-1 - 4) == 0);

	ok1(ui_state_consume(&p1) == 14);
	ok1(ui_state_current() == UI_STATE_QUERY);
	ok1(ui_state_query_run(1) == 1);
	ui_state_query_value(&query, &query_len);
	ok1(query_len == 12);
	ok1(memcmp(query, iqueries+18, 12) == 0);
	ui_state_query_value_reset();
	fifo_peek_all(&p1, outbuf1);
	ok1(memcmp(outbuf1, iqueries + 18 + query_len+1, 
			ARRAY_SIZE(iqueries) - query_len-1 - 18) == 0);

	ok1(ui_state_consume(&p1) == 10);
	ok1(ui_state_current() == UI_STATE_QUERY);
	ok1(ui_state_query_run(1) == 1);
	ui_state_query_value(&query, &query_len);
	ok1(query_len == 8);
	ok1(memcmp(query, iqueries+32, 8) == 0);
	ui_state_query_value_reset();
	fifo_peek_all(&p1, outbuf1);
	ok1(memcmp(outbuf1, iqueries + 32 + query_len+1, 
			ARRAY_SIZE(iqueries) - query_len-1 - 32) == 0);
	
	ok1(fifo_amt(&p1) == 0);
	ok1(ui_state_consume(&p1) == 0);
	ok1(ui_state_current() == UI_STATE_QUERY);
	ok1(ui_state_query_run(0) == 0);
	ui_state_query_value(&query, &query_len);
	ok1(query_len == 0);
	
#define TEST1AMT 3 + 7 + 6 + 6 + 6 + 5
	diag("----test1----\n#");
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

