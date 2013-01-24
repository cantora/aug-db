
AUG_DIR			= aug
OUTPUT			= aug-db.so
BUILD			= ./build
MKBUILD			:= $(shell mkdir -p $(BUILD) )

CCAN_DIR		= ./libccan
LIBCCAN			= $(CCAN_DIR)/libccan.a
LIB 			= -pthread $(LIBCCAN)
INCLUDES		= -iquote"$(AUG_DIR)/include" -I$(CCAN_DIR)

CXX_FLAGS		= -Wall -Wextra $(INCLUDES)
CXX_CMD			= gcc $(CXX_FLAGS)

SRCS			= $(notdir $(wildcard ./src/*.c) )
OBJECTS			= $(patsubst %.c, $(BUILD)/%.o, $(SRCS) ) 
DEP_FLAGS		= -MMD -MP -MF $(patsubst %.o, %.d, $@)

default: all

.PHONY: all
all: $(OUTPUT) 

$(OUTPUT): $(OBJECTS)
	$(CXX_CMD) -shared $+ -o $@

$(BUILD)/%.o: src/%.c
	$(CXX_CMD) $(DEP_FLAGS) -fPIC -c $< -o $@

$(CCAN_DIR):
	git clone 'https://github.com/rustyrussell/ccan.git' $(CCAN_DIR)

$(LIBCCAN): $(CCAN_DIR)
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) -f ./tools/Makefile tools/configurator/configurator
	$(CCAN_DIR)/tools/configurator/configurator > $(CCAN_DIR)/config.h
	cd $(CCAN_DIR) && $(MAKE) $(MFLAGS) 

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT)


-include $(wildcard $(BUILD)/*.d )
