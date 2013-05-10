#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ccan/tap/tap.h>
#include <ccan/array_size/array_size.h>
#include <ccan/str/str.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

#include "test.h"
#include "db.h"

struct test {
	void (*fn)();
	int amt;
};

const char *FILENAME = "/tmp/db_test.sqlite";

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

void test1() {

	unlink(FILENAME);
	db_init(FILENAME);
	diag("++++test1++++");	

#define ADD_TEST_ENTRY(_idx) \
	db_add( \
		TEST_ENTRY_DATA(_idx), ARRAY_SIZE( TEST_ENTRY_DATA(_idx) )-1, 0, \
		TEST_ENTRY_TAGS(_idx), ARRAY_SIZE( TEST_ENTRY_TAGS(_idx) ) \
	)

	ADD_TEST_ENTRY(1);
	ADD_TEST_ENTRY(2);
	ADD_TEST_ENTRY(3);
	ADD_TEST_ENTRY(4);

	pass("added 4 test entries to db");
#define TEST1AMT 1
	diag("----test1----\n#");
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

void test2() {
	const char *queries[] = {"passwd"};
	struct db_query q;
	uint8_t *value;
	size_t size;
	int count, raw, id;

	db_init(FILENAME);
	diag("++++test2++++");	

	db_query_prepare(&q, 0, (const uint8_t **) queries, ARRAY_SIZE(queries), NULL, 0);
	count = 0;
	while(db_query_step(&q) == 0) {
		db_query_value(&q, &value, &size, &raw, &id);
		ok1(raw == 0);
		printf("#value: ");
		print_blob(value, size);
		printf("\n");
		talloc_free(value);
		count++;
	}

	ok1(count == 3);
	db_query_free(&q);

#define TEST2AMT 3 + 1
	diag("----test2----\n#");
	db_free();
}

void test3() {
	const char *queries[] = {"passwd"};
	const char *tags[] = {"sed"};
	struct db_query q;
	int count;

	db_init(FILENAME);
	diag("++++test3++++");	

	db_query_prepare(&q, 0, (const uint8_t **) queries, ARRAY_SIZE(queries), (const uint8_t **) tags, ARRAY_SIZE(tags));
	count = 0;
	while(db_query_step(&q) == 0) {
		count++;
	}

	ok1(count == 0);	
	db_query_free(&q);

#define TEST3AMT 1
	diag("----test3----\n#");
	db_free();
}


void test4() {
	const char *queries[] = {"gues"};
	const char *tags[] = {"cmdlin"};
	struct db_query q;
	uint8_t *value;
	size_t size;
	int count, raw, id;

	db_init(FILENAME);
	diag("++++test4++++");	

	db_query_prepare(&q, 0, (const uint8_t **) queries, ARRAY_SIZE(queries), (const uint8_t **) tags, ARRAY_SIZE(tags));
	count = 0;
	while(db_query_step(&q) == 0) {
		db_query_value(&q, &value, &size, &raw, &id);
		ok1(raw == 0);
		printf("#value: ");
		print_blob(value, size);
		printf("\n");
		talloc_free(value);
		count++;
	}

	ok1(count == 1);	
	db_query_free(&q);

#define TEST4AMT 1 + 1
	diag("----test4----\n#");
	db_free();
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
	for(i = 0; i < len; i++) {
		(*tests[i].fn)();
	}
	test_free_api();

	return exit_status();
}

