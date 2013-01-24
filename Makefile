
AUG_DIR			= aug
OUTPUT			= aug-db.so
BUILD			= ./build
MKBUILD			:= $(shell mkdir -p $(BUILD) )

CCAN_DIR		= ./libccan
LIBCCAN			= $(CCAN_DIR)/libccan.a
INCLUDES		= -iquote"$(AUG_DIR)/include" -I$(CCAN_DIR) -iquote"src"

OPTIMIZE		= -ggdb
CXX_FLAGS		= -Wall -Wextra $(INCLUDES) $(OPTIMIZE)
CXX_CMD			= gcc $(CXX_FLAGS)

SRCS			= $(notdir $(wildcard ./src/*.c) )
OBJECTS			= $(patsubst %.c, $(BUILD)/%.o, $(SRCS) ) 
DEP_FLAGS		= -MMD -MP -MF $(patsubst %.o, %.d, $@)

TESTS 			= $(notdir $(patsubst %.c, %, $(wildcard ./test/*_test.c) ) )
TEST_OUTPUTS	= $(foreach test, $(TESTS), $(BUILD)/$(test))
TEST_LIB		= -pthread $(LIBCCAN) -lncursesw

default: all

.PHONY: all
all: $(OUTPUT) 

$(OUTPUT): $(OBJECTS)
	$(CXX_CMD) -shared $+ -o $@

$(BUILD)/%.o: src/%.c
	$(CXX_CMD) $(DEP_FLAGS) -fPIC -c $< -o $@

$(BUILD)/%.o: test/%.c $(LIBCCAN)
	$(CXX_CMD) $(DEP_FLAGS) -c $< -o $@

$(CCAN_DIR):
	git clone 'https://github.com/rustyrussell/ccan.git' $(CCAN_DIR)

$(LIBCCAN): $(CCAN_DIR)
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) -f ./tools/Makefile tools/configurator/configurator
	$(CCAN_DIR)/tools/configurator/configurator > $(CCAN_DIR)/config.h
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) 

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

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT)


-include $(wildcard $(BUILD)/*.d )
