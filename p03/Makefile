default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h pingpong.c datatypes.h
SRC = main.c
OBJ = $(SRC:.c=.o)
OUT = ppos_3

# Compiler, Linker Defines
CC = gcc
RM = rm -rf

# Build
build:
	@$(CC) $(addprefix src/, $(SRC)) $(addprefix ../lib_0-4/, $(INCL)) -o $(OUT);

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT);
