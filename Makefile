.PHONY: test
test: scheme
	@mkdir -p build
	./scheme test/pro99.scm > build/output
	diff build/output test/expected_output

all : gen test README.md

.PHONY: gen
gen: build/autogen.stamp

build/autogen.stamp: scheme.c
	@mkdir -p build
	utils/gensym.py scheme.c
	cat scheme.c | utils/fmt.py > build/fmt.c
	mv build/fmt.c scheme.c
	touch build/autogen.stamp

CC = gcc
CFLAGS = -Wall -g

scheme : scheme.c
	$(CC) $(CFLAGS) $< -o $@

cut : build/cut.c
	$(CC) $(CFLAGS) $< -o $@

build/cut.c : scheme.c utils/cut.py
	cat $< | utils/cut.py > $@

README.md : scheme.c utils/mkd.py
	cat $< | utils/mkd.py > $@

grind_leaks: scheme
	valgrind --leak-check=full --show-leak-kinds=all ./scheme pro99.scm
	
grind: scheme
	valgrind ./scheme pro99.scm

grind_shell: scheme
	valgrind --leak-check=full --show-leak-kinds=all ./scheme

clean:
	rm -f scheme
	rm -f cut
	rm -rf build

