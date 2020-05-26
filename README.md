## Chapter 1 where the story begins

Every story has to begin somewhere. This one is a tour through a
simplistic Scheme interpreter written in C, and every C program begins
with the `main()` function.

``` c

void setup_runtime(void);
void do_useful_stuff(int argc, const char** argv);
void teardown_runtime(void);

int main(int argc, const char** argv)
{
	setup_runtime();
	do_useful_stuff(argc, argv);
	teardown_runtime();
}

```

Here I can imagine people asking "Wait a minute, is this some kind of
pseudo C? Ain't you supposed to have `#include <stdio.h>` at the
beginning and `main()` function at the very end?"

Well. There is a whole universe of possible implications behind
"Ain't you supposed to do X?" questions, and I'll use them as rant fuel
throughout this story. But considering the most straightforward one,
which is "is C compiler okay with such way of structuring code?" the
answer is yes, it compiles just fine. It will keep failing to *link*
until those functions will be implemented, but as far as compiler is
concerned, forward declarations are good enough.

Moreover, this is the general pattern in this story where I follow the
example of  Quintus Fabius Maximus and keep postponing writing
lower-level implementation of a feature until I get a handle on
how it's going to be used.

This approach is called top-down, and it's not the only way to write
a program. A program can also be written bottom-up, which is to start
with individual nuts and bolts, and then to assemble them into a big
piece of software.

There's a potential problem with the latter though.

Let's say, I start with writing a piece of standard library. I'm
certainly going to need it at some point, so it isn't an obviously
wrong place to start. Oh, and I'll also need a garbage collector, so
I can write one too.

But then I'm running a risk of ending up with an implementation of a
chunk of Scheme standard library that is neat, and cute, and pretty,
and a garbage collector that is as terrific as a garbage collector in
a pet project may possibly be, and **they don't quite fit!**

And then I'll have a dilemma. Either I'll have to redo one or both of
them. Which probably won't be 100% waste, as I hope to have learned
a few things on the way, but it's still double work. Or else I can refuse
to accept sunk costs and then stubbornly work around the
incompatibilities between my own standard library implementation and
my own garbage collector, and that's just dumb.

But as long as something *isn't done at all*, I can be totally sure
that it *isn't done wrong*. There's a certain Zen feeling to it.

Another thing is more subtle, but will get a lot of programmers,
especially more junior ones, nervous once they figure it out. It's
`setup_runtime()` call. It's pretty clear **what** it will do which
is initialize garbage collector and such, but it also implies we're
going to have **the** runtime, probably scattered around in a bunch of
global variables.

I can almost here voices asking "But what if you need to have multiple
runtimes? What if customer comes and asks to make your interpreter
embeddable as a scripting engine? What about multithreading? Why are
you not worried?!"

The answer is "I consciously don't care." This is just a pet project
that started with me willing to tinker with Scheme, and then realizing
that just writing in Scheme is too easy, so I wrote my own interpreter,
and then realizing that **even that is not fun enough**, so I wrapped it
into a sort of "literary programming" exercise. In a (highly unlikely
situation) when I'll have to write my own multithreaded embeddable Scheme
interpreter, I'll just start from scratch, and that's about it.

Anyway, I'll write functions to setup and teardown runtime once I
get a better idea of how said runtime should look like, and for now
I'll focus on doing the useful stuff.

``` c

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

```

This one is pretty straightforward: when program is launched as
`./scheme foo.scm` then execute a file, when it's launced as
`cat foo.scm | ./scheme` do exactly the same, and otherwise fire
up a REPL.

Now that I know that I'm going to have a function that reads code from
a stream and executes it, writing a function that does the same with a
file is trivial, so let's just make one.

``` c

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

```

Some might find error handling in `fopen_or_die()` a bit naive. If you
aren't among those, you can skip the following rant, and if you are,
it's okay, it's nothing to be ashamed of, it's just normal cognitive
inertia.

See, in general, when something goes wrong you have three options:

1. You can handle the problem and continue.

2. You can abort.

3. You can notify the invoker about the problem and let them make their
own choice from these same three options.

In this case option #1 is unavailable. Because, well, failing to open a
file that interpreter is told to execute is clearly a fatal error, and
there's no sane way to recover from it.

Oh, of course there are insane ways to do it, for instance I can just
quietly skip the problematic file, and later collapse in an obscure way
because I can't find functions that were supposed to be defined in that
file, or take your guess, but I'm not even going to spend time
explaining why I'm not doing that.

Option #3 is interesting because this is what many programmers would
consider a natural and only alternative for the case when #1 is not
doable. In fact, if you're coding on top of a rich fat framework (think
Spring or Django) this indeed is the natural and only way to do it. But
in this case there's no framework and operating system is effectively
the invoker ("*there was nothing between us, not even a condom*"...
yeah, really bad joke), and `abort()`ing is a proper way to notify the
operating system about the problem, so #2 is pretty much #3, just
with less boilerplate code.

Anyway, let's implement `execute()`

``` c

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

```

Here we introduce several things.

First one is `struct object` which is going to be the representation
of a Scheme object. It's quite certainly going to be some sort of a
`struct`; internal details of that struct we'll figure out later.

Second and third are `read_object()` and `eval_repl()` functions that,
respectively, read an object from an input stream and evaluate it
in REPL context.

Last one is `decref()` function that is needed because we're going to
have a tracing garbage collector. And tracing garbage collector needs
a reliable way to figure out which objects are still needed, or else it
will do funny things like deallocating variables at random yet always
inconvenient times. And then you either have to implement traversal of
a section of process' memory to gather that knowledge, or else you have
to workaround not doing it.Well, actually, it's a choice you'll face
multiple times and you're absolutely not not obliged to always take the
same path.

In this case doing it "properly" would involve analyzing C call stack
which is pretty cumbersome, so the plan is to get away with a simple
reference counting.

Now comes the REPL...

``` c

void print_object(FILE*, object_t obj);

void repl()
{
	object_t expr;

	printf("> ");
	while ((expr = read_object(stdin))) {
		object_t result = eval_repl(expr);
		decref(expr);

		print_object(stdout, result);
		decref(result);

		printf("\n> ");
		fflush(stdout);
	}
	printf("bye\n");
}

```

...which isn't particularly interesting, and we proceed to
## Chapter 2 where we parse Lisp code

What's neat about implementing a Lisp dialect is that you can be done
with parsing in about three pints of Guinness, and then move on to
funnier stuff.

Of course "funnier" is relative here, and not just grammatically,
but also in a Theory of Relativity kind of sense. I mean, Theory of
Relativity describes extreme conditions where gravity is so high that
common Newtonian laws don't work any more.

Likewise, here we're venturing deep into dark swampy forests of
Nerdyland where common understanding of "fun" doesn't apply. By
the standards of normal folks whose idea of having fun implies
things like mountain skiing and dance festivals, spending evenings
tinkering with implementation of infinite recursion is hopelessly
weird either way. So I mean absolutely no judgement towards those
amazing guys and gals who enjoy messing with lexers, parsers, and
all that shebang. Whatever floats your boat, really!

This had to be said. Anyway, back to parsing.

``` c

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

```

Lisp syntax is famously Spartan. Basically all you get is:
* lists (those thingies with ((astonishingly) copious) amount of
parentheses),
* strings (delimited by "double quotes" or however you call that character),
* quotations (if you don't know who these are, you better look it up in
Scheme spec, but basically it's a way to specify that ```'(+ 1 2)``` is
*literally* a list with three elements and not an expression that adds
two numbers),
* and atoms, which are pretty much everything else including numbers
and symbols.

So what we're doing here is we're looking at the first non-trivial
character in the input stream and if it's an opening parenthesis we
interpret it a beginning of a list etc.

``` c

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

```

Oh, and "the first non-trivial character" means we fast-forward through
the input stream ignoring comments and whitespace until we either
encounter a character that's neither or reach an EOF.

There are four `read_something()` functions that we promised to
implement, let's start with `read_string()`.

``` c

object_t wrap_string(const char*);

int fgetc_read_string(FILE* in)
{
	int ch = fgetc_or_die(in);

	switch (ch) {
	case EOF:
		DIE("Premature end of input");
	case '"':
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

```

Nothing surprising here, just read characters into a buffer until you
reach the closing double quote, then wrap the contents of the buffer
into an `object_t` and call it a day.

Yes, this simplistic implementation will miserably fail to parse a
source file with a string constant that is longer than 10K characters.

If you think about it, hard-coded 10K bytes for buffer size is kind of
interesting here. It's an arbitrary number that on one hand is safely
above any practical limit in terms of usefulness. I mean, of course
you can hard-code entire "Crime and Punishment" as a single string
constant just to humiliate a dimwit interpreter author. But within
any remotely sane coding style such blob must be offloaded to an
external text file, and even an order of magnitude less should be good
enough for all reasonable intents and purposes.

On the other hand it's also safely below any practical limit in terms
of conserving memory. It can easily be an order of magnitude bigger
without causing any issues whatsoever.

At least on a modern general-purpose machine with a couple of gigs
of memory. If you've got a PDP-7 like one that Ken Thompson used for
his early development of Unix then a hundred kilobytes might be your
**entire** RAM and then you have to be more thoughtful with your
throwaway buffers.

``` c

object_t parse_atom(const char*);

bool isatomic(char ch)
{
	if (isspace(ch))
		return false;
	switch (ch) {
	case '(':
	case ')':
	case ';':
	case '"':
		return false;
	}
	return true;
}

int fgetc_read_atom(FILE* in)
{
	int ch = fgetc_or_die(in);
	if (ch == EOF)
		return EOF;
	if (isatomic(ch))
		return ch;
	ungetc(ch, in);
	return EOF;
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
	buffer[fill] = '\0';
	return parse_atom(buffer);
}

```

This one is pretty much

I'm looking at another buffer and
You know what boggles my mind even more?

Remember, in the very beginning of
``` c

object_t wrap_bool(bool v);
object_t wrap_int(int value);
object_t wrap_symbol(const char*);

object_t parse_bool(const char* text)
{
	if (strcmp(text, "#f") == 0)
		return wrap_bool(false);
	if (strcmp(text, "#t") == 0)
		return wrap_bool(true);
	return NULL;
}

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
	if (digits == 0)
		return NULL;
	return wrap_int(sign * accum);
}

object_t parse_atom(const char* buffer)
{
	object_t result;
	if ((result = parse_bool(buffer)))
		return result;
	if ((result = parse_int(buffer)))
		return result;
	return wrap_symbol(buffer);
}

```

``` c

void push_list(object_t* ptr, object_t item);
object_t reverse(object_t list);
object_t wrap_nil(void);

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

object_t read_list(FILE* in)
{
	object_t accum = wrap_nil(), obj;

	while ((obj = read_next_object(in))) {
		push_list(&accum, obj);
		decref(obj);
	}

	object_t result = reverse(accum);
	decref(accum);
	return result;
}

```

``` c

object_t cons(object_t, object_t);

void push_list(object_t* ptr, object_t head)
{
	object_t tail = *ptr;
	*ptr = cons(head, tail);
	decref(tail);
}

```

And while we're at it, let's also implement `reverse()`

``` c

object_t pop_list(object_t*);

object_t reverse(object_t list)
{
	object_t result = wrap_nil(), obj;

	while ((obj = pop_list(&list)))
		push_list(&result, obj);

	return result;
}

```

which simply pops things from one list and pushes them to another.

``` c

struct pair;
typedef struct pair* pair_t;

object_t car(pair_t);
object_t cdr(pair_t);
bool is_nil(object_t);
const char* typename(object_t);
pair_t to_pair(object_t);

object_t pop_list(object_t* ptr)
{
	object_t obj = *ptr;
	if (is_nil(obj))
		return NULL;

	pair_t pair = to_pair(obj);
	if (! pair)
		DIE("Expected a pair when traversing a list, got %s instead", typename(obj));

	*ptr = cdr(pair);
	return car(pair);
}

```



``` c

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

```

and now we're finally done with parsing and can move on to
## Chapter 3 where we evaluate
