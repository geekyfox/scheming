// ## Chapter #1 where the story begins
//
// Every story has to begin somewhere. This one is a tour through a
// simplistic Scheme interpreter written in C, and every C program begins
// with the main() function.
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
// But actually it is proper C code that compiles just fine. In fact, C
// puts only a few requirements on how you layout your code; having to
// declare (but not necessarily implement) a function above a place where
// it's used is of course one of them.
//
// Moreover, this is the general pattern in this story: we postpone
// writing lower-level implementation until we get a grasp on how it's
// going to be used. Rather than building some bits and pieces here
// and there and then trying to stitch them together.
//
// Another thing is more subtle, but will get a lot of programmers,
// especially more junior ones, nervous once they figure it out. It's
// `setup_runtime()` call. It's pretty clear *what* it will do which
// is initialize garbage collector and such, but it also implies we're
// going *the* runtime, stored in a bunch of global variables.
//
// I can almost here voices asking "But what if you need to have multiple
// runtimes? What if customer comes and asks to make your interpreter
// embeddable as a scripting engine? What about multithreading? Why are
// you not worried?!"
//
// The answer is "I consciously don't care." This is just a pet project
// that started with me willing to tinker with Scheme, and then realizing
// that just writing in Scheme is too easy, so I wrote my own interpreter.
// In a (highly unlikely situation) when I'll have to write my own
// multithreaded embeddable Scheme interpreter, I'll just start from
// scratch, and that's about it.
//
// Anyway, we'll write functions to setup and teardown runtime once we
// get a better idea of how said runtime should look like, and for now
// we focus on doing useful stuff.
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
// Now that we're going to have a function that reads code from a stream
// and executes it, writing a function that does the same with a file is
// trivial, so let's just make one.
//

#include <errno.h>
#include <stdlib.h>
#include <string.h>

FILE* fopen_or_die(const char* pathname, const char* mode)
{
	FILE *f = fopen(pathname, mode);
	if (f != NULL)
		return f;
	const char* err = strerror(errno);
	fprintf(stderr, "Error opening file %s: %s\n", pathname, err);
	abort();
}

void execute_file(const char* filename)
{
	FILE* f = fopen_or_die(filename, "r");
	execute(f);
	fclose(f);
}

//
// And now let's implement `execute()`
//

#include <stdbool.h>

void stack_discard(int count);
void eval_in_default_scope(bool force_eager);
bool read_object(FILE*, bool die_on_eof);

void execute(FILE* in)
{
	while (read_object(in, false)) {
		eval_in_default_scope(true);
		stack_discard(1);
	}
}

//
// Here we have a tiny function that raises big questions.
//
// Question number one is "When you *simply* `read_object()` without
// returning it, where the hell is that object that you've just read?"
//
// The answer is, we're going to maintain our own stack, and so the object
// we've just read is there.
//
// Second question then is why do we need to maintain your own stack and
// the answer is because we'll need garbage collector anyway,
//

void print_object(FILE*);

void repl()
{
	printf("> ");
	while (read_object(stdin, false)) {
		eval_in_default_scope(true);
		print_object(stdout);
		printf("\n> ");
		fflush(stdout);
	}
	printf("bye\n");
}

///



// ## Chapter #2 where we parse.

int fgetc_skip(FILE*, bool die_on_eof);
void read_atom(FILE* in);
void read_list(FILE* in);
void read_string(FILE* in);
void read_quote(FILE* in);

bool read_object(FILE* in, bool die_on_eof)
{
	int ch = fgetc_skip(in, die_on_eof);
	switch (ch)
	{
	case EOF:
		return false;
	case '(':
		read_list(in);
		return true;
	case ')':
		fprintf(stderr, "Unmatched ')'\n");
		abort();
	case '\'':
		read_quote(in);
		return true;
	case '"':
		read_string(in);
		return true;
	default:
		ungetc(ch, in);
		read_atom(in);
		return true;
	}
}

//

#include <ctype.h>

int fgetc_or_die(FILE* in, bool die_on_eof)
{
	int ch = fgetc(in);
	if (ch == EOF) {
		if (! feof(in)) {
			fprintf(stderr, "IO error: %s\n", strerror(errno));
			abort();
		}
		if (die_on_eof) {
			fprintf(stderr, "Unexpected end of input\n");
			abort();
		}
	}
	return ch;
}

int fgetc_skip(FILE* f, bool die_on_eof)
{
	bool comment = false;
	while (true) {
		int ch = fgetc_or_die(f, die_on_eof);
		if (ch == EOF)
			return EOF;
		if (comment) {
			if (ch == '\n')
				comment = false;
			continue;
		}
		if (isspace(ch))
			continue;
		if (ch == ';') {
			comment = true;
			continue;
		}
		return ch;
	}
}

//

void push_bool(bool v);
void push_int(int value);
void push_symbol(const char*);

bool parse_and_push_bool(const char* text)
{
	if (strcmp(text, "#f") == 0) {
		push_bool(false);
		return true;
	}
	if (strcmp(text, "#t") == 0) {
		push_bool(true);
		return true;
	}
	return false;
}

bool parse_and_push_int(const char* text)
{
	int accum = 0;
	int index = 0;
	while (text[index]) {
		if (! isdigit(text[index]))
			return false;
		accum = accum * 10 + (text[index] - '0');
		index++;
	}
	push_int(accum);
	return true;
}

bool isatomic(char ch)
{
	if (isspace(ch))
		return false;
	switch (ch) {
	case '(': case ')': case ';': case '"':
		return false;
	}
	return true;
}

void read_atom(FILE* in)
{
	char buffer[10240];
	int fill = 0;

	while (true) {
		int ch = fgetc_or_die(in, false);
		if (ch == EOF)
			break;
		if (! isatomic(ch)) {
			ungetc(ch, in);
			break;
		}
		if (fill >= 10240) {
			fprintf(stderr, "Buffer overflow in read_atom()\n");
			abort();
		}
		buffer[fill++] = ch;
	}
	buffer[fill] = '\0';

	if (parse_and_push_bool(buffer))
		return;

	if (parse_and_push_int(buffer))
		return;

	push_symbol(buffer);
}

//

void make_pair(bool top_is_car);
void push_nil(void);

void construct_list(int count)
{
	push_nil();
	for (; count > 0; count--)
		make_pair(false);
}

void read_list(FILE* in)
{
	int count = 0;
	while (true) {
		int ch = fgetc_skip(in, true);
		if (ch == ')')
			break;
		ungetc(ch, in);
		read_object(in, true);
		count++;
	}
	construct_list(count);
}

//

void push_string(const char*);

void read_string(FILE* in)
{
	char buffer[10240];
	int fill = 0;
	int ch;

	while ((ch = fgetc_or_die(in, true)) != '"')
		buffer[fill++] = ch;
	buffer[fill] = '\0';
	push_string(buffer);
}

//

void read_quote(FILE* in)
{
	read_object(in, true);
	push_nil();
	make_pair(false);
	push_symbol("quote");
	make_pair(true);
}

//
// Chapter 3 where we evaluate
//

struct object DEFAULT_SCOPE;

void eval(bool force_eager);
void push(struct object*);

void eval_in_default_scope(bool force_eager)
{
	push(&DEFAULT_SCOPE);
	eval(force_eager);
}

//

bool is_pair(struct object*);
bool is_symbol(struct object*);
bool is_thunk(struct object*);

void eval_funcall(void);
bool eval_syntax(void);
void eval_thunk(void);
void eval_variable(void);

void stack_car(int offset);
void stack_cdr(int offset);
void stack_duplicate(int offset);
struct object* stack_get(int offset);

void eval(bool force_eager)
{
	// stack is: ... expression scope
	struct object* expr = stack_get(1);

	if (is_pair(expr)) {
		stack_duplicate(1);
		stack_car(0);
		stack_cdr(2);
		// stack is: ... expr-tail scope expr-head
		if (! eval_syntax()) {
			stack_duplicate(1);
			eval(true);
			// stack is: ... args scope func
			eval_funcall();
		}
	} else if (is_symbol(expr)) {
		eval_variable();
	} else {
		stack_discard(1);
	}

	if (force_eager)
		while (is_thunk(stack_get(0)))
			eval_thunk();
}

//

typedef void (*syntax_t)(void);

syntax_t lookup_syntax_handler(const char* name);
const char* unbox_symbol(const struct object*);

bool eval_syntax(void)
{
	if (! is_symbol(stack_get(0)))
		return false;

	const char* name = unbox_symbol(stack_get(0));
	syntax_t handler = lookup_syntax_handler(name);
	if (handler == NULL)
		return false;

	stack_discard(1);
	handler();
	return true;
}

//

void invoke(int argct);
bool is_nil(struct object*);
bool is_lambda(struct object* obj);
void make_thunk(int argct);
void stack_rotate(int offset);

void eval_funcall(void)
{
	// [exprs] scope func
	int argct = 0;
	while (! is_nil(stack_get(2))) {
		stack_duplicate(2);
		stack_car(0);
		stack_cdr(3);
		// [exprs] scope func expr
		stack_duplicate(2);
		eval(true);
		// [exprs] scope func param
		stack_rotate(3);
		// param [exprs] scope func
		argct++;
	}
	// param_1 param_2 nil scope func
	stack_rotate(2);
	stack_discard(2);
	// param_1 param_2 func
	if (is_lambda(stack_get(0))) {
		make_thunk(argct);
	} else {
		invoke(argct);
	}
}

//
// CUTOFF
//

//
// Chapter 4 where we implement stack

struct object TRUE;
struct object FALSE;


void push_bool(bool v)
{
	push(v ? &TRUE : &FALSE);
}

//

struct object NIL;

void push_nil(void)
{
	push(&NIL);
}

//

#define TYPECODE_PAIR   42000

struct pair {
	struct object* p_car;
	struct object* p_cdr;
};

void make(int type, void* vptr, int argct);

void make_pair(bool top_is_car)
{
	struct pair pair;
	pair.p_car = stack_get(top_is_car ? 0 : 1);
	pair.p_cdr = stack_get(top_is_car ? 1 : 0);
	make(TYPECODE_PAIR, &pair, 2);
}

//

#define TYPECODE_INT    42001

void push_int(int value)
{
	make(TYPECODE_INT, &value, 0);
}

//

#define TYPECODE_SYMBOL 42002

void push_symbol(const char* text)
{
	char* copy = strdup(text);
	make(TYPECODE_SYMBOL, &copy, 0);
}

//

#define TYPECODE_STRING 42003

void push_string(const char* text)
{
	char* copy = strdup(text);
	make(TYPECODE_STRING, &copy, 0);
}

//

struct array STACK;

void array_push(struct array*, void* item);

void push(struct object* obj)
{
	array_push(&STACK, obj);
}

// CUTOFF
//


// ## Chapter #3 where we evaluate.

typedef struct object* object_t;



//

#include <stdbool.h>
#include <stdio.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

enum dispose_mode {
	DISPOSE_KEEP = 47000,
	DISPOSE_FREE = 47001
};

struct array {
	void** a_data;
	int a_alc;
	int a_size;
};

struct dict_entry {
	const char* de_key;
	int de_hash;
	void* de_value;
};

struct dict {
	struct dict_entry* d_data;
	int d_alc;
	int d_size;
};

struct scope {
	struct dict s_binds;
	struct object* s_parent;
};

struct thunk {
	struct object* t_proc;
	int t_argct;
};

struct lambda {
	struct object* l_body;
	struct object* l_scope;
	char* l_name;
	struct array l_params;
};

typedef void (*native_t)(int);


#define TYPECODE_BOOL   42004
#define TYPECODE_SCOPE  42007
#define TYPECODE_NIL    42005
#define TYPECODE_THUNK  42006
#define TYPECODE_LAMBDA 42008
#define TYPECODE_NATIVE 42009

union value;


union value {
	int v_int;
	struct pair v_pair;
	bool v_bool;
	struct scope v_scope;
	const char* v_symbol;
	const char* v_string;
	struct thunk v_thunk;
	struct lambda v_lambda;
	native_t v_native;
};

struct object {
	int o_typecode;
	union value o_value;
	bool o_garbage;
};

void* dict_put(struct dict*, const char*, void*);
void* dict_lookup(struct dict*, const char*);
void dict_dispose(struct dict*, enum dispose_mode, enum dispose_mode);

#define IS(type, var) ((var)->o_typecode == TYPECODE_##type)
#define ASSERT_IS(type, var) __assert_is(__func__, TYPECODE_##type, var)
void __assert_is(const char* funcname, int typecode, const struct object*);

const char* typecode_to_name(int typecode);

bool object_to_bool(const struct object* obj);
struct object* bool_to_object(bool);

void object_dispose(struct object*);
void object_reach(struct object*);

void array_init(struct array*);
void array_dispose(struct array*, bool free_elems);
void array_push(struct array*, void*);

void           interpreter_setup();
void           interpreter_teardown();

void object_scope_init(struct object* obj, struct object* parent);
struct object* object_scope_root();

struct object* object_thunk_create(struct object*, struct array*);

void  scope_bind(struct scope* scope, const char* key, struct object* value);
void  scope_dispose(struct scope*);
void  scope_init(struct scope*, struct object* parent);
void* scope_get(struct scope*, const char* key);
void* scope_lookup(struct scope*, const char*);
void  scope_reach(struct scope*);


struct object* pair_car(const struct object* pair);
struct object* pair_cdr(const struct object* pair);
void pair_setcdr(struct object* pair, struct object* value);
int unbox_int(const struct object*);

struct object* eval_quote(struct object* scope, struct object* code);
struct object* eval_if(struct object* scope, struct object* code);
struct object* eval_and(struct object* scope, struct object* code);
void register_stdlib_functions();
struct object* stdlib_nullp(struct array* args);
void assert_arg_count(const char* name, int actual, int expected);
struct object* int_to_object(int);
bool eq(struct object* x, struct object* y);
bool object_is_symbol(struct object* obj, const char* name);
struct object* wrap_symbol(const char*);
struct object* wrap_string(const char*);
void name_lambda(struct lambda* l, const char* name);

void collect_garbage(void);

static struct array ALL_OBJECTS;
static struct array OBJECT_POOL;

void array_init(struct array* arr)
{
	bzero(arr, sizeof(struct array));
}

void array_push(struct array* arr, void* entry)
{
	if (arr->a_size == arr->a_alc) {
		arr->a_alc *= 2;
		if (arr->a_alc < 8)
			arr->a_alc = 8;
		arr->a_data = realloc(arr->a_data, sizeof(void*)*arr->a_alc);
	}
	arr->a_data[arr->a_size++] = entry;
}

void array_dispose(struct array* arr, bool free_elems)
{
	if (free_elems)
		for (int i=arr->a_size-1; i>=0; i--)
			free(arr->a_data[i]);
	free(arr->a_data);
	bzero(arr, sizeof(struct array));
}

void* dict_put(struct dict* dict, const char* key, void* value)
{
	for (int i=0; i<dict->d_size; i++)
		if (strcmp(dict->d_data[i].de_key, key) == 0) {
			void* old = dict->d_data[i].de_value;
			dict->d_data[i].de_value = value;
			return old;
		}
	if (dict->d_size == dict->d_alc) {
		dict->d_alc *= 2;
		if (dict->d_alc < 8)
			dict->d_alc = 8;
		dict->d_data = realloc(dict->d_data, sizeof(struct dict_entry)*dict->d_alc);
	}
	dict->d_data[dict->d_size].de_key = key;
	dict->d_data[dict->d_size].de_value = value;
	dict->d_size++;
	return NULL;
}

void* dict_lookup(struct dict* dict, const char* key)
{
	for (int i=0; i<dict->d_size; i++)
		if (strcmp(dict->d_data[i].de_key, key) == 0)
			return dict->d_data[i].de_value;
	return NULL;
}

void dict_dispose(struct dict* dict, enum dispose_mode keymode, enum dispose_mode valmode)
{
	if (keymode == DISPOSE_FREE)
		for (int i=dict->d_size-1; i>=0; i--)
			free((char*)dict->d_data[i].de_key);
	free(dict->d_data);
	dict->d_size = 0;
	dict->d_data = NULL;
	dict->d_alc = 0;
}

const char* typecode_to_name(int t)
{
	switch(t) {
	case TYPECODE_INT:
		return "int";
	case TYPECODE_PAIR:
		return "pair";
	case TYPECODE_BOOL:
		return "bool";
	case TYPECODE_SCOPE:
		return "scope";
	case TYPECODE_SYMBOL:
		return "symbol";
	case TYPECODE_NIL:
		return "nil";
	case TYPECODE_THUNK:
		return "thunk";
	case TYPECODE_STRING:
		return "string";
	case TYPECODE_LAMBDA:
		return "lambda";
	case TYPECODE_NATIVE:
		return "native";
	default:
		fprintf(stderr, "[%s] Unsupported typecode %d\n", __func__, t);
		abort();
	}
}

void __assert_is(const char* funcname, int type, const struct object* obj)
{
	if (obj->o_typecode == type)
		return;
	const char* obj_typename = typecode_to_name(obj->o_typecode);
	fprintf(stderr, "[%s] Type error: expected %s, got %s\n", funcname, typecode_to_name(type), obj_typename);
	abort();
}

void REPLACE(int count, object_t obj)
{
	stack_discard(count);
	push(obj);
}

void stack_duplicate(int index)
{
	push(stack_get(index));
}

bool eval_boolean()
{
	eval(true);

	object_t obj = stack_get(0);
	bool result;

	switch (obj->o_typecode) {
	case TYPECODE_BOOL:
		result = obj->o_value.v_bool;
		break;
	default:
		result = false;
	}
	stack_discard(1);
	return result;
}

bool is_pair(struct object*);

void pair_write(const struct pair* pair, FILE* out)
{
	fputs("(", out);
	push(pair->p_car);
	print_object(out);

	struct object* ptr = pair->p_cdr;
	while (true) {
		if (is_nil(ptr))
			break;

		if (is_pair(ptr)) {
			fputs(" ", out);
			push(pair_car(ptr));
			print_object(out);
			ptr = pair_cdr(ptr);
			continue;
		}

		fputs(" . ", out);
		push(ptr);
		print_object(out);
		break;
	}
	fputs(")", out);
}

void object_dispose(struct object* obj)
{
	switch (obj->o_typecode)
	{
	case TYPECODE_SCOPE:
		scope_dispose(&obj->o_value.v_scope);
		break;
	case TYPECODE_SYMBOL:
		free((char*)obj->o_value.v_symbol);
		break;
	case TYPECODE_STRING:
		free((char*)obj->o_value.v_string);
		break;
	case TYPECODE_LAMBDA:
		{
		struct lambda* lambda = &obj->o_value.v_lambda;
		array_dispose(&lambda->l_params, true);
		free(lambda->l_name);
		}
		break;
	default:
		break;
	}
}

struct object* pair_car(const struct object* pair)
{
	ASSERT_IS(PAIR, pair);
	return pair->o_value.v_pair.p_car;
}

struct object* pair_cdr(const struct object* pair)
{
	ASSERT_IS(PAIR, pair);
	return pair->o_value.v_pair.p_cdr;
}

void pair_setcdr(struct object* pair, struct object* value)
{
	ASSERT_IS(PAIR, pair);
	pair->o_value.v_pair.p_cdr = value;
}

int unbox_int(const struct object* obj)
{
	ASSERT_IS(INT, obj);
	return obj->o_value.v_int;
}

const char* unbox_symbol(const struct object* obj)
{
	ASSERT_IS(SYMBOL, obj);
	return obj->o_value.v_symbol;
}

void* scope_lookup(struct scope* scope, const char* key)
{
	while (true) {
		void* value = dict_lookup(&scope->s_binds, key);
		if (value != NULL)
			return value;
		struct object* parent = scope->s_parent;
		if (parent == NULL)
			return NULL;
		ASSERT_IS(SCOPE, parent);
		scope = &parent->o_value.v_scope;
	}
}

void* scope_get(struct scope* scope, const char* key)
{
	void* result = scope_lookup(scope, key);
	if (result == NULL) {
		fprintf(stderr, "Undefined variable %s\n", key);
		abort();
	}
	return result;
}

void scope_dispose(struct scope* scope)
{
	dict_dispose(&scope->s_binds, DISPOSE_FREE, DISPOSE_KEEP);
}

void scope_init(struct scope* scope, struct object* parent)
{
	if (parent != NULL)
		ASSERT_IS(SCOPE, parent);
	bzero(&scope->s_binds, sizeof(struct dict));
	scope->s_parent = parent;
}

struct object* object_scope_root()
{
	return &DEFAULT_SCOPE;
}

void register_syntax_handler(const char* name, syntax_t handler);

void setup_syntax();
void teardown_syntax();
void setup_stack();
void teardown_stack();

void setup_runtime()
{
	setup_stack();
	setup_syntax();
	DEFAULT_SCOPE.o_typecode = TYPECODE_SCOPE;
	scope_init(&DEFAULT_SCOPE.o_value.v_scope, NULL);
	array_init(&ALL_OBJECTS);
	NIL.o_typecode = TYPECODE_NIL;
	TRUE.o_typecode = TYPECODE_BOOL;
	TRUE.o_value.v_bool = true;
	FALSE.o_typecode = TYPECODE_BOOL;
	FALSE.o_value.v_bool = false;
	register_stdlib_functions();
	execute_file("stdlib.scm");
}

void teardown_runtime()
{
	teardown_stack();
	object_dispose(&DEFAULT_SCOPE);
	collect_garbage();
	array_dispose(&ALL_OBJECTS, false);
	array_dispose(&OBJECT_POOL, true);
	teardown_syntax();
}

void gc_register(struct object* obj)
{
	array_push(&ALL_OBJECTS, obj);
	obj->o_garbage = false;

	static int count = 0;
	count++;
	if (count > 250) {
		collect_garbage();
		count = 0;
	}
}


void collect_garbage()
{
	// printf("[collect]\n");
	for (int i=ALL_OBJECTS.a_size-1; i>=0; i--) {
		struct object* obj = ALL_OBJECTS.a_data[i];
		obj->o_garbage = true;
	}
	scope_reach(&DEFAULT_SCOPE.o_value.v_scope);
	for (int i=STACK.a_size-1; i>=0; i--)
		object_reach(STACK.a_data[i]);
	int count = ALL_OBJECTS.a_size;
	int index = 0;
	while (index < count) {
		struct object* obj = ALL_OBJECTS.a_data[index];
		if (! obj->o_garbage) {
			index++;
			continue;
		}
		object_dispose(obj);
		// printf("[dispose] %s\n", typecode_to_name(obj->o_typecode));
		// free(obj);
		array_push(&OBJECT_POOL, obj);
		ALL_OBJECTS.a_data[index] = ALL_OBJECTS.a_data[--count];
	}
	ALL_OBJECTS.a_size = count;
}

void scope_reach(struct scope* scope)
{
	struct dict* binds = &scope->s_binds;
	for (int i=binds->d_size-1; i>=0; i--)
		object_reach(binds->d_data[i].de_value);
	object_reach(scope->s_parent);
}

void pair_reach(struct pair* pair)
{
	object_reach(pair->p_car);
	object_reach(pair->p_cdr);
}

void thunk_reach(struct thunk* thunk)
{
	object_reach(thunk->t_proc);
}

void lambda_reach(struct lambda* lambda)
{
	object_reach(lambda->l_body);
	object_reach(lambda->l_scope);
}

void object_reach(struct object* obj)
{
	if (obj == NULL)
		return;

	if (! obj->o_garbage)
		return;

	obj->o_garbage = false;

	switch (obj->o_typecode) {
	case TYPECODE_PAIR:
		pair_reach(&obj->o_value.v_pair);
		break;
	case TYPECODE_THUNK:
		thunk_reach(&obj->o_value.v_thunk);
		break;
	case TYPECODE_SCOPE:
		scope_reach(&obj->o_value.v_scope);
		break;
	case TYPECODE_LAMBDA:
		lambda_reach(&obj->o_value.v_lambda);
		break;
	default:
		break;
	}
}

struct object* bool_to_object(bool v)
{
	return v ? &TRUE : &FALSE;
}

void BINDKEY(const char*);

void BIND()
{
	// key value scope
	object_t key = stack_get(2);
	ASSERT_IS(SYMBOL, key);
	BINDKEY(key->o_value.v_symbol);
	// key
	stack_discard(1);
}

void BINDKEY(const char* key)
{
	// value scope
	object_t scope = stack_get(0);
	ASSERT_IS(SCOPE, scope);
	struct dict* binds = &scope->o_value.v_scope.s_binds;

	char* keyptr = strdup(key);
	void* ptr = dict_put(binds, keyptr, stack_get(1));
	if (ptr != NULL) {
		fprintf(stderr, "Rebind of %s\n", keyptr);
		abort();
	}

	stack_discard(2);
}

void make(int type, void* vptr, int argct)
{
	object_t obj;
	if (OBJECT_POOL.a_size > 0) {
		obj = OBJECT_POOL.a_data[--OBJECT_POOL.a_size];
	} else {
		obj = malloc(sizeof(struct object));
	}
	obj->o_typecode = type;
	obj->o_value = *(union value*)vptr;
	stack_discard(argct);
	push(obj);
	gc_register(obj);
}

void push_scope(struct object* parent)
{
	struct scope value;
	scope_init(&value, parent);
	make(TYPECODE_SCOPE, &value, 0);
}

void native_invoke(native_t func, int argct)
{
	func(argct);
}

void make_thunk(int argct)
{
	struct thunk t = {stack_get(0), argct};
	make(TYPECODE_THUNK, &t, 1);
}

void stack_swap(int offset_a, int offset_b);



object_t TOP(void)
{
	return stack_get(0);
}

#include <assert.h>

object_t stack_get(int offset)
{
	int index = STACK.a_size - offset - 1;
	assert(index >= 0);
	return STACK.a_data[index];
}

void SET(int offset, object_t value)
{
	int index = STACK.a_size - offset - 1;
	assert(index >= 0);
	STACK.a_data[index] = value;
}

void stack_car(int offset)
{
	SET(offset, pair_car(stack_get(offset)));
}

void stack_cdr(int offset)
{
	SET(offset, pair_cdr(stack_get(offset)));
}

void syntax_quote(void) /* syntax: quote */
{
	stack_discard(1);
	stack_car(0);
}

void unpack_pair(int offset, bool top_is_car);

void syntax_if(void) /* syntax: if */
{
	// stack is: ... (if-expr then-expr [else-expr]) scope
	unpack_pair(1, true);
	stack_duplicate(1);

	// stack is: ... (then-expr [else-expr]) scope if-expr scope
	if (eval_boolean()) {
		stack_car(1);
		// stack is: ... then-expr scope
		eval(false);
		return;
	}
	stack_cdr(1);
	// stack is: ... [else-expr] scope
	if (is_nil(stack_get(1))) {
		stack_discard(1);
	} else {
		stack_car(1);
		eval(false);
	}
}

void syntax_and(void) /* syntax: and */
{
	object_t scope = stack_get(0);
	object_t exprs = stack_get(1);

	while (is_pair(exprs)) {
		push(pair_car(exprs));
		push(scope);
		if (! eval_boolean()) {
			stack_discard(2);
			push(&FALSE);
			return;
		}
		exprs = pair_cdr(exprs);
	}
	stack_discard(2);
	push(&TRUE);
}

void native_car(int argct)
{
	assert_arg_count("car", argct, 1);
	stack_car(0);
}

void native_cdr(int argct)
{
	assert_arg_count("cdr", argct, 1);
	stack_cdr(0);
}

void native_cons(int argct)
{
	assert_arg_count("cons", argct, 2);
	make_pair(false);
}

void native_eqp(int argct)
{
	assert_arg_count("eq?", argct, 2);
	object_t b = stack_get(0);
	object_t a = stack_get(1);
	bool result = eq(a, b);
	stack_discard(2);
	push_bool(result);
}

void native_list(int argct)
{
	construct_list(argct);
}

void stack_swap(int offset_a, int offset_b)
{
	int last = STACK.a_size - 1;
	int a = last - offset_a;
	int b = last - offset_b;
	object_t t = STACK.a_data[a];
	STACK.a_data[a] = STACK.a_data[b];
	STACK.a_data[b] = t;
}

void native_fold(int argct)
{
	assert_arg_count("fold", argct, 3);
	object_t seq = stack_get(0);
	object_t seed = stack_get(1);
	object_t func = stack_get(2);

	push(seed);
	while (is_pair(seq)) {
		push(pair_car(seq));
		push(func);
		invoke(2);
		seq = pair_cdr(seq);
	}
	stack_swap(0, 3);
	stack_discard(3);
}

struct object* native_load(struct array* args)
{
	assert_arg_count("load", args->a_size, 1);
	struct object* obj = args->a_data[0];
	ASSERT_IS(STRING, obj);
	execute_file(obj->o_value.v_string);
	return &NIL;
}


int POPINT(void)
{
	object_t obj = stack_get(0);
	int result = unbox_int(obj);
	stack_discard(1);
	return result;
}

void native_modulo(int argct)
{
	assert_arg_count("modulo", argct, 2);
	int b = POPINT();
	int a = POPINT();
	push_int(a % b);
}

void native_newline(int argct)
{
	assert_arg_count("newline", argct, 0);
	fputs("\n", stdout);
	push_nil();
}

void native_nullp(int argct)
{
	assert_arg_count("null?", argct, 1);
	bool result = is_nil(stack_get(0));
	stack_discard(1);
	push_bool(result);
}

void native_num_equals(int argct)
{
	assert_arg_count("=", argct, 2);
	int b = POPINT();
	int a = POPINT();
	push_bool(a == b);
}

void native_num_less(int argct)
{
	assert_arg_count("<", argct, 2);
	int b = POPINT();
	int a = POPINT();
	push_bool(a < b);
}

void native_num_minus(int argct)
{
	assert_arg_count("-", argct, 2);
	int b = POPINT();
	int a = POPINT();
	push_int(a - b);
}

void native_num_mult(int argct)
{
	assert_arg_count("*", argct, 2);
	int b = POPINT();
	int a = POPINT();
	push_int(a * b);
}

void native_num_plus(int argct)
{
	assert_arg_count("+", argct, 2);
	int b = POPINT();
	int a = POPINT();
	push_int(a + b);
}

void native_pairp(int argct)
{
	assert_arg_count("pair?", argct, 1);
	object_t obj = stack_get(0);
	bool result = is_pair(obj);
	stack_discard(1);
	push_bool(result);
}

void native_setcdr(int argct)
{
	assert_arg_count("set-cdr!", argct, 2);
	object_t v = stack_get(0);
	object_t pair = stack_get(1);
	pair_setcdr(pair, v);
	stack_discard(2);
	push_nil();
}

bool is_symbol(struct object*);

void native_symbolp(int argct)
{
	assert_arg_count("symbol?", argct, 1);
	push_bool(is_symbol(stack_get(0)));
	stack_swap(0, 1);
	stack_discard(1);
}

void native_reverse(int argct)
{
	assert_arg_count("reverse", argct, 1);
	// [seq]
	push_nil();
	// [seq] ret
	while (is_pair(stack_get(1))) {
		// seq ret
		stack_duplicate(1);
		// seq ret seq
		stack_car(0);
		// seq ret car
		make_pair(true);
		// seq ret
		stack_cdr(1);
		// [seq] ret
	}
	// nil ret
	stack_swap(0, 1);
	stack_discard(1);
}

void native_write(int argct)
{
	assert_arg_count("write", argct, 1);
	print_object(stdout);
	push_nil();
}


void register_native(const char* name, native_t func)
{
	make(TYPECODE_NATIVE, &func, 0);
	push(&DEFAULT_SCOPE);
	BINDKEY(name);
}

void eval_expr(void);

void eval_variable(void)
{
	object_t scope = stack_get(0);
	ASSERT_IS(SCOPE, scope);
	object_t key = stack_get(1);
	ASSERT_IS(SYMBOL, key);
	object_t result = scope_get(&scope->o_value.v_scope, key->o_value.v_symbol);
	stack_discard(2);
	push(result);
}

void eval_thunk(void);


void assert_arg_count(const char* name, int actual, int expected)
{
	if (actual != expected) {
		fprintf(stderr, "[%s] Expected %d arguments for %s, got %d\n", __func__, expected, name, actual);
		abort();
	}
}

void eval_block(void)
{
	object_t code = stack_get(0);
	object_t scope = stack_get(1);

	push_nil();
	while (is_pair(code)) {
		stack_discard(1);
		push(pair_car(code));
		push(scope);
		eval(true);
		code = pair_cdr(code);
	}
	stack_swap(0, 2);
	stack_discard(2);
}

bool is_lambda(struct object* obj);
bool is_native(struct object* obj);
void invoke_lambda(int argct);
native_t unbox_native(struct object* obj);
const char* object_typename(struct object* obj);

void invoke(int argct)
{
	struct object* func = stack_get(0);
	if (is_lambda(func)) {
		invoke_lambda(argct);
	} else if (is_native(func)) {
		native_t funcptr = unbox_native(func);
		stack_discard(1);
		funcptr(argct);
	} else {
		fprintf(stderr, "Error in invoke(): can't invoke object of type %s\n",
						object_typename(func));
		abort();
	}
}

native_t unbox_native(struct object* obj)
{
	assert(is_native(obj));
	return obj->o_value.v_native;
}

void invoke_lambda(int argct)
{
	struct object* top = stack_get(0);
	ASSERT_IS(LAMBDA, top);
	struct lambda* lambda = &top->o_value.v_lambda;
	struct array* params = &lambda->l_params;
	assert_arg_count(lambda->l_name, argct, params->a_size);

	// value_1 value_2 lambda
	push_scope(lambda->l_scope);
	// value_1 value_2 lambda scope
	for (int i=params->a_size - 1; i>=0; i--) {
		stack_swap(0, 1);
		// value_1 value_2 scope lambda
		stack_swap(0, 2);
		// value_1 lambda scope value_2
		stack_duplicate(1);
		// value_1 lambda scope value_2 scope
		BINDKEY(params->a_data[i]);
		// value_1 lambda scope
	}
	// lambda scope
	push(lambda->l_body);
	// lambda scope code
	stack_swap(0, 2);
	// code scope lambda
	stack_discard(1);
	stack_swap(0, 1);
	// scope code
	eval_block();
}

void eval_thunk()
{
	object_t top = TOP();
	ASSERT_IS(THUNK, top);
	struct thunk* t = &top->o_value.v_thunk;
	int argct = t->t_argct;
	SET(0, t->t_proc);
	invoke(argct);
}

bool eq(struct object* x, struct object* y)
{
	while (true) {
		if (x == y)
			return true;

		if (IS(PAIR, x) && IS(PAIR, y)) {
			if (! eq(pair_car(x), pair_car(y)))
				return false;
			x = pair_cdr(x);
			y = pair_cdr(y);
			continue;
		}

		if (IS(SYMBOL, x) && IS(SYMBOL, y))
			return strcmp(unbox_symbol(x), unbox_symbol(y)) == 0;

		fprintf(stderr, "[%s] Don't know how to eq? %s against %s\n",
			__func__, typecode_to_name(x->o_typecode), typecode_to_name(y->o_typecode));
		abort();
	}
}

bool object_is_symbol(struct object* v, const char* text)
{
	if (IS(SYMBOL, v))
		return strcmp(text, unbox_symbol(v)) == 0;
	return false;
}

void name_lambda(struct lambda* l, const char* name)
{
	if (l->l_name != NULL)
		free(l->l_name);
	l->l_name = strdup(name);
}

void eval_define(void)
{
	// key scope expr
	stack_duplicate(1);
	// key scope expr scope
	eval(true);
	// key scope value
	object_t v = stack_get(0);
	if (IS(LAMBDA, v)) {
		object_t name = stack_get(2);
		ASSERT_IS(SYMBOL, name);
		name_lambda(&v->o_value.v_lambda, name->o_value.v_symbol);
	}
	stack_swap(0, 1);
	// key value scope
	BIND();
}

void unpack_pair(int offset, bool top_is_car)
{
	stack_duplicate(offset);
	if (top_is_car) {
		stack_car(0);
		stack_cdr(offset + 1);
	} else {
		stack_cdr(0);
		stack_car(offset + 1);
	}
}

void LAMBDA(void)
{
	// [params] body scope
	struct lambda lambda = {
		stack_get(1),
		stack_get(0),
		NULL,
		{ NULL, 0, 0 }
	};
	stack_swap(0, 2);
	// scope body [params]
	while (! is_nil(stack_get(0))) {
		// scope body (param [params])
		unpack_pair(0, true);
		// scope body [params] param
		const char* name = unbox_symbol(stack_get(0));
		array_push(&lambda.l_params, strdup(name));
		stack_discard(1);
	}
	stack_discard(1);
	make(TYPECODE_LAMBDA, &lambda, 2);
}

void syntax_cond(void) /* syntax: cond */
{
	// [code] scope
	stack_swap(0, 1);
	// scope [code]
	while (! is_nil(stack_get(0))) {
		// scope code
		unpack_pair(0, true);
		// scope [code] (test expr)
		unpack_pair(0, true);
		// scope [code] expr test
		if (object_is_symbol(stack_get(0), "else")) {
			stack_discard(1);
			// scope [code] expr
			stack_swap(0, 1);
			// scope expr [code]
			stack_discard(1);
			// scope expr
			eval_block();
			return;
		}
		stack_duplicate(3);
		// scope [code] expr test scope
		if (eval_boolean()) {
			// scope [code] expr
			stack_swap(0, 1);
			// scope expr [code]
			stack_discard(1);
			// scope expr
			eval_block();
			return;
		}
		// scope [code] expr
		stack_discard(1);
		// scope [code]
	}
	stack_discard(1);
	push_nil();
}

void syntax_define(void) /* syntax: define */
{
	// code scope
	object_t code = stack_get(1);
	object_t head = pair_car(code);

	if (IS(SYMBOL, head)) {
		push(head);
		// code scope key
		stack_swap(0, 2);
		// key scope code
		stack_cdr(0);
		stack_car(0);
		// key scope expr
		eval_define();
		push_nil();
		return;
	}

	if (IS(PAIR, head)) {
		// code scope
		struct object* args = pair_cdr(head);
		push(args);
		// code scope [params]
		push(pair_car(head));
		// code scope [params] key
		struct object* body = pair_cdr(code);
		push(body);
		// code scope [params] key body
		stack_swap(0, 1);
		// code scope [params] body key
		stack_swap(0, 4);
		// key scope [params] body code
		stack_discard(1);
		// key scope [params] body
		stack_duplicate(2);
		// key scope [params] body scope
		LAMBDA();
		// key scope expr
		eval_define();
		push_nil();
		return;
	}

	fprintf(stderr, "[%s] Syntax error, define is not implemented for %s\n", __func__, typecode_to_name(head->o_typecode));
	abort();
}

void syntax_letrec(void) /* syntax: letrec */
{
	// (bindings code) super_scope
	stack_duplicate(0);
	// (bindings code) super_scope scope
	stack_swap(0, 1);
	// (bindings code) scope super_scope
	stack_discard(1);
	// (bindings code) scope
	stack_swap(0, 1);
	// scope (bindings code)
	unpack_pair(0, true);
	// scope code [bindings]
	while (! is_nil(stack_get(0))) {
		// scope code bindings
		unpack_pair(0, true);
		// scope code [bindings] binding
		unpack_pair(0, false);
		// scope code [bindings] key (expr)
		push(pair_car(stack_get(0)));
		// scope code [bindings] key (expr) expr
		stack_swap(0, 1);
		// scope code [bindings] key expr (expr)
		stack_discard(1);
		// scope code [bindings] key expr
		stack_duplicate(4);
		// scope code [bindings] key expr scope
		stack_swap(0, 1);
		// scope code [bindings] key scope expr
		eval_define();
		// scope code [bindings]
	}
	stack_discard(0);
	// scope code
	eval_block();
}

void syntax_lambda(void) /* syntax: lambda */
{
	object_t scope = stack_get(0);
	object_t code = stack_get(1);
	object_t params = pair_car(code);
	object_t body = pair_cdr(code);

	push(scope);
	SET(1, body);
	SET(2, params);
	LAMBDA();
}


static struct dict SYNTAX_HANDLERS = {NULL, 0, 0};

void register_syntax_handler(const char* name, syntax_t syntax)
{
	dict_put(&SYNTAX_HANDLERS, (char*)name, syntax);
}

void teardown_syntax()
{
	dict_dispose(&SYNTAX_HANDLERS, DISPOSE_KEEP, DISPOSE_KEEP);
}

syntax_t lookup_syntax_handler(const char* name)
{
	return dict_lookup(&SYNTAX_HANDLERS, name);
}

void stack_rotate(int offset)
{
	while (offset > 0) {
		stack_swap(0, offset);
		offset--;
	}
}


void print_object(FILE* out)
{
	object_t obj = stack_get(0);
	switch (obj->o_typecode)
	{
	case TYPECODE_BOOL:
		fputs(obj->o_value.v_bool ? "#t" : "#f", out);
		break;
	case TYPECODE_INT:
		fprintf(out, "%d", obj->o_value.v_int);
		break;
	case TYPECODE_SYMBOL:
		fprintf(out, "%s", obj->o_value.v_symbol);
		break;
	case TYPECODE_NIL:
		fputs("()", out);
		break;
	case TYPECODE_PAIR:
		pair_write(&obj->o_value.v_pair, out);
		break;
	default:
		fprintf(out, "[%s]", typecode_to_name(obj->o_typecode));
		break;
	}
	stack_discard(1);
}

void setup_stack()
{
	array_init(&STACK);
}

void teardown_stack()
{
	array_dispose(&STACK, false);
}

void stack_discard(int count)
{
	assert(STACK.a_size >= count);
	STACK.a_size -= count;
}

void register_stdlib_functions()
{
	register_native("car", native_car);
	register_native("cdr", native_cdr);
	register_native("cons", native_cons);
	register_native("eq?", native_eqp);
	register_native("fold", native_fold);
	register_native("list", native_list);
//	register_native("load", native_load);
	register_native("modulo", native_modulo);
	register_native("newline", native_newline);
	register_native("null?", native_nullp);
	register_native("=", native_num_equals);
	register_native("<", native_num_less);
	register_native("-", native_num_minus);
	register_native("*", native_num_mult);
	register_native("+", native_num_plus);
	register_native("pair?", native_pairp);
	register_native("set-cdr!", native_setcdr);
	register_native("symbol?", native_symbolp);
	register_native("reverse", native_reverse);
	register_native("write", native_write);
}


bool parse_and_push_bool(const char*);
bool parse_and_push_int(const char*);

bool is_nil(struct object* obj)
{
	return obj->o_typecode == TYPECODE_NIL;
}

bool is_pair(struct object* obj)
{
	return obj->o_typecode == TYPECODE_PAIR;
}

bool is_symbol(struct object* obj)
{
	return obj->o_typecode == TYPECODE_SYMBOL;
}

bool is_thunk(struct object* obj)
{
	return obj->o_typecode == TYPECODE_THUNK;
}

bool is_lambda(struct object* obj)
{
	return obj->o_typecode == TYPECODE_LAMBDA;
}

bool is_native(struct object* obj)
{
	return obj->o_typecode == TYPECODE_NATIVE;
}

const char* object_typename(struct object* obj)
{
	return typecode_to_name(obj->o_typecode);
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
