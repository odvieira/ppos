default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h pingpong.c datatypes.h
SRC = pingpong-mqueue.c
OBJ = $(SRC:.c=.o)
OUT = ppos_12

# Compiler, Linker Defines
CC = gcc
RM = rm -rf

# Build
build:
	@$(CC) $(SRC) $(addprefix ./lib_unstable/, $(INCL)) -lm -o $(OUT);

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT);
