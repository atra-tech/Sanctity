TARGET=gem-defence

CC=gcc

SRC=src
ASM=asm
OBJ=obj
BUILD_DIR=bin

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
ASMS=$(patsubst $(SRC)/%.c, $(ASM)/%.S, $(SRCS))

LIBDIR=lib
LIBS=$(LIBDIR)/atra.a $(LIBDIR)/liblua.a

HEADER_FLAGS=-Iinclude/atra -Iinclude/lua

INC_DIRS=$(shell find $(SRC) -type d)
INC_FLAGS=$(addprefix -I,$(INC_DIRS))

LDFLAGS=-lm -ldl -lSDL2 -lSDL2_image -lSDL2_net -lGL -lopenal
CFLAGS=$(INC_FLAGS) $(HEADER_FLAGS) -MP -MMD -Og -Wall -Wextra -Wpedantic -g -std=c99

$(BUILD_DIR)/$(TARGET): $(BUILD_DIR) $(ASM) $(OBJ) $(ASMS) $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o $@ $(LDFLAGS)

.PHONY: clean

$(ASM)/%.S: $(SRC)/%.c $(SRC)/%.h
	$(CC) $(CFLAGS) -S $< -o $@

$(ASM)/%.S: $(SRC)/%.c
	$(CC) $(CFLAGS) -S $< -o $@

$(OBJ)/%.o: $(SRC)/%.c $(SRC)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ):
	mkdir $@

$(ASM):
	mkdir $@

$(LIBDIR):
	mkdir $@

$(BUILD_DIR):
	mkdir $@

clean:
	$(RM) -r $(OBJ) $(BUILD_DIR)
