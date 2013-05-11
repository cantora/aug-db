OS_NAME			:= $(shell uname)

AUG_DIR			= aug
OUTPUT			= aug-db.so
BUILD			= ./build
MKBUILD			:= $(shell mkdir -p $(BUILD) )

CCAN_DIR		= ./libccan
LIBCCAN			= $(CCAN_DIR)/libccan.a
SQLITE_DIR		= ./sqlite3
INCLUDES		= -iquote"$(AUG_DIR)/include" -I$(CCAN_DIR) -iquote"src" -I$(SQLITE_DIR)
DEFINES			= -DAUG_DB_DEBUG
OPTIMIZE		= -ggdb
CXX_FLAGS		= -Wall -Wextra $(INCLUDES) $(OPTIMIZE) $(DEFINES)
CXX_CMD			= gcc $(CXX_FLAGS)

SRCS			= $(notdir $(wildcard ./src/*.c) )
SQLITE_OBJECTS	= $(BUILD)/sqlite3.o
CCAN_MODULES	= tap str_talloc talloc array_size 
OBJECTS			= $(patsubst %.c, $(BUILD)/%.o, $(SRCS) ) 
OBJECTS			+=$(SQLITE_OBJECTS) 
DEP_FLAGS		= -MMD -MP -MF $(patsubst %.o, %.d, $@)

TESTS 			= $(notdir $(patsubst %.c, %, $(wildcard ./test/*_test.c) ) )
TEST_OUTPUTS	= $(foreach test, $(TESTS), $(BUILD)/$(test))
TEST_LIB		= -pthread -lpanel

VALGRIND		= valgrind --leak-check=yes --suppressions=./.aug-db.supp

ifeq ($(OS_NAME), Darwin)
	CCAN_COMMENT_LIBRT		= $(CCAN_DIR)/tools/Makefile
	CCAN_PATCHES			+= $(CCAN_DIR)/.patch_rt

	SO_FLAGS	= -dynamiclib -Wl,-undefined,dynamic_lookup 
	TEST_LIB	+= -lncurses -liconv
	VALGRIND	+= --dsymutil=yes
#	on OSX running valgrind with sqlite3 causes a segfault
	VALGRIND_OK	= $(filter-out db_test query_test, $(TESTS))
else
	SO_FLAGS	= -shared 
	TEST_LIB	+= -lncursesw
	LIB			= -lrt
	VALGRIND_OK	= $(TESTS)
endif
LIB				+= $(LIBCCAN)
TEST_LIB		+= $(LIB)

default: all

.PHONY: all
all: $(OUTPUT)

$(OUTPUT): $(AUG_DIR) $(LIBCCAN) $(SQLITE_DIR) $(OBJECTS)
	$(CXX_CMD) $(SO_FLAGS) $(OBJECTS) $(LIBCCAN) -o $@

define cc-template
$(CXX_CMD) $(DEP_FLAGS) -fPIC -c $< -o $@
endef

$(BUILD)/%.o: src/%.c 
	$(cc-template)

$(BUILD)/%.o: test/%.c $(LIBCCAN)
	$(CXX_CMD) $(DEP_FLAGS) -c $< -o $@

$(AUG_DIR):
	@echo "aug not found at directory $(AUG_DIR)"
	@false

.PHONY: clean_ccan
clean_ccan:
	rm -rf $(CCAN_DIR)

$(CCAN_DIR)/.touch: 
	rm -rf $(CCAN_DIR)
	git clone 'https://github.com/cantora/ccan.git' $(CCAN_DIR)
	cd $(CCAN_DIR) && git checkout cantora
	touch $@

$(CCAN_DIR)/.patch_rt: $(CCAN_DIR)/.touch
	sed 's/\(LDLIBS = -lrt\)/#\1/' $(CCAN_COMMENT_LIBRT) \
			> $(CCAN_COMMENT_LIBRT).tmp
	mv $(CCAN_COMMENT_LIBRT).tmp $(CCAN_COMMENT_LIBRT)
	touch $@

$(CCAN_DIR)/config.h: $(CCAN_DIR)/.touch $(CCAN_PATCHES)
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) -f ./tools/Makefile tools/configurator/configurator
	$(CCAN_DIR)/tools/configurator/configurator > $@

$(LIBCCAN): $(CCAN_DIR)/config.h
	cd $(CCAN_DIR) && CFLAGS="-fPIC" $(MAKE) $(MFLAGS)
	touch $@

$(SQLITE_DIR):
	mkdir -p $(SQLITE_DIR)_tmp
	curl 'http://www.sqlite.org/sqlite-amalgamation-3071502.zip' > $(SQLITE_DIR)_tmp/sqlite.zip
	cd $(SQLITE_DIR)_tmp && unzip sqlite.zip \
		&& mv sqlite-amalgamation-* ../$(SQLITE_DIR) \
		&& rm sqlite.zip && cd .. && rmdir $(SQLITE_DIR)_tmp

$(BUILD)/%.o: $(SQLITE_DIR)/%.c
	$(cc-template)

define test-program-template
$$(BUILD)/$(1): $$(BUILD)/$(1).o $$(filter-out $$(BUILD)/globals.o, $$(OBJECTS)) $$(BUILD)/tglobals.o $$(LIBCCAN)
	$(CXX_CMD) $$+ $$(TEST_LIB) -o $$@

$(1): $$(BUILD)/$(1)
	$(BUILD)/$(1)

valgrind-$(1): $$(BUILD)/$(1)
	$(VALGRIND) --log-file=$(BUILD)/$(1).grind $(BUILD)/$(1)
	@if [ -n "$$$$(grep -E 'ERROR SUMMARY: [1-9][0-9]* errors' -o $(BUILD)/$(1).grind)" ]; then \
		echo $$@ has valgrind errors; \
		false; \
	fi
endef

.PHONY: tests
tests: $(TESTS)
	@echo all tests ok

.PHONY: tests
valgrind-tests: $(foreach test, $(VALGRIND_OK), valgrind-$(test))
	@echo all tests ok under valgrind

.PHONY: $(TESTS)
$(foreach test, $(TESTS), $(eval $(call test-program-template,$(test)) ) )

.PHONY: libclean
libclean: clean clean_ccan
	rm -rf $(SQLITE_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT)


-include $(wildcard $(BUILD)/*.d )
