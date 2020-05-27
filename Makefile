.PHONY: test
test: scheme pro99.scm stdlib.scm
	@mkdir -p temp
	./scheme pro99.scm > temp/output
	diff temp/output test/pro99.out

.PHONY: all
all: codegen test README.md

.PHONY: codegen
codegen:
	@mkdir -p temp
	cat scheme.c | scripts/codegen.rb > temp/scheme.c
	mv temp/scheme.c scheme.c

CC = gcc
CFLAGS = -Wall -g

scheme : scheme.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: cut
cut: temp/cut

temp/cut : temp/cut.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: gaps 
gaps: temp/gaps.txt

temp/gaps.txt : temp/cut.c
	make cut 2>&1 | grep undefined | awk '{print $$NF}' | sort -u | tee $@

temp/cut.c : scheme.c
	@mkdir -p temp
	cat $< | scripts/cut.rb | scripts/codegen.rb > $@

README.md : scheme.c
	cat $< | scripts/cut.rb | scripts/markdown.rb > $@

grind: scheme
	valgrind ./scheme pro99.scm

grind_leaks: scheme
	valgrind --leak-check=full --show-leak-kinds=all ./scheme pro99.scm

profile:
	valgrind --tool=callgrind ./scheme pro99.scm

.PHONY: clean
clean:
	rm -f scheme
	rm -rf temp
