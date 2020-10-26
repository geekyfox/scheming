## Chapter 6, where things become objective

But before that they'll stay subjective for few more paragraphs.

One of the major influences on me and my work on this project is Dan Carlin of "Hardcore History" fame, and if you don't know the "Hardcore History" podcast, you should definitely check it out, it's awesome.

One particular thing I love about that podcast is how Dan tells his stories in such a way that they're incredibly informative, and I myself learned an awful lot about history by listening and re-listening to his podcasts over the past decade.

At the same time, they clearly won't qualify for "educational," and by "educational" I mean certain type of teacher-student dynamics, one that is built around student having specific pragmatic reasons to learn the topic. So that you learn X because you want to find a job, or to pass an exam, or to get a promotion, or to earn money, or anything like that. And then it's teacher's responsibility to maximize the utility of the course, even if it comes at a cost of leaving out things that are exciting, insightful, or just fun, but not immediately practical.

Now, "Hardcore History" (as well as a good bunch of books, YouTube channels and other podcasts) is nothing like a class that you have to pass to get a pay raise. Instead it's much more like a group of amateurs sitting around a campfire and sharing stories about Punic Wars, or exoplanets, or functional programming, and just having fun. Which is pretty much the vibe I want in my own work, and if you expected a programming textbook feel free to ask your money back. Although, it's chapter six already, so I think you should've figured that out by now.

And no, I'm not saying that learning useful stuff is bad. It's good, at the very least it's, well, useful. However. If everything you learn always has to be practical, then you effectively mean that pure unrestrained curiosity is something to be ashamed of.

And that I find sad.

Anyway, let's get objective and let's finally declare the `struct object` that was defined back in the first chapter.

So far there 38 functions (boy, that escalated quickly!) that I promised to implement later, and they can be separated into five categories:

1. Ones with very specific types of either arguments or results: `car`, `cdr`, `define`, `eval_*`s, `to_*`s, `unwrap_*`s, `wrap_*`s.

2. Ones that have something to do with memory management: `decref`, `hasrefs`, `incref`, `get_gc_state`, `set_gc_state`.

3. Ones that are used to implement scoping mechanisms and/or to maintain runtime environment in general: `define`, `get_repl_scope`,  `lookup_in_scope`, `setup_runtime`, `teardown_runtime`.

4. Ones that should do different things depending on input type: `dispose`, `invoke`, `is_nil`, `is_symbol`, `reach`, `typename`, `write_object`.

5. No, actually there's no fifth category, these four pretty much cover everything I've promised so far.

So. There are two important ideas to take away from this analysis. First one is that `struct object` should contain some sort of a type marker to enable **(4)** and also **(1)**. Second one is that the only other functionality that is applicable to every object is the memory management machinery.

So let's do just that. Let's promise to introduce a type marker that (as  always) we'll  figure out later.

``` c

struct type;
typedef struct type* type_t;

```

Now, for memory management it's fairly obvious that we need a counter for stack references and an `enum gc_state` field for, well, GC state, which means we can (finally! hooray!) define `struct object`.

``` c

struct object {
	type_t type;
	unsigned stackrefs;
	enum gc_state gc_state;
};

```

Functions for manipulating reference counter and GC state are as trivial as it ever gets, so I'll just write them down.

``` c

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

```

Now I need to figure out how to represent type information.

One way to do it is by having a simple `enum type` to act as a type marker, and then do the polymorphic dispatch using a simple (although possibly lengthy) `switch`, i.e.

```c
void reach(object_t obj)
{
	switch (obj->type)
	{
	case TYPE_STRING:
		reach_string(obj);
		break;
	case TYPE_PAIR:
		reach_pair(obj);
		break;
	...
	}
}
```

It's not a bad idea per se. In fact this is how early drafts of this story were implemented, and it can work okay. But it has *narrative* issues.

Yes, narrative, remember this is still a literary programming exercise?

Basically, the problem here is that to write such switch I have to list all relevant types. But this mean I'm forced to work out all types before I write the function, and then after I've done I'm forbidden to add any new ones. And these limitations I find fairly incovenient.

In principle I can try to pull out something with fancy code generation in a similar vein to how I register native functions and syntax processors. But I've a gut feeling that it's gonna be nasty: all these functions are slightly different, some return values, others don't, some should do nothing by default, others should do something, and expressing all that in a way that is edible by a code generator, but still looks like normal C code is gonna be tricky and convoluted. Didn't try that, actually, and don't really want to.

What I came up with instead is this:

``` c

struct type {
	const char* name;
	void (*display)(FILE*, void*);
	void (*dispose)(void*);
	object_t (*invoke)(void*, int, object_t*);
	void (*reach)(void*);
	void (*write)(FILE*, void*);
};

```

And yes, after seeing it you're obliged to ask "Dude, but seriously, why don't you just use C++, or, like, literally any language that gives you vtables for free?"

Reason number one was, obviously, because I wanted to make my own vtable! I mean, one can spend many years writing code in something like Java or Python, and just accept virtual functions as magic that simply happens. So I just put together a struct with a bunch of function pointers, and then I realized I'm pretty much done! This was a funny realization.

Reason number number was to, kinda, take a break from object-oriented programming. Don't get me wrong, OOP is a fantastic idea where you have a limited set of simple concepts, and then build entire universe with them. Amazing. But then, since you're using the same concepts to express slightly different things, you might start to lose overview over whether this method is used for polymorphic dispatch and therefore belongs to core functionality of the class. Or maybe it can be a free-standing function and is attached to the class just for the sake of grouping things together. Or maybe it can be a free-standing function that is made overloadable *just in case.*

And then you use patterns to make it all more expressive, all the usual stuff. But it's still a useful exercise to program in an environment where these two goals are achieved by different means and therefore are truly explicit.

Approach that I personally would call the most elegant is how it's done in Haskell where classes are *just slightly too cumbersome* to use for cases where you don't actually need polymorphism.

Anyway, now that we have a type marker, let's write some functions.

``` c

const char* typename(object_t obj)
{
	return obj->type->name;
}

```

Okay, this one is trivial.

``` c

void dispose(object_t obj)
{
	if (obj->type->dispose)
		obj->type->dispose(obj);
	free(obj);
}

```

This one is also pretty self-explanatory. It also demonstrates the way to mark that, in general terms, function doesn't do anything special, which is simply by assigning the pointer to `NULL`.

``` c

void decref_many(int argct, object_t* args)
{
	for (int i=argct-1; i>=0; i--)
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

```

Nothing special here except for `decref()`ing function arguments that we (remember?!) have to do to mark they're not on call stack anymore.

``` c

void reach(object_t obj)
{
	if (obj->gc_state == REACHED)
		return;

	obj->gc_state = REACHED;
	if (obj->type->reach)
		obj->type->reach(obj);
}

```

Pretty straightforward as well.

``` c

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

```

`(display)` and `(write)` do mostly same thing except that latter puts quotes around strings.

And with this functions written down it's a good time to wrap up, but only to continue in