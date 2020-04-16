PROJECT := ether

SRC_DIR := ether
INC_DIR := ether/include

BIN_DIR := bin
OBJ_DIR := obj

ETHER_STDLIB_DIR := ether_stdlib
ETHER_STDLIB := $(ETHER_STDLIB_DIR)/bin/libether_stdlib.a

C_FILES := $(shell find $(SRC_DIR) -name "*.c")
ASM_FILES := $(shell find $(SRC_DIR) -name "*.asm")

OBJ_FILES := $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(C_FILES)))
OBJ_FILES += $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(ASM_FILES)))
BIN_FILE := $(BIN_DIR)/$(PROJECT)

CC := gcc
LD := gcc

CFLAGS := -I$(INC_DIR) -D_DEBUG -Wall -Wextra -Wshadow -std=c99 -m64 -g -O0
LDFLAGS := 

run: ether_stdlib $(BIN_FILE)
	@$(BIN_FILE) res/hello.eth
	ld -o $(BIN_DIR)/a.out res/hello.o -L $(dir $(ETHER_STDLIB)) -lether_stdlib

debug: $(ETHER_STDLIB) $(BIN_FILE)
	@gdb --args $(BIN_FILE) res/hello.eth

$(BIN_FILE): $(OBJ_FILES)
	@mkdir -p $(dir $@)
	$(LD) -o $@ $(OBJ_FILES)

$(OBJ_DIR)/%.c.o: %.c
	@mkdir -p $(OBJ_DIR)/$(dir $^)
	$(CC) -c $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.asm.o: %.asm
	@mkdir -p $(OBJ_DIR)/$(dir $^)
	nasm -felf64 -o $@ $^

ether_stdlib: 
	@echo "Making stdlib..."
	+$(MAKE) -C $(ETHER_STDLIB_DIR)

clean:
	@rm -rf $(OBJ_FILES)
	@rm -rf $(BIN_FILE)

.PHONY: run clean ether_stdlib
