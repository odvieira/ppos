default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h
SRC = pingpong.c datatypes.h
OBJ = $(SRC:.c=.o)
OUT = so_p02

# Compiler, Linker Defines
CC = gcc
RM = rm -rf

# Build
build:
	@$(CC) ./src/main1.c $(addprefix src/, $(SRC)) $(addprefix ../lib/, $(INCL)) -o $(OUT)_1
	@$(CC) ./src/main2.c $(addprefix src/, $(SRC)) $(addprefix ../lib/, $(INCL)) -o $(OUT)_2
	@$(CC) ./src/main3.c $(addprefix src/, $(SRC)) $(addprefix ../lib/, $(INCL)) -o $(OUT)_3

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT)_3 $(OUT)_2 $(OUT)_1
