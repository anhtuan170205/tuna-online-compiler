CC    = gcc
FLEX  = flex
BISON = bison

CFLAGS = -Wall -Wextra -g -Iinclude -Ibuild

TARGET = tuna

SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build

PARSER_C = $(BUILD_DIR)/tuna.tab.c
PARSER_H = $(BUILD_DIR)/tuna.tab.h
LEX_C    = $(BUILD_DIR)/lex.yy.c

OBJS = \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/ast.o \
	$(BUILD_DIR)/env.o \
	$(BUILD_DIR)/interp.o \
	$(BUILD_DIR)/lex.yy.o \
	$(BUILD_DIR)/tuna.tab.o

all: $(TARGET)

$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(PARSER_C) $(PARSER_H): $(SRC_DIR)/tuna.y | $(BUILD_DIR)
	$(BISON) -d -o $(PARSER_C) $(SRC_DIR)/tuna.y

$(LEX_C): $(SRC_DIR)/tuna.l $(PARSER_H) | $(BUILD_DIR)
	$(FLEX) -o $(LEX_C) $(SRC_DIR)/tuna.l

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(PARSER_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast.c $(INC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/env.o: $(SRC_DIR)/env.c $(INC_DIR)/env.h $(INC_DIR)/value.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interp.o: $(SRC_DIR)/interp.c $(INC_DIR)/env.h $(PARSER_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lex.yy.o: $(LEX_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tuna.tab.o: $(PARSER_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
