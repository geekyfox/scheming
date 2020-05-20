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
