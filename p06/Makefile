default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h pingpong.c datatypes.h
SRC = pingpong-contab.c
OBJ = $(SRC:.c=.o)
OUT = ppos_6

# Compiler, Linker Defines
CC = gcc
RM = rm -rf

# Build
build:
	@$(CC) $(addprefix src/, $(SRC)) $(addprefix ./lib/, $(INCL)) -o $(OUT);
	@$(CC) $(addprefix src/, pingpong-contab-prio.c) $(addprefix ./lib/, $(INCL)) -o $(OUT)_2;

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT);
