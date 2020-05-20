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
// is initialize garbage collector and such, but it also implies we're
// going to have **the** runtime, probably scattered around in a bunch of
// global variables.
//
// I can almost here voices asking "But what if you need to have multiple
// runtimes? What if customer comes and asks to make your interpreter
// embeddable as a scripting engine? What about multithreading? Why are
// you not worried?!"
//
// The answer is "I consciously don't care." This is just a pet project
// that started with me willing to tinker with Scheme, and then realizing
// that just writing in Scheme is too easy, so I wrote my own interpreter,
// and then realizing that **even that is not fun enough**, so I wrapped it
// into a sort of "literary programming" exercise. In a (highly unlikely
//situation) when I'll have to write my own multithreaded embeddable Scheme
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
// in this case there's no framework and operating system is effectively
// the invoker ("*there was nothing between us, not even a condom*"...
// yeah, really bad joke), and `abort()`ing is a proper way to notify the
// operating system about the problem, so #2 is pretty much #3, just
// with less boilerplate code.
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
// Here we introduce several things.
//
// First one is `struct object` which is going to be the representation
// of a Scheme object. It's quite certainly going to be some sort of a
// `struct`; internal details of that struct we'll figure out later.
//
// Second and third are `read_object()` and `eval_repl()` functions that,
// respectively, read an object from an input stream and evaluate it
// in REPL context.
//
// Last one is `decref()` function that is needed because we're going to
// have a tracing garbage collector. And tracing garbage collector needs
// a reliable way to figure out which objects are still needed, or else it
// will do funny things like deallocating variables at random yet always
// inconvenient times. And then you either have to implement traversal of
// a section of process' memory to gather that knowledge, or else you have
// to workaround not doing it.Well, actually, it's a choice you'll face
// multiple times and you're absolutely not not obliged to always take the
// same path.
//
// In this case doing it "properly" would involve analyzing C call stack
// which is pretty cumbersome, so the plan is to get away with a simple
// reference counting.
//
// Now comes the REPL...
//

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

//
// ...which isn't particularly interesting, and we proceed to
// ## Chapter 2 where we parse Lisp code
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
// Lisp syntax is famously spartan. Basically all you get is:
// * lists (those thingies with ((astonishingly) copious) amount of
// parentheses),
// * strings (delimited by "double quotes" or however you call that character),
// * quotations (if you don't know who these are, you better look it up in
// Scheme spec, but basically it's a way to specify that ```'(+ 1 2)``` is
// *literally* a list with three elements and not an expression that adds
// two numbers),
// * and atoms, which are pretty much everything else including numbers
// and symbols.
//
// So what we're doing here is we're looking at the first non-trivial
// character in the input stream and if it's an opening parenthesis we
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
// Oh, and "the first non-trivial character" means we fast-forward through
// the input stream ignoring comments and whitespace until we either
// encounter a character that's neither or reach an EOF.
//
// There are four `read_something()` functions that we promised to
// implement, let's start with `read_string()`.
//

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

//
// Nothing surprising here, just read characters into a buffer until you
// reach the closing double quote, then wrap the contents of the buffer
// into an `object_t` and call it a day.
//
// Yes, this simplistic implementation will miserably fail to parse a
// source file with a string constant that is longer than 10K characters.
//
// And you know, if you think about it, hard-coded 10K bytes for buffer
// size is kind of interesting here. It's an arbitrary number that on one
// hand is safely above any practical limit in terms of usefulness. I
// mean, of course you can hard-code entire "Crime and Punishment" as a
// single string constant just to humiliate a dimwit interpreter author,
// but within any remotely sane coding style such blob must be offloaded
// to an external text file.
//
// On the other hand it's safely below any practical limit in terms of
// conserving memory.
//

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

//
// This one isn
//
// I'm looking at another buffer and
// You know what boggles my mind even more?
//
// Remember, in the very beginning of

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

//
// CUTOFF

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
	object_t accum, obj, result;

	accum = wrap_nil();

	while ((obj = read_next_object(in))) {
		push_list(&accum, obj);
		decref(obj);
	}

	result = reverse(accum);
	decref(accum);
	return result;
}

//

object_t cons(object_t, object_t);

void push_list(object_t* ptr, object_t head)
{
	object_t tail;

	tail = *ptr;
	*ptr = cons(head, tail);
	decref(tail);
}

//

object_t read_quote(FILE* in)
{
	object_t head, result;

	if (! (head = read_object(in)))
		DIE("Premature end of input");

	result = wrap_nil();

	push_list(&result, head);
	decref(head);

	head = wrap_symbol("quote");
	push_list(&result, head);
	decref(head);

	return result;
}

//
// Chapter 3 where we evaluate
//

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
	object_t new_result;

	while((thunk = to_thunk(result))) {
		new_result = eval_thunk(thunk);
		decref(result);
		result = new_result;
	}

	return result;
}

//

struct pair;
typedef struct pair* pair_t;

struct symbol;
typedef struct symbol* symbol_t;

pair_t to_pair(object_t);
symbol_t to_symbol(object_t);

object_t eval_sexpr(scope_t, pair_t);
object_t eval_var(scope_t, symbol_t);

void incref(object_t);

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

object_t car(pair_t);
object_t cdr(pair_t);

object_t eval_syntax(scope_t scope, symbol_t keyword, object_t body);
object_t eval_funcall(scope_t scope, object_t func, object_t exprs);

object_t eval_sexpr(scope_t scope, pair_t code)
{
	object_t head, tail, result, func;
	symbol_t head_sym;

	head = car(code);
	tail = cdr(code);

	if ((head_sym = to_symbol(head)))
		if ((result = eval_syntax(scope, head_sym, tail)))
			return result;

	func = eval_eager(scope, head);
	result = eval_funcall(scope, func, tail);

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

object_t scope_get(scope_t, symbol_t key);

object_t eval_var(scope_t scope, symbol_t key)
{
	object_t result = scope_get(scope, key);
	incref(result);
	return result;
}

// CUTOFF
//

struct array {
	void** data;
	int available;
	int size;
};

typedef struct array* array_t;

#include <strings.h>

void array_init(array_t arr)
{
	bzero(arr, sizeof(struct array));
}

void array_dispose(struct array* arr)
{
	free(arr->data);
	bzero(arr, sizeof(struct array));
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

bool is_nil(object_t);

const char* typename(object_t);

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

//

object_t reverse(object_t list)
{
	object_t result = wrap_nil(), obj;

	while ((obj = pop_list(&list)))
		push_list(&result, obj);

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

//

//
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

pair_t to_pair_or_die(const char* name, object_t obj)
{
	pair_t pair = to_pair(obj);
	if (! pair)
		DIE("Expected argument of %s to be a pair, got %s instead", name, typename(obj));
	return pair;
}

object_t native_car(int argct, object_t* args)
{
	assert_arg_count("car", argct, 1);
	pair_t pair = to_pair_or_die("car", args[0]);
	object_t result = car(pair);
	incref(result);
	return result;
}

object_t native_cdr(int argct, object_t* args)
{
	assert_arg_count("cdr", argct, 1);
	pair_t pair = to_pair_or_die("cdr", args[0]);
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

void eval_define(scope_t scope, symbol_t key, object_t expr);
object_t lambda(scope_t scope, object_t params, object_t body);

object_t syntax_define(scope_t scope, object_t code)
{
	object_t head = pop_list_or_die(&code);

	pair_t head_pair = to_pair(head);
	if (head_pair) {
		object_t name_obj = car(head_pair);
		symbol_t name = to_symbol(name_obj);
		if (! name)
			DIE("Expected variable name in define to be a symbol, got %s instead", typename(name_obj));

		object_t args = cdr(head_pair);
		object_t func = lambda(scope, args, code);
		eval_define(scope, name, func);
		decref(func);
		return wrap_nil();
	}

	symbol_t head_sym = to_symbol(head);
	if (head_sym) {
		object_t expr = pop_list_or_die(&code);
		eval_define(scope, head_sym, expr);
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
	const char* key;
	int hash;
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

void* dict_lookup(struct dict*, const char*);
const char* unwrap_symbol(symbol_t);

syntax_t lookup_syntax_handler(symbol_t name)
{
	const char* key = unwrap_symbol(name);
	return dict_lookup(&SYNTAX_HANDLERS, key);
}

//

void dict_dispose(struct dict*);

void teardown_syntax()
{
	dict_dispose(&SYNTAX_HANDLERS);
}

//

void dict_dispose(struct dict* dict)
{
	for (int i=dict->size-1; i>=0; i--)
		if (dict->data[i].key)
			free((char*)dict->data[i].key);
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
int compare_entry(struct dict_entry* entry, int hash, const char* key)
{
	if (entry->hash == hash) {
		return strcmp(entry->key, key);
	} else {
		return entry->hash - hash;
	}
}

int hash_to_index(int hash, int size)
{
	int index = hash % size;
	if (index < 0)
		index += size;
	return index;
}

void* dict_lookup(struct dict* dict, const char* key)
{
	if (dict->size == 0)
		return NULL;

	int hash = strhash(key);
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

void* dict_put(struct dict*, const char*, void*);

void register_syntax_handler(const char* name, syntax_t syntax)
{
	dict_put(&SYNTAX_HANDLERS, name, syntax);
}

//

void dict_enlarge(struct dict* dict);

void dict_reinsert(struct dict*, struct dict_entry*);

void* dict_put(struct dict* dict, const char* key, void* value)
{
	if ((dict->size == 0) || (dict->used * 2 > dict->size))
		dict_enlarge(dict);

	int hash = strhash(key);
	int index = hash_to_index(hash, dict->size);

	while (true) {
		struct dict_entry* entry = &dict->data[index];

		if (entry->key == NULL) {
			entry->key = strdup(key);
			entry->value = value;
			entry->hash = hash;
			dict->used++;
			return NULL;
		}

		int diff = compare_entry(entry, hash, key);
		if (diff < 0) {
			index = (index + 1) % dict->size;
			continue;
		} else if (diff == 0) {
			void* old = entry->value;
			entry->value = value;
			return old;
		} else {
			struct dict_entry present = *entry;
			entry->key = strdup(key);
			entry->value = value;
			entry->hash = hash;
			dict_reinsert(dict, &present);
			return NULL;
		}
	}
}

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

void dict_reinsert(struct dict* dict, struct dict_entry* entry)
{
	int index = hash_to_index(entry->hash, dict->size);
	while (true) {
		struct dict_entry* cell = &dict->data[index];

		if (cell->key == NULL) {
			*cell = *entry;
			dict->used++;
			return;
		}

		int diff = compare_entry(cell, entry->hash, entry->key);
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
	void (*reach)(void*);
	void (*dispose)(void*);
	void (*print)(FILE*, void*);
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

void object_dispose(object_t obj)
{
	if (obj->type->dispose) {
		obj->type->dispose(obj);
	} else {
		free(obj);
	}
}

//

void print_object(FILE* out, object_t obj)
{
	if (obj->type->print) {
		obj->type->print(out, obj);
	} else {
		fprintf(out, "[%s]", obj->type->name);
	}
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

void print_nil(FILE* out, void* obj)
{
	fprintf(out, "()");
}

struct type TYPE_NIL = {
	.name = "nil",
	.print = print_nil,
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

void print_bool(FILE* out, void* obj)
{
	boolean_t b = obj;
	fputs(b->value ? "#t" : "#f", out);
}

struct type TYPE_BOOL = {
	.name = "bool",
	.print = print_bool,
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

void pair_print(FILE* out, void* ptr)
{
	pair_t pair = ptr;

	fputs("(", out);
	print_object(out, car(pair));

	object_t obj = cdr(pair);

	while (true) {
		if (is_nil(obj))
			break;

		pair = to_pair(obj);
		if (pair) {
			fputs(" ", out);
			print_object(out, car(pair));
			obj = cdr(pair);
			continue;
		}

		fputs(" . ", out);
		print_object(out, obj);
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
	.print = pair_print,
	.reach = pair_reach,
};

object_t cons(object_t head, object_t tail)
{
	pair_t result = malloc(sizeof(struct pair));
	result->car = head;
	result->cdr = tail;
	return object_init(result, &TYPE_PAIR);
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

void print_int(FILE* out, void* obj)
{
	integer_t num = obj;
	fprintf(out, "%d", num->value);
}

struct type TYPE_INT = {
	.name = "int",
	.print = print_int
};

struct object* wrap_int(int v)
{
	integer_t num = malloc(sizeof(struct integer));
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

void print_string(FILE* out, void* obj)
{
	string_t str = obj;
	fprintf(out, "%s", str->value);
}

struct type TYPE_STRING = {
	.name = "string",
	.dispose = dispose_string,
	.print = print_string,
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
};

typedef struct symbol* symbol_t;

void symbol_print(FILE* out, void* obj)
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
	.print = symbol_print,
	.dispose = symbol_dispose,
};

struct dict ALL_SYMBOLS;

object_t wrap_symbol(const char* v)
{
	object_t result = dict_lookup(&ALL_SYMBOLS, v);
	if (! result) {
		symbol_t symbol = malloc(sizeof(struct symbol));
		symbol->value = strdup(v);
		result = object_init(symbol, &TYPE_SYMBOL);
		dict_put(&ALL_SYMBOLS, v, result);
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
	int argct;
	object_t* args;
};

void thunk_dispose(void* obj)
{
	thunk_t thunk = obj;
	free(thunk->args);
	free(obj);
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
	.reach = thunk_reach,
	.dispose = thunk_dispose,
};

object_t make_thunk(lambda_t lambda, array_t args)
{
	thunk_t thunk = malloc(sizeof(struct thunk));
	thunk->lambda = lambda;
	thunk->argct = args->size;
	thunk->args = (object_t*)args->data;
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
	return invoke_lambda(thunk->lambda, thunk->argct, thunk->args);
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

object_t scope_lookup(scope_t scope, const char* key)
{
	while (scope) {
		void* value = dict_lookup(&scope->s_binds, key);
		if (value)
			return value;
		scope = scope->s_parent;
	}
	return NULL;
}

object_t scope_get(scope_t scope, symbol_t key)
{
	object_t result = scope_lookup(scope, key->value);
	if (result == NULL)
		DIE("Undefined variable %s", key->value);
	return result;
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
		free(obj);
}

struct type SCOPE = {
	.name = "scope",
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
	void* ptr = dict_put(&scope->s_binds, strkey, value);
	if (ptr != NULL)
		DIE("Rebind of %s", strkey);
	if (! scope->s_parent)
		incref(value);
}

//

void eval_define(scope_t scope, symbol_t key, object_t expr)
{
	object_t result = eval_eager(scope, expr);

	lambda_t lambda = to_lambda(result);
	if (lambda)
		lambda->l_name = key;

	scope_bind(scope, key, result);
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
	scope_t scope = malloc(sizeof(struct scope));
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
	dict_init(&SYNTAX_HANDLERS);
	setup_syntax();
	array_init(&ALL_OBJECTS);
	scope_init(&DEFAULT_SCOPE, NULL);
	dict_init(&ALL_SYMBOLS);
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
	collect_garbage();
	array_dispose(&ALL_OBJECTS);
	teardown_syntax();
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
	print_object(stdout, args[0]);
	return wrap_nil();
}

//

object_t native_newline(int argct, object_t* args)
{
	assert_arg_count("newline", argct, 0);
	fputs("\n", stdout);
	return wrap_nil();
}

// CUTOFF

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
	return cons(args[0], args[1]);
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
	assert_arg_count("+", argct, 2);
	int a = unbox_int_or_die("+", args[0]);
	int b = unbox_int_or_die("+", args[1]);
	return wrap_int(a + b);
}

object_t native_pairp(int argct, object_t* args) // pair?
{
	assert_arg_count("pair?", argct, 1);
	return wrap_bool(to_pair(args[0]));
}

object_t native_setcdr(int argct, object_t* args) // set-cdr!
{
	assert_arg_count("set-cdr!", argct, 2);
	pair_t pair = to_pair_or_die("set-cdr!", args[0]);
	pair->cdr = args[1];
	return wrap_nil();
}

object_t native_symbolp(int argct, object_t* args) // symbol?
{
	assert_arg_count("symbol?", argct, 1);
	bool result = to_symbol(args[0]) != NULL;
	return wrap_bool(result);
}

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

		DIE("Don't know how to eq? %s against %s", typename(x), typename(y));
	}
}

bool object_is_symbol(object_t obj, const char* text)
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
		if (object_is_symbol(test, "else") || eval_boolean(scope, test))
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
		object_t key = pop_list_or_die(&binding);
		symbol_t keysym = to_symbol(key);
		if (! keysym)
			DIE("Expected letrec binding name to be a symbol, got %s instead", typename(key));
		object_t expr = pop_list_or_die(&binding);
		eval_define(scope, keysym, expr);
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

void setup_syntax(void)
{
	register_syntax_handler("and", syntax_and);
	register_syntax_handler("cond", syntax_cond);
	register_syntax_handler("define", syntax_define);
	register_syntax_handler("if", syntax_if);
	register_syntax_handler("lambda", syntax_lambda);
	register_syntax_handler("letrec", syntax_letrec);
	register_syntax_handler("quote", syntax_quote);
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
	register_native("eq?", native_eqp);
	register_native("fold", native_fold);
	register_native("list", native_list);
	register_native("modulo", native_modulo);
	register_native("newline", native_newline);
	register_native("null?", native_nullp);
	register_native("pair?", native_pairp);
	register_native("reverse", native_reverse);
	register_native("set-cdr!", native_setcdr);
	register_native("symbol?", native_symbolp);
	register_native("write", native_write);
}
