###			OPERAÇÃO PADRÃO A SER EXECUTADA (comando: $ make)
default: build

###			DECLARAÇÃO DE VARIÁVEIS
OBJ = main.o queue.o
OBJD = mainD.o queueD.o
output_name = so_p00

###			COMPILADORES
build: clean $(OBJ)
	gcc $(OBJ) -o $(output_name);
	mkdir obj && mv *.o obj;

bdb: clean $(OBJD) # build para depuração
	gcc -g $(OBJD) -o $(output_name);
	mkdir obj && mv *.o obj;

###			OBJETOS
main.o: ./src/testafila.c
	gcc -c ./src/testafila.c -o main.o

mainD.o: ./src/testafila.c
	gcc -g -c ./src/testafila.c -o mainD.o

queue.o: ./src/queue.c
	gcc -c ./src/queue.c -o queue.o

queueD.o: ./src/queue.c
	gcc -g -c ./src/queue.c -o queueD.o

###			OUTRAS FUNÇÕES
run: build
	if [ -e ./$(output_name) ]; then ./$(output_name); else echo "Erro: Programa nao encontrado"; fi;

clean:
	@[ -d ./obj ] && ( rm -r ./obj $(output_name) ) || true;
	@[ -f *.o ] && ( rm  *.o ) || true;

help:
	echo "\n\nmake run:\texecuta\nmake build:\tcompila o programa\nmake bdb:\tcompila com as informação de depuração\n\ndefault:\tbuild\n"
