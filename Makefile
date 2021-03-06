PROJECT := ether

SRC_DIR := ether
INC_DIR := ether/include

BIN_DIR := bin
OBJ_DIR := obj

C_FILES := $(shell find $(SRC_DIR) -name "*.c")
ASM_FILES := $(shell find $(SRC_DIR) -name "*.asm")

OBJ_FILES := $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(C_FILES)))
OBJ_FILES += $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(ASM_FILES)))
BIN_FILE := $(BIN_DIR)/$(PROJECT)

CC := gcc
LD := gcc

CFLAGS := -I$(INC_DIR) -D_DEBUG -Wall -Wextra -Wshadow -std=c99 -m64 -g -O0
LDFLAGS :=

run: $(BIN_FILE)
	$(BIN_FILE) res/hello.eth
	gcc -o $(BIN_DIR)/a.out res/hello.o -lc -lm -Wl,--dynamic-linker=/usr/lib64/ld-linux-x86-64.so.2 

debug: $(ETHER_STDLIB) $(BIN_FILE)
	gdb --args $(BIN_FILE) res/hello.eth

$(BIN_FILE): $(OBJ_FILES)
	echo $(BIN_FILE)
	mkdir -p $(dir $@)
	$(LD) -o $@ $(OBJ_FILES)

$(OBJ_DIR)/%.c.o: %.c
	mkdir -p $(OBJ_DIR)/$(dir $^)
	$(CC) -c $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.asm.o: %.asm
	mkdir -p $(OBJ_DIR)/$(dir $^)
	nasm -felf64 -o $@ $^

clean:
	rm -rf $(OBJ_FILES)
	rm -rf $(BIN_FILE)

loc:
	find ether -name "*.c" -or \
						-name "*.h" -or \
						-name "*.asm" | xargs cat | wc -l

.PHONY: run clean loc
