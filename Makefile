CC = gcc

CFLAGS  = -std=c11 -g3 -Wall -Wextra -Wpedantic
CFLAGS += -Wshadow -Wfloat-equal -Wundef -Wunreachable-code -Wswitch-default \
		  -Wswitch-enum -Wpointer-arith -Wwrite-strings -Wstrict-prototypes

# debug flags
#CFLAGS += -Og -fno-omit-frame-pointer
#CFLAGS += -fanalyzer
#CFLAGS += -fsanitize=undefined -fsanitize=address

# profiling flags
#CFLAGS += -pg

# release flags
#CFLAGS += -DNDEBUG
CFLAGS += -march=native -mtune=native
CFLAGS += -O3 -ffast-math -funroll-loops
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -flto=auto

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
