# Geeky Fox Is Scheming
[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)
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
is initialize garbage collector and such, but it also implies I'm
going to have **the** runtime, probably scattered around in a bunch of
global variables.

I can almost hear voices asking "But what if you need to have multiple
runtimes? What if customer comes and asks to make your interpreter
embeddable as a scripting engine? What about multithreading? Why are
you not worried?!"

The answer is "I consciously don't care." This is just a pet project
that started with me willing to tinker with Scheme, and then realizing
that just writing in Scheme is too easy, so I wrote my own interpreter,
and then realizing that **even that is not fun enough**, so I wrapped it
into a sort of "literary programming" exercise. In a (highly unlikely)
situation when I'll have to write my own multithreaded embeddable Scheme
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
in this case there's no framework, and operating system is effectively
**the** invoker ("*there was nothing between us, not even a condom...*"
yeah, really bad joke), and `abort()`ing is a proper way to notify the
operating system about the problem, so #2 is pretty much #3, just
without boilerplate code to pull error status to `main()` and then abort
there.

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

Several things are introduced here.

First one is `struct object` which is going to be the representation
of a Scheme object. It's quite certainly going to be some sort of a
`struct`; internal details of that struct I'll figure out later.

Second and third are `read_object()` and `eval_repl()` functions that,
respectively, read an object from an input stream and evaluate it
in REPL context.

Last one is `decref()` function that is needed because I'm going to
have automatic memory management. For this I have three options:

1. I can do reference counting. Very simple to do for as long as objects
don't form reference cycles, then it gets quirky.

2. I can make a tracing garbage collector which traverses process'
memory to figure out which objects are still needed.

3. I can apply a sort of a hybrid approach where I do tracing for the
sections of process' memory which are convenient to traverse and
fallback to reference counting for those which aren't.

From this simple function it's already clear that whichever method I
choose must be able to deal with references from C call stack, and
analyzing it in a pure tracing manner is pretty cumbersome, so
I have to count them anyway, and that's what `decref()` will do.

Now comes the REPL...

``` c

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

```

...which isn't particularly interesting, and we proceed to
## Chapter 2 where I parse Lisp code

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
* and atoms, which are pretty much everything else including numbers,
characters, and symbols.

So what I'm doing here is I'm looking at the first non-trivial
character in the input stream and if it's an opening parenthesis I
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

Oh, and "the first non-trivial character" means I fast-forward through
the input stream ignoring comments and whitespace until I encounter
a character that's neither, or reach an EOF.

There are four `read_something()` functions that I promised to
implement, let's start with `read_string()`

``` c

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

```

Nothing particularly surprising here, just read characters into the
buffer until you reach the closing double quote, then wrap the
contents of the buffer into an `object_t` and call it a day.

Yes, this simplistic implementation will miserably fail to parse a
source file with a string constant that is longer than 10K characters.

And if you take some time to think about it, hard-coded 10K bytes for
buffer size is kind of interesting here. It's an arbitrary number that
on one hand is safely above any practical limit in terms of usefulness.
I mean, of course you can hard-code entire "Crime and Punishment" as
a single string constant just to humiliate a dimwit interpreter author.
But within any remotely sane coding style such blob must be offloaded
to an external text file, and even a buffer that is an order of magnitude
smaller should still be good enough for all reasonable intents and
purposes.

On the other hand it's also safely below any practical limit in terms
of conserving memory. It can easily be an order of magnitude bigger
without causing any issues whatsoever.

At least on a modern general-purpose machine with a couple of gigs
of memory. If you've got a PDP-7 like one that Ken Thompson used for
his early development of Unix then a hundred kilobytes might be your
**entire** RAM and then you have to be more thoughtful with your
throwaway buffers.

By the way, it's really impressive how those people in 1960s could
develop an entire real operating system on a computer like that. Well,
not exactly mind-boggling, like, I myself started coding on a
Soviet-made ZX Spectrum clone with 48 kilobytes of RAM, and you
can't write a super-dooper-sophisticated OS on such machine because
**it just won't fit**, but still, it's so cool.

Okay, back to business. Let's `parse_atom()`.

``` c

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

```

Here I use the same approach as in `read_string()`: collect characters
for as long as it looks like an atom, then convert it to an `object_t` and
that's pretty much it.

Well, syntax for characters in Scheme is a bit wonky, you have `#!\x` for
the letter 'x' and `#\!` for an exclamation mark, and, surprisingly,
`#!\newline` and `#!\space` for a newline and space respectively.
Oh, and `#\` is also a space. Go figure.

Most of that wonkiness can be dealt with by simply reading everything
up until a special character and then figuring out what I've got in
`parse_atom()`. Unless **it is** a special character, e.g. `#\)` or `#\;`,
those need a bit of special handling.

And now I'm looking at another buffer and do you know what actually
boggles my mind?

Remember, in the very beginning of this story I mentioned that a C
program is normally supposed to have its `main()` function at the
end? So, what boggles my mind is why are we still doing it?

Well, I don't mean we all do. In some programming languages it is more
common, in some it is less, but really, why would you do it in any
language? It's such a weird way to layout your code where you have to
scroll all the way down to the bottom of the source file and then
work your way up in a Benjamin Button kind of way.

I mean, I know it's the legacy of Pascal where you were required to
have the equivalent of `main()` at the bottom (and finish it with an
`end.` with a period instead of a semicolon). I also understand that
back in those days it made sense in order to simplify the compiler that
had to run on limited hardware. But why we still sometimes do it in
2020 is a mystery to me.

Okay, enough of ranting, let's `parse_atom()`

``` c

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

```

Pretty straightforward really. If it looks a boolean, convert to a
boolean. If it looks like an integer number, convert to an integer.
If it looks like a symbol, convert to a symbol. If it looks like a
floating-point number... Convert to a symbol because screw you!

To give a bit of a background here, this pet project of mine was
never intended to be a feature-complete standards-compliant Scheme
implementation. It started with solving some of "99 Lisp problems"
and then escalated into writing a sufficient Scheme interpreter to
run those. None of those problems relied on floating-point arithmetics,
and so I didn't implement it.

Not that it's particularly hard, just tedious (if done Python-style with
proper type coercions), and JavaScript solution with simply using floats
for everything I find aesthetically unappealing.

What I couldn't possibly skip is lists (they didn't call it LISt
Processing for nothing), so let's `read_list()`

Oh, and a quick remark on naming convention. Functions like
`wrap_symbol()` are named this way intentionally. They could easily
be called, say, `make_symbol()`, but that would imply that it's some
sort of a constructor that really **makes** a *new* object. But by the
time I get to actually implement those I might not want to be bound
by this implication (because I might find out that a proper constructor
doesn't conform to the language standard and/or isn't practical, and I
need and/or want some sort of a cache or a pool or whatever).

So instead it's a vague "wrap" which stands for "get me an objects that
represent this value, and how you make it under the hood is none of
my business."

``` c

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

```

This is one is simple but might need a bit of refresher on how
lists work in Lisp (and other FP(-esque) languages).

So, your basic building block is a two-element tuple (also known as
a pair). If you put some value to the first cell of that tuple, and to the
second cell you put a reference to another tuple with another value in
the first cell et cetera et cetera, and then to the second cell of the
last tuple you instead put a special null value, then you what get is a
singly-linked list. Oh, and representation of the empty list is
simply the null value.

So what I do here is I read objects from input stream and push them
one by one to the front of the list until I see a closing parenthesis.
But then the list ends up reversed, so I need to reverse it back. Easy.

``` c

object_t wrap_pair(object_t, object_t);

void push_list(object_t* ptr, object_t head)
{
	object_t tail = *ptr;
	*ptr = wrap_pair(head, tail);
	decref(tail);
}

```

This is such a pretty little function that utilizes call by pointer
(very much a C idiom) to construct a very Lispy list. Tell me about
multiparadigm programming.

Oh, and while we're at it, let's also implement `reverse_read_list()`

``` c

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

```

which simply pops things from one list and pushes them to another.

Well, except for one gotcha: `.` has special meaning in list notation,
so that `'(a  . b)` is not a list but is equivalent to `(cons 'a 'b)`,
and so I cater for it here.

``` c

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

```

`pop_object()` is pretty much the opposite of `push_object()` with
a bit of typechecking to make sure I'm dealing with a list and not
something dodgy.

``` c

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

```

I still haven't decided what exactly am I going to put into either
`struct object` or `struct pair`, but I already need to be able to
convert one to another. So, I promise to write a `to_pair()`
function that would do just that (or return `NULL` if value that this
object holds is not a pair), and here's write a nice little helper around
it to abort with a human-readable message when conversion fails.

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

Since `'bla` is merely a shorter version of `(quote bla)` parsing it
is trivial, and with that in place we're finally done with parsing
and can move on to
## Chapter 3 where I evaluate
By the way, I don't know if you noticed or not, but I try to use the
word "we" as sparingly as I can. Perhaps it has something to do with me
coming from a culture where "We" is commonly associated with the
dystopian novel by Yevgeny Zamyatin.

Of course there are legit usages for "we," such as academic writing
where all of "us" are listed on the front page of the paper, and the
reader is much more interested in overall results than in the internal
dynamics of the research team.

But using "we did it" when it's actually "I did it" (and it's
stylistically appropriate to say "I did it") feels to me like speaker
is a wimp who wants to avoid the responsibility.

Likewise, using "we" when it's actually "I and Joe, mostly Joe" feels
like reluctanse to give a fair share of credit.

Okay, enough of that, let's implement `eval_repl()`

``` c

struct scope;
typedef struct scope* scope_t;

object_t eval_eager(scope_t scope, object_t expr);
scope_t get_repl_scope(void);

object_t eval_repl(object_t expr)
{
	return eval_eager(get_repl_scope(), expr);
}

```

That's a one-liner function that relies on two important concepts.

First one is the scope. Scope is pretty much just a binding between
variables' names and their values. For now just think of it as a sort
of a dictionary (it's not exactly that, but we'll get there)

Another one is differentiation between eager and lazy evaluation.
Before I go into explaining *what exactly do I mean by eager and
lazy evaluation* in the context of this story, I first have to
elaborate the pragmatics for having all that at the first place.

So. Scheme is a functional programming language, and in functional
programming people don't do loops, but instead they do recursion. And
for infinite loops they do, well, infinite recursion. And "infinite"
here doesn't mean "very very big," but actually infinite.

Consider this example:

```scheme
(define (infinite-loop depth)
	(display (list depth 'bla))
	(display #\newline)
	(infinite-loop (+ 1 depth)))

(infinite-loop 0)

```

Obviously, a direct equivalent of this code in vanilla
C/C++/Ruby/Python/Java will run for some time, and eventually
blow up with a stack overflow. But code in Scheme, well, better
shouldn't.

I have three ways to deal with it:

1. Just do nothing and hope that C stack will not overflow.

2. Do code rewriting so that under the hood the snippet above is
automagically converted into a loop, e.g.

```scheme
(define (infinite-loop depth)
	(loop
		(display depth)
		(display #\newline)
		(set! depth (+ 1 depth))))

```

3. Apply the technique called trampolining. Semantically it means

```scheme
(define (infinite-loop depth)
	(display (list depth 'bla))
	(display #\newline)
	(invoke-later infinite-loop (+ 1 depth)))
```

that instead of calling itself, function... Well, to generalize and
to simplify let's say it informs the evaluator that computation is
incomplete, and also tells what to do next in order to complete it.

#1 looks like a joke, but actually it's a pretty good solution. It's
also a pretty bad solution, but let's get through the upsides first.

First of all, it's very easy to implement (because it doesn't require
writing any specific code), it clearly won't introduce any complicated
bugs (because there's no specific code!), and it won't have any
performance impact (because it does nothing!!)

You see, "*just do nothing*" half of it is all good, it's the "*and hope
that*" part that isn't. Although, for simple examples it doesn't
really matter: with a couple of thousands of stack levels it's gonna be
fine with or without optimizations. But a more complex program may
eventually hit that boundary, and then I'll have to get around
deficiencies of my code in, well, my other code, and that's not a
place I'd like to get myself into.

Which also sets a constraint on what a "proper" solution should be:
it must be provably reliable for a complex piece of code, or else it's
back to square one.

#2 looks like a superfun thing to play with, and for toy snippets it
seems deceptively simple. But thinking just a little bit about
pesky stuff like mutually recursive functions, and
self-modifying-ish code (think `(set! infinite-loop something-else)`
*from within* `infinite-loop`), and escape procedures, and all this
starts to feel like a breeding ground for wacky corner cases, and
I don't want to commit to being able to weed them all out.

#3 on the contrary is very simple, both conceptually and
implementation-wise, so that's what I'll do (although I might
do #2 on top of it later; because it looks like a superfun thing to
play with)

Now let's get back to lazy vs eager. "Lazy" in this context means
that evaluation function may return either a result (if computation
is finished) or a thunk (a special object that describes what to do
next). Whereas "eager" means that evaluation function will always
return the final result.

``` c

object_t eval_lazy(scope_t scope, object_t expr);
object_t force(object_t value);

object_t eval_eager(scope_t scope, object_t expr)
{
	object_t result_or_thunk = eval_lazy(scope, expr);
	return force(result_or_thunk);
}

```

"Eager" evaluation can be easily arranged by getting a "lazy" result
first...

``` c

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

```

...and then reevaluating it until the computation is complete.

You know, I've just realized it's the third time in this story when I
say "I have three ways to deal with it," previous two being
considerations about memory management and error handling in
Chapter 1.

Moreover, I noticed a pattern. In a generalized form, those three
options to choose from are:

1. Consider yourself lucky. Assume things won't go wrong. Don't
worry about your solution being too optimistic.

2. Consider yourself smart. Assume you'll be able to fix every bug.
Don't worry about your solution being too complex.

3. Just bloody admit that you're a dumb loser. Design a balanced
solution that is resilient while still reasonable.

This is such a deep topic that I'm not even going to try to cover it
in one take, but I'm damn sure I'll be getting back to it repeatedly.

For now, let's continue with `eval_lazy()`

``` c

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

```

This is fairly straightforward: if it's a symbol, treat it as a name
of the variable, if it's a list, treat it as a symbolic expression, and
otherwise just evaluate it to itself (so that `(eval "bla")` is simply
`"bla"`)

``` c

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

```

Evaluating a variable is pretty much just look it up in the current
scope and `DIE()` if it's not there.

``` c

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

```

And evaluating an expression is... Well, if you ever wondered why
the hell they use so many of those bloody parentheses in Lisps,
here's your answer.

In most programming languages (mainstream ones anyway) syntax
constructs and functions are two fundamentally different kinds of
creatures. They don't just *behave* differently, but they also *look*
differently, and you can't mix them up.

Much less so in Lisp, where you have `(if foo bar)` for conditional,
and `(+ foo bar)` to add two numbers, and `(cons foo bar)` to make
a pair, and you can't help but notice they look pretty darn similar.

Moreover, even though they behave differently, it's not that *that
dissimilar* either. `+` and `cons` are simply functions that accept
*values* of `foo` and `bar`and do something with them. Whereas
`if` is also simply a function, except that instead of values of its'
arguments it accepts *a chunk of code verbatim.*

Let me reiterate: a syntax construct is merely a *data-manipulation
function* that happens to have *program's code as the data that it
manipulates.* Oh,  and *code as data* is not some runtime
introspection shamanistic voodoo, it's just normal lists and symbols
and what have you.

And all of that is facilitated by having the same notation of data
and code. That's why parentheses are so cool.

So, with explanation above in mind, pretty much all this function does
is: it evaluates the first item of the S-expression, and if it happens to
be an "I want code as is" function, then it's fed code as is, and
otherwise it is treated as an "I want values of the arguments" function
instead. That's it.

If my little story is your first encounter with Lisp, I can imagine
how mind-blowing can this be. Let it sink in, take your time.

``` c

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

```

This one is not very complicated: go through the list, evaluate
the stuff you have there, then either feed it to a function or make
a thunk to evaluate it later.

There are few minor funky optimizations to mention though.

1. I put arguments into a buffer and not to a list. I mean, lists are
cool for everything except two things. They're not as efficient for
"just give me an element at index X" random access, and they're
kinda clumsy when it comes to memory allocation. And these are
two things I really won't mind having for a function call: as much
as I don't care about performance, having to do three `malloc()`s
just to call a function of three arguments feels sorta wasteful.

2. I introduce `lambda_t` type for functions that are implemented
in Scheme and require tail call optimizations. This is done to
separate them from built-in functions that are implemented in
C and are supposed to be hand-optimized so that lazy call overhead
is unnecessary.

3. I cap the maximum number of arguments that a function can have
at 64. I even drafted a rant to rationalize that, but I'm running out of
Chardonnay, so let's park it for now and move on to
## Chapter 4 where I finally write some code in Scheme
