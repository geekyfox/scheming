CC = gcc
CFLAGS = -Wall -g

.PHONY: test
test: scheme pro99.scm stdlib.scm
	@mkdir -p temp
	./scheme pro99.scm > temp/output
	diff temp/output test/pro99.out

.PHONY: sync
sync: all
	git add .
	git commit -m 'Tweaks'
	git push wip master

.PHONY: all
all: clean format test leanpub

.PHONY: clean
clean:
	rm -f scheme
	rm -rf temp
	rm -rf leanpub

.PHONY: format
format:
	@mkdir -p temp
	cat scheme.c | scripts/fmt.rb > temp/scheme.c
	mv temp/scheme.c scheme.c

scheme : scheme.c temp/autogen.c
	$(CC) $(CFLAGS) $^ -o $@

temp/autogen.c : scheme.c
	@mkdir -p temp
	cat $^ | scripts/codegen.rb > $@

.PHONY: cut
cut: temp/cut

temp/cut : temp/cut.c
	$(CC) $(CFLAGS) $< -o $@

temp/cut.c : scheme.c
	@mkdir -p temp
	cat $< | scripts/cut.rb > $@
	cat $@ | scripts/codegen.rb >> $@

gaps: temp/cut.c
	make cut 2>&1 | grep undefined | awk '{print $$NF}' | sort -u | tee $@

grind: scheme
	valgrind ./scheme pro99.scm

grind_leaks: scheme
	valgrind --leak-check=full --show-leak-kinds=all ./scheme pro99.scm

profile.txt: scheme pro99.scm
	rm -f callgrind.out.*
	valgrind --tool=callgrind ./scheme pro99.scm
	callgrind_annotate --tree=both > profile.txt

.PHONY: leanpub
leanpub: README.md
	mkdir -p leanpub
	cat README.md | scripts/leanpub.rb

README.md : scheme.c
	cat $< | scripts/cut.rb | scripts/markdown.rb > $@
