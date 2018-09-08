default = build

# Source, Executable, Includes, Library Defines
INCL = queue.c queue.h
SRC = pingpong.c datatypes.h
OBJ = $(SRC:.c=.o)
OUT = so_p02

# Compiler, Linker Defines
CC = gcc
CFDEBUG = -ansi -pedantic -Wall -g -DDEBUG $(LDFLAGS)
RM = rm -rf

# Build
build:
	@$(CC) ./src/main1.c $(addprefix src/, $(SRC)) $(addprefix ../p00/src/, $(INCL)) -o $(OUT)_1
	@$(CC) ./src/main2.c $(addprefix src/, $(SRC)) $(addprefix ../p00/src/, $(INCL)) -o $(OUT)_2
	@$(CC) ./src/main3.c $(addprefix src/, $(SRC)) $(addprefix ../p00/src/, $(INCL)) -o $(OUT)_3

# Create a gdb/dbx Capable Executable with DEBUG flags turned on
debug:
	@$(CC) $(CFDEBUG) $(./src/:SRC)

# Clean Up Objects, Executables, dumps out of source directory
clean:
	@$(RM) $(OBJ) $(OUT)_3 $(OUT)_2 $(OUT)_1
