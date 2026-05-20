CC = gcc
FLEX = win_flex
BISON = win_bison
CFLAGS = -std=c99 -Wall -Wextra -g -Isrc -Ibuild/generated
LDFLAGS =

TARGET = minilang
GEN_DIR = build/generated
OBJ_DIR = build/obj

OBJS = \
	$(OBJ_DIR)/parser.tab.o \
	$(OBJ_DIR)/lex.yy.o \
	$(OBJ_DIR)/ast.o \
	$(OBJ_DIR)/symbol_table.o \
	$(OBJ_DIR)/semantic.o \
	$(OBJ_DIR)/tac.o \
	$(OBJ_DIR)/optimizer.o \
	$(OBJ_DIR)/codegen_c.o \
	$(OBJ_DIR)/main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

$(GEN_DIR):
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(GEN_DIR)' | Out-Null"

$(OBJ_DIR):
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(OBJ_DIR)' | Out-Null"

$(GEN_DIR)/parser.tab.c $(GEN_DIR)/parser.tab.h: src/parser.y src/ast.h | $(GEN_DIR)
	$(BISON) -d -o $(GEN_DIR)/parser.tab.c src/parser.y

$(GEN_DIR)/lex.yy.c: src/lexer.l $(GEN_DIR)/parser.tab.h | $(GEN_DIR)
	$(FLEX) -o $(GEN_DIR)/lex.yy.c src/lexer.l

$(OBJ_DIR)/parser.tab.o: $(GEN_DIR)/parser.tab.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/lex.yy.o: $(GEN_DIR)/lex.yy.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	powershell -NoProfile -Command "$$paths = @('build','minilang.exe','minilang','out.c','out.exe'); foreach ($$p in $$paths) { if (Test-Path -LiteralPath $$p) { Remove-Item -LiteralPath $$p -Recurse -Force } }"

.PHONY: all clean
