default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h pingpong.c datatypes.h
SRC = pingpong-semaphore.c
OBJ = $(SRC:.c=.o)
OUT = ppos_10

# Compiler, Linker Defines
CC = gcc
RM = rm -rf

# Build
build:
	@$(CC) $(SRC) $(addprefix ./lib10_1/, $(INCL)) -o $(OUT);
	@$(CC) pingpong-racecond.c $(addprefix ./lib10_1/, $(INCL)) -o $(OUT)_race;
	@$(CC) pingpong-prodcons.c $(addprefix ./lib10_1/, $(INCL)) -o $(OUT)_prodcons;

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT);
