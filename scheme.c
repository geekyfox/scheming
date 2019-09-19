#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "scheme.h"

void setup_runtime();
void teardown_runtime();

int main(int argc, const char** argv)
{
	setup_runtime();
	if (argc < 2) {
		repl(stdin, true);
	} else {
		for (int i=1; i<argc; i++)
			load(argv[i]);
	}
	teardown_runtime();
}

// CUTOFF

static struct object ROOT_SCOPE;
static struct array ALL_OBJECTS;
static struct object NIL;
static struct object TRUE;
static struct object FALSE;

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

void array_dispose(struct array* arr, enum dispose_mode mode)
{
	if (mode == DISPOSE_FREE)
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

const char* typecode_to_name(enum typecode t)
{
	switch(t) {
	case TYPECODE_GENERIC:
		return "generic";
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

void __assert_is(const char* funcname, enum typecode type, const struct object* obj)
{
	if (obj->o_typecode == type)
		return;
	const char* obj_typename = typecode_to_name(obj->o_typecode);
	fprintf(stderr, "[%s] Type error: expected %s, got %s\n", funcname, typecode_to_name(type), obj_typename);
	abort();
}

bool object_to_bool(const struct object* obj)
{
	switch (obj->o_typecode) {
	case TYPECODE_BOOL:
		return obj->o_value.v_bool;
	default:
		return false;
	}
}

void pair_write(const struct pair* pair, FILE* out)
{
	fputs("(", out);
	object_write(pair->p_car, out);

	struct object* ptr = pair->p_cdr;
	while (true) {
		if (IS(NIL, ptr))
			break;
		
		if (IS(PAIR, ptr)) {
			fputs(" ", out);
			object_write(pair_car(ptr), out);
			ptr = pair_cdr(ptr);
			continue;
		}

		fputs(" . ", out);
		object_write(ptr, out);
		break;
	}
	fputs(")", out);
}

void object_write(const struct object* obj, FILE* out)
{
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
		array_dispose(&lambda->l_params, DISPOSE_FREE);
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

void object_scope_init(struct object* obj, struct object* parent)
{
	obj->o_typecode = TYPECODE_SCOPE;
	scope_init(&obj->o_value.v_scope, parent);
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
	return &ROOT_SCOPE;
}

void register_syntax_handlers();
void teardown_syntax_handlers();

void setup_runtime()
{
	register_syntax_handlers();
	object_scope_init(&ROOT_SCOPE, NULL);
	array_init(&ALL_OBJECTS);
	NIL.o_typecode = TYPECODE_NIL;
	TRUE.o_typecode = TYPECODE_BOOL;
	TRUE.o_value.v_bool = true;
	FALSE.o_typecode = TYPECODE_BOOL;
	FALSE.o_value.v_bool = false;
	register_stdlib_functions();
	load("stdlib.scm");
}

void teardown_runtime()
{
	object_dispose(&ROOT_SCOPE);
	gc_collect();
	array_dispose(&ALL_OBJECTS, DISPOSE_KEEP);
	teardown_syntax_handlers();
}

void gc_register(struct object* obj)
{
	array_push(&ALL_OBJECTS, obj);
	obj->o_garbage = false;
}

void gc_collect()
{
	for (int i=ALL_OBJECTS.a_size-1; i>=0; i--) {
		struct object* obj = ALL_OBJECTS.a_data[i];
		obj->o_garbage = true;
	}
	scope_reach(&ROOT_SCOPE.o_value.v_scope);
	int count = ALL_OBJECTS.a_size;
	int index = 0;
	while (index < count) {
		struct object* obj = ALL_OBJECTS.a_data[index];
		if (! obj->o_garbage) {
			index++;
			continue;
		}
		object_dispose(obj);
		free(obj);
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

struct object* object_nil()
{
	return &NIL;
}

struct object* bool_to_object(bool v)
{
	return v ? &TRUE : &FALSE;
}

void object_scope_bind(struct object* scope, const char* key,
		       struct object* value)
{
	ASSERT_IS(SCOPE, scope);
	scope_bind(&scope->o_value.v_scope, key, value);
}

void scope_bind(struct scope* scope, const char* key, struct object* value)
{
	char* keyptr = strdup(key);
	void* ptr = dict_put(&scope->s_binds, keyptr, value);
	if (ptr != NULL)
	{
		fprintf(stderr, "Rebind of %s\n", keyptr);
		abort();
	}
}

struct object* object_scope_create(struct object* parent)
{
	struct object* obj = malloc(sizeof(struct object));
	object_scope_init(obj, parent);
	gc_register(obj);
	return obj;
}

struct object* object_scope_get(struct object* obj, const char* key)
{
	ASSERT_IS(SCOPE, obj);
	return scope_get(&obj->o_value.v_scope, key);
}

struct object* native_invoke(native_t func, struct array* args)
{
	struct object* result = func(args);
	array_dispose(args, DISPOSE_KEEP);
	free(args);
	return result;
}

struct object* eval_funcall(struct object* scope, const char* name,
			    struct array* args)
{
	struct object* proc = object_scope_get(scope, name);

	switch (proc->o_typecode) {
	case TYPECODE_LAMBDA:
		return object_thunk_create(proc, args);
	case TYPECODE_NATIVE:
		return native_invoke(proc->o_value.v_native, args);
	default:
		fprintf(stderr,
			"[%s] Type error: expected native or lambda, got %s\n",
			__func__, typecode_to_name(proc->o_typecode));
		abort();
	}
}

struct object* object_thunk_create(struct object* proc, struct array* args)
{
	struct object* obj = malloc(sizeof(struct object));
	obj->o_typecode = TYPECODE_THUNK;
	obj->o_value.v_thunk.t_proc = proc;
	obj->o_value.v_thunk.t_args = args;
	gc_register(obj);
	return obj;
}

struct object* syntax_quote(struct object* scope, struct object* code)
{
	return pair_car(code);
}

struct object* syntax_if(struct object* scope, struct object* code)
{
	if (eval_flag(scope, pair_car(code)))
		return eval(scope, pair_car(pair_cdr(code)));
	struct object* cs = pair_cdr(pair_cdr(code));
	if (IS(NIL, cs))
		return object_nil();
	return eval(scope, pair_car(cs));
}

void repl(FILE* input, bool istty)
{
	while (true) {
		if (istty)
			printf("> ");
		struct object* in = read_object(input, true);
		if (in == NULL) {
			if (istty)
				printf("bye\n");
			return;
		}
		struct object* out = eval_eager(eval(&ROOT_SCOPE, in));
		if (istty) {
			object_write(out, stdout);
			printf("\n");
		} 
		gc_collect();
		fflush(stdout);
	}
}

bool eval_flag(struct object* scope, struct object* expr)
{
	struct object* result = eval_eager(eval(scope, expr));
	return object_to_bool(result);
}

struct object* syntax_and(struct object* scope, struct object* code)
{
	bool result = true;
	while (result && IS(PAIR, code)) {
		result = eval_flag(scope, pair_car(code));
		code = pair_cdr(code);
	}
	return bool_to_object(result);
}

struct object* native_car(struct array* args)
{
	assert_arg_count("car", args->a_size, 1);
	return pair_car(args->a_data[0]);
}

struct object* native_cdr(struct array* args)
{
	assert_arg_count("cdr", args->a_size, 1);
	return pair_cdr(args->a_data[0]);
}

struct object* native_cons(struct array* args)
{
	assert_arg_count("cons", args->a_size, 2);
	return make_pair(args->a_data[0], args->a_data[1]);
}

struct object* native_eqp(struct array* args)
{
	assert_arg_count("eq?", args->a_size, 2);
	return bool_to_object(eq(args->a_data[0], args->a_data[1]));
}

struct object* native_fold(struct array* args)
{
	assert_arg_count("fold", args->a_size, 3);
	struct object* proc = args->a_data[0];
	struct object* value = args->a_data[1];
	struct object* seq = args->a_data[2];

	while (IS(PAIR, seq)) {
		struct array* tmp = calloc(1, sizeof(struct array));
		array_push(tmp, value);
		array_push(tmp, pair_car(seq));
		value = call(proc, tmp);
		seq = pair_cdr(seq);
	}
	return value;
}

struct object* native_list(struct array* args)
{
	struct object* result = &NIL;
	for (int i=args->a_size-1; i>=0; i--)
		result = make_pair(args->a_data[i], result);
	return result;
}

struct object* native_load(struct array* args)
{
	assert_arg_count("load", args->a_size, 1);
	struct object* obj = args->a_data[0];
	ASSERT_IS(STRING, obj);
	load(obj->o_value.v_string);
	return &NIL;
}

struct object* native_modulo(struct array* args)
{
	assert_arg_count("modulo", args->a_size, 2);
	int a = unbox_int(args->a_data[0]);
	int b = unbox_int(args->a_data[1]);
	return int_to_object(a % b);
}

struct object* native_newline(struct array* args)
{
	assert_arg_count("newline", args->a_size, 0);
	fputs("\n", stdout);
	return &NIL;
}

struct object* native_nullp(struct array* args)
{
	assert_arg_count("null?", args->a_size, 1);
	struct object* x = args->a_data[0];
	return bool_to_object(IS(NIL, x));
}

struct object* native_num_equals(struct array* args)
{
	assert_arg_count("=", args->a_size, 2);
	int a = unbox_int(args->a_data[0]);
	int b = unbox_int(args->a_data[1]);
	return bool_to_object(a == b);
}

struct object* native_num_less(struct array* args)
{
	assert_arg_count("<", args->a_size, 2);
	int a = unbox_int(args->a_data[0]);
	int b = unbox_int(args->a_data[1]);
	return bool_to_object(a < b);
}

struct object* native_num_minus(struct array* args)
{
	assert_arg_count("-", args->a_size, 2);
	int a = unbox_int(args->a_data[0]);
	int b = unbox_int(args->a_data[1]);
	return int_to_object(a - b);
}

struct object* native_num_mult(struct array* args)
{
	assert_arg_count("*", args->a_size, 2);
	int a = unbox_int(args->a_data[0]);
	int b = unbox_int(args->a_data[1]);
	return int_to_object(a * b);
}

struct object* native_num_plus(struct array* args)
{
	assert_arg_count("+", args->a_size, 2);
	int a = unbox_int(args->a_data[0]);
	int b = unbox_int(args->a_data[1]);
	return int_to_object(a + b);
}

struct object* native_pairp(struct array* args)
{
	assert_arg_count("pair?", args->a_size, 1);
	struct object* x = args->a_data[0];
	return bool_to_object(IS(PAIR, x));
}

struct object* native_setcdr(struct array* args)
{
	assert_arg_count("set-cdr!", args->a_size, 2);
	pair_setcdr(args->a_data[0], args->a_data[1]);
	return &NIL;
}

struct object* native_symbolp(struct array* args)
{
	assert_arg_count("symbol?", args->a_size, 1);
	struct object* x = args->a_data[0];
	return bool_to_object(IS(PAIR, x));
}

struct object* native_reverse(struct array* args)
{
	assert_arg_count("reverse", args->a_size, 1);
	struct object* src = args->a_data[0];
	struct object* ret = &NIL;

	while (IS(PAIR, src)) {
		ret = make_pair(pair_car(src), ret);
		src = pair_cdr(src);
	}
	return ret;
}

struct object* native_write(struct array* args)
{
	assert_arg_count("write", args->a_size, 1);
	object_write(args->a_data[0], stdout);
	return &NIL;
}

void load(const char* filename)
{
	FILE* handle = fopen(filename, "r");
	if (handle == NULL) {
		fprintf(stderr, "[%s] Error opening file %s: %s\n", __func__, filename, strerror(errno));
		abort();
	}
	repl(handle, false);
	fclose(handle);
	gc_collect();
}

void register_native(const char* name, native_t func)
{
	struct object* obj = malloc(sizeof(struct object));
	obj->o_typecode = TYPECODE_NATIVE;
	obj->o_value.v_native = func;
	gc_register(obj);
	object_scope_bind(&ROOT_SCOPE, name, obj);
}

void register_stdlib_functions()
{
	register_native("car", native_car);
	register_native("cdr", native_cdr);
	register_native("cons", native_cons);
	register_native("eq?", native_eqp);
	register_native("fold", native_fold);
	register_native("list", native_list);
	register_native("load", native_load);
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

struct object* eval(struct object* scope, struct object* expr)
{
	if (IS(PAIR, expr)) {
		const char* keyword = unbox_symbol(pair_car(expr));
		return eval_expr(scope, keyword, pair_cdr(expr));
	}

	if (IS(SYMBOL, expr))
		return object_scope_get(scope, unbox_symbol(expr));
	
	return expr;
}

void assert_arg_count(const char* name, int actual, int expected)
{
	if (actual != expected) {
		fprintf(stderr, "[%s] Expected %d arguments for %s, got %d\n", __func__, expected, name, actual);
		abort();
	}
}

struct object* call(struct object* func, struct array* args)
{
	if (IS(NATIVE, func)) {
		struct object* result = func->o_value.v_native(args);
		array_dispose(args, DISPOSE_KEEP);
		free(args);
		return result;
	}

	ASSERT_IS(LAMBDA, func);
	struct lambda* lambda = &func->o_value.v_lambda;
	assert_arg_count(lambda->l_name, args->a_size, lambda->l_params.a_size);
	struct object* exec_scope = object_scope_create(lambda->l_scope);
	for (int i=lambda->l_params.a_size - 1; i>=0; i--)
		object_scope_bind(exec_scope, lambda->l_params.a_data[i], args->a_data[i]);
	array_dispose(args, DISPOSE_KEEP);
	free(args);
	return eval_block(exec_scope, lambda->l_body);
}

struct object* make_pair(struct object* car, struct object* cdr)
{
	struct object* obj = malloc(sizeof(struct object));
	obj->o_typecode = TYPECODE_PAIR;
	obj->o_value.v_pair.p_car = car;
	obj->o_value.v_pair.p_cdr = cdr;
	gc_register(obj);
	return obj;
}

struct object* int_to_object(int n)
{
	struct object* obj = malloc(sizeof(struct object));
	obj->o_typecode = TYPECODE_INT;
	obj->o_value.v_int = n;
	gc_register(obj);
	return obj;
}

struct object* eval_thunk(struct thunk* t)
{
	struct object* proc = t->t_proc;
	struct array* args = t->t_args;
	return call(proc, args);
}

struct object* eval_eager(struct object* x)
{
	while (IS(THUNK, x)) 
		x = eval_thunk(&x->o_value.v_thunk);
	return x;
}

struct object* eval_block(struct object* scope, struct object* code)
{
	struct object* result = &NIL;
	while (IS(PAIR, code)) {
		result = eval(scope, pair_car(code));
		code = pair_cdr(code);
	}
	return result;
}

bool eq(struct object* x, struct object* y)
{
	while (true) {
		if (x == y)
			return true;
		
		if (IS(PAIR, x) && IS(PAIR, y)) {
			if (! eq(pair_car(x), pair_car(y)))
				return false;
			x = pair_car(x);
			y = pair_car(y);
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

static int _readchar(FILE* in, bool die_on_eof)
{
	int ch = fgetc(in);
	if (ch != EOF)
		return ch;
	if (feof(in)) {
		if (die_on_eof)
		{
			fprintf(stderr, "Unexpected end of input\n");
			abort();
		}
		return EOF;
	}
	fprintf(stderr, "IO error: %s\n", strerror(errno));
	abort();
}

static int _readchar_or_die(FILE* in)
{
	return _readchar(in, true);
}

struct object* read_number(FILE* in, char ch)
{
	int accum = 0;
	do {
		accum = accum * 10 + (ch - '0');
		ch = _readchar_or_die(in);
	} while (isdigit(ch));
	ungetc(ch, in);
	return int_to_object(accum);
}

struct object* read_string(FILE* in)
{
	char buff[10240];
	char *fill = buff;
	
	while (true) {
		int ch = _readchar_or_die(in);
		if (ch == '"') {
			*fill = '\0';
			return wrap_string(buff);
		} else {
			*(fill++) = ch;
		}
	}
}

struct object* read_quote(FILE* in)
{
	struct object* v = read_object(in, false);
	v = make_pair(v, &NIL);
	v = make_pair(wrap_symbol("quote"), v);
	return v;
}

static void _skip_until_eol(FILE* in, bool toplevel)
{
	int ch;
	do {
		ch = _readchar(in, !toplevel);
	} while ((ch != EOF) && (ch != '\n'));
}

static bool _is_first_atomic_char(char ch)
{
	switch (ch) {
	case '+': case '-': case '*': case '?': case '=': case '<':
		return true;
	}
	
	if (isalpha(ch))
		return true;
	return false;
}

static bool _is_inner_atomic_char(char ch)
{
	if(_is_first_atomic_char(ch))
		return true;
	if(ch == '!')
		return true;
	return false;
}

struct object* read_symbol(FILE* in, char ch)
{
	char buff[10240];
	char *fill = buff;

	do {
		*(fill++) = ch;
		ch = _readchar_or_die(in);
	} while (_is_inner_atomic_char(ch));

	*fill = '\0';
	ungetc(ch, in);
	return wrap_symbol(buff);
}

struct object* read_list(FILE* in)
{
	struct object* first = &NIL;
	struct object* last = NULL;

	while (true) {
		struct object* next = read_object(in, false);
		if (next == NULL) {
			break;
		} else if (last == NULL) {
			last = make_pair(next, &NIL);
			first = last;
		} else {
			pair_setcdr(last, make_pair(next, object_nil()));
			last = pair_cdr(last);
		}
	}
	return first;
}

struct object* read_bool(FILE* in)
{
	char ch = _readchar_or_die(in);
	if (ch == 'f') {
		return bool_to_object(false);
	} else if (ch == 't') {
		return bool_to_object(true);
	} else {
		fprintf(stderr, "[%s] Unexpected value for boolean symbol %c %d\n", __func__, ch, ch);
		abort();
	}
}

struct object* read_object(FILE* in, bool toplevel)
{
	while (true)
	{
		int ch = _readchar(in, !toplevel);
		if (ch == EOF)
			return NULL;
		if (ch == '(')
			return read_list(in);
		if (_is_first_atomic_char(ch))
			return read_symbol(in, ch);
		if ((ch == ' ') || (ch == '\r') || (ch == '\n') || (ch == '\t'))
			continue;
		if (ch == ';') {
			_skip_until_eol(in, toplevel);
			continue;
		}
		if ((ch == ')') && (!toplevel))
			return NULL;
		if (ch == '\'')
			return read_quote(in);
		if (ch == '"')
			return read_string(in);
		if (isdigit(ch))
			return read_number(in, ch);
		if (ch == '#')
			return read_bool(in);
		fprintf(stderr, "[%s] Unexpected input %c %d\n", __func__, ch, ch);
		abort();
	}
}

void name_lambda(struct lambda* l, const char* name)
{
	if (l->l_name != NULL)
		free(l->l_name);
	l->l_name = strdup(name);
}

void eval_define(struct object* scope, const char* name, struct object* expr)
{
	struct object* v = eval(scope, expr);
	if (IS(LAMBDA, v))
		name_lambda(&v->o_value.v_lambda, name);
	object_scope_bind(scope, name, v);
}

struct object* make_lambda(struct object* params, struct object* body, struct object* scope)
{
	struct object* obj = malloc(sizeof(struct object));
	obj->o_typecode = TYPECODE_LAMBDA;

	struct lambda* lambda = &obj->o_value.v_lambda;

	lambda->l_body = body;
	lambda->l_scope = scope;
	lambda->l_name = NULL;

	bzero(&lambda->l_params, sizeof(struct array));
	while (IS(PAIR, params)) {
		const char* name = unbox_symbol(pair_car(params));
		array_push(&lambda->l_params, strdup(name));
		params = pair_cdr(params);
	}

	gc_register(obj);

	return obj;
}

struct object* syntax_cond(struct object* scope, struct object* code)
{
	while (IS(PAIR, code)) {
		struct object* clause = pair_car(code);
		struct object* test = pair_car(clause);
		if (object_is_symbol(test, "else") || eval_flag(scope, test))
			return eval_block(scope, pair_cdr(clause));
		code = pair_cdr(code);
	}
	return object_nil();
}

struct object* syntax_define(struct object* scope, struct object* code)
{
	struct object* head = pair_car(code);
	
	if (IS(SYMBOL, head)) {
		const char* key = unbox_symbol(head);
		struct object* expr = pair_car(pair_cdr(code));
		eval_define(scope, key, expr);
		return &NIL;
	}
	
	if (IS(PAIR, head)) {
		const char* key = unbox_symbol(pair_car(head));
		struct object* args = pair_cdr(head);
		struct object* body = pair_cdr(code);
		struct object* lambda = make_lambda(args, body, scope);
		eval_define(scope, key, lambda);
		return &NIL;
	}
	
	fprintf(stderr, "[%s] Syntax error, define is not implemented for %s\n", __func__, typecode_to_name(head->o_typecode));
	abort();
}

struct object* syntax_letrec(struct object* super_scope, struct object* code)
{
	struct object* scope = object_scope_create(super_scope);
	struct object* bindings = pair_car(code);
	while (IS(PAIR, bindings)) {
		struct object* binding = pair_car(bindings);
		const char* key = unbox_symbol(pair_car(binding));
		struct object* expr = pair_car(pair_cdr(binding));
		eval_define(scope, key, expr);
		bindings = pair_cdr(bindings);
	}
	return eval_block(scope, pair_cdr(code));
}

struct object* syntax_lambda(struct object* scope, struct object* code)
{
	return make_lambda(pair_car(code), pair_cdr(code), scope);
}

typedef struct object* (*syntax_t)(struct object*, struct object*);

static struct dict SYNTAX_HANDLERS;

void register_syntax_handlers()
{
	struct dict* d = &SYNTAX_HANDLERS;	

	bzero(d, sizeof(struct dict));
	dict_put(d, (char*)"and",    syntax_and);
	dict_put(d, (char*)"cond",   syntax_cond);
	dict_put(d, (char*)"define", syntax_define);
	dict_put(d, (char*)"if",     syntax_if);
	dict_put(d, (char*)"lambda", syntax_lambda);
	dict_put(d, (char*)"letrec", syntax_letrec);
	dict_put(d, (char*)"quote",  syntax_quote);
}

void teardown_syntax_handlers()
{
	dict_dispose(&SYNTAX_HANDLERS, DISPOSE_KEEP, DISPOSE_KEEP);
}
	
struct object* eval_expr(struct object* scope, const char* name, struct object* expr)
{
	syntax_t rule = dict_lookup(&SYNTAX_HANDLERS, name);
	if (rule != NULL)
		return rule(scope, expr);
	
	struct array* args = calloc(1, sizeof(struct array));
	while (IS(PAIR, expr)) {
		struct object* v = eval(scope, pair_car(expr));
		v = eval_eager(v);
		array_push(args, v);
		expr = pair_cdr(expr);
	}
	return eval_funcall(scope, name, args);
}

struct object* wrap_symbol(const char* text)
{
	struct object* obj = malloc(sizeof(struct object));
	obj->o_typecode = TYPECODE_SYMBOL;
	obj->o_value.v_symbol = strdup(text);
	gc_register(obj);
	return obj;
}

struct object* wrap_string(const char* text)
{
	struct object* obj = malloc(sizeof(struct object));
	obj->o_typecode = TYPECODE_STRING;
	gc_register(obj);
	obj->o_value.v_string = strdup(text);
	return obj;
}

