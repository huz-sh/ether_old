PROJECT := ether
SRC_DIR := ether
OBJ_DIR := obj
BIN_DIR := bin
DEP_DIR := dep
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
DEP_FILES := $(wildcard $(DEP_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
OBJ_FILES += $(patsubst $(DEP_DIR)/%.c,$(OBJ_DIR)/%.o,$(DEP_FILES))
LDFLAGS :=
CFLAGS := -D_DEBUG -Wall -Wextra -Wshadow -std=c99 -m64 -g -O0 -Iether/include

ifeq ($(OS), Windows_NT)
	PROJECT := $(PROJECT).exe
endif

run: $(BIN_DIR)/$(PROJECT)
	@$(BIN_DIR)/$(PROJECT) res/expr_test.eth

$(BIN_DIR)/$(PROJECT): $(OBJ_FILES)
	@mkdir -p $(dir $@)
	@gcc -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@gcc $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(DEP_DIR)/%.c
	@mkdir -p $(dir $@)
	@gcc $(CFLAGS) -c -o $@ $<

clean:
ifeq ($(OS), Windows_NT)
	@del $(BIN_DIR)\$(PROJECT)
	@del $(OBJ_FILES)
else
	@rm -f $(BIN_DIR)/$(PROJECT)
	@rm -f $(OBJ_FILES)
endif
