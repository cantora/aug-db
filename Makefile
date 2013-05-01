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

ifeq ($(OS_NAME), Darwin)
	CCAN_COMMENT_LIBRT		= $(CCAN_DIR)/tools/Makefile
	CCAN_PATCH_CFLAGS		= $(CCAN_DIR)/tools/Makefile-ccan
	SO_FLAGS	= -dynamiclib -Wl,-undefined,dynamic_lookup 
	TEST_LIB	+= -lncurses -liconv
else
	SO_FLAGS	= -shared 
	TEST_LIB	+= -lncurses
	LIB			= -lrt
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

$(CCAN_DIR)/.touch:
	git clone 'https://github.com/rustyrussell/ccan.git' $(CCAN_DIR) \
		&& touch $@

$(CCAN_DIR)/.patch_rt: $(CCAN_DIR)/.touch
	[ -n "$(CCAN_COMMENT_LIBRT)" ] \
		&& sed 's/\(LDLIBS = -lrt\)/#\1/' $(CCAN_COMMENT_LIBRT) \
			> $(CCAN_COMMENT_LIBRT).tmp \
		&& mv $(CCAN_COMMENT_LIBRT).tmp $(CCAN_COMMENT_LIBRT)
	touch $@
CCAN_PATCHES 	= $(CCAN_DIR)/.patch_cflags

$(CCAN_DIR)/.patch_cflags: $(CCAN_DIR)/.touch
	[ -n "$(CCAN_PATCH_CFLAGS)" ] && \
		cat $(CCAN_PATCH_CFLAGS) \
		| sed 's/CCAN_CFLAGS *=/override CCAN_CFLAGS +=/' \
			> $(CCAN_DIR)/tmp.mk \
		&& cp $(CCAN_DIR)/tmp.mk $(CCAN_PATCH_CFLAGS) \
		&& rm $(CCAN_DIR)/tmp.mk
	touch $@
CCAN_PATCHES	+=$(CCAN_DIR)/.patch_rt

$(CCAN_DIR)/config.h: $(CCAN_PATCHES)
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) -f ./tools/Makefile tools/configurator/configurator
	$(CCAN_DIR)/tools/configurator/configurator > $@

$(LIBCCAN): $(CCAN_DIR)/config.h
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) CCAN_CFLAGS="-fPIC" 
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
