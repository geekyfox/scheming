CC = cc
CFLAGS = -Wall -g

.PHONY: test
test: scheme
	@mkdir -p temp
	./scheme pro99.scm > temp/output
	diff temp/output test/pro99.out

scheme : scheme.c
	clang-format -i $<
	awk -i inplace -f scripts/fmt.awk $<
	awk -i inplace -f scripts/codegen.awk $< 
	$(CC) $(CFLAGS) $^ -o $@

temp/autogen.c : scheme.c scripts/codegen.awk
	@mkdir -p temp
	awk -f scripts/codegen.awk $< > $@

.PHONY: all
all: clean test README.md

.PHONY: clean
clean:
	rm -f scheme
	rm -f temp/output
	rm -f temp/*.c
	rm -rf leanpub

README.md: scheme scheme.c
	awk -f scripts/cut.awk scheme.c > $@
	awk -i inplace -f scripts/markdown.awk $@
	awk -i inplace -f scripts/compact.awk $@

.PHONY: leanpub
leanpub: 
	@mkdir -p leanpub
	awk -f scripts/leanpub.awk README.md

gaps: temp/cut.c
	make temp/cut 2>&1 | awk -f scripts/gaps.awk | tee $@

temp/cut : temp/cut.c
	$(CC) $(CFLAGS) $< -o $@

temp/cut.c : scheme.c
	@mkdir -p temp
	awk -f scripts/cut.awk $< > $@
	awk -i inplace -f scripts/codegen.awk $@

.PHONY: push
push: all
	git add .
	git commit -m 'Tweaks'
	git push wip master

.PHONY: pull
pull:
	git pull --rebase wip master

grind: scheme
	valgrind ./scheme pro99.scm

grind_leaks: scheme
	valgrind --leak-check=full --show-leak-kinds=all ./scheme pro99.scm

profile.txt: scheme pro99.scm
	rm -f callgrind.out.*
	valgrind --tool=callgrind ./scheme pro99.scm
	callgrind_annotate --tree=both > profile.txt

.PHONY: grammar
grammar: README.md scripts/grammar.awk
	mkdir -p temp
	awk -f scripts/grammar.awk $<

