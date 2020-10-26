{sample:true}
## Chapter 3 where I evaluate

By the way, I don't know if you noticed or not, but I try to use the word "we" as sparingly as I can. Perhaps it has something to do with me coming from a culture where "We" is commonly associated with the dystopian novel by Yevgeny Zamyatin.

Of course there are legit usages for "we," such as academic writing where all of "us" are listed on the front page of the paper, and the reader is much more interested in overall results than in the internal dynamics of the research team.

But using "we did it" when it's actually "I did it" (and it's stylistically appropriate to say "I did it") feels to me like speaker is a wimp who wants to avoid the responsibility.

Likewise, using "we" when it's actually "I and Joe, mostly Joe" feels like reluctance to give a fair share of credit.

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

First one is the scope. Scope is pretty much just a binding between variables' names and their values. For now just think of it as a sort of a dictionary (it's not exactly that, but we'll get there when we'll get there).

Another one is differentiation between eager and lazy evaluation. Before I go into explaining *what exactly do I mean by eager and lazy evaluation* in the context of this story, I first have to elaborate the pragmatics for having all that at the first place.

So. Scheme is a functional programming language, and in functional programming people don't do loops, but instead they do recursion. And for infinite loops they do, well, infinite recursion. And "infinite" here doesn't mean "very very big," but actually infinite.

Consider this example:

```scheme
(define (infinite-loop depth)
	(display (list depth 'bla))
	(display #\newline)
	(infinite-loop (+ 1 depth)))

(infinite-loop 0)

```

Obviously, a direct equivalent of this code in vanilla C/C++/Ruby/Python/Java will run for some time, and eventually blow up with a stack overflow. But code in Scheme, well, better shouldn't.

I have three ways to deal with it:

1. Just do nothing and hope that C stack will not overflow.

2. Do code rewriting so that under the hood the snippet above is automagically converted into a loop, e.g.

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

that instead of calling itself, function... Well, to generalize and to simplify let's say it informs the evaluator that computation is incomplete, and also tells what to do next in order to complete it.

#1 looks like a joke, but actually it's a pretty good solution. It's also a pretty bad solution, but let's get through the upsides first.

First of all, it's very easy to implement (because it doesn't require writing any specific code), it clearly won't introduce any complicated bugs (because there's no specific code!), and it won't have any performance impact (because it does nothing!!)

You see, "*just do nothing*" half of it is all good, it's the "*and hope that*" part that isn't. Although, for simple examples it doesn't really matter: with a couple of thousands of stack levels it's gonna be fine with or without optimizations. But a more complex program may eventually hit that boundary, and then I'll have to get around deficiencies of my code in, well, my other code, and that's not a place I'd like to get myself into.

Which also sets a constraint on what a "proper" solution should be: it must be provably reliable for a complex piece of code, or else it's back to square one.

#2 looks like a superfun thing to play with, and for toy snippets it seems deceptively simple. But thinking just a little bit about pesky stuff like mutually recursive functions, and self-modifying-ish code (think `(set! infinite-loop something-else)` *from within* `infinite-loop`), and escape procedures, and all this starts to feel like a breeding ground for wacky corner cases, and I don't want to commit to being able to weed them all out.

#3 on the contrary is very simple, both conceptually and implementation-wise, so that's what I'll do (although I might do #2 on top of it later; because it looks like a superfun thing to play with).

Now let's get back to lazy vs eager. "Lazy" in this context means that evaluation function may return either a result (if computation is finished) or a thunk (a special object that describes what to do next). Whereas "eager" means that evaluation function will always return the final result.

"Eager" evaluation can be easily arranged by getting a "lazy" result first...

``` c

object_t eval_lazy(scope_t scope, object_t expr);
object_t force(object_t value);

object_t eval_eager(scope_t scope, object_t expr)
{
	object_t result_or_thunk = eval_lazy(scope, expr);
	return force(result_or_thunk);
}

```

...and then reevaluating it until the computation is complete.

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

You know, I've just realized it's the third time in this story when I say "I have three ways to deal with it," previous two being considerations about memory management and error handling in Chapter 1.

Moreover, I noticed a pattern. In a generalized form, those three options to choose from are:

1. Consider yourself lucky. Assume things won't go wrong. Don't worry about your solution being too optimistic.

2. Consider yourself smart. Assume you'll be able to fix every bug. Don't worry about your solution being too complex.

3. Just bloody admit that you're a dumb loser. Design a balanced solution that is resilient while still reasonable.

This is such a deep topic that I'm not even going to try to cover it in one take, but I'm damn sure I'll be getting back to it repeatedly.

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

This is fairly straightforward: if it's a symbol, treat it as a name of the variable, if it's a list, treat it as a symbolic expression, and otherwise just evaluate it to itself (so that `(eval "bla")` is simply `"bla"`)

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

Evaluating a variable is pretty much just look it up in the current scope and `DIE()` if it's not there.

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

And evaluating an expression is... Well, if you ever wondered why the hell they use so many of those bloody parentheses in Lisps, here's your answer.

In most programming languages (mainstream ones anyway) syntax constructs and functions are two fundamentally different kinds of creatures. They don't just *behave* differently, but they also *look* differently, and you can't mix them up.

Much less so in Lisp, where you have `(if foo bar)` for conditional, and `(+ foo bar)` to add two numbers, and `(cons foo bar)` to make a pair, and you can't help but notice they look pretty darn similar.

Moreover, even though they behave differently, it's not that *that dissimilar* either. `+` and `cons` are simply functions that accept *values* of `foo` and `bar` and do something with them. Whereas `if` is also simply a function, except that instead of values of its' arguments it accepts *a chunk of code verbatim.*

Let me reiterate: a syntax construct is merely a *data manipulation function* that happens to have *program's code as the data that it manipulates.* Oh,  and *code as data* is not some runtime introspection shamanistic voodoo, it's just normal lists and symbols and what have you.

And all of that is enabled by using the same notation for data and for code. That's why parentheses are so cool.

So, with explanation above in mind, pretty much all this function does is: it evaluates the first item of the S-expression, and if it happens to be an "I want code as is" function, then it's fed code as is, and otherwise it is treated as an "I want values of the arguments" function instead. That's it.

If my little story is your first encounter with Lisp, I can imagine how mind-blowing can this be. Let it sink in, take your time.

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

This one is not very complicated: go through the list, evaluate the stuff you have there, then either feed it to a function or make a thunk to evaluate it later.

There are few minor funky optimizations to mention though.

1. I put arguments into a buffer and not to a list. I mean, lists are cool for everything except two things. They're not as efficient for "just give me an element at index X" random access, and they're kinda clumsy when it comes to memory allocation. And these are two things I really won't mind having for a function call: as much as I don't care about performance, having to do three `malloc()`s just to call a function of three arguments feels sorta wasteful.

2. I introduce `lambda_t` type for functions that are implemented in Scheme and require tail call optimizations. This is done to separate them from built-in functions that are implemented in C and are supposed to be hand-optimized so that lazy call overhead is avoidable and unnecessary.

3. I cap the maximum number of arguments that a function can have at 64. I even drafted a rant to rationalize that, but I'm running out of Chardonnay, so let's park it for now and move on to