test: scheme
	./scheme pro99.scm

CC = gcc
CFLAGS = -Wall -g

scheme : scheme.c scheme.h
	$(CC) $(CFLAGS) $^ -o $@

grind_leaks: scheme
	valgrind --leak-check=full --show-leak-kinds=all ./scheme pro99/pro99.scm
	
grind: scheme
	valgrind ./scheme pro99/pro99.scm

grind_shell: scheme
	valgrind --leak-check=full --show-leak-kinds=all ./scheme

clean:
	rm -f scheme
	rm -rf build

