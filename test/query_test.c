#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ccan/tap/tap.h>
#include <ccan/array_size/array_size.h>
#include <ccan/str/str.h>
#include <unistd.h>
#include <locale.h>

#include "test.h"
#include "db.h"
#include "query.h"
#include "encoding.h"

struct test {
	void (*fn)();
	int amt;
};


const char *fn = "/tmp/db_test.sqlite";

#define TEST_ENTRY_DATA(_num) \
	entry ## _num ## data

#define TEST_ENTRY_TAGS(_num) \
	entry ## _num ## tags

#define TEST_ENTRY(_num, _str, ...) \
	const char TEST_ENTRY_DATA(_num)[] = _str; \
	const char *TEST_ENTRY_TAGS(_num)[] = { __VA_ARGS__ }

TEST_ENTRY(1, "awk '{ print }' /etc/passwd", "awk", "cmdline examples");
TEST_ENTRY(2, "awk -F\":\" '{ print $1 }' /etc/passwd", "awk", "cmdline examples");
TEST_ENTRY(3, "awk '/guest-[a-zA-Z]:/ { print $1 }' /etc/passwd", "awk", "cmdline examples");
TEST_ENTRY(4, "sed -i 's/old/new/g' file", "in place sed", "sed", "cmdline examples");

void initdb() {

	unlink(fn);
	db_init(fn);
	diag("++++initdb++++");	

#define ADD_TEST_ENTRY(_idx) \
	db_add( \
		TEST_ENTRY_DATA(_idx), ARRAY_SIZE( TEST_ENTRY_DATA(_idx) )-1, 0, \
		TEST_ENTRY_TAGS(_idx), ARRAY_SIZE( TEST_ENTRY_TAGS(_idx) ) \
	)

	ADD_TEST_ENTRY(1);
	ADD_TEST_ENTRY(2);
	ADD_TEST_ENTRY(3);
	ADD_TEST_ENTRY(4);

	diag("----initdb----\n#");
	db_free();
}

void print_blob(const uint8_t *value, size_t size) {
	size_t i;
	for(i = 0; i < size; i++) {
		if(value[i] >= 0x20 && value[i] <= 0x7e)
			printf("%c", value[i]);
		else
			printf("\\x%02x", value[i]);
	}
}

void test_pre() {
	db_init(fn);
	assert(encoding_init() == 0);	
}

void test_suf() {
	encoding_free();
	db_free();
}

void test1() {
	size_t size, i;
	struct query q;
	uint8_t *data;
	int raw, id;

	test_pre();
	diag("++++test1++++");	
	diag("test basics");

	memset(&q, 0, sizeof(q));
	query_init(&q);
	ok1(query_clear(&q) == 0);

	query_value(&q, NULL, &size);
	ok1(size == 0);

	ok1(query_size(&q) == 0);
	
	ok1(query_delete(&q) == 0);
	ok1(query_offset_decr(&q) == 0);
	ok1(query_offset_incr(&q) == 0);

	ok1(query_add_ch(&q, 'A') != 0);
	ok1(query_delete(&q) != 0);	
	ok1(query_size(&q) == 0);

	query_prepare(&q);
	query_next(&q, &data, &size, &raw, &id);
	ok1(raw == 0);
	ok1(id == 1);
	printf("#data: ");
	for(i = 0; i < size; i++) {
		printf("%c", data[i]);		
	}
	printf("\n");
	ok1(size == 27);
	talloc_free(data);
	ok1(query_finalize(&q) == 0);
	
#define TEST1AMT 1 + 1 + 1 + 3 + 3 + 4
	diag("----test1----\n#");
	test_suf();
}

void test2() {
	size_t size, i;
	struct query q;
	char value[] = "guest";
	const uint32_t *val;
	uint8_t *data;
	int raw, id;

	test_pre();
	diag("++++test2++++");	
	diag("test non-empty query value");

	memset(&q, 0, sizeof(q));
	query_init(&q);

	for(i = 0; i < ARRAY_SIZE(value)-1; i++) {
		ok1(query_add_ch(&q, value[i]) != 0);
		ok1(query_size(&q) == i+1);
	}		

	query_value(&q, &val, &size);	
	ok1(size == 5);
	for(i = 0; i < size; i++) {
		ok1(val[i] == (uint32_t) value[i]);
	}

	query_prepare(&q);
	query_next(&q, &data, &size, &raw, &id);
	ok1(size == 48);
	ok1(raw == 0);

	printf("#data: ");
	for(i = 0; i < size; i++) {
		printf("%c", data[i]);		
	}
	printf("\n");
	
	talloc_free(data);
	query_finalize(&q);


#define TEST2AMT 2*5 + 1 + 5 + 2
	diag("----test2----\n#");
	test_suf();
}

static int g_count;
int cb_fn(uint8_t *data, size_t n, int raw, int id, int i, void *user) {
	ok1(g_count == i);
	ok1(raw == 0);
	ok1(id == i+1);
	ok1(user == (void *)0xc0ffee);
	printf("#data: ");
	print_blob(data, n);	
	printf("\n");
	g_count++;

	return 0;
}

void test3() {
	struct query q;

	test_pre();
	diag("++++test3++++");	
	diag("test query_foreach_result");

	memset(&q, 0, sizeof(q));
	query_init(&q);

	ok1(query_foreach_result(&q, cb_fn, (void *)0xc0ffee) == 4);
	
#define TEST3AMT 4*4 + 1
	diag("----test3----\n#");
	test_suf();
}

void test4() {
	size_t size, i;
	struct query q;
	char value[] = "guest";
	uint8_t *data;
	int raw, id;

	test_pre();
	diag("++++test4++++");	
	diag("test query_first_result");

	memset(&q, 0, sizeof(q));
	query_init(&q);
	for(i = 0; i < ARRAY_SIZE(value)-1; i++) {
		ok1(query_add_ch(&q, value[i]) != 0);
	}		

	ok1(query_first_result(&q, &data, &size, &raw, &id) == 0);
	ok1(size > 0);
	diag("size: %zu", size);
	printf("#data: ");
	print_blob(data, size);
	printf("\n");
	ok1(size == 48);
	ok1(raw == 0);
	ok1(id == 3);

	talloc_free(data);
		
#define TEST4AMT 5 + 2 + 3
	diag("----test4----\n#");
	test_suf();
}

int main()
{
	int i, len, total_tests;
#define TESTN(_num) {test##_num, TEST##_num##AMT}
	struct test tests[] = {
		TESTN(1),
		TESTN(2),
		TESTN(3),
		TESTN(4)
	};

	setlocale(LC_ALL,"");
	total_tests = 0;
	len = ARRAY_SIZE(tests);
	for(i = 0; i < len; i++) {
		total_tests += tests[i].amt;
	}

	plan_tests(total_tests);

	test_init_api();
	initdb();
	for(i = 0; i < len; i++) {
		(*tests[i].fn)();
	}
	test_free_api();

	return exit_status();
}

