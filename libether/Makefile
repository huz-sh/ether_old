PROJECT := libether

SRC_DIR := libether
INC_DIR := 

BIN_DIR := bin
OBJ_DIR := obj

C_FILES := $(shell find $(SRC_DIR) -name "*.c")
ASM_FILES := $(shell find $(SRC_DIR) -name "*.asm")

OBJ_FILES := $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(C_FILES)))
OBJ_FILES += $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(ASM_FILES)))
BIN_FILE := $(BIN_DIR)/$(PROJECT).a

CC := gcc
ASM := nasm
AR := ar

CFLAGS := -w -g -nostdlib
ASMFLAGS := -felf64
ARFLAGS := rcs

$(BIN_FILE): $(OBJ_FILES)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $$(find $(OBJ_DIR)/ -name "*.c.o" -or -name "*.asm.o")

$(OBJ_DIR)/%.c.o: %.c
	@mkdir -p $(OBJ_DIR)/$(dir $^)
	$(CC) -c $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.asm.o: %.asm
	@mkdir -p $(OBJ_DIR)/$(dir $^)
	$(ASM) $(ASMFLAGS) -o $@ $^

clean:
	rm -rf $(OBJ_FILES)
	rm -rf $(BIN_FILE)

.PHONY: run clean
