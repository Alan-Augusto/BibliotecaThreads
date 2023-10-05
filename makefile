# Compilador
CC = gcc

# Opções de compilação
CFLAGS = -g -Wall -I.

# Nome dos arquivos de objetos
OBJS = dccthread.o dlist.o

# Nome dos executáveis dos testes
TESTS = test0 test1 test2 test3 test4 test5 test6 test7 test8 test9 test10 test11 test12

# Alvo padrão
default: $(OBJS)

# Compilação dos objetos
dccthread.o: dccthread.c dccthread.h
	$(CC) $(CFLAGS) -c dccthread.c -o dccthread.o

dlist.o: dlist.c dlist.h
	$(CC) $(CFLAGS) -c dlist.c -o dlist.o

# Alvo para compilar os testes
test: $(OBJS)
	@echo "Escolha um número de teste para executar:"
	@read choice; \
	./tests/test$$choice.sh

# Alvo para limpar os arquivos de objeto e executáveis
clean:
	rm -f $(OBJS) $(TESTS)
