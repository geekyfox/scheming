## Chapter 4 where I finally write some code in Scheme

Somewhere around Chapter 2 I mentioned that this whole story began with the list of "99 Lisp problems."

So, let's start with the first task on the list, which is:

A> Find the last box of a list.
A>
A> Example:
A>
A> `* (my-last '(a b c d))`
A>
A> `(D)`

That's an easy puzzle, here's the solution:

```scheme
(define (my-last x)
	(if (null? (cdr x))
		x
		(my-last (cdr x))))

(write (my-last '(a b c d)))
(newline)

```

Lists of problems like that one, where you start with something very simple, and then work your way towards something more complex is an awesome way to learn a programming language. I mean, you start at level 1, and you figure out the least sufficient fraction of the language that enables you to write a simple yet complete and self-contained program. Once that knowledge sinks in, you move on to level 2, and learn a bit more, and in the end you have a pretty good command of the whole thing.

At least this is how it works for me. If there are anybody out there, who needs to memorize the whole language standard and pass theory exams before they can start writing any code, that's fine, I don't judge.

Oh, and of course it only applies when I'm in a relatively unfamiliar territory. If you ask me, after all those years I spent writing backends in Python and Java, to do some, I dunno, Node.js, I'd be like "Hmm, so you guys use curly braces here? That's neat." and then just pick a real problem straight away.

But also, and maybe even more so, I find it an awesome way to *implement* a language. So, instead of spending months and months to build a whole standards-compliant thing, just make something that enables that little self-contained program to run, and then iterate from there.

That said. To get that little function above running I'm gonna need two syntax constructs, namely `define` and `if`. And I'm also gonna need four standard library functions which are `null?`, `cdr`, `write` and `newline`.

Let's start with `null?`

``` c

void assert_arg_count(const char* name, int actual, int expected);

object_t native_nullp(int argct, object_t* args) // null?
{
	assert_arg_count("null?", argct, 1);
	return wrap_bool(is_nil(args[0]));
}

```

Functional part of this function is fairly trivial: I've defined `wrap_bool()` and `is_nil()` earlier, and now I just use them here.

Code that surrounds that logic needs some clarifications though.

One thing is passing arguments as an array and not... Well, it's a bloody good question what else can you do here.

See, in Scheme: * some functions accept a fixed number of arguments (e.g. `car` accepts exactly one and `cons` accepts exactly two) * some functions accept a variable number of arguments (like `write` that we'll get to in a bit which can have one or two) * and some functions accept an arbitrary number of arguments (like `list` that can have as many arguments as you give it)

Oh, and the host language is still good ol' C, so you can have as much run-time metadata attached to a function as you want. For as long as you can figure out a way to do it all yourself.

That's why the most sane interface I could come up with is to simply pass an array of arguments, and then let the function itself figure out if it's happy with what it got, or if it isn't. Which is exactly what I'm doing here.

The other thing that needs explanation is that little `// null?` comment. Why it is there is because I need this function to eventually end up in REPL scope, which means I'll have to have a registration function that will put it there. But I'm also not eager to maintain it manually, so I'm going to put together a little script that scans the source file and generates the code for me.

Well, and since Scheme has more valid characters for identifiers than C does, I can't simply name this function `native_null?`, hence I have to provide the Scheme name separately, and this is the purpose of that little comment.

Okay, now let's do `cdr`

``` c

object_t native_cdr(int argct, object_t* args) // cdr
{
	assert_arg_count("cdr", argct, 1);
	pair_t pair = assert_pair("as an argument #1 of cdr", args[0]);
	object_t result = cdr(pair);
	incref(result);
	return result;
}

```

With all gotchas already explained, this code is so straightforward that I don't want to talk about it.

Instead I want to talk about sports. My favourite kind of physical activity is long-distance running. *Really* long distance running. Which means, marathons.

Now, as we all know, some things are easy, and some are hard. Also, some things are simple, and some are complex. And also, there's a belief among certain people from Nerd and Nerdish tribes that simple things must be easy, and that hard things must be complex.

I admit I had few too many White Zinfandels on my way here, so you'll have to endure me rambling all over the place. That said. Nerd tribe is a well established notion, so I'm not gonna explain what it is. While Nerdish tribe is a certain type of people who were taught that being smart is kinda cool, but weren't taught how to be properly smart. So they just keep pounding you with their erudition and intellect, even if situation is such that a properly smart thing to do would be to just shut the fuck up.

Anyway. Marathons prove that belief to be wrong. Conceptually, running a marathon is very **simple**: you just put one foot in front of the other, and that's pretty much it. But sheer amount of how many times you should do that to complete a 42.2 kilometer distance is what makes a marathon pretty **hard**, and dealing with that hardness is what makes it more complex than it seems.

In fact, there's a whole bunch of stuff that come into play: physical training regimen, mental prep, choice of gear, diet, rhythm, pace, handling of weather conditions, even choice of music for the playlist.

In broad terms, options I choose fall into three categories:

1. Stuff that works **for me**. It has never been perfect, but if I generally get it right, then by 38th kilometer I'm enjoying myself, and singing along to whatever Russian punk rock I've got in my headphones, and looking forward to a medal and a beer.

2. Stuff that doesn't work for me. If there's too much of that, then by 38th kilometer I'm lying on the tarmac and hope that race organizers have a gun to end my suffering.

3. Stuff that I really really want to work because it looks so awesome in theory, **but it still doesn't**, so in the end it's still lying on the tarmac and praying for a quick death.

As a matter of fact, doing marathons really helped me to crystallize my views on engineering: if an idea looks really great on paper, but hasn't been tested in battle, then, well, you know, it looks really great. On paper.

Okay, while we're at it, let's also do `car`

``` c

object_t native_car(int argct, object_t* args) // car
{
	assert_arg_count("car", argct, 1);
	pair_t pair = assert_pair("as an argument #1 of car", args[0]);
	object_t result = car(pair);
	incref(result);
	return result;
}

```

This function is as straightforward as the previous one, so I don't want to talk about it either. Instead I want to talk about philosophy.

Quite a few times in this story I said "there are three ways to do it / three options to choose from / three categories it falls into." And it's not just my personal gimmick with number 3, but in fact it's Hegelian(-ish) dialectic.

Just to be clear, I'm not nearly as good at philosophy as I am at programming, and you can see how mediocre my code is. With that in mind, the best way I can explain it is this.

Imagine you've been told that the language you can use in an upcoming project can be either Java or Ancient Greek. And you're, like, wait a minute, but how can I choose between Java and Ancient Greek?

This is a perfectly valid question. But we're going to take one step deeper and look into why it's perfectly valid. And, well, it's because while both Java and Ancient Greek are languages, there's otherwise not much common between them, and that's why there's no readily available frame of reference in which they can be compared.

And what it means is that you can only compare things that are sufficiently similar or else you simply can't define criteria for such comparison.

This idea sounds obscenely trivial (like most philosophy does after stripping out all the smartass fluff), but it's immensly powerful.

One way to practically apply this idea is: when choosing between multiple options don't jump immediately into the differences between them, but instead start with challenging the commonalities.

Say you're deciding whether to build an application with Django or with Rails. Both are rich web frameworks that utilize dynamically-typed languages. Which means you implicitly excluded:

* statically typed languages (Java, Go, Kotlin, ...)

* micro-frameworks (Flask, Express, ...)

* non-web applications (desktop, mobile, CLI, ...)

* using an off-the-shelf solution instead of building your own

Do you agree with all these choices? If yes, great, go ahead with your Django versus Rails analysis. If no, well, you might have just saved yourself from putting a lot of effort into a wrong thing.

More advanced method is this:

1. Start with some idea.

2. Design an alternative idea that is different from the first one in every way you could think of.

3. Look at two options at hand and find the similarities between them **that you didn't even think about,** those are your blind spots.

4. Come up with a synthetic idea that takes your newly found blind spots into account.

5. Repeat until you run out of blind spots and cover entire decision space for the problem you're solving.

Just try it out and you'll see how powerful these techniques are.

Okay, enough philosophy, let's get back to coding and do some I/O.

``` c

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

```

Nothing particularly spectacular here. "Port" is how they call a stream (or a file handler or what have you) in Scheme. Oh, and also it can accept one or two arguments. Aside from that it's all straightforward.

``` c

object_t native_newline(int argct, object_t* args) // newline
{
	assert_vararg_count("newline", argct, 0, 1);
	FILE* out = (argct == 0)
		? stdout
		: unwrap_port(assert_port("as an argument #2 of newline", args[0]));
	fputs("\n", out);
	return wrap_nil();
}

```

So is this one. Now I'll do something more complex and evaluate syntax for `if`

``` c

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

```

A tiny bit more complex really. See, when code you interpret is in a common convenient data structure, it really takes effort and determination to make things hard. And although I (obviously) have a rant on this topic, I think I put enough random ramblings into this chapter already, so I'm going to park it for now and instead evaluate `define`

``` c

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

```

This one is a little bit trickier, but that's just because Scheme has alternative form for `define` so that `(define (foo bar) bla bla)` is equivalent to `(define foo (lambda (bar) bla bla))`, and I have to handle both cases.

Now, I promised to make a handful of simple helpers, so I'm just going to do it to get them out of the way.

``` c

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

```

And this is a good place to wrap up this chapter and move on to