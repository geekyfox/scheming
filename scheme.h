#ifndef SCHEME_HEADER
#define SCHEME_HEADER

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

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

struct pair {
	struct object* p_car;
	struct object* p_cdr;
};

struct scope {
	struct dict s_binds;
	struct object* s_parent;
};

struct thunk {
	struct object* t_proc;
	struct array* t_args;
};

struct lambda {
	struct object* l_body;
	struct object* l_scope;
	char* l_name;
	struct array l_params;
};

typedef struct object* (*native_t)(struct array*);

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

enum typecode {
	TYPECODE_GENERIC = 0,
	TYPECODE_PAIR    = 42000,
	TYPECODE_INT     = 42001,
	TYPECODE_BOOL    = 42002,
	TYPECODE_SCOPE   = 42003,
	TYPECODE_SYMBOL  = 42004,
	TYPECODE_NIL     = 42005,
	TYPECODE_THUNK   = 42006,
	TYPECODE_STRING  = 42007,
	TYPECODE_LAMBDA  = 42008,
	TYPECODE_NATIVE  = 42009,
};

struct object {
	enum typecode o_typecode;
	union value o_value;
	bool o_garbage;
};


void* dict_put(struct dict*, const char*, void*);
void* dict_lookup(struct dict*, const char*);
void dict_dispose(struct dict*, enum dispose_mode, enum dispose_mode);

#define IS(type, var) ((var)->o_typecode == TYPECODE_##type)
#define ASSERT_IS(type, var) __assert_is(__func__, TYPECODE_##type, var)
void __assert_is(const char* funcname, enum typecode, const struct object*);

const char* typecode_to_name(enum typecode);

bool object_to_bool(const struct object* obj);
struct object* bool_to_object(bool);

void object_write(const struct object*, FILE*);
void object_dispose(struct object*);
void object_reach(struct object*);

void array_init(struct array*);
void array_dispose(struct array*, enum dispose_mode);
void array_push(struct array*, void*);

void           interpreter_setup();
void           interpreter_teardown();

void           gc_register(struct object*);
void           gc_collect();

struct object* object_nil();

void object_scope_bind(struct object* scope, const char* key,
		       struct object* value);
struct object* object_scope_create(struct object* scope);
void object_scope_init(struct object* obj, struct object* parent);
struct object* object_scope_root();
struct object* object_scope_get(struct object*, const char*);

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
const char* unbox_symbol(const struct object*);

struct object* eval(struct object* scope, struct object* expr);
struct object* eval_funcall(struct object*, const char*, struct array*);
struct object* eval_quote(struct object* scope, struct object* code);
struct object* eval_eager(struct object* x);
bool eval_flag(struct object* scope, struct object* expr);
struct object* eval_if(struct object* scope, struct object* code);
struct object* eval_and(struct object* scope, struct object* code);
void register_stdlib_functions();
void load(const char*);
void repl(FILE*, bool istty);	
struct object* read_object(FILE*, bool);
struct object* stdlib_nullp(struct array* args);
void assert_arg_count(const char* name, int actual, int expected);
struct object* eval_expr(struct object* scope, const char* name, struct object* expr);
struct object* int_to_object(int);
struct object* call(struct object* func, struct array* args);
struct object* make_pair(struct object* car, struct object* cdr);
bool eq(struct object* x, struct object* y);
struct object* eval_block(struct object* scope, struct object* expr);
bool object_is_symbol(struct object* obj, const char* name);
struct object* wrap_symbol(const char*);
struct object* wrap_string(const char*);
void name_lambda(struct lambda* l, const char* name);

#ifdef __cplusplus
}
#endif

#endif

