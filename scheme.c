// ## Chapter 1, where the story begins
//
// Every story has to start somewhere. This one is a tour through a
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
// Here I can imagine people asking, "Wait a minute, is this some kind of
// pseudo-C? Ain't you supposed to have `#include <stdio.h>` at the
// beginning and `main()` function at the very end?"
//
// Well. There is a whole universe of possible implications behind "Ain't
// you supposed to do X?" questions, and I'll use them as rant fuel
// throughout this story. But considering the most straightforward one,
// which is "is C compiler okay with such way of structuring code?" the
// answer is "yes, it compiles just fine". It will keep failing to *link*
// until those functions will be implemented, but, as far as the compiler
// is concerned, forward declarations are good enough.
//
// Moreover, this is the general pattern in this story where I follow the
// example of Quintus Fabius Maximus and keep postponing writing
// lower-level implementation of a feature until I get a handle on how it's
// going to be used.
//
// This approach is called top-down, and it's not the only way to write a
// program. A program can also be written bottom-up by starting with
// individual nuts and bolts and then assembling them into a big piece of
// software.
//
// There's a potential problem with the latter, though.
//
// Let's say I start with writing a piece of the standard library. I'm
// certainly going to need it at some point, so it isn't an obviously wrong
// place to start. Oh, and I'll also need a garbage collector, so I can
// write one too.
//
// But then I'm running a risk of ending up with an implementation of a
// chunk of Scheme standard library that is neat, and cute, and pretty, and
// a garbage collector that is as terrific as a garbage collector in a pet
// project may possibly be, and **they don't quite fit!**
//
// And then I'll have a dilemma. Either I'll have to redo one or both of
// those pieces. Which probably won't be 100% waste, as I hope to have
// learned a few things on the way, but it's still double work. Or else I
// can refuse to accept sunk costs and then stubbornly work around the
// incompatibilities between my own standard library implementation and my
// own garbage collector. And that's just dumb.
//
// But as long as something *isn't done at all*, I can be totally sure that
// it *isn't done wrong*. There's a certain Zen feeling to it.
//
// Another thing is more subtle but will get many programmers, especially
// more junior ones, nervous once they figure it out. It's
// `setup_runtime()` call. It's pretty clear **what** it will do, which is
// initialize garbage collector and such, but it also implies I'm going to
// have **the** runtime, probably scattered around in a bunch of global
// variables.
//
// I can almost hear voices asking, "But what if you need to have multiple
// runtimes? What if a customer comes and asks to make your interpreter
// embeddable as a scripting engine? What about multithreading? Why are you
// not worried?!"
//
// The answer is, "I consciously don't care." This is just a pet project
// that started with me willing to tinker with Scheme. And then realizing
// that just writing in Scheme is too easy, so I wrote my own interpreter.
// And then figuring out that **even that is not fun enough**, so I wrapped
// it into a sort of "literary programming" exercise. In a (highly
// improbable) situation when I'll have to write my own multithreaded
// embeddable Scheme interpreter, I'll just start from scratch, and that's
// about it.
//
// Anyway, I'll write functions to set up and tear down runtime once said
// runtime will take a more concrete shape. And for now, I'll focus on
// doing the useful stuff.
//

#include <stdio.h>
#include <unistd.h>

void execute(FILE*);
void execute_file(const char* filename);
void repl(void);

void do_useful_stuff(int argc, const char** argv)
{
	if (argc >= 2) {
		for (int i = 1; i < argc; i++)
			execute_file(argv[i]);
	} else if (isatty(fileno(stdin))) {
		repl();
	} else {
		execute(stdin);
	}
}

//
// This one is pretty straightforward: when a program is launched as
// `./scheme foo.scm`, then execute a file; when it's started as `cat
// foo.scm | ./scheme` do precisely the same, and otherwise fire up a REPL.
//
// Now that I know that I'm going to have a function that reads code from a
// stream and executes it, writing a function that does the same with a
// file is trivial, so let's just make one.
//

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define DIE(fmt, ...)                                                          \
	do {                                                                   \
		fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__);               \
		fprintf(stderr, fmt, ##__VA_ARGS__);                           \
		fprintf(stderr, "\n");                                         \
		abort();                                                       \
	} while (0)

FILE* fopen_or_die(const char* pathname, const char* mode)
{
	FILE* f = fopen(pathname, mode);
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
// it's okay, there's nothing to be ashamed of, it's just natural cognitive
// inertia.
//
// See, in general, when something goes wrong, you have three options:
//
// 1. You can handle the problem and continue.
//
// 2. You can abort.
//
// 3. You can notify the invoker about the problem and let them make their
// own choice from these same three options.
//
// In this case, option #1 is unavailable. Because, well, failing to open a
// file that the interpreter is told to execute is clearly a fatal error,
// and there's no sane way to recover from it.
//
// Oh, of course, there are insane ways to do it. For instance, I can just
// quietly skip the problematic file and later collapse in an obscure way
// because I can't find functions that were supposed to be defined in that
// file, or take your guess, but I'm not even going to spend time
// explaining why I'm not doing that.
//
// Option #3 is an interesting one to reflect on as this is what many
// programmers would consider a natural and only alternative when #1 is not
// available. In fact, if you're coding on top of a rich fat framework
// (think Spring or Django), this indeed is the natural and only way to do
// it. But there's no framework here, and the operating system is
// effectively **the** invoker ("*there was nothing between us... not even
// a condom...*" yeah, horrible joke), and `abort()`ing is a proper way to
// notify the operating system about the problem. So #2 is pretty much #3,
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
// The first one is `struct object`, which is going to be the
// representation of a Scheme object. It's clearly going to be some sort of
// a `struct` (I mean, we're doing C here, what else can it be); internal
// details of that struct I'll figure out later.
//
// The second and the third are `read_object()` and `eval_repl()` functions
// that, respectively, read an object from an input stream and evaluate it
// in REPL context.
//
// The last one is the `decref()` function that is needed because I'm going
// to have automatic memory management. For this, I have three options:
//
// 1. I can do reference counting. Very simple to do for as long as objects
// don't form reference cycles, then it gets quirky.
//
// 2. I can make a tracing garbage collector that traverses the process'
// memory to figure out which objects are still needed.
//
// 3. I can apply a sort of a hybrid approach where I do tracing for the
// sections of process' memory that are convenient to traverse and fall
// back to reference counting for those which aren't.
//
// From this simple function, it's already clear that whichever method I
// choose must be able to deal with references from the C call stack.
// Analyzing them in a pure tracing manner is pretty cumbersome, so I have
// to count them anyway, and that's what `decref()` will do.
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
// ## Chapter 2, where I parse Lisp code
//
// What's neat about implementing a Lisp dialect is that you can be done
// with parsing in about three pints of Guinness and then move on to
// funnier stuff.
//
// Of course, "funnier" is relative here, and not just grammatically, but
// also in a Theory of Relativity kind of sense. I mean, the Theory of
// Relativity describes extreme conditions where gravity is so high that
// common Newtonian laws don't work any more.
//
// Likewise, here we're venturing deep into the dark swampy forests of
// Nerdyland, where the common understanding of "fun" doesn't apply. By the
// standards of ordinary folks whose idea of having fun involves such
// activities as mountain skiing and dance festivals, spending evenings
// tinkering with the implementation of infinite recursion is hopelessly
// weird either way. So I mean absolutely no judgement towards those
// fantastic guys and gals who enjoy messing with lexers, parsers, and all
// that shebang. Whatever floats your boat, really!
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
	switch (ch) {
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
// Lisp syntax is famously Spartan. Basically, all you get is:
//
// * lists (those thingies with (the (astonishingly) copious) amount of
// parentheses),
//
// * strings (delimited by "double quotes" or however you call that
// character),
//
// * quotations (if you don't know who these are, you better look it up in
// Scheme spec, but basically it's a way to specify that ```'(+ 1 2)``` is
// *literally* a list with three elements and not an expression that adds
// two numbers),
//
// * and atoms, which are pretty much everything else, including numbers,
// characters, and symbols.
//
// So what I'm doing here is I'm looking at the first non-trivial character
// in the input stream, and if it's an opening parenthesis, I interpret it
// as a beginning of a list etc.
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
// the input stream ignoring comments and whitespace until I encounter a
// character that's neither or reach an EOF.
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
// Nothing particularly surprising here. Just read characters into the
// buffer until you reach the closing double quote, then wrap the contents
// of the buffer into an `object_t` and call it a day.
//
// Yes, this simplistic implementation will miserably fail to parse a
// source file with a string constant that is longer than 10K characters.
//
// And if you take some time to think about it, hard-coded 10K bytes for
// buffer size is kind of interesting here. It's an arbitrary number that,
// on the one hand, is safely above any practical limit in terms of
// usefulness. I mean, of course, you can hard-code the entirety of "Crime
// and Punishment" as a single string constant just to humiliate a dimwit
// interpreter author. But within any remotely sane coding style, such blob
// must be offloaded to an external text file, and even a buffer that is an
// order of magnitude smaller should still be good enough for all
// reasonable intents and purposes.
//
// On the other hand, it's also safely below any practical limit in terms
// of conserving memory. It can easily be an order of magnitude larger
// without causing any issues whatsoever.
//
// At least on a modern general-purpose machine with a couple of gigs of
// memory. If you've got a PDP-7 like one that Ken Thompson used for his
// early development of Unix, then a hundred kilobytes might be your
// **entire** RAM, and then you have to be more thoughtful with your
// throwaway buffers.
//
// By the way, it's awe-inspiring how those people in the 1960s could
// develop an entire real operating system on a computer like that. Well,
// not precisely mind-boggling, like, I myself started coding on a
// Soviet-made ZX Spectrum clone with 48 kilobytes of RAM, and you can't
// write a super-duper-sophisticated OS on such machine because **it just
// won't fit**, but still, it's so cool.
//
// Okay, back to business. Let's `parse_atom()`.
//

bool isspecial(char ch)
{
	switch (ch) {
	case '(':
	case ')':
	case ';':
	case '\"':
	case '\'':
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
// for as long as it looks like an atom, then convert it to an `object_t`,
// and that's pretty much it.
//
// Well, the syntax for characters in Scheme is a bit wonky: you have
// `#!\x` for the letter 'x' and `#\!` for an exclamation mark, and,
// surprisingly, `#!\newline` and `#!\space` for a newline and space
// respectively. Oh, and `#\` is also a space. Go figure.
//
// Most of that wonkiness can be dealt with by simply reading everything up
// until a special character and then figuring out what I've got in
// `parse_atom()`. Unless **it is** a special character, e.g. `#\)` or
// `#\;`, those need a bit of special handling.
//
// And now I'm looking at another buffer, and do you know what actually
// boggles my mind?
//
// Remember, at the very beginning of this story, I mentioned that a C
// program is typically supposed to have its `main()` function at the end?
// So, what boggles my mind is why are we still doing it?
//
// Well, I don't mean we all do. In some programming languages, it is more
// common, and in some, it is less, but really, why would you do it in any
// language? It's such a weird way to layout your code where you have to
// scroll all the way down to the bottom of the source file and then work
// your way up in a Benjamin Button kind of way.
//
// I mean, I know it's the legacy of Pascal where you were required to have
// the equivalent of `main()` at the bottom (and finish it with an `end.`
// with a period instead of a semicolon). I also understand that, back in
// those days, it made sense in order to simplify the compiler that had to
// run on limited hardware. But why we still sometimes do it in the 2020s
// is a mystery to me.
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

	for (; text[index]; index++) {
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
// Pretty straightforward, really. If it looks like a boolean, convert it
// to a boolean. If it appears to be an integer number, convert it to an
// integer. If it seems to be a symbol, convert it to a symbol. If it looks
// like a floating-point number... Convert it to a symbol because screw
// you!
//
// To give a bit of a background here, this pet project of mine was never
// intended to be a feature-complete standards-compliant Scheme
// implementation. It started with solving some of "99 Lisp problems" and
// then escalated into writing a sufficient Scheme interpreter to run
// those. None of those problems relied on floating-point arithmetics, and
// so I didn't implement it.
//
// Not that it's particularly hard, just tedious (if done Python-style with
// proper type coercions), and JavaScript solution with simply using floats
// for everything I find aesthetically unappealing.
//
// What I couldn't possibly skip is lists (they didn't call it LISt
// Processing for nothing), so let's `read_list()`
//
// Oh, and a quick remark on the naming convention. Functions like
// `wrap_symbol()` are named this way intentionally. They could easily be
// called, say, `make_symbol()`, but that would imply that it's some sort
// of a constructor that really **makes** a *new* object. But by the time I
// get to actually implement those, I might not want to be bound by this
// implication (because I might find out that a proper constructor doesn't
// conform to the language standard and/or isn't practical, and I need
// and/or want some sort of a cache or a pool or whatever).
//
// So, instead, it's a vague "wrap", which stands for "get me an object
// that represents this value, and how you make it under the hood is none
// of my business."
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

void push_to_list(object_t* ptr, object_t item);
object_t reverse_read_list(object_t list);
object_t wrap_nil(void);

object_t read_list(FILE* in)
{
	object_t accum = wrap_nil(), obj;

	while ((obj = read_next_object(in))) {
		push_to_list(&accum, obj);
		decref(obj);
	}

	object_t result = reverse_read_list(accum);
	decref(accum);
	return result;
}

//
// This one is simple but might need a bit of refresher on how lists work
// in Lisp (and other FP(-esque) languages).
//
// So, your basic building block is a two-element tuple (also known as a
// pair). If you make a tuple with some value in the first cell and in the
// second cell a reference to another tuple with another value in the first
// cell et cetera et cetera... And then you put a special null value to the
// second cell of the last tuple, then you what get is a singly-linked
// list. Oh, and the representation of the empty list is simply the null
// value.
//
// So what I do here is I read objects from the input stream and push them
// one by one to the front of the list until I see a closing parenthesis.
// But then the list ends up reversed, so I need to reverse it back. Easy.
//

object_t wrap_pair(object_t, object_t);

void push_to_list(object_t* ptr, object_t head)
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

pair_t assert_pair(object_t obj, const char* context);
object_t car(pair_t);

bool is_symbol(const char* text, object_t obj);

object_t pop_from_list(object_t*);

void incref(object_t);

object_t reverse_read_list(object_t list)
{
	object_t result = wrap_nil(), obj;

	while ((obj = pop_from_list(&list))) {
		if (! is_symbol(".", obj)) {
			push_to_list(&result, obj);
			continue;
		}

		pair_t pair = assert_pair(result, "when parsing a list");
		obj = car(pair);
		incref(obj);
		decref(result);
		result = obj;
	}

	return result;
}

//
// that simply pops things from one list and pushes them to another.
//
// Well, except for one gotcha: `.` has special meaning in list notation,
// so that `'(a  . b)` is not a list but is equivalent to `(cons 'a 'b)`,
// and so I cater for it here.
//

object_t cdr(pair_t);
bool is_nil(object_t);

object_t pop_from_list(object_t* ptr)
{
	object_t obj = *ptr;
	if (is_nil(obj))
		return NULL;
	pair_t pair = assert_pair(obj, "when traversing a list");
	*ptr = cdr(pair);
	return car(pair);
}

//
// `pop_from_list()` is pretty much the opposite of `push_to_list()` with a
// bit of type checking to make sure I'm dealing with a list and not
// something dodgy.
//

#define DEBUG(key, obj)                                                        \
	do {                                                                   \
		fprintf(stderr, "[%s:%d] %s = ", __FILE__, __LINE__, key);     \
		write_object(stderr, obj);                                     \
		fprintf(stderr, "\n");                                         \
	} while (0)

const char* typename(object_t);
pair_t to_pair(object_t);

pair_t assert_pair(object_t obj, const char* context)
{
	pair_t pair = to_pair(obj);
	if (pair)
		return pair;

	DEBUG("obj", obj);
	DIE("Expected a pair %s, got %s instead", context, typename(obj));
}

//
// I still haven't decided what exactly I will put into either `struct
// object` or `struct pair`, but I already need to be able to convert one
// to another. So, I promise to write a `to_pair()` function that would do
// just that (or return `NULL` if the value that this object holds is not a
// pair), and here's write a neat little helper around it to abort with a
// human-readable message when the conversion fails.
//

object_t read_quote(FILE* in)
{
	object_t obj = read_object(in);
	if (! obj)
		DIE("Premature end of input");

	object_t result = wrap_nil();
	push_to_list(&result, obj);
	decref(obj);

	object_t keyword = wrap_symbol("quote");
	push_to_list(&result, keyword);
	decref(keyword);

	return result;
}

//
// Since `'bla` is merely a shorter version of `(quote bla)` parsing it is
// trivial, and with that in place, we're finally done with parsing and can
// move on to
// ## Chapter 3, where I evaluate
//
// By the way, I don't know if you noticed or not, but I try to use the
// word "we" as sparingly as I can. Perhaps it has something to do with me
// coming from a culture where "We" is commonly associated with the
// dystopian novel by Yevgeny Zamyatin.
//
// Of course, there are legit usages for "we," such as academic writing
// where all of "us" are listed on the paper's first page, and the reader
// is interested in overall results rather than the internal dynamics of
// the research team.
//
// But using "we did it" when it's actually "I did it" (and it's
// stylistically appropriate to say "I did it") feels to me like the
// speaker is a wimp who wants to avoid the responsibility.
//
// Likewise, using "we" when it's actually "I and Joe, mostly Joe" feels
// like reluctance to give a fair share of the credit.
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
// That's a one-liner function that relies on two crucial concepts.
//
// The first one is the scope. The scope is pretty much just a binding
// between variables' names and their values. For now, just think of it as
// a sort of a dictionary (it's not exactly that, but we'll get there when
// we get there).
//
// Another one is the differentiation between eager and lazy evaluation.
// Before I go into explaining *what exactly do I mean by eager and lazy
// evaluation* in the context of this story, I first have to elaborate on
// the pragmatics for having all that in the first place.
//
// So. Scheme is a functional programming language, and in functional
// programming, people don't do loops, but instead, they do recursion. And
// for infinite loops, they do, well, infinite recursion. And "infinite"
// here doesn't mean "enormously big," but properly infinite.
//
// Consider this example:
//
// embed snippets/infinite-loop.scm : infinite loop
//
// Obviously, a direct equivalent of this code in vanilla
// C/C++/Ruby/Python/Java will run for some time and eventually blow up
// with a stack overflow. But code in Scheme, well, better shouldn't.
//
// I have three ways to deal with it:
//
// 1. Just do nothing and hope that the C stack will not overflow.
//
// 2. Do code rewriting so that, under the hood, the snippet above is
// automagically converted into a loop, e.g.
//
// embed snippets/infinite-loop.scm : infinite loop rewritten
//
// 3. Apply the technique called trampolining. Semantically it means
//
// embed snippets/infinite-loop.scm : infinite loop trampoline
//
// that instead of calling itself, function... Well, to generalize and to
// simplify, let's say it informs the evaluator that computation is
// incomplete and also tells what to do next in order to complete it.
//
// #1 looks like a joke, but actually, it's a pretty good solution. It's
// also a pretty bad solution, but let's get through the upsides first.
//
// First of all, it's trivial to implement (because it doesn't require
// writing any specific code). It clearly won't introduce any quirky bugs
// (because there's no specific code!). And it won't have any performance
// impact (because it does nothing!!)
//
// You see, "*just do nothing*" half of it is all good; it's the "*and hope
// that*" part that isn't. Although for simple examples, it doesn't really
// matter: provided there are a couple of thousands of stack levels
// available, it's gonna be okay with or without optimizations. But a more
// complex program may eventually hit that boundary, and then I'll have to
// get around deficiencies of my code in, well, my other code, and that's
// not a place I'd like to get myself into.
//
// This also sets a constraint on what a "proper" solution should be: it
// must be provably reliable for a complex piece of code, or else it's back
// to square one.
//
// #2 looks like a super fun thing to play with, and it seems deceptively
// simple for toy snippets. But thinking just a tiny bit about pesky stuff
// like mutually recursive functions, and self-modifying-ish code (think
// `(set! infinite-loop something-else)` *from within* `infinite-loop`),
// and escape procedures and whatnot... And all this starts to feel like a
// breeding ground for wacky corner cases, and I don't want to commit to
// being able to weed them all out.
//
// #3, on the contrary, is pretty straightforward, both conceptually and
// implementation-wise, so that's what I'll do (although I might do #2 on
// top of it later; because it looks like a super fun thing to play with).
//
// Now let's get back to lazy vs eager. "Lazy" in this context means that
// the evaluation function may return either a result (if computation is
// finished) or a thunk (a special object that describes what to do next).
// Whereas "eager" means that the evaluation function will always return
// the final result.
//
// "Eager" evaluation can be easily arranged by getting a "lazy" result
// first...
//

object_t eval_lazy(scope_t scope, object_t expr);
object_t force(object_t value);

object_t eval_eager(scope_t scope, object_t expr)
{
	object_t result_or_thunk = eval_lazy(scope, expr);
	return force(result_or_thunk);
}

//
// ...and then reevaluating it until the computation is complete.
//

struct thunk;
typedef struct thunk* thunk_t;

object_t eval_thunk(thunk_t thunk);
thunk_t to_thunk(object_t obj);

object_t force(object_t value)
{
	thunk_t thunk;

	while ((thunk = to_thunk(value))) {
		object_t new_value = eval_thunk(thunk);
		decref(value);
		value = new_value;
	}

	return value;
}

//
// You know, I've just realized it's the third time in this story when I
// say, "I have three ways to deal with it," the previous two being
// considerations about memory management and error handling in Chapter 1.
//
// Moreover, I noticed a pattern. In a generalized form, those three
// options to choose from are:
//
// 1. Consider yourself lucky. Assume things won't go wrong. Don't worry
// that your solution is too optimistic.
//
// 2. Consider yourself clever. Assume you'll be able to fix every bug.
// Don't worry that your solution is too complex.
//
// 3. Just bloody admit that you're a dumb loser. Design a balanced
// solution that is resilient while still reasonable.
//
// This is such a deep topic that I'm not even going to try to cover it in
// one take, but I'm damn sure I'll be getting back to it repeatedly.
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
// This is relatively straightforward: if it's a symbol, treat it as a name
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
// And evaluating an expression is... Well, if you ever wondered why the
// hell they use so many of those bloody parentheses in Lisps, here's your
// answer.
//
// In most programming languages (mainstream ones anyway), syntax
// constructs and functions are two fundamentally different kinds of
// creatures. They don't just *behave* differently, but they also *look*
// differently, and you can't mix them up.
//
// Much less so in Lisp, where you have `(if foo bar)` for conditional, and
// `(+ foo bar)` to add two numbers, and `(cons foo bar)` to make a pair,
// and you can't help but notice they look pretty darn similar.
//
// Moreover, even though they behave differently, it's not *that
// dissimilar* either. `+` and `cons` are functions that accept *values* of
// `foo` and `bar` and do something with them. Whereas `if` is also simply
// a function, only instead of values of its' arguments, it accepts *a
// chunk of code verbatim.*
//
// Let me reiterate: a syntax construct is merely a *data manipulation
// function* that happens to have *program's code as the data that it
// manipulates.* Oh,  and *code as data* is not some runtime introspection
// shamanistic voodoo, but it's just regular lists and symbols and what
// have you.
//
// And all of that is enabled by using the same notation for data and for
// code. That's why parentheses are so cool.
//
// So, with the explanation above in mind, pretty much all this function
// does is: first, it evaluates the first item of the S-expression, and
// then looks at what it is. If it happens to be an "I want the code as is"
// function, then it's fed code as-is. Otherwise, it is treated as an "I
// want values of the arguments" function instead. That's it.
//
// If my little story is your first encounter with Lisp, I can imagine how
// mind-blowing can this be. Let it sink in, take your time.
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

	while ((expr = pop_from_list(&exprs))) {
		if (argct >= 64)
			DIE("Buffer overflow");
		args[argct++] = eval_eager(scope, expr);
	}

	lambda_t lambda = to_lambda(func);
	if (lambda)
		return wrap_thunk(lambda, argct, args);

	return invoke(func, argct, args);
}

//
// This one is not very complicated: go through the list, evaluate the
// stuff you have there, then either feed it to a function or make a thunk
// to evaluate it later.
//
// There are few minor funky optimizations to mention, though.
//
// 1. I put arguments into a buffer and not to a list. I mean, lists are
// superb for everything except two things. They're not as efficient for
// "just give me an element at index X" random access, and they're kinda
// clumsy when it comes to memory allocation. And these are two things I
// really won't mind having for a function call: as much as I don't care
// about performance, having to do three `malloc()`s just to call a
// function of three arguments feels sorta wasteful.
//
// 2. I introduce `lambda_t` type for functions that are implemented in
// Scheme and require tail-call optimizations. This is done to separate
// them from built-in functions, which are written in C and are supposed to
// be hand-optimized, making lazy call overhead avoidable and unnecessary.
//
// 3. I cap the maximum number of arguments that a function may have at 64.
// I even drafted a tirade to rationalize that... But I'm running out of
// Chardonnay, so let's park it for now and move on to
// ## Chapter 4, where I finally write some code in Scheme
//
// Somewhere around Chapter 2, I mentioned that this whole story began with
// the list of "99 Lisp problems."
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
// A list of problems like that one, where you start with something trivial
// and then work your way towards something more complex, is a great way to
// learn a programming language. I mean, you start at level 1, and you
// figure out the least sufficient fraction of the language that enables
// you to write a simple yet complete and self-contained program. Once that
// knowledge sinks in, you move on to level 2 and learn a bit more, and by
// the time you reach the finale, you have a pretty good command of the
// whole thing.
//
// At least, this is how it works for me. If there is anybody out there,
// who needs to memorize the whole language standard and pass theory exams
// before they can start writing any code, that's fine, I don't judge.
//
// Oh, and of course, it only applies when I'm in relatively unfamiliar
// territory. If you ask me, after all those years I spent writing backends
// in Python and Java, to do some, I dunno, Node.js, I'd be, like, "Hmm, so
// you guys use curly braces here? That's neat." and then just pick a real
// problem straight away.
//
// But also, and maybe even more so, I find it an outstanding way to
// *implement* a language. So, instead of spending months and months to
// build a whole standards-compliant thing, just make something that
// enables that little self-contained program to run and then iterate from
// there.
//
// That said. To get that little function above running, I need three
// syntax constructs, namely `define`, `if`, and `quote`. And I also need
// four standard library functions, which are `null?`, `cdr`, `write`, and
// `newline`.
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
// The functional part of this function is pretty trivial: I've defined
// `wrap_bool()` and `is_nil()` earlier, and now I just use them here.
//
// The code that surrounds that logic needs some clarifications, though.
//
// One thing is passing arguments as an array and not... Well, what else
// can you do here is a bloody good question.
//
// See, in Scheme:
//
// * some functions accept a fixed number of arguments (e.g. `car` accepts
// exactly one and `cons` accepts exactly two)
//
// * some functions accept a variable number of arguments (like `write`
// that we'll get to in a bit which can have one or two)
//
// * and some functions accept an arbitrary number of arguments (like
// `list` that can have as many arguments as you give it)
//
// Oh, and the host language is still good ol' C, so you can have as much
// run-time metadata attached to a function as you want. For as long as you
// can figure out a way to do it all yourself.
//
// That's why the sanest interface I could come up with is simply passing
// an array of arguments and then letting the function itself figure out if
// it's happy with what it got or if it isn't, and here I'm doing exactly
// that.
//
// The other thing that needs explanation is that little `// null?`
// comment. Why it is there is because I need this function to eventually
// end up in REPL scope, which means I'll have to have a registration
// function that will put it there. But I'm also not eager to maintain it
// manually, so I've put together a script that scans the source file and
// generates the code for me.
//
// Well, and since Scheme allows some characters for identifiers that C
// doesn't, I can't simply name this function `native_null?`, hence I have
// to provide the Scheme name separately, which is the purpose of that
// little comment.
//
// In fact, here's the script. People with superstitions about
// programmatically patching sources during the build might need a few deep
// breaths.
//
// embed scripts/codegen.awk
//
// Looks tad awkward (it's because it's written in AWK... yes, a mandatory
// lame pun), but it does the trick.
//
// This setup relies on two functions being available, so let's promise to
// do them later.
//

void register_syntax(const char* name, object_t (*func)(scope_t, object_t));
void register_native(const char* name, object_t (*func)(int, object_t*));

//
// Okay, now let's do `cdr`.
//

object_t native_cdr(int argct, object_t* args) // cdr
{
	assert_arg_count("cdr", argct, 1);
	pair_t pair = assert_pair(args[0], "as an argument #1 of cdr");
	object_t result = cdr(pair);
	incref(result);
	return result;
}

//
// With all the gotchas already explained, this code is so straightforward
// that I don't want to talk about it.
//
// Instead, I want to talk about sports. My favourite kind of physical
// activity is long-distance running. *Really long* distance running. Which
// means marathons.
//
// Now, as we all know, some things are easy, and some are hard. Also, some
// things are simple, and some are complex. And also, there's a belief
// among certain people from Nerd and Nerdish tribes that simple things
// must be easy and that hard things must be complex.
//
// I admit I had few too many White Zinfandels on my way here, so you'll
// have to endure me rambling all over the place. That said. Nerd tribe is
// a well-established notion, so I'm not gonna explain what it is. While
// the Nerdish tribe is a particular type of people, who were taught that
// being clever is kinda cool but weren't taught how to be properly smart.
// So they just keep pounding you with their erudition and intellect, even
// if the situation is such that a properly smart thing to do would be to
// just shut the fuck up.
//
// Anyway. Marathons prove that belief to be wrong. Conceptually, running a
// marathon is very **simple**: you just put one foot in front of the
// other, and that's pretty much it. But the sheer amount of how many times
// you should do that to complete a 42.2-kilometre distance is what makes a
// marathon pretty **hard**, and dealing with that hardness is what makes
// it more complex than it seems.
//
// In fact, there's a whole bunch of stuff that comes into play: physical
// training regimen, mental prep, choice of gear, diet, rhythm, pace,
// handling of weather conditions, even choice of music for the playlist.
//
// In broad terms, the options I choose can be split into three categories:
//
// 1. Stuff that works **for me**. It has never been perfect, but if I
// generally get it right, then by 38th kilometre, I'm enjoying myself and
// singing along to whatever Russian punk rock I've got in my headphones
// and looking forward to a medal and a beer.
//
// 2. Stuff that doesn't work for me. If I get too much stuff wrong, then
// by 38th kilometre, I'm lying on the tarmac and hope that the race
// organizers have a gun to end my suffering.
//
// 3. Stuff that I really really want to work as it looks so awesome in
// theory, **but it still doesn't**, so in the end, it's still lying on the
// tarmac and praying for a quick death.
//
// As a matter of fact, doing marathons has really helped me to crystallize
// my views on engineering: if an idea looks really great on paper, but
// hasn't been tested in battle, then, well, you know, it looks really
// great. On paper.
//
// Okay, while we're at it, let's also do `car`.
//

object_t native_car(int argct, object_t* args) // car
{
	assert_arg_count("car", argct, 1);
	pair_t pair = assert_pair(args[0], "as an argument #1 of car");
	object_t result = car(pair);
	incref(result);
	return result;
}

//
// This function is as straightforward as the previous one, so I don't want
// to talk about it either. Instead, I want to talk about philosophy.
//
// Quite a few times in this story, I said, "there are three ways to do it
// / three options to choose from / three categories it falls into." And
// it's not just my personal gimmick with number 3, but in fact, it's
// Hegelian(-ish) dialectic.
//
// Just to be clear, I'm not nearly as good at philosophy as I am at
// programming, and you can see how mediocre my code is. With that in mind,
// the best way I can explain it is this.
//
// Imagine you've been told that the language you can use in an upcoming
// project can be either Java or Ancient Greek. And you're, like, wait a
// minute, but how can I choose between Java and Ancient Greek?
//
// This is a perfectly valid question. But we're going to take one step
// deeper and look into what makes it perfectly valid. And, well, it's
// because while both Java and Ancient Greek are languages, there's
// otherwise not much common between them, and that's why there's no
// readily available frame of reference in which they can be compared.
//
// And what it means is that you can only compare sufficiently similar
// things, or else you simply can't define criteria for such comparison.
//
// This idea sounds obscenely trivial (like most philosophy does after
// stripping out all the smartass fluff), but in fact, it's immensely
// powerful.
//
// One way to practically apply this idea is: when choosing between
// multiple options, don't jump immediately into the differences between
// them, but instead start by challenging the commonalities.
//
// Say you're deciding whether to build an application with Django or with
// Rails. Both are rich web frameworks that utilize dynamically typed
// languages. This means you implicitly excluded:
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
// yourself from putting a lot of effort into the wrong thing.
//
// A more advanced method is this:
//
// 1. Start with some idea.
//
// 2. Design an alternative idea that is different from the first one in
// every way you could think of.
//
// 3. Look at two options at hand and find the similarities between them
// **that you didn't even think about.** Those are your blind spots.
//
// 4. Come up with a synthetic idea that takes your newly found blind spots
// into account.
//
// 5. Repeat until you run out of blind spots and cover the entire decision
// space for the problem you're solving.
//
// Just try it out, and you'll see how powerful these techniques are.
//
// Okay, enough philosophy, let's get back to coding and do some I/O.
//

struct port;
typedef struct port* port_t;

void assert_vararg_count(const char* name, int actual, int least, int most);
port_t assert_port(object_t obj, const char* context);
FILE* unwrap_port(port_t);

object_t native_write(int argct, object_t* args) // write
{
	assert_vararg_count("write", argct, 1, 2);

	FILE* out = stdout;
	if (argct > 1) {
		port_t port =
			assert_port(args[1], "as an argument #2 of write");
		out = unwrap_port(port);
	}

	write_object(out, args[0]);
	return wrap_nil();
}

//
// Nothing particularly spectacular here. "Port" is how they call a stream
// (or a file handler or what have you) in Scheme. Oh, and it also can
// accept one or two arguments. Aside from that, it's all straightforward.
//

object_t native_newline(int argct, object_t* args) // newline
{
	assert_vararg_count("newline", argct, 0, 1);

	FILE* out = stdout;
	if (argct > 0) {
		port_t port =
			assert_port(args[0], "as an argument #2 of newline");
		out = unwrap_port(port);
	}

	fputs("\n", out);
	return wrap_nil();
}

//
// So is this one. Now I'll do something more complex and evaluate syntax
// for `if`
//

#define ASSERT(expr, fmt, ...)                                                 \
	do {                                                                   \
		if (! (expr))                                                  \
			DIE(fmt, ##__VA_ARGS__);                               \
	} while (0)

bool is_true(object_t);

bool eval_boolean(scope_t scope, object_t code)
{
	object_t value = eval_eager(scope, code);
	bool flag = is_true(value);
	decref(value);
	return flag;
}

object_t syntax_if(scope_t scope, object_t code) // if
{
	object_t test = pop_from_list(&code);
	ASSERT(test, "`if` without <test> part");

	object_t consequent = pop_from_list(&code);
	ASSERT(consequent, "`if` without <consequent> part");

	object_t alternate = pop_from_list(&code);
	ASSERT(is_nil(code), "Malformed `if`");

	if (eval_boolean(scope, test))
		return eval_lazy(scope, consequent);
	else if (alternate)
		return eval_lazy(scope, alternate);
	else
		return wrap_nil();
}

//
// A tiny bit more complex, really. See, when the code you interpret is in
// a convenient standard data structure, it really takes effort and
// determination to make things hard. And although I (obviously) have a
// rant on this topic, I think I put enough random ramblings into this
// chapter already, so I'm going to park it for now and instead evaluate
// `define`.
//

symbol_t assert_symbol(object_t, const char* context);

void define(scope_t, symbol_t key, object_t value);
object_t wrap_lambda(scope_t, object_t formals, object_t code);

object_t syntax_define(scope_t scope, object_t code) // define
{
	object_t head = pop_from_list(&code);
	ASSERT(head, "Malformed `define`");

	symbol_t key;
	object_t value;
	pair_t key_params;

	if ((key = to_symbol(head))) {
		object_t expr = pop_from_list(&code);
		ASSERT(expr, "`define` block without <expression>");
		ASSERT(is_nil(code), "Malformed `define`");
		value = eval_eager(scope, expr);
	} else if ((key_params = to_pair(head))) {
		key = assert_symbol(car(key_params),
				    "variable name in `define`");
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
// This one is a little bit trickier, but that's just because Scheme has an
// alternative form for `define` so that `(define (foo bar) bla bla)` is
// equivalent to `(define foo (lambda (bar) bla bla))`, and I have to
// handle both cases.
//

void bind_to_scope(scope_t, symbol_t key, object_t value);
void set_label(object_t obj, symbol_t label);

void define(scope_t scope, symbol_t key, object_t value)
{
	bind_to_scope(scope, key, value);
	set_label(value, key);
}

//
// And `define()` does just two things: it binds the value to the scope and
// it assigns a label to an object. Rationale for the latter is that if you
// have a function and that function does something dumb, then for error
// reporting it's convenient to know it's called `do-something-dumb`.
//

object_t syntax_quote(scope_t scope, object_t code) // quote
{
	object_t result = pop_from_list(&code);
	ASSERT(result, "Malformed `quote`");
	incref(result);
	return result;
}

//
// `quote` is the shortest of the three, and is also the most descriptive
// when it comes to symmetry between code and data (because here the former
// quite literally becomes the latter).
//
// Now, I promised to make a handful of simple helpers, so I'm just going
// to do it to get them out of the way.
//

void assert_arg_count(const char* name, int actual, int expected)
{
	if (actual != expected)
		DIE("Expected %d arguments for %s, got %d",
		    expected,
		    name,
		    actual);
}

void assert_vararg_count(const char* name, int actual, int least, int most)
{
	if (actual < least)
		DIE("Expected at least %d arguments for %s, got %d",
		    least,
		    name,
		    actual);

	if (actual > most)
		DIE("Expected at most %d arguments for %s, got %d",
		    most,
		    name,
		    actual);
}

port_t to_port(object_t);

port_t assert_port(object_t obj, const char* context)
{
	port_t port = to_port(obj);
	if (port)
		return port;

	DEBUG("obj", obj);
	DIE("Expected a port %s, got %s instead", context, typename(obj));
}

symbol_t assert_symbol(object_t obj, const char* context)
{
	symbol_t sym = to_symbol(obj);
	if (sym)
		return sym;

	DEBUG("obj", obj);
	DIE("Expected a symbol %s, got %s instead", context, typename(obj));
}

//
// And this is a neat place to wrap up this chapter and move on to
// ## Chapter 5, where I reinvent the wheel and then collect garbage
//
// I think I mentioned Pascal somewhere in Chapter 2 of this story. As I
// remember, I haven't touched it since high school, which makes it two
// decades, give or take a year. But, truth be told, I (very briefly)
// dabbled with the idea of using Pascal in this very project. I mean,
// implementing an obscure programming language in another obscure
// programming language, just how cool is that!
//
// Then I scanned the manual for Free Pascal and realized that no, this is
// definitely not going to fly. The show-stopper was Pascal's lack of
// forward declaration for types. So that you must define a type above the
// places where you use it, or else it's an error. And if it doesn't seem
// like such a big deal, have you noticed that by now, I wrote a
// full-fledged parser, and a good chunk of an evaluation engine, and few
// bits of the standard library, and I still haven't defined *a single*
// concrete data type. I just said, "oh yeah, this is a pointer to `struct
// object`, don't ask," and the C compiler was, like, "okay, I'm cool with
// that." But in Pascal, it's apparently illegal.
//
// And no, this is not just a fun gimmick. In fact, it's an experiment in
// tackling one of the most challenging problems in the entire field of
// software engineering. Which is: how to demonstrate that the design of
// your program makes any sense?
//
// One solution is to have a design spec... But wait, let's take a step
// back.
//
// One option is to go, let's call it aggressively ultra-pragmatic, and to
// perform a variation of "This program WORKS! People USE IT! How DARE you
// ask for more than that?! Why do you even CARE?!"
//
// Or something like that. And that's fine. I mean, if your boss is cool
// with that, and your customers are cool with that, and your coworkers are
// cool with that, who am I to judge, really?
//
// But this disclaimer had to be made. Before I dive into ranting about
// suboptimal solutions, I'm obliged to give proper credit to people who at
// least acknowledge the problem. Or else it'd be like being told that
// "compared to serious runners, you're below mediocre" by people who
// haven't exercised for, I dunno, four reincarnations.
//
// So. One way to tackle this problem is to have a design spec where you
// use natural human language to elaborate the underlying logic,
// interactions between individual pieces, decisions made, and all that
// stuff. It's not a bad idea per se, but it has... Not so much a pitfall,
// but more like a fundamental limitation: you can't just give your design
// document to a computer and tell it to execute it. You still have to
// write machine-readable code. So you end up with two intrinsically
// independent artefacts, and unless you have a proper process to prevent
// it, they get out of sync, and your spec becomes "more what you call
// guidelines" than actual spec.
//
// Another option is to stick with code and code only and then make sure
// it's excellent: concise functions, transparent interfaces, meaningful
// naming, all the good stuff. And while it's unquestionably a great idea,
// it has its own fundamental limitation: programming languages just aren't
// particularly fit for expressing intent. I mean, using code itself, you
// can get pretty far with explaining *how* your program works, but not
// such much *why.* For that, you still need some kind of documentation.
//
// And so my idea, and, keep in mind, it's an ongoing experiment and not
// The Next Silver Bullet for which I'm selling certifications, was to
// essentially combine the two in such a way that the whole thing has a
// logical flow of a spec. But then the code is organically embedded into
// it. But then, in the end, it's just a regular program (albeit with
// profuse comments) that can be compiled by a vanilla compiler rather than
// some "literate programming" contraption.
//
// And for this, the code layout rules in the underlying programming
// language must be as libertarian as possible. Or else you end up with a
// lot of handwaving that this data type must have such and such fields
// even though you won't be able to justify their necessity until fifty
// pages later.
//
// All this was an extremely long introduction to the very first proper
// data type in this program. But finally, I proudly present you
//

struct array {
	object_t* data;
	int avail;
	int size;
};

typedef struct array* array_t;

//
// and after seeing it, you're obliged to ask, "Dude, but why don't you use
// C++? At least you'll get `std::vector` for free!"
//
// And that's indeed a reasonable question. I even agree that from a
// practical standpoint C++ would be at least as good as plain C and likely
// better.
//
// However, for a project like this one, the very concept of a "practical
// standpoint" is a slippery slope. Because right after "but why don't you
// use C++ and get `std::vector` for free?" comes "but why don't you use
// Java and get garbage collector for free?" and then "but why you don't
// just `apt install scheme`?" and eventually "but why you don't learn some
// hipper language?"
//
// And then you find yourself surrounded by weirdos at a shady Kotlin
// meetup, and you lost count of how many tasteless beers you had, and
// you're wondering if things will end very badly or even worse.
//
// No, really, the purpose of this whole undertaking is not to build
// anything. It's not even to learn, not something specific anyway, but
// mostly just to have fun doing it. Essentially it's simply one extensive
// coding exercise. And that's why one of the guiding principles here is
// when choosing between a more practical option and one that's less
// practical but still reasonable, go for the latter.
//
// Oh, and "still reasonable" explains why I use plain C and not, I dunno,
// Fortran 66. Writing a Scheme interpreter in Fortran 66 will get you deep
// into "pain and humiliation for the sake of pain and humiliation"
// territory. Of course, there's absolutely nothing wrong with enjoying
// some pain and humiliation, except that a computer is not the best home
// appliance to use for these purposes if you get what I mean.
//
// Anyway. A dynamically expandable array is one of the most trivial things
// in this entire story, and I'm not going to spend any time explaining it.
//

void init_array(array_t arr)
{
	arr->data = malloc(sizeof(object_t) * 8);
	arr->avail = 8;
	arr->size = 0;
}

void dispose_array(array_t arr)
{
	free(arr->data);
}

void push_to_array(array_t arr, object_t entry)
{
	if (arr->size == arr->avail) {
		arr->avail *= 2;
		arr->data = realloc(arr->data, sizeof(object_t) * arr->avail);
	}
	arr->data[arr->size++] = entry;
}

object_t pop_from_array(array_t arr)
{
	if (arr->size == 0)
		return NULL;
	return arr->data[--arr->size];
}

//
// But the reason *why* I suddenly decided I need a dynamically expandable
// list is because I'm gonna need it to build a garbage collector. Not that
// I really need a garbage collector, but when you have a parser, and an
// evaluation engine, and a chunk of the standard library, it's so hard to
// resist from a not-so-subtle Hunter S. Thompson reference.
//
// Of course, I need a garbage collector in my Scheme interpreter, but it
// doesn't have to be very sophisticated. A simplistic mark-and-sweep would
// suffice.
//
// During the mark-and-sweep garbage collection, an object can be in one of
// three states:
//

enum gc_state {
	REACHED,
	REACHABLE,
	GARBAGE,
};

//
// `REACHED` means this object is reachable from the call stack, global
// scope and/or other reachable objects, and we've already propagated its
// reachability to the objects it refers to (if any).
//
// `REACHABLE` means this object is reachable, but we haven't propagated
// its reachability yet.
//
// `GARBAGE` means this object is not reachable and (assuming we examined
// all objects we have) can be safely disposed of.
//
// And then entire algorithm consists of three steps.
//

void mark_globally_reachable(void);
void propagate_reachability(void);
void dispose_garbage(void);

void collect_garbage()
{
	mark_globally_reachable();
	propagate_reachability();
	dispose_garbage();
}

//
// First, mark globally reachable objects as such.
//

struct array ALL_OBJECTS;
struct array REACHABLE_OBJECTS;

bool hasrefs(object_t);
void set_gc_state(object_t, enum gc_state);

void mark_globally_reachable(void)
{
	REACHABLE_OBJECTS.size = 0;

	for (int i = ALL_OBJECTS.size - 1; i >= 0; i--) {
		object_t obj = ALL_OBJECTS.data[i];

		if (! hasrefs(obj)) {
			set_gc_state(obj, GARBAGE);
			continue;
		}

		set_gc_state(obj, REACHABLE);
		push_to_array(&REACHABLE_OBJECTS, obj);
	}
}

//
// Then propagate reachability.
//

enum gc_state get_gc_state(object_t);
void reach(object_t);

void propagate_reachability(void)
{
	object_t obj;

	while ((obj = pop_from_array(&REACHABLE_OBJECTS)))
		if (get_gc_state(obj) == REACHABLE)
			reach(obj);
}

//
// And finally, dispose of garbage.
//

void dispose(object_t);

void dispose_garbage(void)
{
	int scan, keep = 0, count = ALL_OBJECTS.size;

	for (scan = 0; scan < count; scan++) {
		object_t obj = ALL_OBJECTS.data[scan];
		if (get_gc_state(obj) == REACHED) {
			ALL_OBJECTS.data[keep++] = obj;
			continue;
		}
		dispose(obj);
	}

	ALL_OBJECTS.size = keep;
}

//
// And now we can proceed to
// ## Chapter 6, where things become objective
//
// But before that, they'll stay subjective for few more paragraphs.
//
// One of the major influences on me and my work on this project is Dan
// Carlin of "Hardcore History" fame, and if you don't know the "Hardcore
// History" podcast, you should definitely check it out; it's outstanding.
//
// One particular thing I love about that podcast is how Dan tells his
// stories in such a way that they're incredibly informative, and I myself
// learned an awful lot about history by listening and re-listening his
// podcasts over the past decade.
//
// At the same time, they clearly won't qualify for "educational," and by
// "educational," I mean a particular type of teacher-student dynamics, one
// that is built around student having precise pragmatic reasons to learn
// the topic. So that you study X because you want to find a job, pass an
// exam, get a promotion, earn money, or anything like that. And then it's
// the teacher's responsibility to maximize the utility of the course, even
// if it comes at the cost of leaving out things that are exciting,
// insightful, or just fun but not immediately practical.
//
// Now, "Hardcore History" (as well as a good bunch of books, YouTube
// channels and other podcasts) is nothing like a class that you have to
// pass to get a pay raise. Instead, it's much more like a group of
// amateurs sitting around a campfire and sharing stories about Punic Wars,
// or exoplanets, or functional programming, and just having fun. This is
// pretty much the vibe I want in my own work, and if you expected a
// programming textbook, feel free to ask your money back. Although, it's
// chapter six already, so I think you should've figured that out by now.
//
// And no, I'm not saying that learning useful stuff is somehow wrong. It's
// good, at the very least, it's, well, useful. However, if everything you
// study always has to be practical, you effectively imply that pure
// unrestrained curiosity is something to be embarrassed about.
//
// And that I find sad.
//
// Anyway, let's get objective and let's finally declare the `struct
// object` that was defined back in the first chapter.
//
// So far, there are 41 functions (boy, that escalated quickly!) that I
// promised to implement later, and they can be separated into five
// categories:
//
// 1. Ones with very specific types of either arguments or results: `car`,
// `cdr`, `define`, `eval_syntax`, `eval_thunk`, `is_nil`, `is_symbol`,
// `is_true`, `to_*`s, `unwrap_*`s, `wrap_*`s.
//
// 2. Ones that have something to do with memory management: `decref`,
// `hasrefs`, `incref`, `get_gc_state`, `set_gc_state`.
//
// 3. Ones that are used to implement scoping mechanisms and/or to maintain
// runtime environment in general: `define`, `get_repl_scope`,
// `lookup_in_scope`, `register_syntax`, `register_native`,
// `setup_runtime`, `teardown_runtime`.
//
// 4. Ones that should do different things depending on input type:
// `dispose`, `invoke`, `reach`, `set_label`, `typename`, `write_object`.
//
// 5. No, actually, there's no fifth category. These four pretty much cover
// everything I've promised so far.
//
// So. There are two important ideas to take away from this analysis. The
// first one is that `struct object` should contain some sort of a type
// marker to enable **(4)** and also **(1)**. The second one is that the
// only other functionality relevant for every object is the memory
// management machinery.
//
// So let's do just that. Let's promise to introduce a type marker that (as
// always) we'll  figure out later.
//

struct type;
typedef struct type* type_t;

//
// Now, for memory management, it's pretty evident that we need a counter
// for stack references and an `enum gc_state` field for, well, GC state,
// which means we can (finally! hooray!) define `struct object`.
//

struct object {
	type_t type;
	unsigned stackrefs;
	enum gc_state gc_state;
};

//
// Functions for manipulating reference counter and GC state are as trivial
// as it ever gets, so I'll just write them down.
//

void decref(object_t obj)
{
	ASSERT(obj->stackrefs > 0, "decref() without matching incref()");
	obj->stackrefs--;
}

bool hasrefs(object_t obj)
{
	return obj->stackrefs > 0;
}

void incref(object_t obj)
{
	obj->stackrefs++;
}

enum gc_state get_gc_state(object_t obj)
{
	return obj->gc_state;
}

void set_gc_state(object_t obj, enum gc_state s)
{
	obj->gc_state = s;
}

//
// Now I need to figure out how to represent type information.
//
// One way to do it is by having a simple `enum type` to act as a type
// marker, and then do the polymorphic dispatch using a simple (although
// possibly lengthy) `switch`, i.e.
//
// embed snippets/typeswitch.c : type switch
//
// It's not a bad idea per se. In fact, this is how early drafts of this
// story were implemented, and it can work okay. But it has *narrative*
// issues.
//
// Yes, narrative, remember this is still a literary programming exercise?
//
// Basically, the problem here is that when writing such a switch, I have
// to list all applicable types. But this means I'm forced to work out all
// types before I write the function, and then I'm forbidden to add any new
// ones after I'm done. And these limitations I find pretty inconvenient.
//
// In principle, I can try to pull out something with fancy code generation
// in a similar vein to how I register native functions and syntax
// processors. But I've a gut feeling that it's gonna be nasty: all these
// functions are slightly different, some return values, others don't, some
// should do nothing by default, others should do something, and expressing
// all that in a way that is edible by a code generator, but still looks
// like proper C code is gonna be tricky and convoluted. Didn't try that,
// actually, and don't really want to.
//
// What I came up with instead is this:
//

struct type {
	const char* name;
	void (*display)(FILE*, object_t);
	void (*dispose)(object_t);
	object_t (*invoke)(object_t func, int argc, object_t* args);
	void (*label)(object_t obj, symbol_t label);
	void (*reach)(object_t);
	void (*write)(FILE*, object_t);
};

//
// And yes, after seeing it, you're obliged to ask, "Dude, but seriously,
// why don't you just use C++, or, like, literally any language that gives
// you vtables for free?"
//
// Reason number one was, obviously, because I wanted to make my own
// vtable! I mean, one can spend many years writing code in something like
// Java or Python and just accept virtual functions as magic that simply
// happens.
//
// So I started making one, and I put together a struct with a handful of
// function pointers, and then I realized I'm pretty much done! This was a
// funny realization.
//
// Reason number two was to, kinda, take a break from object-oriented
// programming. Don't get me wrong, OOP is a fantastic idea where you have
// a limited set of simple concepts and then build the entire universe with
// them. Amazing.
//
// But then, since you're using the same notions to express slightly
// different things, you might start to lose an overview of what exactly do
// you mean here. Does this method belong to the class because it's used
// for polymorphic dispatch? Or maybe it's attached to the class just for
// the sake of grouping things together, but otherwise can be a
// free-standing function? Or perhaps it can be a free-standing function
// that is made overloadable *just in case*?
//
// And then you use patterns to make it all more expressive, all the usual
// stuff. But it's still a valuable exercise to program in an environment
// where these two goals are achieved by different means and therefore are
// genuinely explicit.
//
// The approach that I personally would call the most elegant is how it's
// done in Haskell, where classes are *just slightly too cumbersome* to use
// for cases where you don't actually need polymorphism.
//
// Anyway, now that we have a type marker let's write some functions.
//

const char* typename(object_t obj)
{
	return obj->type->name;
}

//
// Okay, this one is trivial.
//

void dispose(object_t obj)
{
	if (obj->type->dispose)
		obj->type->dispose(obj);
	free(obj);
}

//
// This one is also pretty self-explanatory. It also demonstrates how to
// mark that the function doesn't do anything special by default, which is
// simply by assigning the pointer to "NULL".
//

void set_label(object_t obj, symbol_t label)
{
	if (obj->type->label)
		obj->type->label(obj, label);
}

//
// Labelling is also optional: knowing the function's name is helpful, but
// one of an integer number, not so much.
//

void decref_many(int argct, object_t* args)
{
	for (int i = argct - 1; i >= 0; i--)
		decref(args[i]);
}

object_t invoke(object_t func, int argct, object_t* args)
{
	if (! func->type->invoke)
		DIE("Can't invoke object of type %s", typename(func));
	object_t result = func->type->invoke(func, argct, args);
	decref_many(argct, args);
	return result;
}

//
// Nothing special in this one except for `decref()`ing function arguments
// that we (remember?!) have to do to mark they're not on call stack
// anymore.
//

void reach(object_t obj)
{
	if (obj->gc_state == REACHED)
		return;

	obj->gc_state = REACHED;
	if (obj->type->reach)
		obj->type->reach(obj);
}

//
// Pretty straightforward as well.
//

void display_object(FILE* out, object_t obj)
{
	if (obj->type->display)
		obj->type->display(out, obj);
	else
		write_object(out, obj);
}

void write_object(FILE* out, object_t obj)
{
	if (obj->type->write)
		obj->type->write(out, obj);
	else
		fprintf(out, "[%s@%p]", obj->type->name, obj);
}

//
// `(display)` and `(write)` do almost the same thing except that latter
// puts quotes around strings.
//
// While we're at it, let's bend our rules a bit and write a pair of
// functions that weren't previously promised but belong here anyway.
//
// First one puts a newly initialized object into garbage collection
// machinery (as well as triggers the garbage collection itself); it'll be
// used in various `wrap_*()` functions.
//

void register_object(object_t obj)
{
	static int threshold = 100;

	if (ALL_OBJECTS.size > threshold) {
		collect_garbage();
		threshold = ALL_OBJECTS.size * 2;
	}

	push_to_array(&ALL_OBJECTS, obj);
}

//
// And the second one marks an object as reachable; it'll be used in
// different `reach_*()` implementations.
//

void mark_reachable(object_t obj)
{
	if (! obj)
		return;

	if (obj->gc_state == GARBAGE) {
		obj->gc_state = REACHABLE;
		push_to_array(&REACHABLE_OBJECTS, obj);
	}
}

//
// And with these functions written down, it's a good time to wrap up, but
// only to continue in
// ## Chapter 7, where I do some typing and some pairing
//
// For a change, no random ramblings at the beginning of this chapter.
// Let's go straight to coding.
//
// In the previous chapter, I've built up a kind of framework to define the
// types, and now I'm going to use it to make one. And to make an
// informative first example, it should be something simple (so we don't
// get lost in type's inherent complexity), but also not trivial (so we get
// a good example).
//
// So, I'm going to make a string, and I'm going to start with `struct
// string`:
//

struct string {
	struct object self;
	char* value;
};

typedef struct string* string_t;

//
// This structure (as well as its' sister data structures throughout this
// story) contains a neat little hack with a `struct object` at the
// beginning of it. This makes every valid pointer to a `struct string`
// also a valid pointer to a `struct object`, so I can get from one to the
// other via mere type casting, which is pretty convenient.
//
// I know some people who'll frown at this solution, saying that this is a
// hack that can be abused and misused by people who have no clear idea
// what they're doing and are in a hurry.
//
// Which is sort of true. However. There's one thing I've never seen in my
// entire professional software engineering career. And that is the code
// that is so perfect that it can resist an indefinite amount of violence
// by people who have no clear idea what they're doing and are in a hurry.
// Everything can be broken given enough blunt force. So at the end of the
// day, you have to draw a proverbial "line in the sand" somewhere. And
// it's not an objective "right thing" to do, but rather a compromise to
// achieve an optimal ratio between the effort spent on making code
// fool-proof and the value gained... Well, let's just be blunt here, from
// having fools hacking your codebase.
//
// And in the case of this story, as a hobbyist auteur without deadlines,
// customers, or coworkers, I'm legally allowed to skip all this mental
// jiu-jitsu altogether.
//
// Let's continue implementing a string data type.
//

void display_string(FILE* out, object_t obj)
{
	string_t str = (string_t)obj;
	fputs(str->value, out);
}

void dispose_string(object_t obj)
{
	string_t str = (string_t)obj;
	free(str->value);
}

void write_string(FILE* out, object_t obj)
{
	string_t str = (string_t)obj;
	fprintf(out, "\"%s\"", str->value);
}

struct type TYPE_STRING = {
	.name = "string",
	.display = display_string,
	.dispose = dispose_string,
	.write = write_string,
};

//
// What we've got here is:
//
// * custom `.display` function to make `(display "bla")` emit `bla`
//
// * custom `.write` function to make `(write "bla")` emit `"bla"` with
// quotes
//
// * custom `.dispose` function to `free()` the pointer to `.value`
//
// * and for the rest we're fine with the defaults
//
// Absolutely straightforward, really.
//
// The only remaining piece needed to wrap up strings is, well,
// `wrap_string()`. And here it goes.
//

#include <strings.h>

void* alloc_object(const type_t type, size_t size)
{
	object_t obj = malloc(size);
	obj->type = type;
	obj->stackrefs = 1;
	register_object(obj);
	return obj;
}

object_t wrap_string(const char* v)
{
	string_t str = alloc_object(&TYPE_STRING, sizeof(*str));
	str->value = strdup(v);
	return (object_t)str;
}

//
// Yeah, it's pretty dull, just allocating a structure and populating it
// with exactly what you'd expect.
//
// Okay, we're done with strings. Now lets's make what's perhaps the most
// iconic Lisp data type of all, the Mighty Pair.
//

struct pair {
	struct object self;
	object_t car;
	object_t cdr;
};

//
// Yes, exactly how you'd expect a pair of pointers to look. Let's write
// `car()`.
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
// And also `cdr()`.
//

void reach_pair(object_t obj)
{
	pair_t pair = (pair_t)obj;
	mark_reachable(pair->car);
	mark_reachable(pair->cdr);
}

//
// Propagating reachability is unsophisticated as well.
//

void write_pair(FILE* out, object_t obj)
{
	pair_t pair = (pair_t)obj;

	fputs("(", out);

	while (pair) {
		write_object(out, car(pair));

		obj = cdr(pair);
		if ((pair = to_pair(obj)))
			fputs(" ", out);
	}

	if (! is_nil(obj)) {
		fputs(" . ", out);
		write_object(out, obj);
	}

	fputs(")", out);
}

//
// Writing a pair is a tiny bit more involved because pairs are used to
// construct lists, and lists should be represented appropriately, i.e.
// `(write (cons 'a (cons 'b ())))` should produce `(a b)`.
//

struct type TYPE_PAIR = {
	.name = "pair",
	.reach = reach_pair,
	.write = write_pair,
};

//
// Now we can assemble a type.
//

object_t wrap_pair(object_t head, object_t tail)
{
	pair_t pair = alloc_object(&TYPE_PAIR, sizeof(*pair));
	pair->car = head;
	pair->cdr = tail;
	return (object_t)pair;
}

pair_t to_pair(object_t obj)
{
	if (obj->type == &TYPE_PAIR)
		return (pair_t)obj;
	return NULL;
}

//
// Then implement two previously promised pairs-related functions, and now
// we have pairs, lists, and all other fun things that can be made from
// them.
//
// If you're bored to tears by ultra-trivial snippets of code accompanied
// by my remarks that "oh, that's ultra-trivial," trust me, I'm also sick
// and tired of writing those remarks.
//
// This is a dark legacy of me being a paid coder for a long time. And what
// I learned from all that experience is that there are two techniques to
// do programming. One is tedious, and the other is hard.
//
// How do I explain the difference... Well, once, many years ago, I had an
// interview with co-founders of a startup. They've been at a very early
// stage and didn't have an office yet, we met in a cafe. And during the
// conversation, the tech co-founder dropped a remark to the effect of
// "come on, writing code is easy, fixing all the bugs is the hard part."
// As I said, it was many years ago, back when I was young, shy, and
// polite. If I had such a conversation today, I'd just stand up and pull
// out a plausible excuse to leave immediately.
//
// For sure, only a Sith deals in absolutes, but generally speaking, when
// debugging is hard, it means you're not in full control of your own
// software. Which is not necessarily your personal fault: maybe you've got
// a legacy fossilised turd to maintain, or your domain area is knotty, or
// whatnot. But still, having problems is one thing, but not even realising
// you have them is a whole different level.
//
// So, in a nutshell, these are two fundamental options. Either you strive
// to stay in control, and for that, you ensure your code is easy to
// comprehend, and your architecture is sound and reasonable, and your
// system is easy to test. With that in place, you're left with a ton of
// "paint by numbers" kind of work to get done that is bleak and boring but
// not particularly hard.
//
// Or else you can skip all that bureaucratic dreck, and then your life
// gets, well, significantly more adventurous.
//
// And to illustrate this statement, I'm going to implement `integer_t`.
//

struct integer {
	struct object self;
	int value;
};

typedef struct integer* integer_t;

void write_int(FILE* out, object_t obj)
{
	integer_t num = (integer_t)obj;
	fprintf(out, "%d", num->value);
}

struct type TYPE_INT = {
	.name = "int",
	.write = write_int,
};

object_t wrap_int(int v)
{
	integer_t num = alloc_object(&TYPE_INT, sizeof(*num));
	num->value = v;
	return (object_t)num;
}

//
// Yep, nothing to write home about. Let's do `char_t`.
//

struct character {
	struct object self;
	char value;
};

typedef struct character* char_t;

void display_char(FILE* out, object_t obj)
{
	char_t ch = (char_t)obj;
	fputc(ch->value, out);
}

void write_char(FILE* out, object_t obj)
{
	char_t ch = (char_t)obj;
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

//
// Okay, this one at least needs some knowledge of Scheme's notation for
// characters; the rest is pure sleepwalking.
//

struct type TYPE_CHAR = {
	.name = "character",
	.display = display_char,
	.write = write_char,
};

object_t wrap_char(char v)
{
	char_t ch = alloc_object(&TYPE_CHAR, sizeof(*ch));
	ch->value = v;
	return (object_t)ch;
}

//
// While we're at it, let's also do `port_t`.
//

struct port {
	struct object self;
	FILE* value;
};

FILE* unwrap_port(port_t port)
{
	return port->value;
}

void dispose_port(object_t obj)
{
	port_t port = (port_t)obj;
	fclose(port->value);
}

struct type TYPE_PORT = {
	.name = "port",
	.dispose = dispose_port,
};

struct object* wrap_file(FILE* v)
{
	port_t port = alloc_object(&TYPE_PORT, sizeof(*port));
	port->value = v;
	return &port->self;
}

port_t to_port(object_t obj)
{
	if (obj->type == &TYPE_PORT)
		return (port_t)obj;
	return NULL;
}

//
// And now let's do `nil`.
//

void write_nil(FILE* out, object_t obj)
{
	fprintf(out, "()");
}

struct type TYPE_NIL = {
	.name = "nil",
	.write = write_nil,
};

struct object NIL = {
	.type = &TYPE_NIL,
};

object_t wrap_nil(void)
{
	object_t obj = &NIL;
	incref(obj);
	return obj;
}

bool is_nil(object_t obj)
{
	return obj == &NIL;
}

//
// So, this is what happens when you strive to make your code as simple as
// possible: eventually, you transcend the mere triviality and reach pure
// Zen. An empty list is just a special object with no internal structure,
// so it's natural to implement it as a constant without a data structure
// to accompany it.
//
// Now let's do the same for booleans.
//

void write_bool(FILE* out, object_t obj);

struct type TYPE_BOOL = {
	.name = "bool",
	.write = write_bool,
};

struct object TRUE = {
	.type = &TYPE_BOOL,
};

struct object FALSE = {
	.type = &TYPE_BOOL,
};

bool is_false(object_t obj)
{
	return obj == &FALSE;
}

bool is_true(object_t obj)
{
	return ! is_false(obj);
}

//
// A minor gotcha here is that in Scheme, the only "false-ish" value is
// boolean false, and everything else is "true-ish."
//

void write_bool(FILE* out, object_t obj)
{
	fputs(obj == &TRUE ? "#t" : "#f", out);
}

object_t wrap_bool(bool v)
{
	object_t obj = v ? &TRUE : &FALSE;
	incref(obj);
	return obj;
}

//
// And now it's time to end this chapter and begin
// ## Chapter 8, where I reinvent one more wheel
// CUTOFF
//

struct symbol {
	struct object self;
	const char* value;
	unsigned int hash;
};

void write_symbol(FILE* out, object_t obj)
{
	symbol_t symbol = (symbol_t)obj;
	fprintf(out, "%s", symbol->value);
}

void dispose_symbol(object_t obj)
{
	symbol_t symbol = (symbol_t)obj;
	free((char*)symbol->value);
}

struct type TYPE_SYMBOL = {
	.name = "symbol",
	.dispose = dispose_symbol,
	.write = write_symbol,
};

bool is_symbol(const char* text, object_t obj)
{
	symbol_t sym = to_symbol(obj);
	if (sym)
		return strcmp(text, unwrap_symbol(sym)) == 0;
	return false;
}

symbol_t to_symbol(object_t obj)
{
	if (obj->type == &TYPE_SYMBOL)
		return (symbol_t)obj;
	return NULL;
}

const char* unwrap_symbol(symbol_t sym)
{
	return sym->value;
}

//

struct dict_entry {
	symbol_t key;
	object_t value;
};

typedef struct dict_entry* dict_entry_t;

struct dict {
	struct dict_entry* data;
	unsigned int size;
	unsigned int used;
};

typedef struct dict* dict_t;

void init_dict(dict_t dict)
{
	bzero(dict, sizeof(*dict));
}

void dispose_dict(dict_t dict)
{
	free(dict->data);
	bzero(dict, sizeof(*dict));
}

//

unsigned strhash(const char* key)
{
	unsigned result = 0;
	while (*key)
		result = result * 7 + *(key++);
	return result;
}

//

scope_t get_symbol_pool(void);

object_t wrap_symbol(const char* text)
{
	scope_t pool = get_symbol_pool();
	int hash = strhash(text);

	struct symbol tmp = {
		.value = text,
		.hash = hash,
	};

	object_t result = lookup_in_scope(pool, &tmp);
	if (result) {
		incref(result);
		return result;
	}

	symbol_t sym = alloc_object(&TYPE_SYMBOL, sizeof(*sym));
	sym->value = strdup(text);
	sym->hash = hash;
	result = (object_t)sym;
	bind_to_scope(pool, sym, result);
	return result;
}

//

int compare_symbols(symbol_t, symbol_t);
void enlarge_dict(dict_t);

object_t put_in_dict(struct dict* dict, symbol_t key, object_t value)
{
	if ((dict->size == 0) || (dict->used * 2 > dict->size))
		enlarge_dict(dict);

	for (unsigned int index = key->hash;; index++) {
		dict_entry_t entry = &dict->data[index % dict->size];

		if (entry->key == NULL) {
			entry->key = key;
			entry->value = value;
			dict->used++;
			return NULL;
		}

		int diff = compare_symbols(entry->key, key);
		if (diff == 0) {
			object_t result = entry->value;
			entry->value = value;
			return result;
		}

		if (diff > 0) {
			struct dict_entry tmp = *entry;
			entry->key = key;
			entry->value = value;
			key = tmp.key;
			value = tmp.value;
		}
	}
}

//

int compare_symbols(symbol_t a, symbol_t b)
{
	if (a->hash < b->hash)
		return -1;
	if (a->hash > b->hash)
		return 1;
	return strcmp(a->value, b->value);
}

object_t lookup_in_dict(dict_t dict, symbol_t sym)
{
	if (dict->size == 0)
		return NULL;

	for (unsigned index = sym->hash;; index++) {
		dict_entry_t entry = &dict->data[index % dict->size];
		if (entry->key == NULL)
			return NULL;

		int diff = compare_symbols(entry->key, sym);
		if (diff == 0)
			return entry->value;
		if (diff > 0)
			return NULL;
	}
}

//

void enlarge_dict(dict_t d)
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

	for (int i = 0; i < old.size; i++) {
		struct dict_entry* entry = &old.data[i];
		if (entry->key)
			put_in_dict(d, entry->key, entry->value);
	}
	free(old.data);
}

void reach_dict(dict_t dict)
{
	for (int i = dict->size - 1; i >= 0; i--)
		if (dict->data[i].key)
			mark_reachable(dict->data[i].value);
}

//
// ## Chapter 9
//

struct scope {
	struct object self;
	struct dict binds;
	scope_t parent;
};

void bind_to_scope(scope_t scope, symbol_t key, object_t value)
{
	object_t ptr = put_in_dict(&scope->binds, key, value);
	if (ptr != NULL) {
		const char* strkey = unwrap_symbol(key);
		DIE("%s is already defined", strkey);
	}
}

object_t lookup_in_scope(scope_t scope, symbol_t key)
{
	while (scope) {
		object_t value = lookup_in_dict(&scope->binds, key);
		if (value)
			return value;
		scope = scope->parent;
	}
	return NULL;
}

scope_t derive_scope(scope_t parent);

scope_t get_repl_scope()
{
	static scope_t repl_scope = NULL;

	if (repl_scope == NULL)
		repl_scope = derive_scope(NULL);

	return repl_scope;
}

scope_t get_symbol_pool(void)
{
	static scope_t pool = NULL;

	if (pool == NULL)
		pool = derive_scope(NULL);

	return pool;
}

void reach_scope(object_t obj)
{
	scope_t scope = (scope_t)obj;
	reach_dict(&scope->binds);
	mark_reachable((object_t)scope->parent);
}

void dispose_scope(object_t obj)
{
	scope_t scope = (scope_t)obj;
	dispose_dict(&scope->binds);
}

struct type TYPE_SCOPE = {
	.name = "scope",
	.reach = reach_scope,
	.dispose = dispose_scope,
};

scope_t derive_scope(scope_t parent)
{
	scope_t scope = alloc_object(&TYPE_SCOPE, sizeof(*scope));
	init_dict(&scope->binds);
	scope->parent = parent;
	return scope;
}

struct native {
	struct object self;
	object_t (*invoke)(int, object_t*);
};

typedef struct native* native_t;

object_t invoke_native(object_t obj, int argct, object_t* args)
{
	native_t native = (native_t)obj;
	return native->invoke(argct, args);
}

struct type TYPE_NATIVE = {
	.name = "native",
	.invoke = invoke_native,
};

void register_native(const char* name, object_t (*func)(int, object_t*))
{
	native_t native = alloc_object(&TYPE_NATIVE, sizeof(*native));
	native->invoke = func;

	object_t key = wrap_symbol(name);

	define(get_repl_scope(), (symbol_t)key, (object_t)native);
	decref((object_t)native);
	decref(key);
}

struct syntax {
	struct object self;
	object_t (*eval)(scope_t, object_t);
};

typedef struct syntax* syntax_t;

syntax_t to_syntax(object_t);

object_t eval_syntax(scope_t scope, object_t obj, object_t code)
{
	syntax_t syntax = to_syntax(obj);
	if (syntax)
		return syntax->eval(scope, code);
	else
		return NULL;
}

struct type TYPE_SYNTAX = {
	.name = "syntax",
};

void register_syntax(const char* name, object_t (*func)(scope_t, object_t))
{
	object_t key = wrap_symbol(name);

	syntax_t syntax = alloc_object(&TYPE_SYNTAX, sizeof(*syntax));
	syntax->eval = func;

	define(get_repl_scope(), (symbol_t)key, (object_t)syntax);

	decref((object_t)syntax);
	decref(key);
}

struct lambda {
	struct object self;
	object_t body;
	scope_t scope;
	symbol_t label;
	struct array params;
};

void reach_lambda(object_t obj)
{
	lambda_t lambda = (lambda_t)obj;
	mark_reachable(lambda->body);
	mark_reachable((object_t)lambda->scope);
	mark_reachable((object_t)lambda->label);
	for (int i = lambda->params.size - 1; i >= 0; i--)
		mark_reachable(lambda->params.data[i]);
}

void dispose_lambda(object_t obj)
{
	lambda_t lambda = (lambda_t)obj;
	dispose_array(&lambda->params);
}

object_t eval_block(scope_t, object_t);

object_t invoke_lambda(object_t obj, int argct, object_t* args)
{
	lambda_t lambda = (lambda_t)obj;
	struct array* params = &lambda->params;
	const char* name = lambda->label ? unwrap_symbol(lambda->label) : NULL;

	assert_arg_count(name, argct, params->size);

	scope_t scope = derive_scope(lambda->scope);

	for (int i = 0; i < params->size; i++)
		define(scope, (symbol_t)params->data[i], args[i]);
	object_t result = eval_block(scope, lambda->body);

	decref((object_t)scope);
	return result;
}

void label_lambda(object_t obj, symbol_t label)
{
	lambda_t lambda = (lambda_t)obj;
	if (! lambda->label)
		lambda->label = label;
}

void write_lambda(FILE* out, object_t ptr)
{
	lambda_t lambda = (lambda_t)ptr;

	fputs("(lambda (", out);
	for (int i = 0; i < lambda->params.size; i++) {
		if (i > 0)
			fputc(' ', out);
		write_object(out, lambda->params.data[i]);
	}
	fputc(')', out);
	object_t body = lambda->body, obj;

	while ((obj = pop_from_list(&body))) {
		fputc(' ', out);
		write_object(out, obj);
	}

	fputc(')', out);
}

struct type TYPE_LAMBDA = {
	.name = "lambda",
	.dispose = dispose_lambda,
	.invoke = invoke_lambda,
	.label = label_lambda,
	.reach = reach_lambda,
	.write = write_lambda,
};

lambda_t to_lambda(object_t obj)
{
	if (obj->type == &TYPE_LAMBDA)
		return (lambda_t)obj;
	return NULL;
}

object_t wrap_lambda(scope_t scope, object_t params, object_t body)
{
	lambda_t lambda = alloc_object(&TYPE_LAMBDA, sizeof(*lambda));
	lambda->body = body;
	lambda->scope = scope;
	lambda->label = NULL;
	init_array(&lambda->params);

	object_t param;
	while ((param = pop_from_list(&params))) {
		assert_symbol(param, "function argument");
		push_to_array(&lambda->params, param);
	}

	return (object_t)lambda;
}

object_t eval_block(scope_t scope, object_t code)
{
	object_t result = wrap_nil(), expr;

	while ((expr = pop_from_list(&code))) {
		result = force(result);
		decref(result);
		result = eval_lazy(scope, expr);
	}

	return result;
}

struct thunk {
	struct object self;
	lambda_t lambda;
	int argct;
	object_t* args;
};

void thunk_dispose(object_t obj)
{
	thunk_t thunk = (thunk_t)obj;
	free(thunk->args);
}

void thunk_reach(object_t obj)
{
	thunk_t thunk = (thunk_t)obj;
	mark_reachable((object_t)thunk->lambda);
	for (int i = thunk->argct - 1; i >= 0; i--)
		mark_reachable(thunk->args[i]);
}

struct type TYPE_THUNK = {
	.name = "thunk",
	.reach = thunk_reach,
	.dispose = thunk_dispose,
};

object_t wrap_thunk(lambda_t lambda, int argct, object_t* args)
{
	thunk_t thunk = alloc_object(&TYPE_THUNK, sizeof(*thunk));

	size_t sz = argct * sizeof(object_t);
	thunk->lambda = lambda;
	thunk->argct = argct;
	thunk->args = malloc(sz);
	memcpy(thunk->args, args, sz);
	decref_many(argct, args);

	return (object_t)thunk;
}

object_t eval_thunk(thunk_t thunk)
{
	return invoke_lambda(
		(object_t)thunk->lambda, thunk->argct, thunk->args);
}

thunk_t to_thunk(object_t obj)
{
	if (obj->type == &TYPE_THUNK)
		return (thunk_t)obj;
	return NULL;
}

void register_builtins(void);

void setup_runtime()
{
	init_array(&ALL_OBJECTS);
	init_array(&REACHABLE_OBJECTS);
	register_builtins();
	execute_file("stdlib.scm");
}

//

void teardown_runtime()
{
	decref((object_t)get_repl_scope());
	decref((object_t)get_symbol_pool());
	collect_garbage();
	dispose_array(&ALL_OBJECTS);
	dispose_array(&REACHABLE_OBJECTS);
}

object_t pop_from_list_or_die(object_t* ptr)
{
	object_t result = pop_from_list(ptr);
	if (! result)
		DIE("Premature end of list");
	return result;
}

// CUTOFF

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

	for (int i = argct - 1; i >= 0; i--)
		push_to_list(&result, args[i]);

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

	while ((item = pop_from_list(&seq))) {
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
	for (int i = 0; i < argct; i++)
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
	pair_t pair = assert_pair(args[0], "as argument #1 of set-cdr!");
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

object_t native_reverse(int argct, object_t* args) // reverse-native
{
	assert_arg_count("reverse-native", argct, 1);
	object_t result = reverse(args[0]);
	return result;
}

int unbox_int_or_die(const char* name, object_t arg)
{
	int result;
	if (unbox_int(&result, arg))
		return result;
	DIE("Expected argument for %s to be an integer, got %s instead",
	    name,
	    typename(arg));
}

struct character;

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
			return strcmp(unwrap_symbol(sx), unwrap_symbol(sy)) ==
			       0;

		char_t cx = to_char(x), cy = to_char(y);
		if (cx && cy)
			return unwrap_char(cx) == unwrap_char(cy);

		DIE("Don't know how to eq? %s against %s",
		    typename(x),
		    typename(y));
	}
}

object_t syntax_cond(scope_t scope, object_t code) // cond
{
	object_t clause, test;

	while ((clause = pop_from_list(&code))) {
		test = pop_from_list_or_die(&clause);

		bool flag;
		if (is_symbol("else", test)) {
			flag = true;
		} else {
			object_t result = eval_eager(scope, test);
			flag = is_true(result);
			decref(result);
		}

		if (flag)
			return eval_block(scope, clause);
	}
	return wrap_nil();
}

object_t syntax_letrec(scope_t outer_scope, object_t code) // letrec
{
	scope_t scope = derive_scope(outer_scope);

	object_t bindings = pop_from_list_or_die(&code);
	object_t binding;

	while ((binding = pop_from_list(&bindings))) {
		object_t keyobj = pop_from_list_or_die(&binding);
		object_t expr = pop_from_list_or_die(&binding);
		symbol_t key =
			assert_symbol(keyobj, "as `letrec` binding name");
		object_t value = eval_eager(scope, expr);
		define(scope, key, value);
		decref(value);
	}

	object_t result = eval_block(scope, code);

	decref((object_t)scope);
	return result;
}

object_t syntax_let(scope_t outer_scope, object_t code) // let
{
	scope_t scope = derive_scope(outer_scope);

	object_t bindings = pop_from_list_or_die(&code);
	object_t binding;

	while ((binding = pop_from_list(&bindings))) {
		object_t keyobj = pop_from_list_or_die(&binding);
		symbol_t key = assert_symbol(keyobj, "as `let` binding name");
		object_t expr = pop_from_list_or_die(&binding);
		object_t value = eval_eager(outer_scope, expr);
		define(scope, key, value);
		decref(value);
	}

	object_t result = eval_block(scope, code);

	decref((object_t)scope);
	return result;
}

object_t syntax_lambda(scope_t scope, object_t code) // lambda
{
	pair_t pair = assert_pair(code, "body of lambda");
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

object_t syntax_letseq(scope_t scope, object_t code) // let*
{
	object_t bindings = pop_from_list_or_die(&code);

	object_t binding;

	incref((object_t)scope);

	while ((binding = pop_from_list(&bindings))) {
		object_t key = pop_from_list_or_die(&binding);
		symbol_t keysym = to_symbol(key);
		if (! keysym)
			DIE("Expected letrec binding name to be a symbol, got "
			    "%s instead",
			    typename(key));
		object_t expr = pop_from_list_or_die(&binding);
		scope_t new_scope = derive_scope(scope);

		object_t value = eval_eager(scope, expr);
		define(new_scope, keysym, value);
		decref(value);

		decref((object_t)scope);
		scope = new_scope;
	}

	object_t result = eval_block(scope, code);

	decref((object_t)scope);
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
		DIE("Expected argument of %s to be a str, got %s instead",
		    name,
		    typename(obj));
	return str;
}

const char* unwrap_string(string_t str)
{
	return str->value;
}

object_t native_open_input_file(int argct, object_t* args) // open-input-file
{
	assert_arg_count("open-input-file", argct, 1);
	string_t name = to_string_or_die("open-input-file", args[0]);
	FILE* f = fopen_or_die(unwrap_string(name), "r");
	return wrap_file(f);
}

object_t native_read_char(int argct, object_t* args) // read-char
{
	assert_arg_count("read-char", argct, 1);
	port_t port = assert_port(args[0], "as an argument #1 of read-char");
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
	object_t value = wrap_bool(false);

	while ((expr = pop_from_list(&code))) {
		decref(value);
		value = eval_eager(scope, expr);
		if (is_true(value))
			break;
	}
	return value;
}

object_t syntax_and(scope_t scope, object_t code) // and
{
	object_t expr;
	object_t value = wrap_bool(true);

	while ((expr = pop_from_list(&code))) {
		decref(value);
		value = eval_eager(scope, expr);
		if (is_false(value))
			break;
	}

	return value;
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
	if (! ch) {
		DIE("Expected argument of %s to be a character, got %s instead",
		    name,
		    typename(obj));
	}
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

	while ((obj = pop_from_list(&list))) {
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
		push_to_list(&acc, ch);
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
	while (scope) {
		object_t old_value = lookup_in_dict(&scope->binds, key);
		if (old_value) {
			put_in_dict(&scope->binds, key, value);
			if (! scope->parent) {
				incref(value);
				decref(old_value);
			}
			return;
		}
		scope = scope->parent;
	}
	DIE("Variable %s is not bound to anything", key->value);
}

object_t syntax_set(scope_t scope, object_t code) // set!
{
	object_t head = pop_from_list_or_die(&code);
	symbol_t key = assert_symbol(head, "as a variable name in set!");
	object_t expr = pop_from_list_or_die(&code);
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
	for (int i = 0; i < argct; i++)
		display_object(stdout, args[i]);
	return wrap_nil();
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
	for (int i = 0; i < argct; i++) {
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

	for (int i = start; i < end; i++) {
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

	while ((obj = pop_from_list(&list)))
		push_to_list(&result, obj);

	return result;
}

//

void decref_many(int argct, object_t* args);

//

void array_decref(array_t arr)
{
	decref_many(arr->size, (object_t*)arr->data);
}

native_t to_native(object_t obj)
{
	if (obj->type == &TYPE_NATIVE)
		return (native_t)obj;
	return NULL;
}

syntax_t to_syntax(object_t obj)
{
	if (obj->type == &TYPE_SYNTAX)
		return (syntax_t)obj;
	return NULL;
}

object_t syntax_identity(scope_t scope, object_t code) // $
{
	return eval_lazy(scope, code);
}

// CODE BELOW IS AUTOGENERATED
void register_builtins(void)
{
	register_syntax("define", syntax_define);
	register_syntax("let", syntax_let);
	register_syntax("quote", syntax_quote);
	register_syntax("or", syntax_or);
	register_syntax("set!", syntax_set);
	register_syntax("and", syntax_and);
	register_syntax("let*", syntax_letseq);
	register_syntax("letrec", syntax_letrec);
	register_syntax("cond", syntax_cond);
	register_syntax("lambda", syntax_lambda);
	register_syntax("$", syntax_identity);
	register_syntax("if", syntax_if);
	register_native("list->string", native_list_to_string);
	register_native("symbol?", native_symbolp);
	register_native("display", native_display);
	register_native("pair?", native_pairp);
	register_native("substring", native_substring);
	register_native("string-copy", native_string_copy);
	register_native("string-ref", native_string_ref);
	register_native("string->list", native_string_to_list);
	register_native("string-set!", native_string_set);
	register_native("*", native_num_mult);
	register_native("modulo", native_modulo);
	register_native("+", native_num_plus);
	register_native("car", native_car);
	register_native("open-input-file", native_open_input_file);
	register_native("eq?", native_eqp);
	register_native("-", native_num_minus);
	register_native("newline", native_newline);
	register_native("read-char", native_read_char);
	register_native("write", native_write);
	register_native("cdr", native_cdr);
	register_native("reverse-native", native_reverse);
	register_native("set-cdr!", native_setcdr);
	register_native("<", native_num_less);
	register_native("/", native_num_divide);
	register_native("list", native_list);
	register_native("=", native_num_equals);
	register_native("not", native_not);
	register_native("string-length", native_string_length);
	register_native("cons", native_cons);
	register_native("null?", native_nullp);
	register_native("string-append", native_string_append);
	register_native("string=?", native_string_equal);
	register_native("fold", native_fold);
}
