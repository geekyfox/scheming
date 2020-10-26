## Chapter 5 where I reinvent the wheel and then collect garbage

I think I mentioned Pascal somewhere in Chapter 2 of this story. As I remember, I haven't touched it since high school, which makes it two decades, give or take a year. But, truth be told, I (very briefly) dabbled with an idea of using Pascal in this very project. I mean, implementing an obscure programming language in another obscure programming language, just how cool is that!

Then I scanned the manual for Free Pascal and realized that no, this is definitely not gonna fly. The show-stopper was Pascal's lack of forward declaration for types. So that you must define a type above the places where you use it, or else it's an error. And if it doesn't seem like such a big deal, have you noticed that by now I wrote a full-fledged parser, and a good chunk of an evaluation engine, and few bits of standard library, and I still haven't defined *a single* concrete data type. I just said "oh yeah, this is a pointer to `struct object`, don't ask," and C compiler is, like, "okay, I'm cool with that." But in Pascal it's apparently illegal.

And no, this is not just a funny gimmick. In fact it's an experiment in tackling one of the hardest problems in software engineering which is: how to demonstrate that design of your program makes any sense?

One solution is to have a design spec... But wait, let's take a step back.

One option is to go, let's call it aggressively ultra-pragmatic, and to perform a variation of "This program WORKS! People USE IT! How DARE you ask for more than that?! Why do you even CARE?!"

Or something like that. And that's fine. I mean, if your boss is okay with that, and your customers are okay with that, and your coworkers are okay with that, who am I to judge, really?

But this disclaimer had to be made. Before I dive into ranting about suboptimal solutions, I'm obliged to give proper credit to people who at least acknowledge the problem. Or else it'd be like being told that "compared to serious runners you're below mediorce" by people who haven't exercised for, I dunno, four reincarnations.

So. One way to tackle this problem is to have a design spec where you use natural human language to elaborate the underlying logic, interactions between individual pieces, decisions made, and all that stuff. It's not a bad idea per se, but it has... Not so much a pitfall, but more like a fundamental limitation: you can't just give your design document to a computer and tell it to execute it. You still have to write machine-readable code. So you end up with two intrinsically independent artifacts, and unless you have a proper process to prevent it they get out of sync, and your spec becomes "more what you call guidelines" than actual spec.

Another option is to stick with just code, and then make sure your code is excellent: concise functions, transparent interfaces, meaningful naming, all the good stuff. And while it's certainly a great idea it has its own fundamental limitation: programming languages just aren't particularly good for expressing intent. I mean, you can get pretty far with explaining *how* your program works by code itself, but not such much *why.* For that you still need some kind of documentation.

And so my idea, and keep in mind it's an ongoing experiment and not The Next Silver Bullet for which I'm selling certifications, was to essentially combine the two in such a way that the whole thing has a logical flow of a spec, but then the code is organically embedded into it, but then in the end it's just a normal program (albeit with copious comments) that can be compiled by a vanilla compiler rather than some "literate programming" contraption.

And this is why it's important that underlying programming language is liberal with regards to code layout, or else you end up with a Sysyphean task when you must state that this data type has such and such fields even though you won't be able to justify their necessity until fifty pages later.

All this was an extremely long introduction to the very first real data type in this program. But finally I proudly present you

``` c

struct array {
	void** data;
	int available;
	int size;
};

typedef struct array* array_t;

```

and after seeing it you're obliged to ask "Dude, but why don't you use C++, at least you'll get `std::vector` for free?"

And that's indeed a reasonable question. I even agree that from a practical standpoint C++ would be at least as good as plain C, and likely better.

However. For a project like this one the very concept of a "practical standpoint" is a slippery slope. Because right after "but why don't you use C++ and get `std::vector` for free?" comes "but why don't you use Java and get garbage collector for free?" and then "but why you don't just `apt install scheme`?" and eventually "but why you don't learn some hipper language?"

And then you find yourself surrounded by weirdos at a shady Kotlin meetup, and you lost count how many tasteless beers you had, and you're wondering if things will end very bad or even worse.

No, really, the purpose of this whole undertaking is not to build anything. It's not even to learn, not something specific anyway, but mostly just to have fun doing it. Essentially it's simply one big coding exercise. And that's why one of the guiding principles here is when choosing between a more practical option and one that's less practical but still reasonable, go for the latter.

Oh, and "still reasonable" explains why I use plain C and not, I dunno, Fortran 66. Writing a Scheme interpreter in Fortran 66 will get you deep into "pain and humiliation for the sake of pain and humiliation" territory. Of course there's absolutely nothing wrong with enjoying some pain and humiliation, except that a computer is not the best home appliance to use for these purposes, if you get what I mean.

Anyway. Dynamically expandable array is one of the most trivial things in this entire story, and I'm not going to spend any time explaining it.

``` c

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

```

But the reason *why* I suddenly decided I need a dynamically expandable list is because I'm gonna need it to build a garbage collector. Not that I really need a garbage collector, but when you have a parser, and an evaluation engine, and a chunk of the standard library, it's so hard to resist from a not-so-subtle Hunter S. Thompson reference.

Of course I need a garbage collector in my Scheme interpreter, but it doesn't have to be very sophisticated. A simplistic mark-and-sweep would suffice.

During mark-and-sweep garbage collection can be in one of three states:

``` c

enum gc_state {
	REACHED,
	REACHABLE,
	GARBAGE,
};

```

`REACHED` means this object is reachable from call stack, global scope and/or other reachable objects, and we've already propagated its reachability to the objects it refers to (if any).

`REACHABLE` means this object is reachable, but we haven't propagated it's reachability yet.

`GARBAGE` means this object is not reachable and (assuming we examined all objects we have) can be safely disposed.

And then entire algorithm consists of three steps.

``` c

void mark_globally_reachable(void);
void propagate_reachability(void);
void dispose_garbage(void);

void collect_garbage()
{
	mark_globally_reachable();
	propagate_reachability();
	dispose_garbage();
}

```

First mark globally reachable objects as such.

``` c

struct array ALL_OBJECTS;
struct array REACHABLE_OBJECTS;

bool hasrefs(object_t);
void set_gc_state(object_t, enum gc_state);

void mark_globally_reachable(void)
{
	REACHABLE_OBJECTS.size = 0;

	for (int i=ALL_OBJECTS.size-1; i>=0; i--) {
		object_t obj = ALL_OBJECTS.data[i];

		if (! hasrefs(obj)) {
			set_gc_state(obj, GARBAGE);
			continue;
		}

		set_gc_state(obj, REACHABLE);
		push_array(&REACHABLE_OBJECTS, obj);
	}
}

```

Then propagate reachability.

``` c

enum gc_state get_gc_state(object_t);
void reach(object_t);

void propagate_reachability(void)
{
	object_t obj;

	while ((obj = pop_array(&REACHABLE_OBJECTS)))
		if (get_gc_state(obj) == REACHABLE)
			reach(obj);
}

```

And finally dispose garbage.

``` c

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

```

And now we can proceed to