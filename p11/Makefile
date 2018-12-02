default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h pingpong.c datatypes.h
SRC = pingpong-barrier.c
OBJ = $(SRC:.c=.o)
OUT = ppos_11

# Compiler, Linker Defines
CC = gcc
RM = rm -rf

# Build
build:
	@$(CC) $(SRC) $(addprefix ../lib_unstable/, $(INCL)) -o $(OUT);

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT);
