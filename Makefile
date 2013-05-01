
AUG_DIR			= aug
OUTPUT			= aug-db.so
BUILD			= ./build
MKBUILD			:= $(shell mkdir -p $(BUILD) )

CCAN_DIR		= ./libccan
LIBCCAN			= $(CCAN_DIR)/libccan.a
SQLITE_DIR		= ./sqlite3
INCLUDES		= -iquote"$(AUG_DIR)/include" -I$(CCAN_DIR) -iquote"src" -I$(SQLITE_DIR)
LIB				= -lrt $(LIBCCAN)
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
TEST_LIB		= -pthread -lncursesw -lpanel $(LIB)

default: all

.PHONY: all
all: $(SQLITE_DIR) $(LIBCCAN) $(OUTPUT) 

$(OUTPUT): $(OBJECTS) 
	$(CXX_CMD) -shared $+ $(LIB) -o $@

define cc-template
$(CXX_CMD) $(DEP_FLAGS) -fPIC -c $< -o $@
endef

$(BUILD)/%.o: src/%.c
	$(cc-template)

$(BUILD)/%.o: test/%.c $(LIBCCAN)
	$(CXX_CMD) $(DEP_FLAGS) -c $< -o $@

$(CCAN_DIR):
	git clone 'https://github.com/rustyrussell/ccan.git' tmp_ccan
	cd tmp_ccan/tools && ./create-ccan-tree -b make+config ../../$(CCAN_DIR) $(CCAN_MODULES)
	rm -rf tmp_ccan/
	cat $(CCAN_DIR)/Makefile-ccan | sed 's/CCAN_CFLAGS *=/override CCAN_CFLAGS +=/' > $(CCAN_DIR)/tmp.mk \
		&& cp $(CCAN_DIR)/tmp.mk $(CCAN_DIR)/Makefile-ccan && rm $(CCAN_DIR)/tmp.mk

$(LIBCCAN): $(CCAN_DIR)
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) CCAN_CFLAGS="-fPIC"

$(SQLITE_DIR):
	mkdir -p $(SQLITE_DIR)_tmp
	curl 'http://www.sqlite.org/sqlite-amalgamation-3071502.zip' > $(SQLITE_DIR)_tmp/sqlite.zip
	cd $(SQLITE_DIR)_tmp && unzip sqlite.zip \
		&& mv sqlite-amalgamation-* ../$(SQLITE_DIR) \
		&& rm sqlite.zip && cd .. && rmdir $(SQLITE_DIR)_tmp

$(BUILD)/%.o: $(SQLITE_DIR)/%.c $(SQLITE_DIR) 
	$(cc-template)

define test-program-template
$$(BUILD)/$(1): $$(BUILD)/$(1).o $$(filter-out $$(BUILD)/globals.o, $$(OBJECTS)) $$(BUILD)/tglobals.o $$(LIBCCAN)
	$(CXX_CMD) $$+ $$(TEST_LIB) -o $$@

$(1): $$(BUILD)/$(1)
	$(BUILD)/$(1) 
endef

.PHONY: run-tests
run-tests: tests $(foreach test, $(TEST_OUTPUTS), $(notdir $(test) ) )

.PHONY: tests
tests: $(TESTS)

.PHONY: $(TESTS)
$(foreach test, $(TESTS), $(eval $(call test-program-template,$(test)) ) )

.PHONY: libclean
libclean: clean
	rm -rf $(CCAN_DIR)
	rm -rf $(SQLITE_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT)


-include $(wildcard $(BUILD)/*.d )
