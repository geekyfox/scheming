// # Geeky Fox Is Scheming
// [![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)
// ## Chapter 1 where the story begins
//
// Every story has to begin somewhere. This one is a tour through a
// simplistic Scheme interpreter written in C, and every C program begins
// with the `main()` function.
//

void setup_runtime(void);
void do_useful_stuff(int argc, const char** argv);
void teardown_runtime(void);

int main(int argc, const char** argv)
{
	setup_runtime();
	do_useful_stuff(argc, argv);
	teardown_runtime();
}

//
// Here I can imagine people asking "Wait a minute, is this some kind of
// pseudo C? Ain't you supposed to have `#include <stdio.h>` at the
// beginning and `main()` function at the very end?"
//
// Well. There is a whole universe of possible implications behind
// "Ain't you supposed to do X?" questions, and I'll use them as rant fuel
// throughout this story. But considering the most straightforward one,
// which is "is C compiler okay with such way of structuring code?" the
// answer is yes, it compiles just fine. It will keep failing to *link*
// until those functions will be implemented, but as far as compiler is
// concerned, forward declarations are good enough.
//
// Moreover, this is the general pattern in this story where I follow the
// example of  Quintus Fabius Maximus and keep postponing writing
// lower-level implementation of a feature until I get a handle on
// how it's going to be used.
//
// This approach is called top-down, and it's not the only way to write
// a program. A program can also be written bottom-up, which is to start
// with individual nuts and bolts, and then to assemble them into a big
// piece of software.
//
// There's a potential problem with the latter though.
//
// Let's say, I start with writing a piece of standard library. I'm
// certainly going to need it at some point, so it isn't an obviously
// wrong place to start. Oh, and I'll also need a garbage collector, so
// I can write one too.
//
// But then I'm running a risk of ending up with an implementation of a
// chunk of Scheme standard library that is neat, and cute, and pretty,
// and a garbage collector that is as terrific as a garbage collector in
// a pet project may possibly be, and **they don't quite fit!**
//
// And then I'll have a dilemma. Either I'll have to redo one or both of
// them. Which probably won't be 100% waste, as I hope to have learned
// a few things on the way, but it's still double work. Or else I can refuse
// to accept sunk costs and then stubbornly work around the
// incompatibilities between my own standard library implementation and
// my own garbage collector. And that's just dumb.
//
// But as long as something *isn't done at all*, I can be totally sure
// that it *isn't done wrong*. There's a certain Zen feeling to it.
//
// Another thing is more subtle, but will get a lot of programmers,
// especially more junior ones, nervous once they figure it out. It's
// `setup_runtime()` call. It's pretty clear **what** it will do which
// is initialize garbage collector and such, but it also implies I'm
// going to have **the** runtime, probably scattered around in a bunch of
// global variables.
//
// I can almost hear voices asking "But what if you need to have multiple
// runtimes? What if customer comes and asks to make your interpreter
// embeddable as a scripting engine? What about multithreading? Why are
// you not worried?!"
//
// The answer is "I consciously don't care." This is just a pet project
// that started with me willing to tinker with Scheme, and then realizing
// that just writing in Scheme is too easy, so I wrote my own interpreter,
// and then realizing that **even that is not fun enough**, so I wrapped it
// into a sort of "literary programming" exercise. In a (highly unlikely)
// situation when I'll have to write my own multithreaded embeddable Scheme
// interpreter, I'll just start from scratch, and that's about it.
//
// Anyway, I'll write functions to setup and teardown runtime once I
// get a better idea of how said runtime should look like, and for now
// I'll focus on doing the useful stuff.
//

#include <stdio.h>
#include <unistd.h>

void execute(FILE*);
void execute_file(const char* filename);
void repl(void);

void do_useful_stuff(int argc, const char** argv)
{
	if (argc >= 2) {
		for (int i=1; i<argc; i++)
			execute_file(argv[i]);
	} else if (isatty(fileno(stdin))) {
		repl();
	} else {
		execute(stdin);
	}
}

//
// This one is pretty straightforward: when program is launched as
// `./scheme foo.scm` then execute a file, when it's launced as
// `cat foo.scm | ./scheme` do exactly the same, and otherwise fire
// up a REPL.
//
// Now that I know that I'm going to have a function that reads code from
// a stream and executes it, writing a function that does the same with a
// file is trivial, so let's just make one.
//

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define DIE(fmt, ...) do { \
	fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
	fprintf(stderr, fmt, ##__VA_ARGS__); \
	fprintf(stderr, "\n"); \
	abort(); \
} while (0)

FILE* fopen_or_die(const char* pathname, const char* mode)
{
	FILE *f = fopen(pathname, mode);
	if (f)
		return f;
	const char* err = strerror(errno);
	DIE("Error opening file %s: %s", pathname, err);
}

void execute_file(const char* filename)
{
	FILE* f = fopen_or_die(filename, "r");
	execute(f);
	fclose(f);
}

//
// Some might find error handling in `fopen_or_die()` a bit naive. If you
// aren't among those, you can skip the following rant, and if you are,
// it's okay, it's nothing to be ashamed of, it's just normal cognitive
// inertia.
//
// See, in general, when something goes wrong you have three options:
//
// 1. You can handle the problem and continue.
//
// 2. You can abort.
//
// 3. You can notify the invoker about the problem and let them make their
//    own choice from these same three options.
//
// In this case option #1 is unavailable. Because, well, failing to open a
// file that interpreter is told to execute is clearly a fatal error, and
// there's no sane way to recover from it.
//
// Oh, of course there are insane ways to do it, for instance I can just
// quietly skip the problematic file, and later collapse in an obscure way
// because I can't find functions that were supposed to be defined in that
// file, or take your guess, but I'm not even going to spend time
// explaining why I'm not doing that.
//
// Option #3 is interesting because this is what many programmers would
// consider a natural and only alternative for the case when #1 is not
// doable. In fact, if you're coding on top of a rich fat framework (think
// Spring or Django) this indeed is the natural and only way to do it. But
// in this case there's no framework, and operating system is effectively
// **the** invoker ("*there was nothing between us... not even a
// condom...*" yeah, really bad joke), and `abort()`ing is a proper way to
// notify the operating system about the problem, so #2 is pretty much #3,
// just without boilerplate code to pull error status to `main()` and then
// abort there.
//
// Anyway, let's implement `execute()`
//

#include <stdbool.h>

struct object;
typedef struct object* object_t;

void decref(object_t);

object_t eval_repl(object_t);
object_t read_object(FILE*);

void execute(FILE* in)
{
	object_t expr;

	while ((expr = read_object(in))) {
		object_t result = eval_repl(expr);
		decref(expr);
		decref(result);
	}
}

//
// Several things are introduced here.
//
// First one is `struct object` which is going to be the representation
// of a Scheme object. It's quite certainly going to be some sort of a
// `struct`; internal details of that struct I'll figure out later.
//
// Second and third are `read_object()` and `eval_repl()` functions that,
// respectively, read an object from an input stream and evaluate it
// in REPL context.
//
// Last one is `decref()` function that is needed because I'm going to
// have automatic memory management. For this I have three options:
//
// 1. I can do reference counting. Very simple to do for as long as objects
// don't form reference cycles, then it gets quirky.
//
// 2. I can make a tracing garbage collector which traverses process'
// memory to figure out which objects are still needed.
//
// 3. I can apply a sort of a hybrid approach where I do tracing for the
// sections of process' memory which are convenient to traverse and
// fallback to reference counting for those which aren't.
//
// From this simple function it's already clear that whichever method I
// choose must be able to deal with references from C call stack, and
// analyzing it in a pure tracing manner is pretty cumbersome, so
// I have to count them anyway, and that's what `decref()` will do.
//
// Now comes the REPL...
//

void write_object(FILE*, object_t obj);

void repl()
{
	object_t expr;

	printf("> ");
	while ((expr = read_object(stdin))) {
		object_t result = eval_repl(expr);
		decref(expr);

		write_object(stdout, result);
		decref(result);

		printf("\n> ");
		fflush(stdout);
	}
	printf("bye\n");
}

//
// ...which isn't particularly interesting, and we proceed to
// ## Chapter 2 where I parse Lisp code
//
// What's neat about implementing a Lisp dialect is that you can be done
// with parsing in about three pints of Guinness, and then move on to
// funnier stuff.
//
// Of course "funnier" is relative here, and not just grammatically,
// but also in a Theory of Relativity kind of sense. I mean, Theory of
// Relativity describes extreme conditions where gravity is so high that
// common Newtonian laws don't work any more.
//
// Likewise, here we're venturing deep into dark swampy forests of
// Nerdyland where common understanding of "fun" doesn't apply. By
// the standards of normal folks whose idea of having fun involves
// activities like mountain skiing and dance festivals, spending evenings
// tinkering with implementation of infinite recursion is hopelessly
// weird either way. So I mean absolutely no judgement towards those
// amazing guys and gals who enjoy messing with lexers, parsers, and
// all that shebang. Whatever floats your boat, really!
//
// This had to be said. Anyway, back to parsing.
//

int fgetc_skip(FILE*);

object_t read_atom(FILE* in);
object_t read_list(FILE* in);
object_t read_quote(FILE* in);
object_t read_string(FILE* in);

object_t read_object(FILE* in)
{
	int ch = fgetc_skip(in);
	switch (ch)
	{
	case EOF:
		return NULL;
	case '(':
		return read_list(in);
	case ')':
		DIE("Unmatched ')'");
	case '\'':
		return read_quote(in);
	case '"':
		return read_string(in);
	default:
		ungetc(ch, in);
		return read_atom(in);
	}
}

//
// Lisp syntax is famously Spartan. Basically all you get is:
// * lists (those thingies with ((astonishingly) copious) amount of
//   parentheses),
// * strings (delimited by "double quotes" or however you call that
//   character),
// * quotations (if you don't know who these are, you better look it up in
//   Scheme spec, but basically it's a way to specify that ```'(+ 1 2)```
//   is *literally* a list with three elements and not an expression that
//   adds two numbers),
// * and atoms, which are pretty much everything else including numbers,
//   characters, and symbols.
//
// So what I'm doing here is I'm looking at the first non-trivial
// character in the input stream and if it's an opening parenthesis I
// interpret it a beginning of a list etc.
//

#include <ctype.h>

int fgetc_or_die(FILE* in)
{
	int ch = fgetc(in);
	if ((ch == EOF) && (! feof(in))) {
		const char* err = strerror(errno);
		DIE("IO error: %s", err);
	}
	return ch;
}

int fgetc_skip(FILE* in)
{
	bool comment = false, skip;
	int ch;

	do {
		ch = fgetc_or_die(in);
		if (ch == ';')
			comment = true;
		skip = comment || isspace(ch);
		if (ch == '\n')
			comment = false;
	} while ((ch != EOF) && skip);

	return ch;
}

//
// Oh, and "the first non-trivial character" means I fast-forward through
// the input stream ignoring comments and whitespace until I encounter
// a character that's neither, or reach an EOF.
//
// There are four `read_something()` functions that I promised to
// implement, let's start with `read_string()`
//

int fgetc_read_string(FILE* in)
{
	int ch = fgetc_or_die(in);

	switch (ch) {
	case EOF:
		DIE("Premature end of input");
	case '\"':
		return EOF;
	case '\\':
		ch = fgetc_or_die(in);
		switch (ch) {
		case EOF:
			DIE("Premature end of input");
		case 'n':
			return '\n';
		}
	}

	return ch;
}

object_t wrap_string(const char*);

object_t read_string(FILE* in)
{
	char buffer[10240];
	int fill = 0, ch;

	while ((ch = fgetc_read_string(in)) != EOF) {
		buffer[fill++] = ch;
		if (fill >= 10240)
			DIE("Buffer overflow");
	}

	buffer[fill] = '\0';
	return wrap_string(buffer);
}

//
// Nothing particularly surprising here, just read characters into the
// buffer until you reach the closing double quote, then wrap the
// contents of the buffer into an `object_t` and call it a day.
//
// Yes, this simplistic implementation will miserably fail to parse a
// source file with a string constant that is longer than 10K characters.
//
// And if you take some time to think about it, hard-coded 10K bytes for
// buffer size is kind of interesting here. It's an arbitrary number that
// on one hand is safely above any practical limit in terms of usefulness.
// I mean, of course you can hard-code entire "Crime and Punishment" as
// a single string constant just to humiliate a dimwit interpreter author.
// But within any remotely sane coding style such blob must be offloaded
// to an external text file, and even a buffer that is an order of magnitude
// smaller should still be good enough for all reasonable intents and
// purposes.
//
// On the other hand it's also safely below any practical limit in terms
// of conserving memory. It can easily be an order of magnitude bigger
// without causing any issues whatsoever.
//
// At least on a modern general-purpose machine with a couple of gigs
// of memory. If you've got a PDP-7 like one that Ken Thompson used for
// his early development of Unix then a hundred kilobytes might be your
// **entire** RAM and then you have to be more thoughtful with your
// throwaway buffers.
//
// By the way, it's really impressive how those people in 1960s could
// develop an entire real operating system on a computer like that. Well,
// not exactly mind-boggling, like, I myself started coding on a
// Soviet-made ZX Spectrum clone with 48 kilobytes of RAM, and you
// can't write a super-dooper-sophisticated OS on such machine because
// **it just won't fit**, but still, it's so cool.
//
// Okay, back to business. Let's `parse_atom()`.
//

bool isspecial(char ch)
{
	switch (ch)
	{
	case '(': case ')': case ';': case '\"': case '\'':
		return true;
	default:
		return false;
	}
}

int fgetc_read_atom(FILE* in)
{
	int ch = fgetc_or_die(in);
	if (ch == EOF)
		return EOF;
	if (isspace(ch) || isspecial(ch)) {
		ungetc(ch, in);
		return EOF;
	}
	return ch;
}

object_t parse_atom(const char*);
object_t wrap_char(char ch);

object_t read_character(FILE* in)
{
	int ch = fgetc_or_die(in);
	if ((ch == EOF) || isspace(ch))
		return wrap_char(' ');
	else
		return wrap_char(ch);
}

object_t read_atom(FILE* in)
{
	char buffer[10240];
	int fill = 0, ch;

	while ((ch = fgetc_read_atom(in)) != EOF) {
		buffer[fill++] = ch;
		if (fill >= 10240)
			DIE("Buffer overflow");
	}

	if ((fill == 2) && (buffer[0] == '#') && (buffer[1] == '\\'))
		return read_character(in);

	buffer[fill] = '\0';
	return parse_atom(buffer);
}

//
// Here I use the same approach as in `read_string()`: collect characters
// for as long as it looks like an atom, then convert it to an `object_t` and
// that's pretty much it.
//
// Well, syntax for characters in Scheme is a bit wonky, you have `#!\x` for
// the letter 'x' and `#\!` for an exclamation mark, and, surprisingly,
// `#!\newline` and `#!\space` for a newline and space respectively.
// Oh, and `#\` is also a space. Go figure.
//
// Most of that wonkiness can be dealt with by simply reading everything
// up until a special character and then figuring out what I've got in
// `parse_atom()`. Unless **it is** a special character, e.g. `#\)` or `#\;`,
// those need a bit of special handling.
//
// And now I'm looking at another buffer and do you know what actually
// boggles my mind?
//
// Remember, in the very beginning of this story I mentioned that a C
// program is normally supposed to have its `main()` function at the
// end? So, what boggles my mind is why are we still doing it?
//
// Well, I don't mean we all do. In some programming languages it is more
// common, in some it is less, but really, why would you do it in any
// language? It's such a weird way to layout your code where you have to
// scroll all the way down to the bottom of the source file and then
// work your way up in a Benjamin Button kind of way.
//
// I mean, I know it's the legacy of Pascal where you were required to
// have the equivalent of `main()` at the bottom (and finish it with an
// `end.` with a period instead of a semicolon). I also understand that
// back in those days it made sense in order to simplify the compiler that
// had to run on limited hardware. But why we still sometimes do it in
// 2020 is a mystery to me.
//
// Okay, enough of ranting, let's `parse_atom()`
//

object_t wrap_bool(bool v);

object_t parse_bool(const char* text)
{
	if (strcmp(text, "#f") == 0)
		return wrap_bool(false);
	if (strcmp(text, "#t") == 0)
		return wrap_bool(true);
	return NULL;
}

object_t parse_char(const char* text)
{
	if (strcmp(text, "#\\newline") == 0)
		return wrap_char('\n');
	if (strcmp(text, "#\\space") == 0)
		return wrap_char(' ');
	if (strcmp(text, "#\\") == 0)
		return wrap_char(' ');
	if ((strlen(text) == 3) && (text[0] == '#') && (text[1] == '\\'))
		return wrap_char(text[2]);
	return NULL;
}

object_t wrap_int(int value);

object_t parse_int(const char* text)
{
	int index = 0, digits = 0, accum = 0, sign = 1;

	if (text[0] == '-') {
		index = 1;
		sign = -1;
	}

	for ( ; text[index] ; index++) {
		if (! isdigit(text[index]))
			return NULL;
		accum = accum * 10 + (text[index] - '0');
		digits++;
	}

	return (digits == 0) ? NULL : wrap_int(sign * accum);
}

object_t wrap_symbol(const char*);

object_t parse_atom(const char* buffer)
{
	object_t result;
	if ((result = parse_bool(buffer)))
		return result;
	if ((result = parse_int(buffer)))
		return result;
	if ((result = parse_char(buffer)))
		return result;
	return wrap_symbol(buffer);
}

//
// Pretty straightforward really. If it looks a boolean, convert to a
// boolean. If it looks like an integer number, convert to an integer.
// If it looks like a symbol, convert to a symbol. If it looks like a
// floating-point number... Convert to a symbol because screw you!
//
// To give a bit of a background here, this pet project of mine was
// never intended to be a feature-complete standards-compliant Scheme
// implementation. It started with solving some of "99 Lisp problems"
// and then escalated into writing a sufficient Scheme interpreter to
// run those. None of those problems relied on floating-point arithmetics,
// and so I didn't implement it.
//
// Not that it's particularly hard, just tedious (if done Python-style with
// proper type coercions), and JavaScript solution with simply using floats
// for everything I find aesthetically unappealing.
//
// What I couldn't possibly skip is lists (they didn't call it LISt
// Processing for nothing), so let's `read_list()`
//
// Oh, and a quick remark on naming convention. Functions like
// `wrap_symbol()` are named this way intentionally. They could easily
// be called, say, `make_symbol()`, but that would imply that it's some
// sort of a constructor that really **makes** a *new* object. But by the
// time I get to actually implement those I might not want to be bound
// by this implication (because I might find out that a proper constructor
// doesn't conform to the language standard and/or isn't practical, and I
// need and/or want some sort of a cache or a pool or whatever).
//
// So instead it's a vague "wrap" which stands for "get me an objects that
// represent this value, and how you make it under the hood is none of
// my business."
//

object_t read_next_object(FILE* in)
{
	int ch = fgetc_skip(in);
	if (ch == EOF)
		DIE("Premature end of input");
	if (ch == ')')
		return NULL;
	ungetc(ch, in);
	return read_object(in);
}

void push_list(object_t* ptr, object_t item);
object_t reverse_read_list(object_t list);
object_t wrap_nil(void);

object_t read_list(FILE* in)
{
	object_t accum = wrap_nil(), obj;

	while ((obj = read_next_object(in))) {
		push_list(&accum, obj);
		decref(obj);
	}

	object_t result = reverse_read_list(accum);
	decref(accum);
	return result;
}

//
// This is one is simple but might need a bit of refresher on how
// lists work in Lisp (and other FP(-esque) languages).
//
// So, your basic building block is a two-element tuple (also known as
// a pair). If you put some value to the first cell of that tuple, and to the
// second cell you put a reference to another tuple with another value in
// the first cell et cetera et cetera, and then to the second cell of the
// last tuple you instead put a special null value, then you what get is a
// singly-linked list. Oh, and representation of the empty list is
// simply the null value.
//
// So what I do here is I read objects from input stream and push them
// one by one to the front of the list until I see a closing parenthesis.
// But then the list ends up reversed, so I need to reverse it back. Easy.
//

object_t wrap_pair(object_t, object_t);

void push_list(object_t* ptr, object_t head)
{
	object_t tail = *ptr;
	*ptr = wrap_pair(head, tail);
	decref(tail);
}

//
// This is such a pretty little function that utilizes call by pointer
// (very much a C idiom) to construct a very Lispy list. Tell me about
// multiparadigm programming.
//
// Oh, and while we're at it, let's also implement `reverse_read_list()`
//

struct pair;
typedef struct pair* pair_t;

pair_t assert_pair(const char* context, object_t obj);
object_t car(pair_t);

bool is_symbol(const char* text, object_t obj);

object_t pop_list(object_t*);

void incref(object_t);

object_t reverse_read_list(object_t list)
{
	object_t result = wrap_nil(), obj;

	while ((obj = pop_list(&list))) {
		if (is_symbol(".", obj)) {
			pair_t pair = assert_pair("when parsing a list", result);
			obj = car(pair);
			incref(obj);
			decref(result);
			result = obj;
			continue;
		}
		push_list(&result, obj);
	}

	return result;
}

//
// which simply pops things from one list and pushes them to another.
//
// Well, except for one gotcha: `.` has special meaning in list notation,
// so that `'(a  . b)` is not a list but is equivalent to `(cons 'a 'b)`,
// and so I cater for it here.
//

object_t cdr(pair_t);
bool is_nil(object_t);

object_t pop_list(object_t* ptr)
{
	object_t obj = *ptr;
	if (is_nil(obj))
		return NULL;
	pair_t pair = assert_pair("when traversing a list", obj);
	*ptr = cdr(pair);
	return car(pair);
}

//
// `pop_object()` is pretty much the opposite of `push_object()` with
// a bit of typechecking to make sure I'm dealing with a list and not
// something dodgy.
//

#define DEBUG(key, obj) do { \
	fprintf(stderr, "[%s:%d] %s = ", __FILE__, __LINE__, key); \
	write_object(stderr, obj); \
	fprintf(stderr, "\n"); \
} while (0)

const char* typename(object_t);
pair_t to_pair(object_t);

pair_t assert_pair(const char* context, object_t obj)
{
	pair_t pair = to_pair(obj);
	if (! pair) {
		DEBUG("obj", obj);
		DIE("Expected a pair %s, got %s instead", context, typename(obj));
	}
	return pair;
}

//
// I still haven't decided what exactly am I going to put into either
// `struct object` or `struct pair`, but I already need to be able to
// convert one to another. So, I promise to write a `to_pair()`
// function that would do just that (or return `NULL` if value that this
// object holds is not a pair), and here's write a nice little helper around
// it to abort with a human-readable message when conversion fails.
//

object_t read_quote(FILE* in)
{
	object_t obj = read_object(in);
	if (! obj)
		DIE("Premature end of input");

	object_t result = wrap_nil();
	push_list(&result, obj);
	decref(obj);

	object_t keyword = wrap_symbol("quote");
	push_list(&result, keyword);
	decref(keyword);

	return result;
}

//
// Since `'bla` is merely a shorter version of `(quote bla)` parsing it
// is trivial, and with that in place we're finally done with parsing
// and can move on to
// ## Chapter 3 where I evaluate
//
// By the way, I don't know if you noticed or not, but I try to use the
// word "we" as sparingly as I can. Perhaps it has something to do with me
// coming from a culture where "We" is commonly associated with the
// dystopian novel by Yevgeny Zamyatin.
//
// Of course there are legit usages for "we," such as academic writing
// where all of "us" are listed on the front page of the paper, and the
// reader is much more interested in overall results than in the internal
// dynamics of the research team.
//
// But using "we did it" when it's actually "I did it" (and it's
// stylistically appropriate to say "I did it") feels to me like speaker
// is a wimp who wants to avoid the responsibility.
//
// Likewise, using "we" when it's actually "I and Joe, mostly Joe" feels
// like reluctanse to give a fair share of credit.
//
// Okay, enough of that, let's implement `eval_repl()`
//

struct scope;
typedef struct scope* scope_t;

object_t eval_eager(scope_t scope, object_t expr);
scope_t get_repl_scope(void);

object_t eval_repl(object_t expr)
{
	return eval_eager(get_repl_scope(), expr);
}

//
// That's a one-liner function that relies on two important concepts.
//
// First one is the scope. Scope is pretty much just a binding between
// variables' names and their values. For now just think of it as a sort
// of a dictionary (it's not exactly that, but we'll get there)
//
// Another one is differentiation between eager and lazy evaluation.
// Before I go into explaining *what exactly do I mean by eager and
// lazy evaluation* in the context of this story, I first have to
// elaborate the pragmatics for having all that at the first place.
//
// So. Scheme is a functional programming language, and in functional
// programming people don't do loops, but instead they do recursion. And
// for infinite loops they do, well, infinite recursion. And "infinite"
// here doesn't mean "very very big," but actually infinite.
//
// Consider this example:
//
// embed snippets/infinite-loop.scm : infinite loop
//
// Obviously, a direct equivalent of this code in vanilla
// C/C++/Ruby/Python/Java will run for some time, and eventually
// blow up with a stack overflow. But code in Scheme, well, better
// shouldn't.
//
// I have three ways to deal with it:
//
// 1. Just do nothing and hope that C stack will not overflow.
//
// 2. Do code rewriting so that under the hood the snippet above is
// automagically converted into a loop, e.g.
//
// embed snippets/infinite-loop.scm : infinite loop rewritten
//
// 3. Apply the technique called trampolining. Semantically it means
//
// embed snippets/infinite-loop.scm : infinite loop trampoline
//
// that instead of calling itself, function... Well, to generalize and
// to simplify let's say it informs the evaluator that computation is
// incomplete, and also tells what to do next in order to complete it.
//
// #1 looks like a joke, but actually it's a pretty good solution. It's
// also a pretty bad solution, but let's get through the upsides first.
//
// First of all, it's very easy to implement (because it doesn't require
//  writing any specific code), it clearly won't introduce any complicated
// bugs (because there's no specific code!), and it won't have any
// performance impact (because it does nothing!!)
//
// You see, "*just do nothing*" half of it is all good, it's the "*and hope
// that*" part that isn't. Although, for simple examples it doesn't
// really matter: with a couple of thousands of stack levels it's gonna be
// fine with or without optimizations. But a more complex program may
// eventually hit that boundary, and then I'll have to get around
// deficiencies of my code in, well, my other code, and that's not a
// place I'd like to get myself into.
//
// Which also sets a constraint on what a "proper" solution should be:
// it must be provably reliable for a complex piece of code, or else it's
// back to square one.
//
// #2 looks like a superfun thing to play with, and for toy snippets it
// seems deceptively simple. But thinking just a little bit about
// pesky stuff like mutually recursive functions, and
// self-modifying-ish code (think `(set! infinite-loop something-else)`
// *from within* `infinite-loop`), and escape procedures, and all this
// starts to feel like a breeding ground for wacky corner cases, and
// I don't want to commit to being able to weed them all out.
//
// #3 on the contrary is very simple, both conceptually and
// implementation-wise, so that's what I'll do (although I might
// do #2 on top of it later; because it looks like a superfun thing to
// play with)
//
// Now let's get back to lazy vs eager. "Lazy" in this context means
// that evaluation function may return either a result (if computation
// is finished) or a thunk (a special object that describes what to do
// next). Whereas "eager" means that evaluation function will always
// return the final result.
//

object_t eval_lazy(scope_t scope, object_t expr);
object_t force(object_t value);

object_t eval_eager(scope_t scope, object_t expr)
{
	object_t result_or_thunk = eval_lazy(scope, expr);
	return force(result_or_thunk);
}

//
// "Eager" evaluation can be easily arranged by getting a "lazy" result
// first...
//

struct thunk;
typedef struct thunk* thunk_t;

object_t eval_thunk(thunk_t thunk);
thunk_t to_thunk(object_t obj);

object_t force(object_t value)
{
	thunk_t thunk;

	while((thunk = to_thunk(value))) {
		object_t new_value = eval_thunk(thunk);
		decref(value);
		value = new_value;
	}

	return value;
}

//
// ...and then reevaluating it until the computation is complete.
//
// You know, I've just realized it's the third time in this story when I
// say "I have three ways to deal with it," previous two being
// considerations about memory management and error handling in
// Chapter 1.
//
// Moreover, I noticed a pattern. In a generalized form, those three
// options to choose from are:
//
// 1. Consider yourself lucky. Assume things won't go wrong. Don't
// worry about your solution being too optimistic.
//
// 2. Consider yourself smart. Assume you'll be able to fix every bug.
// Don't worry about your solution being too complex.
//
// 3. Just bloody admit that you're a dumb loser. Design a balanced
// solution that is resilient while still reasonable.
//
// This is such a deep topic that I'm not even going to try to cover it
// in one take, but I'm damn sure I'll be getting back to it repeatedly.
//
// For now, let's continue with `eval_lazy()`
//

struct symbol;
typedef struct symbol* symbol_t;

symbol_t to_symbol(object_t);

object_t eval_sexpr(scope_t, object_t head, object_t body);
object_t eval_var(scope_t scope, symbol_t key);

object_t eval_lazy(scope_t scope, object_t expr)
{
	symbol_t varname = to_symbol(expr);
	if (varname)
		return eval_var(scope, varname);

	pair_t sexpr = to_pair(expr);
	if (sexpr)
		return eval_sexpr(scope, car(sexpr), cdr(sexpr));

	incref(expr);
	return expr;
}

//
// This is fairly straightforward: if it's a symbol, treat it as a name
// of the variable, if it's a list, treat it as a symbolic expression, and
// otherwise just evaluate it to itself (so that `(eval "bla")` is simply
// `"bla"`)
//

object_t lookup_in_scope(scope_t scope, symbol_t key);
const char* unwrap_symbol(symbol_t);

object_t eval_var(scope_t scope, symbol_t key)
{
	object_t result = lookup_in_scope(scope, key);
	if (! result)
		DIE("Undefined variable %s", unwrap_symbol(key));
	incref(result);
	return result;
}

//
// Evaluating a variable is pretty much just look it up in the current
// scope and `DIE()` if it's not there.
//

object_t eval_funcall(scope_t scope, object_t func, object_t exprs);
object_t eval_syntax(scope_t scope, object_t syntax, object_t body);

object_t eval_sexpr(scope_t scope, object_t head, object_t body)
{
	object_t syntax_or_func = eval_eager(scope, head);

	object_t result = eval_syntax(scope, syntax_or_func, body);
	if (! result)
		result = eval_funcall(scope, syntax_or_func, body);

	decref(syntax_or_func);
	return result;
}

//
// And evaluating an expression is... Well, if you ever wondered why
// the hell they use so many of those bloody parentheses in Lisps,
// here's your answer.
//
// In most programming languages (mainstream ones anyway) syntax
// constructs and functions are two fundamentally different kinds of
// creatures. They don't just *behave* differently, but they also *look*
// differently, and you can't mix them up.
//
// Much less so in Lisp, where you have `(if foo bar)` for conditional,
// and `(+ foo bar)` to add two numbers, and `(cons foo bar)` to make
// a pair, and you can't help but notice they look pretty darn similar.
//
// Moreover, even though they behave differently, it's not that *that
// dissimilar* either. `+` and `cons` are simply functions that accept
// *values* of `foo` and `bar` and do something with them. Whereas
// `if` is also simply a function, except that instead of values of its'
// arguments it accepts *a chunk of code verbatim.*
//
// Let me reiterate: a syntax construct is merely a *data-manipulation
// function* that happens to have *program's code as the data that it
// manipulates.* Oh,  and *code as data* is not some runtime
// introspection shamanistic voodoo, it's just normal lists and symbols
// and what have you.
//
// And all of that is facilitated by having the same notation of data
// and code. That's why parentheses are so cool.
//
// So, with explanation above in mind, pretty much all this function does
// is: it evaluates the first item of the S-expression, and if it happens to
// be an "I want code as is" function, then it's fed code as is, and
// otherwise it is treated as an "I want values of the arguments" function
// instead. That's it.
//
// If my little story is your first encounter with Lisp, I can imagine
// how mind-blowing can this be. Let it sink in, take your time.
//

struct lambda;
typedef struct lambda* lambda_t;

lambda_t to_lambda(object_t);

object_t invoke(object_t, int, object_t*);
object_t wrap_thunk(lambda_t, int, object_t*);

object_t eval_funcall(scope_t scope, object_t func, object_t exprs)
{
	object_t args[64], expr;
	int argct = 0;

	while ((expr = pop_list(&exprs))) {
		if (argct >= 64)
			DIE("Buffer overflow");
		args[argct++]  = eval_eager(scope, expr);
	}

	lambda_t lambda = to_lambda(func);
	if (lambda)
		return wrap_thunk(lambda, argct, args);

	return invoke(func, argct, args);
}

//
// This one is not very complicated: go through the list, evaluate
// the stuff you have there, then either feed it to a function or make
// a thunk to evaluate it later.
//
// There are few minor funky optimizations to mention though.
//
// 1. I put arguments into a buffer and not to a list. I mean, lists are
// cool for everything except two things. They're not as efficient for
// "just give me an element at index X" random access, and they're
// kinda clumsy when it comes to memory allocation. And these are
// two things I really won't mind having for a function call: as much
// as I don't care about performance, having to do three `malloc()`s
// just to call a function of three arguments feels sorta wasteful.
//
// 2. I introduce `lambda_t` type for functions that are implemented
// in Scheme and require tail call optimizations. This is done to
// separate them from built-in functions that are implemented in
// C and are supposed to be hand-optimized so that lazy call overhead
// is unnecessary.
//
// 3. I cap the maximum number of arguments that a function can have
// at 64. I even drafted a rant to rationalize that, but I'm running out of
// Chardonnay, so let's park it for now and move on to
// ## Chapter 4 where I finally write some code in Scheme
//
// Somewhere around Chapter 2 I mentioned that this whole story
// began with the list of "99 Lisp problems."
//
// So, let's start with the first task on the list, which is:
//
// > Find the last box of a list.
// >
// > Example:
// >
// > `* (my-last '(a b c d))`
// >
// > `(D)`
//
// That's an easy puzzle, here's the solution:
//
// embed pro99.scm : problem #1
//
// Lists of problems like that one, where you start with something very
// simple, and then work your way towards something more complex is
// an awesome way to learn a programming language. I mean, you start
// at level 1, and you figure out the least sufficient fraction of the
// language that enables you to write a simple yet complete and
// self-contained program. Once that knowledge sinks in, you move on to
// level 2, and learn a bit more, and in the end you have a pretty good
// command of the whole thing.
//
// At least this is how it works for me. If there are anybody out there,
// who needs to memorize the whole language standard and pass theory
// exams before they can start writing any code, that's fine, I don't
// judge.
//
// Oh, and of course it only applies when I'm in a relatively unfamiliar
// territory. If you ask me, after all those years I spent writing backends
// in Python and Java, to do some, I dunno, Node.js, I'd be like "Hmm,
// so you guys use curly braces here? That's neat." and then just pick a
// real problem straight away.
//
// But also, and maybe even more so, I find it an awesome way to
// *implement* a language. So, instead of spending months and
// months to build a whole standards-compliant thing, just make
// something that enables that little self-contained program to run,
// and then iterate from there.
//
// That said. To get that little function above running I'm gonna need
// two syntax constructs, namely `define` and `if`. And I'm also gonna
// need four standard library functions which are `null?`, `cdr`,
// `write` and `newline`.
//
// Let's start with `null?`
//

void assert_arg_count(const char* name, int actual, int expected);

object_t native_nullp(int argct, object_t* args) // null?
{
	assert_arg_count("null?", argct, 1);
	return wrap_bool(is_nil(args[0]));
}

//
// Functional part of this function is fairly trivial: I've defined
// `wrap_bool()` and `is_nil()` earlier, and now I just use them here.
//
// Code that surrounds that logic needs some clarifications though.
//
// One thing is passing arguments as an array and not... Well, it's
// a bloody good question what else can you do here.
//
// See, in Scheme:
// * some functions accept a fixed number of arguments (e.g. `car`
// accepts exactly one and `cons` accepts exactly two)
// * some functions accept a variable number of arguments (like `write`
// that we'll get to in a bit which can have one or two)
// * and some functions accept an arbitrary number of arguments
// (like `list` that can have as many arguments as you give it)
//
// Oh, and the host language is still good ol' C, so you can have as
// much run-time metadata attached to a function as you want. For as
// long as you can figure out a way to do it all yourself.
//
// That's why the most sane interface I could come up with is to
// simply pass an array of arguments, and then let the function itself
// figure out if it's happy with what it got, or if it isn't. Which is
// exactly what I'm doing here.
//
// The other thing that needs explanation is that little `// null?`
// comment. Why it is there is because I need this function to
// eventually end up in REPL scope, which means I'll have to have a
// registration function that will put it there. But I'm also not eager to
// maintain it manually, so I'm going to put together a little script that
// scans the source file and generates the code for me.
//
// Well, and since Scheme has more valid characters for identifiers
// than C does, I can't simply name this function `native_null?`, hence
// I have to provide the Scheme name separately, and this is the purpose
// of that little comment.
//
// Okay, now let's do `cdr`
//

object_t native_cdr(int argct, object_t* args) // cdr
{
	assert_arg_count("cdr", argct, 1);
	pair_t pair = assert_pair("as an argument #1 of cdr", args[0]);
	object_t result = cdr(pair);
	incref(result);
	return result;
}

//
// With all gotchas already explained, this code is so straightforward
// that I don't want to talk about it.
//
// Instead I want to talk about sports. My favourite kind of physical
// activity is long-distance running. *Really* long distance running.
// Which means, marathons.
//
// Now, as we all know, some things are easy, and some are hard.
// Also, some things are simple, and some are complex. And also, there's
// a belief among certain people from Nerd and Nerdish tribes that
// simple things must be easy, and that hard things must be complex.
//
// I admit I had few too many White Zinfandels on my way here, so
// you'll have to endure me rambling all over the place. That said.
// Nerd tribe is a well established notion, so I'm not gonna explain
// what it is. While Nerdish tribe is a certain type of people who were
// taught that being smart is kinda cool, but weren't taught how to be
// properly smart. So they just keep pounding you with their erudition
// and intellect, even if situation is such that a properly smart thing
// to do would be to just shut the fuck up.
//
// Anyway. Marathons prove that belief to be wrong. Conceptually,
// running a marathon is very **simple**: you just put one foot in front
// of the other, and that's pretty much it. But sheer amount of how
// many times you should do that to complete a 42.2 kilometer distance is
// what makes a marathon pretty **hard**, and dealing with that hardness
// is what makes it more complex than it seems.
//
// In fact, there's a whole bunch of stuff that come into play: physical
// training regimen, mental prep, choice of gear, diet, rhythm, pace,
// handling of weather conditions, even choice of music for the playlist.
//
// In broad terms, options I choose fall into three categories:
//
// 1. Stuff that works **for me**. It has never been perfect, but if
// I generally get it right, then by 38th kilometer I'm enjoying
// myself, and singing along to whatever Russian punk rock I've got
// in my headphones, and looking forward to a medal and a beer.
//
// 2. Stuff that doesn't work for me. If there's too much of that, then
// by 38th kilometer I'm lying on the tarmac and hope that race
// organizers have a gun to end my suffering.
//
// 3. Stuff that I really really want to work because it looks so
// awesome in theory, **but it still doesn't**, so in the end it's
// still lying on the tarmac and praying for a quick death.
//
// As a matter of fact, doing marathons really helped me to
// crystallize my views on engineering: if an idea looks really great
// on paper, but hasn't been tested in battle, then, well, you know,
// it looks really great. On paper.
//
// Okay, while we're at it, let's also do `car`
//

object_t native_car(int argct, object_t* args) // car
{
	assert_arg_count("car", argct, 1);
	pair_t pair = assert_pair("as an argument #1 of car", args[0]);
	object_t result = car(pair);
	incref(result);
	return result;
}

//
// This function is as straightforward as the previous one, so I don't
// want to talk about it either. Instead I want to talk about philosophy.
//
// Quite a few times in this story I said "there are three ways to do
// it / three options to choose from / three categories it falls
// into." And it's not just my personal gimmick with number 3, but
// in fact it's Hegelian(-ish) dialectic.
//
// Just to be clear, I'm not nearly as good at philosophy as I am at
// programming, and you can see how mediocre my code is. With that
// in mind, the best way I can explain it is this.
//
// Imagine you've been told that the language you can use in an
// upcoming project can be either Java or Ancient Greek. And you're,
// like, wait a minute, but how can I choose between Java and Ancient
// Greek?
//
// This is a perfectly valid question. But we're going to take one step
// deeper and look into why it's perfectly valid. And, well, it's because
// while both Java and Ancient Greek are languages, there's otherwise
// not much common between them, and that's why there's no readily
// available frame of reference in which they can be compared.
//
// And what it means is that you can only compare things that are
// sufficiently similar or else you simply can't define criteria for such
// comparison.
//
// This idea sounds obscenely trivial (like most philosophy does after
// stripping out all the smartass fluff), but it's immensly powerful.
//
// One way to practically apply this idea is: when choosing between
// multiple options don't jump immediately into the differences between
// them, but instead start with challenging the commonalities.
//
// Say you're deciding whether to build an application with Django or
// with Rails. Both are rich web frameworks that utilize dynamically-typed
// languages. Which means you implicitly excluded:
//
// * statically typed languages (Java, Go, Kotlin, ...)
//
// * micro-frameworks (Flask, Express, ...)
//
// * non-web applications (desktop, mobile, CLI, ...)
//
// * using an off-the-shelf solution instead of building your own
//
// Do you agree with all these choices? If yes, great, go ahead with your
// Django versus Rails analysis. If no, well, you might have just saved
// yourself from putting a lot of effort into a wrong thing.
//
// More advanced method is this:
//
// 1. Start with some idea.
//
// 2. Design an alternative idea that is different from the first one in
// every way you could think of.
//
// 3. Look at two options at hand and find the similarities between them
// **that you didn't even think about,** those are your blind spots.
//
// 4. Come up with a synthetic idea that takes your newly found blind
// spots into account.
//
// 5. Repeat until you run out of blind spots and cover entire decision
// space for the problem you're solving.
//
// Just try it out and you'll see how powerful these techniques are.
//
// Okay, enough philosophy, let's get back to coding and do some I/O.
//

struct port;
typedef struct port* port_t;

void assert_vararg_count(const char* name, int actual, int least, int most);
port_t assert_port(const char* context, object_t obj);
FILE* unwrap_port(port_t);

object_t native_write(int argct, object_t* args) // write
{
	assert_vararg_count("write", argct, 1, 2);
	FILE* out = (argct == 1)
		? stdout
		: unwrap_port(assert_port("as an argument #2 of write", args[1]));
	write_object(out, args[0]);
	return wrap_nil();
}

//
// Nothing particularly spectacular here. "Port" is how they call a stream
// (or a file handler or what have you) in Scheme. Oh, and also it can
// accept one or two arguments. Aside from that it's all straightforward.
//

object_t native_newline(int argct, object_t* args) // newline
{
	assert_vararg_count("newline", argct, 0, 1);
	FILE* out = (argct == 0)
		? stdout
		: unwrap_port(assert_port("as an argument #2 of newline", args[0]));
	fputs("\n", out);
	return wrap_nil();
}

//
// So is this one. Now I'll do something more complex and evaluate syntax
// for `if`
//

#define ASSERT(expr, fmt, ...) do { \
	if (! (expr)) \
		DIE(fmt, ##__VA_ARGS__); \
} while(0)

bool eval_boolean(scope_t scope, object_t expr);

object_t syntax_if(scope_t scope, object_t code) // if
{
	object_t test = pop_list(&code);
	ASSERT(test, "`if` without <test> part");

	object_t consequent = pop_list(&code);
	ASSERT(consequent, "`if` without <consequent> part");

	object_t alternate = pop_list(&code);
	ASSERT(is_nil(code), "Malformed `if`");

	if (eval_boolean(scope, test))
		return eval_lazy(scope, consequent);
	else if (alternate)
		return eval_lazy(scope, alternate);
	else
		return wrap_nil();
}

//
// A tiny bit more complex really. See, when code you interpret is in a
// common convenient data structure, it really takes effort and
// determination to make things hard. And although I (obviously) have
// a rant on this topic, I think I put enough random ramblings into this
// chapter already, so I'm going to park it for now and instead evaluate
// `define`
//

symbol_t assert_symbol(const char* context, object_t);

void define(scope_t, symbol_t key, object_t value);
object_t wrap_lambda(scope_t, object_t formals, object_t code);

object_t syntax_define(scope_t scope, object_t code) // define
{
	object_t head = pop_list(&code);
	ASSERT(head, "Malformed `define`");

	symbol_t key;
	object_t value;
	pair_t key_params;

	if ((key = to_symbol(head))) {
		object_t expr = pop_list(&code);
		ASSERT(expr, "`define` block without <expression>");
		ASSERT(is_nil(code), "Malformed `define`");
		value = eval_eager(scope, expr);
	} else if ((key_params = to_pair(head))) {
		key = assert_symbol("variable name in `define`", car(key_params));
		value = wrap_lambda(scope, cdr(key_params), code);
	} else {
		DEBUG("head", head);
		DIE("Malformed `define`");
	}

	define(scope, key, value);
	decref(value);
	return wrap_nil();
}

//
// This one is a little bit trickier, but that's just because Scheme has
// alternative form for `define` so that `(define (foo bar) bla bla)` is
// equivalent to `(define foo (lambda (bar) bla bla))`, and I have to
// handle both cases.
//
// Now, I promised to make a handful of simple helpers, so I'm
// just going to do it to get them out of the way.
//

void assert_arg_count(const char* name, int actual, int expected)
{
	if (actual != expected)
		DIE("Expected %d arguments for %s, got %d", expected, name, actual);
}

void assert_vararg_count(const char* name, int actual, int least, int most)
{
	if (actual < least)
		DIE("Expected at least %d arguments for %s, got %d", least, name, actual);
	if (actual > most)
		DIE("Expected at most %d arguments for %s, got %d", most, name, actual);
}

port_t to_port(object_t);

port_t assert_port(const char* name, object_t obj)
{
	port_t port = to_port(obj);
	if (! port)
		DIE("Expected port %s, got %s instead", name, typename(obj));
	return port;
}

symbol_t assert_symbol(const char* context, object_t obj)
{
	symbol_t sym = to_symbol(obj);
	if (sym)
		return sym;
	DIE("Expected %s to be a symbol, got %s instead", context, typename(obj));
}

//
// And this is a good place to wrap up this chapter and move on to
// ## Chapter 5 where I reinvent the wheel and then collect garbage
//
// I think I mentioned Pascal somewhere in Chapter 2 of this story. As
// I remember, I haven't touched it since high school, which makes it two
// decades, give or take a year. But, truth be told, I (very briefly)
// dabbled with an idea of using Pascal in this very project. I mean,
// implementing an obscure programming language in another obscure
// programming language, just how cool is that!
//
// Then I scanned the manual for Free Pascal and realized that no, this
// is definitely not gonna fly. The show-stopper was Pascal's lack of
// forward declaration for types, so you must define a type above the
// places where you use it, or else it's an error. And if it doesn't seem
// like such a big deal, have you noticed that by now I wrote a
// full-fledged parser, and a good chunk of an evaluation engine, and few
// bits of standard library, and I still haven't defined *a single* data
// type. I just said "oh yeah, this is a pointer to `struct object`,
// don't ask," and C compiler is, like, "okay, I'm cool with that." But
// in Pascal it's apparently illegal.
//
// And no, this is not just a funny gimmick. In fact it's an experiment
// in tackling one of the hardest problems in software engineering which
// is: how to demonstrate that design of your program makes any sense?
//
// One solution is to have a design spec... But wait, let's take a step
// back.
//
// One option is to go, let's call it aggressively ultra-pragmatic, and to
// perform a variation of "This program WORKS! People USE IT! How DARE you
// ask for more than that?! Why do you even CARE?!"
//
// Or something like that. And that's fine. I mean, if your boss is
// okay with that, and your customers are okay with that, and your
// coworkers are okay with that, who am I to judge, really?
//
// But this disclaimer had to be made. Before I dive into ranting about
// suboptimal solutions, I'm obliged to give proper credit to people
// who at least acknowledge the problem. Or else it'd be like being
// told that "compared to serious runners you're below mediorce" by
// people  who haven't exercised for, I dunno, four reincarnations.
//
// So. One way to tackle this problem is to have a design spec where
// you use natural human language to elaborate the underlying logic,
// interactions between individual pieces, decisions made, and all
// that stuff. It's not a bad idea per se, but it has... Not so much
// a pitfall, but more like a fundamental limitation: you can't
// just give your design document to a computer and tell it to
// execute it. You still have to write machine-readable code. So
// you end up with two intrinsically independent artifacts, and
// unless you have a proper process to prevent it they get out of
// sync, and your spec becomes "more what you call guidelines" than
// actual spec.
//
// Another option is to stick with just code, and then make sure
// your code is excellent: concise functions, transparent interfaces,
// meaningful naming, all the good stuff. And while it's certainly a great
// idea it has its own fundamental limitation: programming languages just
// aren't particularly good for expressing intent. I mean, you can get
// pretty far with explaining *how* your program works by code itself,
// but not such much *why.* For that you still need some kind of
// documentation.
//
// And so my idea, and keep in mind it's an ongoing experiment and
// not The Next Silver Bullet for which I'm selling certifications,
// was to essentially combine the two in such a way that the whole
// thing has a logical flow of a spec, but then the code is organically
// embedded into it, but then in the end it's just a normal program
// (albeit with copious comments) that can be compiled by a vanilla
// compiler rather than some "literate programming" contraption.
//
// And this is why it's important that underlying programming
// language is liberal with regards to code layout, or else you end
// up with a Sysyphean task when you must state that this data type
// has such and such fields even though you won't be able to justify
// their necessity until fifty pages later.
//
// All this was an extremely long introduction to the very first real
// data type in this program. But finally I proudly present you
//

struct array {
	void** data;
	int available;
	int size;
};

typedef struct array* array_t;

//
// and after seeing it you're obliged to ask "Dude, but why don't
// you use C++, at least you'll get `std::vector` for free?"
//
// And that's indeed a reasonable question. I even agree that from
// a practical standpoint C++ would be at least as good as plain C,
// and likely better.
//
// However. For a project like this one the very concept of a
// "practical standpoint" is a slippery slope. Because right after
// "but why don't you use C++ and get `std::vector` for free?" comes
// "but why don't you use Java and get garbage collector for free?"
// and then "but why you don't just `apt install scheme`?" and
// eventually "but why you don't learn some hipper language?"
//
// And then you find yourself surrounded by weirdos at a shady Kotlin
// meetup, and you lost count how many tasteless beers you had, and
// you're wondering if things will end very bad or even worse.
//
// No, really, the purpose of this whole undertaking is not to build
// anything. It's not even to learn, not something specific anyway, but
// mostly just to have fun doing it. Essentially it's simply one big
// coding exercise. And that's why one of the guiding principles here is
// when choosing between a more practical option and one that's less
// practical but still reasonable, go for the latter.
//
// Oh, and "still reasonable" explains why I use plain C and not,
// I dunno, Fortran 66. Writing a Scheme interpreter in Fortran 66
// will get you deep into "pain and humiliation for the sake of
// pain and humiliation" territory. Of course there's absolutely
// nothing wrong with enjoying some pain and humiliation, except
// that a computer is not the best home appliance to use for these
// purposes, if you get what I mean.
//
// Anyway. Dynamically expandable array is one of the most trivial
// things in this entire story, and I'm not going to spend any time
// explaining it.
//

void init_array(array_t arr)
{
	arr->data = malloc(sizeof(void*) * 8);
	arr->available = 8;
	arr->size = 0;
}

void dispose_array(array_t arr)
{
	free(arr->data);
}

void push_array(array_t arr, void* entry)
{
	if (arr->size == arr->available) {
		arr->available *= 2;
		arr->data = realloc(arr->data, sizeof(void*)*arr->available);
	}
	arr->data[arr->size++] = entry;
}

void* pop_array(array_t arr)
{
	if (arr->size == 0)
		return NULL;
	return arr->data[--arr->size];
}

//
// But the reason *why* I suddenly decided I need a dynamically expandable
// list is because I'm gonna need it to build a garbage collector. Not
// that I really need a garbage collector, but when you have a parser, and
// an evaluation engine, and a chunk of the standard library, it's so
// hard to resist from a not-so-subtle Hunter S. Thompson reference.
//
// Of course I need a garbage collector in my Scheme interpreter, but it
// doesn't have to be very sophisticated.
//

#include <strings.h>

bool is_garbage(object_t);
void mark_reachable(void*);
void mark_reached(void*);
void reclaim_object(object_t obj);
void reset_gc_state(object_t);

struct array ALL_OBJECTS;
struct array REACHABLE_OBJECTS;

void collect_garbage()
{
	object_t obj;

	REACHABLE_OBJECTS.size = 0;

	for (int i=ALL_OBJECTS.size-1; i>=0; i--)
		reset_gc_state(ALL_OBJECTS.data[i]);

	while ((obj = pop_array(&REACHABLE_OBJECTS)))
		mark_reached(obj);

	int count = ALL_OBJECTS.size, index = 0;

	while (index < count) {
		obj = ALL_OBJECTS.data[index];
		if (is_garbage(obj)) {
			reclaim_object(obj);
			ALL_OBJECTS.data[index] = ALL_OBJECTS.data[--count];
		} else {
			index++;
		}
	}

	ALL_OBJECTS.size = count;
}

//
// And so I manage to get away with a simplistic mark-and-sweep, which
// makes a nice segue to
// ## Chapter 6, where things become objectionable
// CUTOFF

struct type {
	const char* name;
	void (*reach)(void*);
	void (*dispose)(void*);
	void (*write)(FILE*, void*);
	object_t (*invoke)(void*, int, object_t*);
};

typedef struct type* type_t;

//

struct object {
	type_t type;
	unsigned stackrefs;
	enum {
		GARBAGE,
		REACHABLE,
		REACHED
	} gc_state;
};

//

object_t register_object(void* ptr, const type_t type)
{
	static int threshold = 2560;

	object_t obj = ptr;
	obj->type = type;
	obj->stackrefs = 1;
	push_array(&ALL_OBJECTS, obj);

	if (ALL_OBJECTS.size > threshold) {
		collect_garbage();
		threshold = ALL_OBJECTS.size * 2;
	}

	return obj;
}

void reset_gc_state(object_t obj)
{
	if (obj->stackrefs == 0) {
		obj->gc_state = GARBAGE;
	} else {
		obj->gc_state = REACHABLE;
		push_array(&REACHABLE_OBJECTS, obj);
	}
}

//

void write_object(FILE* out, object_t obj)
{
	if (obj->type->write)
		obj->type->write(out, obj);
	else
		fprintf(out, "[%s@%p]", obj->type->name, obj);
}

//

#include <assert.h>

void decref(struct object* obj)
{
	assert(obj->stackrefs > 0);
	obj->stackrefs--;
}

void incref(struct object* obj)
{
	obj->stackrefs++;
}

bool is_garbage(object_t obj)
{
	return obj->gc_state == GARBAGE;
}

const char* typename(object_t obj)
{
	return obj->type->name;
}

//

void mark_reached(void* ptr)
{
	if (! ptr)
		return;

	object_t obj = ptr;
	if (obj->gc_state == REACHED)
		return;

	obj->gc_state = REACHED;
	if (obj->type->reach)
		obj->type->reach(obj);
}

//

// Chapter Eight

struct pair {
	struct object self;
	object_t car;
	object_t cdr;
};

//

object_t car(pair_t pair)
{
	return pair->car;
}

object_t cdr(pair_t pair)
{
	return pair->cdr;
}

//

void pair_reach(void* obj)
{
	pair_t pair = obj;
	mark_reachable(pair->car);
	mark_reachable(pair->cdr);
}

void write_pair(FILE* out, void* ptr)
{
	pair_t pair = ptr;

	fputs("(", out);
	write_object(out, car(pair));

	object_t obj = cdr(pair);

	while (true) {
		if (is_nil(obj))
			break;

		pair = to_pair(obj);
		if (pair) {
			fputs(" ", out);
			write_object(out, car(pair));
			obj = cdr(pair);
			continue;
		}

		fputs(" . ", out);
		write_object(out, obj);
		break;
	}
	fputs(")", out);
}

struct type TYPE_PAIR = {
	.name = "pair",
	.write = write_pair,
	.reach = pair_reach,
};

object_t wrap_pair(object_t head, object_t tail)
{
	pair_t pair = malloc(sizeof(*pair));
	pair->car = head;
	pair->cdr = tail;
	return register_object(pair, &TYPE_PAIR);
}

pair_t to_pair(object_t obj)
{
	if (obj->type == &TYPE_PAIR)
		return (pair_t)obj;
	return NULL;
}

// CUTOFF

//

struct dict_entry {
	symbol_t key;
	void* value;
};

struct dict {
	struct dict_entry* data;
	int size;
	int used;
};

//

void* dict_lookup(struct dict*, const char*, int hash);

int strhash(const char*);

//

void dict_dispose(struct dict*);

//

void dict_dispose(struct dict* dict)
{
	free(dict->data);
	bzero(dict, sizeof(struct dict));
}

//

void dict_init(struct dict* dict)
{
	bzero(dict, sizeof(struct dict));
}

//

int strhash(const char*);
int compare_entry(struct dict_entry* entry, int hash, const char* key);

int hash_to_index(int hash, int size)
{
	int index = hash % size;
	if (index < 0)
		index += size;
	return index;
}

void* dict_lookup_fast(struct dict* dict, const char* key, int hash)
{
	if (dict->size == 0)
		return NULL;

	int size = dict->size;
	int index = hash_to_index(hash, size);
	while (true) {
		struct dict_entry* entry = &dict->data[index];
		if (entry->key == NULL)
			return NULL;

		int diff = compare_entry(entry, hash, key);
		if (diff < 0) {
			index = (index + 1) % dict->size;
		} else if (diff == 0) {
			return entry->value;
		} else {
			return NULL;
		}
	}
}

//

void* dict_put(struct dict*, symbol_t, void*);

struct type TYPE_SYNTAX = {
	.name = "syntax"
};

struct syntax {
	struct object self;
	object_t (*eval)(scope_t, object_t);
};

typedef struct syntax* syntax_t;

//

void dict_enlarge(struct dict* dict);

void dict_reinsert(struct dict*, struct dict_entry*);

//

void* dict_put_impl(struct dict*, const char*, void*, int);

//

#include <assert.h>

void dict_reinsert(struct dict* dict, struct dict_entry* entry);

int strhash(const char* key)
{
	int result = 0;
	while (*key)
		result = result * 7 + *(key++);
	return result;
}

//

bool is_garbage(object_t);
bool hasrefs(object_t);
void mark_garbage(object_t);

void reach_object(void*);

//

void mark_reachable(void*);

void dict_reach(struct dict* dict)
{
	for (int i=dict->size-1; i>=0; i--)
		if (dict->data[i].key)
			mark_reachable(dict->data[i].value);
}

//

//

bool hasrefs(object_t obj)
{
	return obj->stackrefs > 0;
}

void mark_garbage(object_t obj)
{
	obj->gc_state = GARBAGE;
}

//

array_t get_object_pool(type_t);

void reclaim_object(object_t obj)
{
	if (obj->type->dispose)
		obj->type->dispose(obj);
	free(obj);
}

//

void write_nil(FILE* out, void* obj)
{
	fprintf(out, "()");
}

struct type TYPE_NIL = {
	.name = "nil",
	.write = write_nil,
};

struct object NIL = {
	.type = &TYPE_NIL,
	.stackrefs = 1,
};

object_t wrap_nil(void)
{
	object_t obj = &NIL;
	obj->stackrefs++;
	return obj;
}

bool is_nil(struct object* obj)
{
	return obj->type == &TYPE_NIL;
}

//

struct boolean {
	struct object self;
	bool value;
};

typedef struct boolean* boolean_t;

void write_bool(FILE* out, void* obj)
{
	boolean_t b = obj;
	fputs(b->value ? "#t" : "#f", out);
}

struct type TYPE_BOOL = {
	.name = "bool",
	.write = write_bool,
};

struct boolean TRUE = {
	.self = {
		.type = &TYPE_BOOL,
		.stackrefs = 1,
	},
	.value = true,
};

struct boolean FALSE = {
	.self = {
		.type = &TYPE_BOOL,
		.stackrefs = 1,
	},
	.value = false,
};

object_t wrap_bool(bool v)
{
	boolean_t b = v ? &TRUE : &FALSE;
	object_t obj = (object_t)b;
	obj->stackrefs++;
	return (object_t)obj;
}

bool eval_boolean(scope_t scope, object_t expr)
{
	object_t obj = eval_eager(scope, expr);
	bool result = (obj != wrap_bool(false));
	decref(obj);
	return result;
}

//

struct integer {
	struct object self;
	int value;
};

typedef struct integer* integer_t;

void write_int(FILE* out, void* obj)
{
	integer_t num = obj;
	fprintf(out, "%d", num->value);
}

struct type TYPE_INT = {
	.name = "int",
	// .size = sizeof(struct integer),
	.write = write_int
};

object_t wrap_int(int v)
{
	integer_t num = malloc(sizeof(*num));
	num->value = v;
	return register_object(num, &TYPE_INT);
}

//

struct string {
	struct object self;
	char* value;
};

typedef struct string* string_t;

void dispose_string(void* obj)
{
	string_t str = obj;
	free(str->value);
}

void write_string(FILE* out, void* obj)
{
	string_t str = obj;
	fprintf(out, "\"%s\"", str->value);
}

struct type TYPE_STRING = {
	.name = "string",
	.dispose = dispose_string,
	.write = write_string,
};

object_t wrap_string(const char* v)
{
	string_t str = malloc(sizeof(*str));
	str->value = strdup(v);
	return register_object(str, &TYPE_STRING);
}

//

struct symbol {
	struct object self;
	char* value;
	int hash;
};

typedef struct symbol* symbol_t;

void write_symbol(FILE* out, void* obj)
{
	symbol_t symbol = obj;
	fprintf(out, "%s", symbol->value);
}

void symbol_dispose(void* obj)
{
	symbol_t symbol = obj;
	free(symbol->value);
}

struct type TYPE_SYMBOL = {
	.name = "symbol",
	.write = write_symbol,
	.dispose = symbol_dispose,
};

struct dict ALL_SYMBOLS;

object_t wrap_symbol(const char* v)
{
	int hash = strhash(v);
	object_t result = dict_lookup_fast(&ALL_SYMBOLS, v, hash);
	if (! result) {
		symbol_t sym = malloc(sizeof(*sym));
		sym->value = strdup(v);
		sym->hash = hash;
		result = register_object(sym, &TYPE_SYMBOL);
		dict_put(&ALL_SYMBOLS, sym, result);
	}
	incref(result);
	return result;
}

const char* unwrap_symbol(symbol_t sym)
{
	return sym->value;
}

symbol_t to_symbol(object_t obj)
{
	if (obj->type == &TYPE_SYMBOL)
		return (symbol_t)obj;
	return NULL;
}

//

struct lambda {
	struct object self;
	object_t body;
	scope_t l_scope;
	symbol_t l_name;
	struct array params;
};

typedef struct lambda* lambda_t;

void lambda_reach(void* obj)
{
	lambda_t lambda = obj;
	mark_reachable(lambda->body);
	mark_reachable(lambda->l_scope);
	mark_reachable(lambda->l_name);
	for (int i=lambda->params.size-1; i>=0; i--)
		mark_reachable(lambda->params.data[i]);
}

void lambda_dispose(void* obj)
{
	lambda_t lambda = obj;
	dispose_array(&lambda->params);
}

object_t invoke_lambda(void* lambda, int argct, object_t* args);

void write_lambda(FILE* out, void* ptr)
{
	lambda_t lambda = ptr;

	fputs("(lambda (", out);
	for (int i=0; i<lambda->params.size; i++) {
		if (i > 0)
			fputc(' ', out);
		write_object(out, lambda->params.data[i]);
	}
	fputc(')', out);
	object_t body = lambda->body, obj;

	while ((obj = pop_list(&body))) {
		fputc(' ', out);
		write_object(out, obj);
	}

	fputc(')', out);
}

struct type TYPE_LAMBDA = {
	.name = "lambda",
	.reach = lambda_reach,
	.dispose = lambda_dispose,
	.invoke = invoke_lambda,
	.write = write_lambda
};

lambda_t to_lambda(object_t obj)
{
	if (obj->type == &TYPE_LAMBDA)
		return (lambda_t)obj;
	return NULL;
}

object_t optimize(scope_t scope, object_t body);

object_t wrap_lambda(scope_t scope, object_t params, object_t body)
{
	object_t optimized_body = optimize(scope, body);

	lambda_t lambda = malloc(sizeof(*lambda));
	lambda->l_name = NULL;
	lambda->body = optimized_body;
	lambda->l_scope = scope;
	init_array(&lambda->params);

	object_t param;
	while ((param = pop_list(&params))) {
		assert(to_symbol(param));
		push_array(&lambda->params, param);
	}

	object_t result = register_object(lambda, &TYPE_LAMBDA);

	decref(optimized_body);

	return result;
}

//

struct thunk {
	struct object self;
	lambda_t lambda;
	int argct;
	object_t* args;
};

void thunk_dispose(void* obj)
{
	thunk_t thunk = obj;
	free(thunk->args);
}

void thunk_reach(void* obj)
{
	thunk_t thunk = obj;
	mark_reachable(thunk->lambda);
	for (int i=thunk->argct-1; i>=0; i--)
		mark_reachable(thunk->args[i]);
}

struct type THUNK = {
	.name = "thunk",
	// .size = sizeof(struct thunk),
	.reach = thunk_reach,
	.dispose = thunk_dispose,
};

thunk_t to_thunk(object_t obj)
{
	if (obj->type == &THUNK)
		return (thunk_t)obj;
	return NULL;
}

//

struct scope {
	struct object self;
	struct dict s_binds;
	scope_t parent;
};

object_t lookup_in_scope(scope_t scope, symbol_t key)
{
	while (scope) {
		void* value = dict_lookup_fast(&scope->s_binds, key->value, key->hash);
		if (value)
			return value;
		scope = scope->parent;
	}
	return NULL;
}

void scope_reach(void* obj)
{
	scope_t scope = obj;
	struct dict* binds = &scope->s_binds;
	dict_reach(binds);
	mark_reachable(scope->parent);
}

void scope_dispose(void* obj)
{
	scope_t scope = obj;
	dict_dispose(&scope->s_binds);
}

struct type SCOPE = {
	.name = "scope",
	// .size = sizeof(struct scope),
	.reach = scope_reach,
	.dispose = scope_dispose,
};

scope_t REPL_SCOPE = NULL;

object_t derive_scope(scope_t parent);

scope_t get_repl_scope()
{
	return REPL_SCOPE;
}

//

void assign_name(object_t value, symbol_t key);

void define(scope_t scope, symbol_t key, object_t value)
{
	const char* strkey = unwrap_symbol(key);
	void* ptr = dict_put(&scope->s_binds, key, value);
	if (ptr != NULL)
		DIE("%s is already defined", strkey);
	if (! scope->parent)
		incref(value);
	assign_name(value, key);
}

//

void eval_define(scope_t eval_scope, scope_t bind_scope, symbol_t key, object_t expr)
{
	object_t result = eval_eager(eval_scope, expr);
	define(bind_scope, key, result);
	decref(result);
}

//

struct native {
	struct object self;
	object_t (*invoke)(int, object_t*);
};

typedef struct native* native_t;

object_t invoke_native(void*, int, object_t*);

struct type TYPE_NATIVE = {
	.name = "native",
	.invoke = invoke_native
};

//

object_t derive_scope(scope_t parent)
{
	scope_t scope = malloc(sizeof(*scope));
	dict_init(&scope->s_binds);
	scope->parent = parent;
	return register_object(scope, &SCOPE);
}

//

scope_t to_scope(object_t obj)
{
	if (obj->type == &SCOPE)
		return (scope_t)obj;
	return NULL;
}

//

object_t eval_block(scope_t scope, object_t code)
{
	object_t result = wrap_nil(), expr;

	while ((expr = pop_list(&code))) {
		result = force(result);
		decref(result);
		result = eval_lazy(scope, expr);
	}

	return result;
}

//

object_t invoke_lambda(void* ptr, int argct, object_t* args)
{
	lambda_t lambda = ptr;
	struct array* params = &lambda->params;
	const char* name = lambda->l_name ? unwrap_symbol(lambda->l_name) : NULL;

	assert_arg_count(name, argct, params->size);

	object_t scope_obj = derive_scope(lambda->l_scope);
	scope_t scope = to_scope(scope_obj);

	for (int i=0; i<params->size; i++)
		define(scope, params->data[i], args[i]);
	object_t result = eval_block(scope, lambda->body);

	decref(scope_obj);
	return result;
}

//

void register_stdlib_syntax(void);
void register_stdlib_functions(void);

void setup_runtime()
{
	dict_init(&ALL_SYMBOLS);
	init_array(&ALL_OBJECTS);
	init_array(&REACHABLE_OBJECTS);
	REPL_SCOPE = (scope_t)derive_scope(NULL);
	register_stdlib_syntax();
	register_stdlib_functions();
	execute_file("stdlib.scm");
}

//

void dict_decref(struct dict* d)
{
	for (int i=d->size-1; i>=0; i--)
		if (d->data[i].key)
			decref(d->data[i].value);
}

//

void teardown_runtime()
{
	dict_decref(&REPL_SCOPE->s_binds);
	// scope_dispose(REPL_SCOPE);
	decref(&REPL_SCOPE->self);
	dict_decref(&ALL_SYMBOLS);
	dict_dispose(&ALL_SYMBOLS);
	collect_garbage();
	dispose_array(&ALL_OBJECTS);
	dispose_array(&REACHABLE_OBJECTS);
}

//

object_t pop_list_or_die(object_t* ptr)
{
	object_t result = pop_list(ptr);
	if (! result)
		DIE("Premature end of list");
	return result;
}

object_t syntax_quote(scope_t scope, object_t code) // quote
{
	object_t result = pop_list_or_die(&code);
	incref(result);
	return result;
}

//

bool unbox_int(int*, object_t);

bool eq(struct object* x, struct object* y);

object_t native_cons(int argct, object_t* args) // cons
{
	assert_arg_count("cons", argct, 2);
	return wrap_pair(args[0], args[1]);
}

object_t native_eqp(int argct, object_t* args) // eq?
{
	assert_arg_count("eq?", argct, 2);
	return wrap_bool(eq(args[0], args[1]));
}

object_t native_list(int argct, object_t* args) // list
{
	object_t result = wrap_nil();

	for (int i=argct-1; i>=0; i--)
		push_list(&result, args[i]);

	return result;
}

object_t native_fold(int argct, object_t* args) // fold
{
	assert_arg_count("fold", argct, 3);

	object_t func = args[0];
	object_t seed = args[1];
	object_t seq = args[2];

	object_t result = seed;
	incref(result);

	object_t item;

	while ((item = pop_list(&seq))) {
		object_t args[] = {result, item};
		incref(item);
		result = invoke(func, 2, args);
	}
	return result;
}

int unbox_int_or_die(const char*, object_t);

object_t native_modulo(int argct, object_t* args) // modulo
{
	assert_arg_count("modulo", argct, 2);
	int a = unbox_int_or_die("modulo", args[0]);
	int b = unbox_int_or_die("modulo", args[1]);
	return wrap_int(a % b);
}

object_t native_num_divide(int argct, object_t* args) // /
{
	assert_arg_count("/", argct, 2);
	int a = unbox_int_or_die("/", args[0]);
	int b = unbox_int_or_die("/", args[1]);
	return wrap_int(a / b);
}

object_t native_num_equals(int argct, object_t* args) // =
{
	assert_arg_count("=", argct, 2);
	int a = unbox_int_or_die("=", args[0]);
	int b = unbox_int_or_die("=", args[1]);
	return wrap_bool(a == b);
}

object_t native_num_less(int argct, object_t* args) // <
{
	assert_arg_count("<", argct, 2);
	int a = unbox_int_or_die("<", args[0]);
	int b = unbox_int_or_die("<", args[1]);
	return wrap_bool(a < b);
}

object_t native_num_minus(int argct, object_t* args) // -
{
	assert_arg_count("-", argct, 2);
	int a = unbox_int_or_die("-", args[0]);
	int b = unbox_int_or_die("-", args[1]);
	return wrap_int(a - b);
}

object_t native_num_mult(int argct, object_t* args) // *
{
	assert_arg_count("*", argct, 2);
	int a = unbox_int_or_die("*", args[0]);
	int b = unbox_int_or_die("*", args[1]);
	return wrap_int(a * b);
}

object_t native_num_plus(int argct, object_t* args) // +
{
	int n = 0;
	for (int i=0; i<argct; i++)
		n += unbox_int_or_die("+", args[i]);
	return wrap_int(n);
}

object_t native_pairp(int argct, object_t* args) // pair?
{
	assert_arg_count("pair?", argct, 1);
	return wrap_bool(to_pair(args[0]));
}

object_t native_setcdr(int argct, object_t* args) // set-cdr!
{
	assert_arg_count("set-cdr!", argct, 2);
	pair_t pair = assert_pair("as argument #1 of set-cdr!", args[0]);
	pair->cdr = args[1];
	return wrap_nil();
}

object_t native_symbolp(int argct, object_t* args) // symbol?
{
	assert_arg_count("symbol?", argct, 1);
	bool result = to_symbol(args[0]) != NULL;
	return wrap_bool(result);
}

object_t reverse(object_t list);

object_t native_reverse(int argct, object_t* args) // reverse
{
	assert_arg_count("reverse", argct, 1);
	object_t result = reverse(args[0]);
	return result;
}

int unbox_int_or_die(const char* name, object_t arg)
{
	int result;
	if (unbox_int(&result, arg))
		return result;
	DIE("Expected argument for %s to be an integer, got %s instead", name, typename(arg));
}

struct character;
typedef struct character* char_t;

char_t to_char(object_t);
char unwrap_char(char_t);

bool eq(struct object* x, struct object* y)
{
	while (true) {
		if (x == y)
			return true;

		pair_t pair_x = to_pair(x);
		pair_t pair_y = to_pair(y);
		if (pair_x && pair_y) {
			if (! eq(car(pair_x), car(pair_y)))
				return false;
			x = cdr(pair_x);
			y = cdr(pair_y);
			continue;
		}

		symbol_t sx = to_symbol(x), sy = to_symbol(y);
		if (sx && sy)
			return strcmp(unwrap_symbol(sx), unwrap_symbol(sy)) == 0;

		char_t cx = to_char(x), cy = to_char(y);
		if (cx && cy)
			return unwrap_char(cx) == unwrap_char(cy);

		DIE("Don't know how to eq? %s against %s", typename(x), typename(y));
	}
}

bool is_symbol(const char* text, object_t obj)
{
	symbol_t sym = to_symbol(obj);
	if (sym)
		return strcmp(text, unwrap_symbol(sym)) == 0;
	return false;
}

object_t syntax_cond(scope_t scope, object_t code) // cond
{
	object_t clause, test;

	while ((clause = pop_list(&code))) {
		test = pop_list_or_die(&clause);
		if (is_symbol("else", test) || eval_boolean(scope, test))
			return eval_block(scope, clause);
	}
	return wrap_nil();
}

object_t syntax_letrec(scope_t outer_scope, object_t code) // letrec
{
	object_t scope_obj = derive_scope(outer_scope);
	scope_t scope = to_scope(scope_obj);

	object_t bindings = pop_list_or_die(&code);
	object_t binding;

	while ((binding = pop_list(&bindings))) {
		object_t keyobj = pop_list_or_die(&binding);
		object_t expr = pop_list_or_die(&binding);
		symbol_t key = assert_symbol("letrec binding name", keyobj);
		object_t value = eval_eager(scope, expr);
		define(scope, key, value);
		decref(value);
	}

	object_t result = eval_block(scope, code);

	decref(scope_obj);
	return result;
}

object_t syntax_let(scope_t outer_scope, object_t code) // let
{
	object_t scope_obj = derive_scope(outer_scope);
	scope_t scope = to_scope(scope_obj);

	object_t bindings = pop_list_or_die(&code);
	object_t binding;

	while ((binding = pop_list(&bindings))) {
		object_t keyobj = pop_list_or_die(&binding);
		symbol_t key = assert_symbol("let binding name", keyobj);
		object_t expr = pop_list_or_die(&binding);
		object_t value = eval_eager(outer_scope, expr);
		define(scope, key, value);
		decref(value);
	}

	object_t result = eval_block(scope, code);

	decref(scope_obj);
	return result;
}

object_t syntax_lambda(scope_t scope, object_t code) // lambda
{
	pair_t pair = to_pair(code);
	assert(pair);
	return wrap_lambda(scope, car(pair), cdr(pair));
}

bool unbox_int(int* ptr, object_t obj)
{
	if (obj->type != &TYPE_INT)
		return false;
	integer_t num = (integer_t)obj;
	*ptr = num->value;
	return true;
}

void mark_reachable(void* ptr)
{
	if (! ptr)
		return;

	object_t obj = ptr;
	if (obj->gc_state == GARBAGE) {
		obj->gc_state = REACHABLE;
		push_array(&REACHABLE_OBJECTS, obj);
	}
}

object_t syntax_letseq(scope_t scope, object_t code) // let*
{
	object_t scope_obj = NULL;

	object_t bindings = pop_list_or_die(&code);

	object_t binding;

	while ((binding = pop_list(&bindings))) {
		object_t key = pop_list_or_die(&binding);
		symbol_t keysym = to_symbol(key);
		if (! keysym)
			DIE("Expected letrec binding name to be a symbol, got %s instead", typename(key));
		object_t expr = pop_list_or_die(&binding);
		object_t new_scope_obj = derive_scope(scope);
		if (scope_obj)
			decref(scope_obj);
		scope_t new_scope = to_scope(new_scope_obj);

		object_t value = eval_eager(scope, expr);
		define(new_scope, keysym, value);
		decref(value);

		scope_obj = new_scope_obj;
		scope = new_scope;
	}

	object_t result = eval_block(scope, code);

	decref(scope_obj);
	return result;
}

string_t to_string(object_t obj)
{
	if (obj->type == &TYPE_STRING)
		return (string_t)obj;
	return NULL;
}

string_t to_string_or_die(const char* name, object_t obj)
{
	string_t str = to_string(obj);
	if (! str)
		DIE("Expected argument of %s to be a str, got %s instead", name, typename(obj));
	return str;
}

const char* unwrap_string(string_t str)
{
	return str->value;
}

struct port {
	struct object self;
	FILE* value;
};

void dispose_port(void* obj)
{
	port_t port = obj;
	fclose(port->value);
}

struct type TYPE_PORT = {
	.name = "port",
	.dispose = dispose_port,
};

struct object* wrap_file(FILE* v)
{
	port_t port = malloc(sizeof(*port));
	port->value = v;
	return register_object(port, &TYPE_PORT);
}

object_t native_open_input_file(int argct, object_t* args) // open-input-file
{
	assert_arg_count("open-input-file", argct, 1);
	string_t name = to_string_or_die("open-input-file", args[0]);
	FILE* f = fopen_or_die(unwrap_string(name), "r");
	return wrap_file(f);
}

port_t to_port(object_t obj)
{
	if (obj->type == &TYPE_PORT)
		return (port_t)obj;
	return NULL;
}

FILE* unwrap_port(port_t port)
{
	return port->value;
}

struct character {
	struct object self;
	char value;
};

void write_char(FILE* out, void* obj)
{
	char_t ch = obj;
	switch (ch->value) {
	case '\n':
		fputs("#\\newline", out);
		break;
	case ' ':
		fputs("#\\space", out);
		break;
	default:
		fprintf(out, "#\\%c", ch->value);
	}
}

struct type TYPE_CHAR = {
	.name = "character",
	// .size = sizeof(struct character),
	.write = write_char
};

struct object* wrap_char(char v)
{
	char_t ch = malloc(sizeof(*ch));
	ch->value = v;
	return register_object(ch, &TYPE_CHAR);
}

object_t native_read_char(int argct, object_t* args) // read-char
{
	assert_arg_count("read-char", argct, 1);
	port_t port = assert_port("as an argument #1 of read-char", args[0]);
	FILE* f = unwrap_port(port);
	int ch = fgetc_or_die(f);
	if (ch == EOF)
		return wrap_nil();
	else
		return wrap_char(ch);
}

object_t syntax_or(scope_t scope, object_t code) // or
{
	object_t expr;

	while ((expr = pop_list(&code)))
		if (eval_boolean(scope, expr))
			return wrap_bool(true);
	return wrap_bool(false);
}

char_t to_char(object_t obj)
{
	if (obj->type == &TYPE_CHAR)
		return (char_t)obj;
	return NULL;
}

char_t to_char_or_die(const char* name, object_t obj)
{
	char_t ch = to_char(obj);
	if (! ch)
		DIE("Expected argument of %s to be a character, got %s instead", name, typename(obj));
	return ch;
}

char unwrap_char(char_t ch)
{
	return ch->value;
}

object_t native_list_to_string(int argct, object_t* args) // list->string
{
	assert_arg_count("list->string", argct, 1);
	object_t list = args[0], obj;
	char buffer[10240];
	int fill = 0;

	while ((obj = pop_list(&list))) {
		if (fill >= 10240)
			DIE("Buffer overflow");
		char_t ch = to_char_or_die("list->string", obj);
		buffer[fill++] = unwrap_char(ch);
	}
	buffer[fill++] = '\0';

	return wrap_string(buffer);
}

object_t native_string_to_list(int argct, object_t* args) // string->list
{
	assert_arg_count("string->list", argct, 1);
	string_t str = to_string_or_die("string->list", args[0]);
	const char* text = unwrap_string(str);

	object_t acc = wrap_nil();
	while (*text) {
		object_t ch = wrap_char(*text);
		push_list(&acc, ch);
		decref(ch);
		text++;
	}
	object_t result = reverse(acc);
	decref(acc);
	return result;
}

object_t native_string_equal(int argct, object_t* args) // string=?
{
	assert_arg_count("string=?", argct, 2);
	string_t a = to_string(args[0]);
	string_t b = to_string(args[1]);

	if (a && b) {
		const char* sa = unwrap_string(a);
		const char* sb = unwrap_string(b);
		return wrap_bool(strcmp(sa, sb) == 0);
	}

	return wrap_bool(false);
}

object_t native_string_length(int argct, object_t* args) // string-length
{
	assert_arg_count("string-length", argct, 1);
	string_t obj = to_string_or_die("string-length", args[0]);
	const char* str = unwrap_string(obj);
	return wrap_int(strlen(str));
}

object_t native_string_ref(int argct, object_t* args) // string-ref
{
	assert_arg_count("string-ref", argct, 2);
	string_t obj = to_string_or_die("string-ref", args[0]);
	int index = unbox_int_or_die("string-ref", args[1]);
	char ch = unwrap_string(obj)[index];
	return wrap_char(ch);
}

void set_in_scope(scope_t scope, symbol_t key, object_t value)
{
	const char* strkey = unwrap_symbol(key);
	scope_t s = scope;

	while (s) {
		object_t old_value = dict_lookup_fast(&s->s_binds, strkey, strhash(strkey));
		if (old_value) {
			dict_put(&s->s_binds, key, value);
			if (! s->parent) {
				incref(value);
				decref(old_value);
			}
			return;
		}
		s = s->parent;
	}
	DIE("Variable %s is not bound to anything", strkey);
}

object_t syntax_set(scope_t scope, object_t code) // set!
{
	object_t head = pop_list_or_die(&code);
	symbol_t key =assert_symbol("variable name in set!", head);
	object_t expr = pop_list_or_die(&code);
	object_t value = eval_eager(scope, expr);
	set_in_scope(scope, key, value);
	decref(value);
	return wrap_nil();
}

object_t native_string_set(int argct, object_t* args) // string-set!
{
	assert_arg_count("string-set!", argct, 3);
	string_t str = to_string_or_die("string-set!", args[0]);
	int index = unbox_int_or_die("string-set!", args[1]);
	char_t ch = to_char_or_die("string-set!", args[2]);
	str->value[index - 1] = ch->value;
	incref(args[0]);
	return args[0];
}

object_t native_not(int argct, object_t* args) // not
{
	assert_arg_count("not", argct, 1);
	return wrap_bool(args[0] == wrap_bool(false));
}

object_t native_display(int argct, object_t* args) // display
{
	for (int i=0; i<argct; i++) {
		string_t str = to_string(args[i]);
		if (str) {
			printf("%s", str->value);
			continue;
		}
		char_t ch = to_char(args[i]);
		if (ch) {
			printf("%c", ch->value);
			continue;
		}
		write_object(stdout, args[i]);
	}
	return wrap_nil();
}

void* dict_put(struct dict* dict, symbol_t sym, void* value)
{
	if ((dict->size == 0) || (dict->used * 2 > dict->size))
		dict_enlarge(dict);

	int hash = sym->hash;
	int index = hash_to_index(hash, dict->size);

	while (true) {
		struct dict_entry* entry = &dict->data[index];

		if (entry->key == NULL) {
			entry->key = sym;
			entry->value = value;
			dict->used++;
			return NULL;
		}

		int diff = compare_entry(entry, hash, sym->value);
		if (diff < 0) {
			index = (index + 1) % dict->size;
			continue;
		} else if (diff == 0) {
			void* old = entry->value;
			entry->value = value;
			return old;
		} else {
			struct dict_entry present = *entry;
			entry->key = sym;
			entry->value = value;
			dict_reinsert(dict, &present);
			return NULL;
		}
	}
}

int compare_entry(struct dict_entry* entry, int hash, const char* key)
{
	if (entry->key->hash < hash)
		return -1;
	if (entry->key->hash > hash)
		return 1;
	return strcmp(entry->key->value, key);
}

void dict_reinsert(struct dict* dict, struct dict_entry* entry)
{
	int index = hash_to_index(entry->key->hash, dict->size);
	while (true) {
		struct dict_entry* cell = &dict->data[index];

		if (cell->key == NULL) {
			*cell = *entry;
			dict->used++;
			return;
		}

		int diff = compare_entry(cell, entry->key->hash, entry->key->value);
		if (diff < 0) {
			index = (index + 1) % dict->size;
		} else if (diff == 0) {
			DEBUG("value", entry->value);
			DIE("Should never happen : %s vs %s", cell->key->value, entry->key->value);
		} else {
			struct dict_entry tmp = *cell;
			*cell = *entry;
			*entry = tmp;
		}
	}
}

object_t native_string_copy(int argct, object_t* args) // string-copy
{
	assert_arg_count("string-copy", argct, 1);
	string_t str = to_string_or_die("string-copy", args[0]);
	return wrap_string(unwrap_string(str));
}

object_t native_string_append(int argct, object_t* args) // string-append
{
	char buffer[10240];
	int fill = 0;
	for (int i=0; i<argct; i++) {
		string_t strobj = to_string_or_die("string-append", args[i]);
		const char* str = unwrap_string(strobj);
		while (*str) {
			if (fill >= 10239)
				DIE("Buffer overflow");
			buffer[fill++] = *(str++);
		}
	}
	buffer[fill] = '\0';
	return wrap_string(buffer);
}

object_t native_substring(int argct, object_t* args) // substring
{
	assert_arg_count("string-copy", argct, 3);
	string_t strobj = to_string_or_die("substring", args[0]);
	const char* str = unwrap_string(strobj);
	int start = unbox_int_or_die("substring", args[1]);
	int end = unbox_int_or_die("substring", args[2]);
	char buffer[10240];
	int fill = 0;

	for (int i=start; i<end; i++) {
		if (fill >= 10239)
			DIE("Buffer overflow");
		buffer[fill++] = str[i];
	}
	buffer[fill] = '\0';
	return wrap_string(buffer);
}

object_t reverse(object_t list)
{
	object_t result = wrap_nil(), obj;

	while ((obj = pop_list(&list)))
		push_list(&result, obj);

	return result;
}

//

native_t to_native(object_t);

void decref_many(int argct, object_t* args);

object_t invoke(object_t func, int argct, object_t* args)
{
	if (! func->type->invoke) {
		DIE("Can't invoke object of type %s", typename(func));
	}
	object_t result = func->type->invoke(func, argct, args);
	decref_many(argct, args);
	return result;
}

//

void decref_many(int argct, object_t* args)
{
	for (int i=argct-1; i>=0; i--)
		decref(args[i]);
}
void array_decref(array_t arr)
{
	decref_many(arr->size, (object_t*)arr->data);
}

object_t wrap_thunk(lambda_t lambda, int argct, object_t* args)
{
	thunk_t thunk = malloc(sizeof(*thunk));

	size_t sz = argct * sizeof(object_t);
	thunk->lambda = lambda;
	thunk->argct = argct;
	thunk->args = malloc(sz);
	memcpy(thunk->args, args, sz);
	decref_many(argct, args);

	return register_object(thunk, &THUNK);
}

object_t eval_thunk(thunk_t thunk)
{
	return invoke_lambda(thunk->lambda, thunk->argct, thunk->args);
}
native_t to_native(object_t obj)
{
	if (obj->type == &TYPE_NATIVE)
		return (native_t)obj;
	return NULL;
}

object_t invoke_native(void* ptr, int argct, object_t* args)
{
	native_t native = ptr;
	return native->invoke(argct, args);
}

void register_native(const char* name, object_t (*func)(int, object_t*))
{
	object_t key = wrap_symbol(name);

	native_t native = malloc(sizeof(*native));
	native->invoke = func;
	object_t obj = register_object(native, &TYPE_NATIVE);

	define(REPL_SCOPE, (symbol_t)key, obj);
	decref(obj);
	decref(key);
}

void dict_enlarge(struct dict* d)
{
	if (d->size == 0) {
		d->data = calloc(sizeof(struct dict_entry), 8);
		d->size = 8;
		return;
	}

	struct dict old = *d;
	d->used = 0;
	d->size *= 2;
	d->data = calloc(sizeof(struct dict_entry), d->size);

	for (int i=0; i<old.size; i++) {
		struct dict_entry* entry = &old.data[i];
		if (entry->key)
			dict_reinsert(d, entry);
	}
	free(old.data);
	// d->size = tmp.size;
	// d->data = tmp.data;
}

syntax_t to_syntax(object_t obj)
{
	if (obj->type == &TYPE_SYNTAX)
		return (syntax_t)obj;
	return NULL;
}

typedef struct macro* macro_t;

macro_t to_macro(object_t);
object_t expand_macro(scope_t scope, macro_t macro, object_t body);

object_t eval_syntax(scope_t scope, object_t head, object_t body)
{
	syntax_t syntax = to_syntax(head);
	if (syntax)
		return syntax->eval(scope, body);

	macro_t macro = to_macro(head);
	if (macro) {
		object_t new_code = expand_macro(scope, macro, body);
		object_t result = eval_lazy(scope, new_code);
		decref(new_code);
		return result;
	}

	return NULL;
}

//

void register_syntax(const char* name, object_t (*func)(scope_t, object_t))
{
	object_t key = wrap_symbol(name);

	syntax_t syntax = malloc(sizeof(struct syntax));
	syntax->eval = func;
	object_t obj = register_object(syntax, &TYPE_SYNTAX);

	define(REPL_SCOPE, (symbol_t)key, obj);

	decref(obj);
	decref(key);
}

object_t syntax_define_syntax(scope_t scope, object_t code) // define-syntax
{
	return syntax_define(scope, code);
}

object_t syntax_identity(scope_t scope, object_t code) // $
{
	return eval_lazy(scope, code);
}

struct macro {
	struct object self;
	object_t literals;
	object_t rules;
	symbol_t name;
};

void reach_macro(void* obj)
{
	macro_t macro = obj;
	mark_reachable(macro->literals);
	mark_reachable(macro->rules);
}

struct type TYPE_MACRO = {
	.name = "macro",
	.reach = reach_macro
};

object_t wrap_macro(object_t literals, object_t rules)
{
	macro_t macro = malloc(sizeof(*macro));
	macro->literals = literals;
	macro->rules = rules;
	macro->name = NULL;
	return register_object(macro, &TYPE_MACRO);
}

object_t syntax_rules(scope_t scope, object_t code) // syntax-rules
{
	object_t literals = pop_list(&code);
	if (! literals)
		DIE("Malformed syntax-rules");
	return wrap_macro(literals, code);
}

macro_t to_macro(object_t obj)
{
	if (obj->type == &TYPE_MACRO)
		return (macro_t)obj;
	return NULL;
}

object_t append(object_t first, object_t second)
{
	object_t rev, scan, result, obj;

	if (is_nil(first)) {
		incref(second);
		return second;
	}

	rev = reverse(first);
	scan = rev;
	result = second;

	while ((obj = pop_list(&scan)))
		push_list(&result, obj);

	decref(rev);
	return result;
}

object_t substitute_template(object_t template, symbol_t key, object_t value)
{
	object_t result;

	pair_t pair = to_pair(template);
	if (pair) {
		object_t new_car = substitute_template(car(pair), key, value);
		object_t new_cdr = substitute_template(cdr(pair), key, value);

		if (! new_cdr) {
			incref(value);
			new_cdr = value;
		}

		if (! new_car) {
			result = append(value, new_cdr);
			decref(new_cdr);
			return result;
		}

		if ((new_car == car(pair)) && (new_cdr == cdr(pair))) {
			result = template;
			incref(template);
		} else {
			result = wrap_pair(new_car, new_cdr);
		}
		decref(new_car);
		decref(new_cdr);
		return result;
	}

	symbol_t sym = to_symbol(template);
	if (sym) {
		if (strcmp(unwrap_symbol(sym), unwrap_symbol(key)) == 0) {
			if (is_symbol("...", template))
				return NULL;
			result = value;
			incref(result);
		} else {
			result = template;
			incref(result);
		}
		return result;
	}

	incref(template);
	return template;
}

object_t transform_syntax(object_t rule, object_t literals, object_t body)
{
	// pair_t pattern_template = assert_pair("syntax rule", rule);
	object_t pattern = pop_list(&rule);
	if (! pattern)
		DIE("Malformed syntax rule");

	object_t template = pop_list(&rule);
	if (! template)
		DIE("Malformed syntax rule");
	incref(template);

	object_t tag = pop_list(&pattern);
	if (! tag)
		DIE("Malformed syntax rule");

	// DEBUG("template", template);

	object_t token;
	while ((token = pop_list(&pattern))) {
		symbol_t sym = assert_symbol("syntax rule pattern", token);
		if (is_symbol("...", token)) {
			object_t result = substitute_template(template, sym, body);
			decref(template);
			return result;
		}
		object_t item = pop_list(&body);
		if (! item)
			return NULL;

		object_t new_template = substitute_template(template, sym, item);
		decref(template);
		template = new_template;
	}

	if (is_nil(body))
		return template;

	decref(template);
	return NULL;
}

object_t expand_macro(scope_t scope, macro_t macro, object_t body)
{
	object_t rules, rule, new_body;

	rules = macro->rules;

	while ((rule = pop_list(&rules))) {
		new_body = transform_syntax(rule, macro->literals, body);
		if (new_body)
			return new_body;
	}

	if (macro->name)
		DIE("Unable to expand macro `%s`", unwrap_symbol(macro->name));
	else
		DIE("Unable to expand macro");
}

void assign_name(object_t obj, symbol_t name)
{
	lambda_t lambda = to_lambda(obj);
	if (lambda) {
		if (! lambda->l_name)
			lambda->l_name = name;
		return;
	}

	macro_t macro = to_macro(obj);
	if (macro) {
		if (! macro->name)
			macro->name = name;
		return;
	}
}

object_t optimize(scope_t scope, object_t code)
{
	if (scope->parent) {
		incref(code);
		return code;
	}

	object_t result = code;

	symbol_t sym = to_symbol(code);
	if (sym) {
		object_t obj = lookup_in_scope(scope, sym);
		if (obj && (to_syntax(obj) || to_macro(obj) || to_native(obj)))
			result = obj;
		incref(result);
		return result;
	}

	pair_t pair = to_pair(code);
	if (pair) {
		object_t head = car(pair);
		if (is_symbol("quote", head))
			goto done;
		object_t tail = cdr(pair);
		object_t new_head = optimize(scope, head);
		object_t new_tail = optimize(scope, tail);
		if ((head != new_head) || (tail != new_tail))
			result = wrap_pair(new_head, new_tail);
		decref(new_head);
		decref(new_tail);
	}

done:
	if (code == result)
		incref(result);
	return result;
}
