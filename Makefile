CC = gcc

CFLAGS  = -std=c99 -g3 -Wall -Wextra -Wpedantic
CFLAGS += -Wshadow -Wfloat-equal -Wundef -Wunreachable-code -Wswitch-default
CFLAGS += -Wswitch-enum -Wpointer-arith -Wwrite-strings -Wstrict-prototypes

# debug flags
#CFLAGS += -Og -fsanitize=undefined,address #-fanalyzer

# profiling flags
#CFLAGS += -Og -pg

# release flags
CFLAGS += -O2 -march=native -flto=auto -DNDEBUG

SRC = $(shell find src -type f -name '*c')
OBJ = $(SRC:%.c=%.o)
RUN = spell.c
BIN = $(RUN:%.c=%)

CFLAGS += -MMD -MP
DEP = $(OBJ:.o=.d) $(BIN:=.d)
-include $(DEP)

$(BIN): %: %.c $(OBJ) Makefile
	-$(CC) $(CFLAGS) -Isrc $< $(OBJ) -o $@

$(OBJ): %.o: %.c Makefile
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

clean:
	-rm -rf $(BIN) $(OBJ) $(DEP) perf.data*

check:
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem *.c

perf: $(BIN)
	perf record ./$(BIN) && perf report --percent-limit 2

.PHONY: clean check perf
