default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h pingpong.c datatypes.h
SRC = main.c
OBJ = $(SRC:.c=.o)
OUT = so_p03

# Compiler, Linker Defines
CC = gcc
RM = rm -rf

# Build
build:
	@$(CC) $(addprefix src/, $(SRC)) $(addprefix ../lib/, $(INCL)) -o $(OUT)_1

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT)_3 $(OUT)_2 $(OUT)_1
