
AUG_DIR			= aug
OUTPUT			= aug-db.so
BUILD			= ./build
MKBUILD			:= $(shell mkdir -p $(BUILD) )
INCLUDES		= -iquote"$(AUG_DIR)/include"
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

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT)


-include $(wildcard $(BUILD)/*.d )
