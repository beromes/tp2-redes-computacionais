
CC=gcc # compilador, troque para gcc se preferir utilizar C
#CFLAGS=-Wall -Wextra # compiler flags, troque o que quiser, exceto bibliotecas externas
#EXEC=./tp3 # nome do executavel que sera gerado, nao troque
#TMPOUT=tp3.testresult

#$(EXEC): main.cpp arvore_binaria.o tipo_no.o tipo_item.o
#	$(CC) $(CFLAGS) main.cpp arvore_binaria.o tipo_no.o tipo_item.o -o $(EXEC)

all: equipment server

equipment: equipment.c
	$(CC) -o equipment equipment.c common.c

server: server.c
	$(CC) -o server server.c common.c

clean: # remove todos os arquivos temporarios que forem gerados pela compilacao
	rm -rf equipment server
