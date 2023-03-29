CC = gcc

CFLAGS  = -std=c99 -g3 -Wall -Wextra -Wpedantic
CFLAGS += -Wshadow -Wfloat-equal -Wundef -Wunreachable-code -Wswitch-default
CFLAGS += -Wswitch-enum -Wpointer-arith -Wwrite-strings -Wstrict-prototypes

# debug flags
#CFLAGS += -Og -fsanitize=undefined,address
#CFLAGS += -fanalyzer -Wno-analyzer-malloc-leak

# profiling flags
#CFLAGS += -Og -pg

# release flags
CFLAGS += -O2 -march=native -flto=auto -DNDEBUG

SRC = $(shell find src -type f -name '*c')
OBJ = $(SRC:%.c=%.o)
BIN = spell

default: $(BIN)

CFLAGS += -MMD -MP
DEP = $(OBJ:.o=.d) $(BIN:=.d)
-include $(DEP)

$(OBJ): %.o: %.c Makefile
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(BIN): %: %.c $(OBJ) Makefile
	-$(CC) $(CFLAGS) -Isrc $< $(OBJ) -o $@

clean:
	-rm -rf $(BIN) $(OBJ) $(DEP) perf.data*

check:
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem \
		--project=compile_commands.json

perf: $(BIN)
	perf record ./$(BIN) && perf report --percent-limit 2

.PHONY: default clean check perf
