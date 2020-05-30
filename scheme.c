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
// my own garbage collector, and that's just dumb.
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
//situation when I'll have to write my own multithreaded embeddable Scheme
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
// **the** invoker ("*there was nothing between us, not even a condom...*"
// yeah, really bad joke), and `abort()`ing is a proper way to notify the
// operating system about the problem, so #2 is pretty much #3, just
// without boilerplate code to pull error status to `main()` and then abort
// there.
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
// the standards of normal folks whose idea of having fun implies
// things like mountain skiing and dance festivals, spending evenings
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
// parentheses),
// * strings (delimited by "double quotes" or however you call that character),
// * quotations (if you don't know who these are, you better look it up in
// Scheme spec, but basically it's a way to specify that ```'(+ 1 2)``` is
// *literally* a list with three elements and not an expression that adds
// two numbers),
// * and atoms, which are pretty much everything else including numbers,
// characters, and symbols.
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
// CUTOFF
// ## Chapter 3 where I evaluate

struct scope;
typedef struct scope* scope_t;

object_t eval_eager(scope_t scope, object_t expr);
scope_t default_scope(void);

object_t eval_repl(object_t expr)
{
	return eval_eager(default_scope(), expr);
}

//

object_t eval_force(object_t);
object_t eval_lazy(scope_t scope, object_t expr);

object_t eval_eager(scope_t scope, object_t expr)
{
	object_t result = eval_lazy(scope, expr);
	return eval_force(result);
}

//

struct thunk;
typedef struct thunk* thunk_t;

object_t eval_thunk(thunk_t thunk);
thunk_t to_thunk(object_t obj);

object_t eval_force(object_t result)
{
	thunk_t thunk;

	while((thunk = to_thunk(result))) {
		object_t new_result = eval_thunk(thunk);
		decref(result);
		result = new_result;
	}

	return result;
}

//

struct symbol;
typedef struct symbol* symbol_t;

symbol_t to_symbol(object_t);

object_t eval_sexpr(scope_t, pair_t);
object_t eval_var(scope_t scope, symbol_t key);

object_t eval_lazy(scope_t scope, object_t expr)
{
	pair_t sexpr = to_pair(expr);
	if (sexpr)
		return eval_sexpr(scope, sexpr);

	symbol_t varname = to_symbol(expr);
	if (varname)
		return eval_var(scope, varname);

	incref(expr);
	return expr;
}

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

object_t eval_syntax(scope_t scope, symbol_t keyword, object_t body);
object_t eval_funcall(scope_t scope, object_t func, object_t exprs);

object_t eval_sexpr(scope_t scope, pair_t code)
{
	object_t head = car(code), tail = cdr(code);
	symbol_t head_sym = to_symbol(head);

	if (head_sym) {
		object_t result = eval_syntax(scope, head_sym, tail);
		if (result)
			return result;
	}

	object_t func = eval_eager(scope, head);
	object_t result = eval_funcall(scope, func, tail);
	decref(func);

	return result;
}

//

typedef object_t (*syntax_t)(scope_t scope, object_t code);
syntax_t lookup_syntax_handler(symbol_t name);

object_t eval_syntax(scope_t scope, symbol_t keyword, object_t body)
{
	syntax_t handler = lookup_syntax_handler(keyword);
	return (handler == NULL) ? NULL : handler(scope, body);
}

//

struct array {
	void** data;
	int available;
	int size;
};

typedef struct array* array_t;

#include <strings.h>

struct array ARRAY_POOL[1024];
int ARRAY_POOL_CT = 0;

void array_init(array_t arr)
{
	if (ARRAY_POOL_CT > 0)
		*arr = ARRAY_POOL[--ARRAY_POOL_CT];
	else
		bzero(arr, sizeof(struct array));
}

void array_dispose(struct array* arr)
{
	if (ARRAY_POOL_CT == 1024) {
		free(arr->data);
	} else {
		arr->size = 0;
		ARRAY_POOL[ARRAY_POOL_CT++] = *arr;
	}

	bzero(arr, sizeof(struct array));
}

void dispose_array_pool()
{
	while (ARRAY_POOL_CT)
		free(ARRAY_POOL[--ARRAY_POOL_CT].data);
}

void array_push(array_t arr, void* entry)
{
	if (arr->size == arr->available) {
		arr->available *= 2;
		if (arr->available < 8)
			arr->available = 8;
		arr->data = realloc(arr->data, sizeof(void*)*arr->available);
	}
	arr->data[arr->size++] = entry;
}

void decref_many(int argct, object_t* args)
{
	for (int i=argct-1; i>=0; i--)
		decref(args[i]);
}

void array_decref(array_t arr)
{
	decref_many(arr->size, (object_t*)arr->data);
}

//

struct lambda;
typedef struct lambda* lambda_t;

lambda_t to_lambda(object_t);

void eval_args(array_t, scope_t, object_t);
object_t invoke(object_t func, int argct, object_t* args);
object_t make_thunk(lambda_t, array_t);

object_t eval_funcall(scope_t scope, object_t func, object_t exprs)
{
	struct array args;
	object_t result;
	lambda_t lambda;

	array_init(&args);
	eval_args(&args, scope, exprs);

	if ((lambda = to_lambda(func))) {
		result = make_thunk(lambda, &args);
	} else {
		result = invoke(func, args.size, (object_t*)args.data);
		array_dispose(&args);
	}
	return result;
}

//

void array_init(array_t);

void eval_args(array_t args, scope_t scope, object_t exprs)
{
	object_t expr;

	while ((expr = pop_list(&exprs)))
		array_push(args, eval_eager(scope, expr));
}

// CUTOFF

// ## Chapter six
//
// embed pro99.scm : problem #1
//

void assert_arg_count(const char* name, int actual, int expected)
{
	if (actual != expected)
		DIE("Expected %d arguments for %s, got %d", expected, name, actual);
}

object_t native_nullp(int argct, object_t* args) // null?
{
	assert_arg_count("null?", argct, 1);
	return wrap_bool(is_nil(args[0]));
}

//

object_t native_car(int argct, object_t* args)
{
	assert_arg_count("car", argct, 1);
	pair_t pair = assert_pair("as argument #1 of car", args[0]);
	object_t result = car(pair);
	incref(result);
	return result;
}

object_t native_cdr(int argct, object_t* args)
{
	assert_arg_count("cdr", argct, 1);
	pair_t pair = assert_pair("as argument #1 of cdr", args[0]);
	object_t result = cdr(pair);
	incref(result);
	return result;
}

//

object_t pop_list_or_die(object_t* ptr)
{
	object_t result = pop_list(ptr);
	if (! result)
		DIE("Premature end of list");
	return result;
}

//

symbol_t assert_symbol(const char* context, object_t obj)
{
	symbol_t sym = to_symbol(obj);
	if (sym)
		return sym;
	DIE("Expected %s to be a symbol, got %s instead", context, typename(obj));
}

//

void eval_define(scope_t eval_scope, scope_t bind_scope, symbol_t key, object_t expr);
object_t lambda(scope_t scope, object_t params, object_t body);

object_t syntax_define(scope_t scope, object_t code)
{
	object_t head = pop_list_or_die(&code);

	pair_t head_pair = to_pair(head);
	if (head_pair) {
		object_t name_obj = car(head_pair);
		symbol_t name = assert_symbol("variable name in define", name_obj);

		object_t args = cdr(head_pair);
		object_t func = lambda(scope, args, code);
		eval_define(scope, scope, name, func);
		decref(func);
		return wrap_nil();
	}

	symbol_t head_sym = to_symbol(head);
	if (head_sym) {
		object_t expr = pop_list_or_die(&code);
		eval_define(scope, scope, head_sym, expr);
		return wrap_nil();
	}

	DIE("define is not implemented for %s", typename(head));
}

//

bool eval_boolean(scope_t scope, object_t expr);

object_t syntax_if(scope_t scope, object_t code)
{
	object_t if_expr = pop_list_or_die(&code);
	object_t then_expr = pop_list_or_die(&code);

	if (eval_boolean(scope, if_expr))
		return eval_lazy(scope, then_expr);

	object_t else_expr = pop_list(&code);
	if (else_expr)
		return eval_lazy(scope, else_expr);

	return wrap_nil();
}

//

bool eval_boolean(scope_t scope, object_t expr)
{
	object_t obj = eval_eager(scope, expr);
	bool result = (obj != wrap_bool(false));
	decref(obj);
	return result;
}

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

static struct dict SYNTAX_HANDLERS;
void setup_syntax();
void teardown_syntax();

void register_syntax_handler(const char* name, syntax_t handler);

void* dict_lookup(struct dict*, const char*, int hash);

int strhash(const char*);

//

void dict_dispose(struct dict*);

void teardown_syntax()
{
	// for (int i=SYNTAX_HANDLERS.size-1; i>=0; i--)
	// 	if (SYNTAX_HANDLERS.data[i].key)
	// 		decref((object_t)SYNTAX_HANDLERS.data[i].key);
	dict_dispose(&SYNTAX_HANDLERS);
}

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

void register_syntax_handler(const char* name, syntax_t syntax)
{
	symbol_t sym = to_symbol(wrap_symbol(name));
	dict_put(&SYNTAX_HANDLERS, sym, syntax);
	decref((object_t)sym);
}

//

void dict_enlarge(struct dict* dict);

void dict_reinsert(struct dict*, struct dict_entry*);

//

void* dict_put_impl(struct dict*, const char*, void*, int);

void dict_enlarge(struct dict* d)
{
	if (d->size == 0) {
		d->data = calloc(sizeof(struct dict_entry), 8);
		d->size = 8;
		return;
	}
	struct dict tmp;
	tmp.used = 0;
	tmp.size = d->size * 2;
	tmp.data = calloc(sizeof(struct dict_entry), tmp.size);
	for (int i=0; i<d->size; i++) {
		struct dict_entry* entry = &d->data[i];
		if (entry->key)
			dict_reinsert(&tmp, entry);
	}
	free(d->data);
	d->size = tmp.size;
	d->data = tmp.data;
}

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

struct array ALL_OBJECTS;

bool is_garbage(object_t);
bool hasrefs(object_t);
void mark_garbage(object_t);

void reach_object(void*);

void object_dispose(object_t);

struct array REACHABLE_OBJECTS;

void reset_gc_state(object_t obj);

void* pop_array(array_t arr)
{
	if (arr->size == 0)
		return NULL;
   return arr->data[--arr->size];
}

void collect_garbage()
{
	array_init(&REACHABLE_OBJECTS);
	for (int i=ALL_OBJECTS.size-1; i>=0; i--) {
		object_t obj = ALL_OBJECTS.data[i];
		reset_gc_state(obj);
	}
	while (REACHABLE_OBJECTS.size) {
		object_t obj = pop_array(&REACHABLE_OBJECTS);
		reach_object(obj);
	}
	free(REACHABLE_OBJECTS.data);
	int count = ALL_OBJECTS.size;
	int index = 0;
	while (index < count) {
		object_t obj = ALL_OBJECTS.data[index];
		if (is_garbage(obj)) {
			object_dispose(obj);
			ALL_OBJECTS.data[index] = ALL_OBJECTS.data[--count];
		} else {
			index++;
		}
	}
	ALL_OBJECTS.size = count;
}

//

void mark_reachable(void*);

void dict_reach(struct dict* dict)
{
	for (int i=dict->size-1; i>=0; i--)
		if (dict->data[i].key)
			mark_reachable(dict->data[i].value);
}

//

struct type {
	const char* name;
	size_t size;
	void (*reach)(void*);
	void (*dispose)(void*);
	void (*write)(FILE*, void*);
};

typedef struct type* type_t;

//

enum gc_state {
	GARBAGE,
	REACHABLE,
	REACHED
};

struct object {
	type_t type;
	enum gc_state gc_state;
	int stackrefs;
};

const char* typename(object_t obj)
{
	return obj->type->name;
}

void incref(struct object* obj)
{
	obj->stackrefs++;
}

void decref(struct object* obj)
{
	assert(obj->stackrefs > 0);
	obj->stackrefs--;
}

bool hasrefs(object_t obj)
{
	return obj->stackrefs > 0;
}

void mark_garbage(object_t obj)
{
	obj->gc_state = GARBAGE;
}

bool is_garbage(object_t obj)
{
	return obj->gc_state == GARBAGE;
}

//

void reach_object(void* ptr)
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

array_t get_object_pool(type_t);

void reclaim(object_t obj)
{
	if (obj->type->size) {
		array_t mempool = get_object_pool(obj->type);
		array_push(mempool, obj);
	} else {
		free(obj);
	}
}

void object_dispose(object_t obj)
{
	if (obj->type->dispose)
		obj->type->dispose(obj);
	else
		reclaim(obj);
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

void gc_register(object_t);

object_t object_init(void* ptr, const type_t type)
{
	object_t obj = ptr;
	obj->type = type;
	obj->stackrefs = 1;
	gc_register(obj);
	return obj;
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

//

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

void pair_reach(void* obj)
{
	pair_t pair = obj;
	mark_reachable(pair->car);
	mark_reachable(pair->cdr);
}

struct type TYPE_PAIR = {
	.name = "pair",
	.size = sizeof(struct pair),
	.write = write_pair,
	.reach = pair_reach,
};

struct array POOL_24;
struct array POOL_32;
struct array POOL_40;

array_t get_object_pool(type_t type)
{
	switch(type->size) {
	case 24:
		return &POOL_24;
	case 32:
		return &POOL_32;
	case 40:
		return &POOL_40;
	default:
		DIE("No memory pool for type %s (%lu bytes)", type->name, type->size);
	}
}

void dispose_object_pools()
{
	object_t obj;

	while ((obj = pop_array(&POOL_24)))
		free(obj);
	array_dispose(&POOL_24);
	while ((obj = pop_array(&POOL_32)))
		free(obj);
	array_dispose(&POOL_32);
	while ((obj = pop_array(&POOL_40)))
		free(obj);
	array_dispose(&POOL_40);
}

void* allocate_object(type_t type)
{
	array_t pool = get_object_pool(type);
	object_t obj = pop_array(pool);
	if (! obj)
		obj = malloc(type->size);
	obj->type = type;
	return obj;
}

object_t register_object(void* ptr)
{
	object_t obj = ptr;
	return object_init(ptr, obj->type);
}

object_t wrap_pair(object_t head, object_t tail)
{
	pair_t result = allocate_object(&TYPE_PAIR);
	result->car = head;
	result->cdr = tail;
	return register_object(result);
}

pair_t to_pair(object_t obj)
{
	if (obj->type == &TYPE_PAIR)
		return (pair_t)obj;
	return NULL;
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
	.size = sizeof(struct integer),
	.write = write_int
};

struct object* wrap_int(int v)
{
	integer_t num = allocate_object(&TYPE_INT);
	num->value = v;
	return object_init(num, &TYPE_INT);
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
	free(obj);
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

struct object* wrap_string(const char* v)
{
	string_t str = malloc(sizeof(struct string));
	str->value = strdup(v);
	return object_init(str, &TYPE_STRING);
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
	free(obj);
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
		symbol_t symbol = malloc(sizeof(struct symbol));
		symbol->value = strdup(v);
		symbol->hash = hash;
		result = object_init(symbol, &TYPE_SYMBOL);
		dict_put(&ALL_SYMBOLS, symbol, result);
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
	object_t l_body;
	scope_t l_scope;
	symbol_t l_name;
	struct array l_params;
};

typedef struct lambda* lambda_t;

void lambda_reach(void* obj)
{
	lambda_t lambda = obj;
	mark_reachable(lambda->l_body);
	mark_reachable(lambda->l_scope);
	mark_reachable(lambda->l_name);
	for (int i=lambda->l_params.size-1; i>=0; i--)
		mark_reachable(lambda->l_params.data[i]);
}

void lambda_dispose(void* obj)
{
	lambda_t lambda = obj;
	array_dispose(&lambda->l_params);
	free(obj);
}

struct type TYPE_LAMBDA = {
	.name = "lambda",
	.reach = lambda_reach,
	.dispose = lambda_dispose,
};

lambda_t to_lambda(object_t obj)
{
	if (obj->type == &TYPE_LAMBDA)
		return (lambda_t)obj;
	return NULL;
}

object_t lambda(scope_t scope, object_t params, object_t body)
{
	object_t param;

	lambda_t lambda = malloc(sizeof(struct lambda));
	lambda->l_name = NULL;
	lambda->l_body = body;
	lambda->l_scope = scope;
	array_init(&lambda->l_params);

	while ((param = pop_list(&params))) {
		assert(to_symbol(param));
		array_push(&lambda->l_params, param);
	}

	return object_init(lambda, &TYPE_LAMBDA);
}

//

struct thunk {
	struct object self;
	lambda_t lambda;
	struct array args;
};

void thunk_dispose(void* obj)
{
	thunk_t thunk = obj;
	array_dispose(&thunk->args);
	reclaim(obj);
}

void thunk_reach(void* obj)
{
	thunk_t thunk = obj;
	mark_reachable(thunk->lambda);
	for (int i=thunk->args.size-1; i>=0; i--)
		mark_reachable(thunk->args.data[i]);
}

struct type THUNK = {
	.name = "thunk",
	.size = sizeof(struct thunk),
	.reach = thunk_reach,
	.dispose = thunk_dispose,
};

object_t make_thunk(lambda_t lambda, array_t args)
{
	thunk_t thunk = allocate_object(&THUNK);
	thunk->lambda = lambda;
	thunk->args = *args;
	object_t result = object_init(thunk, &THUNK);
	array_decref(args);
	return result;
}

thunk_t to_thunk(object_t obj)
{
	if (obj->type == &THUNK)
		return (thunk_t)obj;
	return NULL;
}

//

struct native;
typedef struct native* native_t;

native_t to_native(object_t);

object_t invoke_lambda(lambda_t, int, object_t*);
object_t invoke_native(native_t, int, object_t*);

object_t invoke(object_t func, int argct, object_t* args)
{
	native_t native;
	lambda_t lambda;
	object_t result;

	if ((native = to_native(func))) {
		result = invoke_native(native, argct, args);
	} else if ((lambda = to_lambda(func))) {
		result = invoke_lambda(lambda, argct, args);
	} else {
		DIE("Can't invoke object of type %s", typename(func));
	}
	decref_many(argct, args);
	return result;
}

//

object_t eval_thunk(thunk_t thunk)
{
	return invoke_lambda(thunk->lambda, thunk->args.size, (object_t*)thunk->args.data);
}

//

void gc_register(struct object* obj)
{
	static int threshold = 256;
	array_push(&ALL_OBJECTS, obj);
	if (ALL_OBJECTS.size > threshold) {
		collect_garbage();
		threshold = ALL_OBJECTS.size * 2;
	}
}

//

struct scope {
	struct object self;
	struct dict s_binds;
	struct scope* s_parent;
};

typedef struct scope* scope_t;

object_t lookup_in_scope(scope_t scope, symbol_t key)
{
	while (scope) {
		void* value = dict_lookup_fast(&scope->s_binds, key->value, key->hash);
		if (value)
			return value;
		scope = scope->s_parent;
	}
	return NULL;
}

void scope_reach(void* obj)
{
	scope_t scope = obj;
	struct dict* binds = &scope->s_binds;
	dict_reach(binds);
	mark_reachable(scope->s_parent);
}

void scope_dispose(void* obj)
{
	scope_t scope = obj;
	dict_dispose(&scope->s_binds);
	if (scope->s_parent)
		reclaim(obj);
}

struct type SCOPE = {
	.name = "scope",
	.size = sizeof(struct scope),
	.reach = scope_reach,
	.dispose = scope_dispose,
};

struct scope DEFAULT_SCOPE;

scope_t default_scope()
{
	return &DEFAULT_SCOPE;
}

//

void scope_bind(scope_t scope, symbol_t key, object_t value)
{
	const char* strkey = unwrap_symbol(key);
	void* ptr = dict_put(&scope->s_binds, key, value);
	if (ptr != NULL)
		DIE("Rebind of %s", strkey);
	if (! scope->s_parent)
		incref(value);
}

//

void eval_define(scope_t eval_scope, scope_t bind_scope, symbol_t key, object_t expr)
{
	object_t result = eval_eager(eval_scope, expr);

	lambda_t lambda = to_lambda(result);
	if (lambda)
		lambda->l_name = key;

	scope_bind(bind_scope, key, result);
	decref(result);
}

//

struct native {
	struct object self;
	object_t (*invoke)(int, object_t*);
};

struct type TYPE_NATIVE = {
	.name = "native",
};

native_t to_native(object_t obj)
{
	if (obj->type == &TYPE_NATIVE)
		return (native_t)obj;
	return NULL;
}

//

object_t invoke_native(native_t native, int argct, object_t* args)
{
	return native->invoke(argct, args);
}

//

void scope_init(scope_t scope, scope_t parent)
{
	dict_init(&scope->s_binds);
	scope->s_parent = parent;
	object_init(&scope->self, &SCOPE);
}

//

object_t derive_scope(scope_t parent)
{
	scope_t scope = allocate_object(&SCOPE);
	scope_init(scope, parent);
	return (object_t)scope;
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
		result = eval_force(result);
		decref(result);
		result = eval_lazy(scope, expr);
	}

	return result;
}

//

object_t invoke_lambda(struct lambda* lambda, int argct, object_t* args)
{
	struct array* params = &lambda->l_params;
	const char* name = lambda->l_name ? unwrap_symbol(lambda->l_name) : NULL;

	assert_arg_count(name, argct, params->size);

	object_t scope_obj = derive_scope(lambda->l_scope);
	scope_t scope = to_scope(scope_obj);

	for (int i=0; i<params->size; i++)
		scope_bind(scope, params->data[i], args[i]);
	object_t result = eval_block(scope, lambda->l_body);

	decref(scope_obj);
	return result;
}

//

void register_stdlib_functions(void);

void setup_runtime()
{
	dict_init(&ALL_SYMBOLS);
	array_init(&ALL_OBJECTS);
	dict_init(&SYNTAX_HANDLERS);
	setup_syntax();
	scope_init(&DEFAULT_SCOPE, NULL);
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
	dict_decref(&DEFAULT_SCOPE.s_binds);
	scope_dispose(&DEFAULT_SCOPE);
	dict_decref(&ALL_SYMBOLS);
	dict_dispose(&ALL_SYMBOLS);
	teardown_syntax();
	collect_garbage();
	array_dispose(&ALL_OBJECTS);
	dispose_object_pools();
	dispose_array_pool();
}

//

object_t syntax_quote(scope_t scope, object_t code)
{
	object_t result = pop_list_or_die(&code);
	incref(result);
	return result;
}

//

void register_native(const char* name, object_t (*func)(int, object_t*))
{
	native_t native = malloc(sizeof(struct native));
	native->invoke = func;

	object_t obj = object_init(native, &TYPE_NATIVE);
	object_t key = wrap_symbol(name);
	scope_bind(&DEFAULT_SCOPE, (symbol_t)key, obj);
	decref(obj);
	decref(key);
}

//

object_t native_write(int argct, object_t* args)
{
	assert_arg_count("write", argct, 1);
	write_object(stdout, args[0]);
	return wrap_nil();
}

//

object_t native_newline(int argct, object_t* args)
{
	assert_arg_count("newline", argct, 0);
	fputs("\n", stdout);
	return wrap_nil();
}

bool unbox_int(int*, object_t);

bool eq(struct object* x, struct object* y);

object_t syntax_and(scope_t scope, object_t code)
{
	object_t expr;

	while ((expr = pop_list(&code)))
		if (! eval_boolean(scope, expr))
			return wrap_bool(false);

	return wrap_bool(true);
}

object_t native_cons(int argct, object_t* args)
{
	assert_arg_count("cons", argct, 2);
	return wrap_pair(args[0], args[1]);
}

object_t native_eqp(int argct, object_t* args) // eq?
{
	assert_arg_count("eq?", argct, 2);
	return wrap_bool(eq(args[0], args[1]));
}

object_t native_list(int argct, object_t* args)
{
	object_t result = wrap_nil();

	for (int i=argct-1; i>=0; i--)
		push_list(&result, args[i]);

	return result;
}

object_t native_fold(int argct, object_t* args)
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

object_t native_modulo(int argct, object_t* args)
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

object_t native_reverse(int argct, object_t* args)
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

object_t syntax_cond(scope_t scope, object_t code)
{
	object_t clause, test;

	while ((clause = pop_list(&code))) {
		test = pop_list_or_die(&clause);
		if (is_symbol("else", test) || eval_boolean(scope, test))
			return eval_block(scope, clause);
	}
	return wrap_nil();
}

object_t syntax_letrec(scope_t outer_scope, object_t code)
{
	object_t scope_obj = derive_scope(outer_scope);
	scope_t scope = to_scope(scope_obj);

	object_t bindings = pop_list_or_die(&code);
	object_t binding;

	while ((binding = pop_list(&bindings))) {
		object_t keyobj = pop_list_or_die(&binding);
		object_t expr = pop_list_or_die(&binding);
		symbol_t key = assert_symbol("letrec binding name", keyobj);
		eval_define(scope, scope, key, expr);
	}

	object_t result = eval_block(scope, code);

	decref(scope_obj);
	return result;
}

object_t syntax_let(scope_t outer_scope, object_t code)
{
	object_t scope_obj = derive_scope(outer_scope);
	scope_t scope = to_scope(scope_obj);

	object_t bindings = pop_list_or_die(&code);
	object_t binding;

	while ((binding = pop_list(&bindings))) {
		object_t keyobj = pop_list_or_die(&binding);
		symbol_t key = assert_symbol("let binding name", keyobj);
		object_t expr = pop_list_or_die(&binding);
		eval_define(outer_scope, scope, key, expr);
	}

	object_t result = eval_block(scope, code);

	decref(scope_obj);
	return result;
}

object_t syntax_lambda(scope_t scope, object_t code)
{
	pair_t pair = to_pair(code);
	assert(pair);
	return lambda(scope, car(pair), cdr(pair));
}

bool unbox_int(int* ptr, object_t obj)
{
	if (obj->type != &TYPE_INT)
		return false;
	integer_t num = (integer_t)obj;
	*ptr = num->value;
	return true;
}

void reset_gc_state(object_t obj)
{
	if (obj->stackrefs == 0) {
		obj->gc_state = GARBAGE;
	} else {
		obj->gc_state = REACHABLE;
		array_push(&REACHABLE_OBJECTS, obj);
	}
}

void mark_reachable(void* ptr)
{
	if (! ptr)
		return;

	object_t obj = ptr;
	if (obj->gc_state == GARBAGE) {
		obj->gc_state = REACHABLE;
		array_push(&REACHABLE_OBJECTS, obj);
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

		eval_define(scope, new_scope, keysym, expr);

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

struct file {
	struct object self;
	FILE* value;
};

typedef struct file* file_t;

void dispose_file(void* obj)
{
	file_t file = obj;
	fclose(file->value);
	free(obj);
}

struct type TYPE_FILE = {
	.name = "file",
	.dispose = dispose_file,
};

struct object* wrap_file(FILE* v)
{
	file_t file = malloc(sizeof(struct file));
	file->value = v;
	return object_init(file, &TYPE_FILE);
}

object_t native_open_input_file(int argct, object_t* args) // open-input-file
{
	assert_arg_count("open-input-file", argct, 1);
	string_t name = to_string_or_die("open-input-file", args[0]);
	FILE* f = fopen_or_die(unwrap_string(name), "r");
	return wrap_file(f);
}

file_t to_file(object_t obj)
{
	if (obj->type == &TYPE_FILE)
		return (file_t)obj;
	return NULL;
}

file_t to_file_or_die(const char* name, object_t obj)
{
	file_t file = to_file(obj);
	if (! file)
		DIE("Expected argument of %s to be a port, got %s instead", name, typename(obj));
	return file;
}

FILE* unwrap_file(file_t file)
{
	return file->value;
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
	.size = sizeof(struct character),
	.write = write_char
};

struct object* wrap_char(char v)
{
	char_t ch = allocate_object(&TYPE_CHAR);
	ch->value = v;
	return object_init(ch, &TYPE_CHAR);
}

object_t native_read_char(int argct, object_t* args) // read-char
{
	assert_arg_count("read-char", argct, 1);
	file_t file = to_file_or_die("read-char", args[0]);
	FILE* f = unwrap_file(file);
	int ch = fgetc_or_die(f);
	if (ch == EOF)
		return wrap_nil();
	else
		return wrap_char(ch);
}

object_t syntax_or(scope_t scope, object_t code)
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
			if (! s->s_parent) {
				incref(value);
				decref(old_value);
			}
			return;
		}
		s = s->s_parent;
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

object_t native_not(int argct, object_t* args)
{
	assert_arg_count("not", argct, 1);
	return wrap_bool(args[0] == wrap_bool(false));
}

object_t native_display(int argct, object_t* args)
{
	assert_arg_count("display", argct, 1);
	string_t str = to_string(args[0]);
	if (str)
		printf("%s", str->value);
	else
		write_object(stdout, args[0]);
	return wrap_nil();
}

syntax_t lookup_syntax_handler(symbol_t name)
{
	const char* key = unwrap_symbol(name);
	return dict_lookup_fast(&SYNTAX_HANDLERS, key, name->hash);
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
	if (entry->key->hash == hash) {
		return strcmp(entry->key->value, key);
	} else {
		return entry->key->hash - hash;
	}
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
			DIE("Should never happen");
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

object_t native_substring(int argct, object_t* args)
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
void setup_syntax(void)
{
	register_syntax_handler("and", syntax_and);
	register_syntax_handler("cond", syntax_cond);
	register_syntax_handler("define", syntax_define);
	register_syntax_handler("if", syntax_if);
	register_syntax_handler("lambda", syntax_lambda);
	register_syntax_handler("let", syntax_let);
	register_syntax_handler("let*", syntax_letseq);
	register_syntax_handler("letrec", syntax_letrec);
	register_syntax_handler("or", syntax_or);
	register_syntax_handler("quote", syntax_quote);
	register_syntax_handler("set!", syntax_set);
}

void register_stdlib_functions(void)
{
	register_native("*", native_num_mult);
	register_native("+", native_num_plus);
	register_native("-", native_num_minus);
	register_native("/", native_num_divide);
	register_native("<", native_num_less);
	register_native("=", native_num_equals);
	register_native("car", native_car);
	register_native("cdr", native_cdr);
	register_native("cons", native_cons);
	register_native("display", native_display);
	register_native("eq?", native_eqp);
	register_native("fold", native_fold);
	register_native("list", native_list);
	register_native("list->string", native_list_to_string);
	register_native("modulo", native_modulo);
	register_native("newline", native_newline);
	register_native("not", native_not);
	register_native("null?", native_nullp);
	register_native("open-input-file", native_open_input_file);
	register_native("pair?", native_pairp);
	register_native("read-char", native_read_char);
	register_native("reverse", native_reverse);
	register_native("set-cdr!", native_setcdr);
	register_native("string->list", native_string_to_list);
	register_native("string-append", native_string_append);
	register_native("string-copy", native_string_copy);
	register_native("string-length", native_string_length);
	register_native("string-ref", native_string_ref);
	register_native("string-set!", native_string_set);
	register_native("string=?", native_string_equal);
	register_native("substring", native_substring);
	register_native("symbol?", native_symbolp);
	register_native("write", native_write);
}
