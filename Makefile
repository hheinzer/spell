CC = gcc

CFLAGS  = -std=c99 -g3 -Wall -Wextra -Wpedantic
CFLAGS += -Wshadow -Wfloat-equal -Wundef -Wunreachable-code -Wswitch-default \
		  -Wswitch-enum -Wpointer-arith -Wwrite-strings -Wstrict-prototypes

# debug flags
#CFLAGS += -Og
#CFLAGS += -fanalyzer
#CFLAGS += -fsanitize=undefined -fsanitize=address

# profiling flags
#CFLAGS += -pg

# release flags
CFLAGS += -DNDEBUG
CFLAGS += -march=native -O2

spell: spell.c

default: spell

clean:
	-rm -rf spell perf.data*

check:
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem spell.c

perf: spell
	perf record ./spell
	perf report --percent-limit 2

.PHONY: default clean check perf
