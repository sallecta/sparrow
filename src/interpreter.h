/* File: General
 * Things defined in interpreter.h.
 */
#ifndef interpreter_H
#define interpreter_H

#include <setjmp.h>
#include <sys/stat.h>
#ifndef __USE_ISOC99
#define __USE_ISOC99
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#ifdef __GNUC__
#define interpreter_inline __inline__
#endif

#ifdef _MSC_VER
#ifdef NDEBUG
#define interpreter_inline __inline
#else
/* don't inline in debug builds (for easier debugging) */
#define interpreter_inline
#endif
#endif

#ifndef interpreter_inline
#error "Unsuported compiler"
#endif

/*  #define interpreter_malloc(x) calloc((x),1)
    #define interpreter_realloc(x,y) realloc(x,y)
    #define interpreter_free(x) free(x) */

/* #include <gc/gc.h>
   #define interpreter_malloc(x) GC_MALLOC(x)
   #define interpreter_realloc(x,y) GC_REALLOC(x,y)
   #define interpreter_free(x)*/

enum {
    interpreter_NONE,interpreter_NUMBER,interpreter_STRING,interpreter_DICT,
    interpreter_LIST,interpreter_FNC,interpreter_DATA,
};

typedef double interpreter_num;

typedef struct interpreter_number_ {
    int type;
    interpreter_num val;
} interpreter_number_;
typedef struct interpreter_string_ {
    int type;
    struct _interpreter_string *info;
    char const *val;
    int len;
} interpreter_string_;
typedef struct interpreter_list_ {
    int type;
    struct _interpreter_list *val;
} interpreter_list_;
typedef struct interpreter_dict_ {
    int type;
    struct _interpreter_dict *val;
    int dtype;
} interpreter_dict_;
typedef struct interpreter_fnc_ {
    int type;
    struct _interpreter_fnc *info;
    int ftype;
    void *cfnc;
} interpreter_fnc_;
typedef struct interpreter_data_ {
    int type;
    struct _interpreter_data *info;
    void *val;
    int magic;
} interpreter_data_;

/* Type: interpreter_obj
 * interpreter's object representation.
 * 
 * Every object in interpreter is of this type in the C API.
 *
 * Fields:
 * type - This determines what kind of objects it is. It is either interpreter_NONE, in
 *        which case this is the none type and no other fields can be accessed.
 *        Or it has one of the values listed below, and the corresponding
 *        fields can be accessed.
 * number - interpreter_NUMBER
 * number.val - A double value with the numeric value.
 * string - interpreter_STRING
 * string.val - A pointer to the string data.
 * string.len - Length in bytes of the string data.
 * dict - interpreter_DICT
 * list - interpreter_LIST
 * fnc - interpreter_FNC
 * data - interpreter_DATA
 * data.val - The user-provided data pointer.
 * data.magic - The user-provided magic number for identifying the data type.
 */
typedef union interpreter_obj {
    int type;
    interpreter_number_ number;
    struct { int type; int *data; } gci;
    interpreter_string_ string;
    interpreter_dict_ dict;
    interpreter_list_ list;
    interpreter_fnc_ fnc;
    interpreter_data_ data;
} interpreter_obj;

typedef struct _interpreter_string {
    int gci;
    int len;
    char s[1];
} _interpreter_string;
typedef struct _interpreter_list {
    int gci;
    interpreter_obj *items;
    int len;
    int alloc;
} _interpreter_list;
typedef struct interpreter_item {
    int used;
    int hash;
    interpreter_obj key;
    interpreter_obj val;
} interpreter_item;
typedef struct _interpreter_dict {
    int gci;
    interpreter_item *items;
    int len;
    int alloc;
    int cur;
    int mask;
    int used;
    interpreter_obj meta;
} _interpreter_dict;
typedef struct _interpreter_fnc {
    int gci;
    interpreter_obj self;
    interpreter_obj globals;
    interpreter_obj code;
} _interpreter_fnc;


typedef union interpreter_code {
    unsigned char i;
    struct { unsigned char i,a,b,c; } regs;
    struct { char val[4]; } string;
    struct { float val; } number;
} interpreter_code;

typedef struct interpreter_frame_ {
/*    interpreter_code *codes; */
    interpreter_obj code;
    interpreter_code *cur;
    interpreter_code *jmp;
    interpreter_obj *regs;
    interpreter_obj *ret_dest;
    interpreter_obj fname;
    interpreter_obj name;
    interpreter_obj line;
    interpreter_obj globals;
    int lineno;
    int cregs;
} interpreter_frame_;

#define interpreter_GCMAX 4096
#define interpreter_FRAMES 256
#define interpreter_REGS_EXTRA 2
/* #define interpreter_REGS_PER_FRAME 256*/
#define interpreter_REGS 16384

/* Type: type_Vm
 * Representation of a interpreter virtual machine instance.
 * 
 * A new type_Vm struct is created with <interpreter_init>, and will be passed to most
 * interpreter functions as first parameter. It contains all the data associated
 * with an instance of a interpreter virtual machine - so it is easy to have
 * multiple instances running at the same time. When you want to free up all
 * memory used by an instance, call <interpreter_deinit>.
 * 
 * Fields:
 * These fields are currently documented: 
 * 
 * builtins - A dictionary containing all builtin objects.
 * modules - A dictionary with all loaded modules.
 * params - A list of parameters for the current function call.
 * frames - A list of all call frames.
 * cur - The index of the currently executing call frame.
 * frames[n].globals - A dictionary of global sybmols in callframe n.
 */
typedef struct type_Vm {
    interpreter_obj builtins;
    interpreter_obj modules;
    interpreter_frame_ frames[interpreter_FRAMES];
    interpreter_obj _params;
    interpreter_obj params;
    interpreter_obj _regs;
    interpreter_obj *regs;
    interpreter_obj root;
    jmp_buf buf;
#ifdef CPYTHON_MOD
    jmp_buf nextexpr;
#endif
    int jmp;
    interpreter_obj ex;
    char chars[256][2];
    int cur;
    /* gc */
    _interpreter_list *white;
    _interpreter_list *grey;
    _interpreter_list *black;
    int steps;
    /* sandbox */
    clock_t clocks;
    double time_elapsed;
    double time_limit;
    unsigned long mem_limit;
    unsigned long mem_used;
    int mem_exceeded;
} type_Vm;

#define TP type_Vm *tp
typedef struct _interpreter_data {
    int gci;
    void (*free)(TP,interpreter_obj);
} _interpreter_data;

#define interpreter_True interpreter_number(1)
#define interpreter_False interpreter_number(0)

extern interpreter_obj interpreter_None;

#define interpreter_malloc(TP,x) calloc((x),1)
#define interpreter_realloc(TP,x,y) realloc(x,y)
#define interpreter_free(TP,x) free(x)

void interpreter_sandbox(TP, double, unsigned long);
void interpreter_time_update(TP);
void interpreter_mem_update(TP);

void interpreter_run(TP,int cur);
void interpreter_set(TP,interpreter_obj,interpreter_obj,interpreter_obj);
interpreter_obj interpreter_get(TP,interpreter_obj,interpreter_obj);
interpreter_obj interpreter_has(TP,interpreter_obj self, interpreter_obj k);
interpreter_obj interpreter_len(TP,interpreter_obj);
void interpreter_del(TP,interpreter_obj,interpreter_obj);
interpreter_obj interpreter_str(TP,interpreter_obj);
int interpreter_bool(TP,interpreter_obj);
int interpreter_cmp(TP,interpreter_obj,interpreter_obj);
void _interpreter_raise(TP,interpreter_obj);
interpreter_obj interpreter_printf(TP,char const *fmt,...);
interpreter_obj interpreter_track(TP,interpreter_obj);
void interpreter_grey(TP,interpreter_obj);
interpreter_obj interpreter_call(TP, interpreter_obj fnc, interpreter_obj params);
interpreter_obj interpreter_add(TP,interpreter_obj a, interpreter_obj b) ;

/* __func__ __VA_ARGS__ __FILE__ __LINE__ */

/* Function: interpreter_raise
 * Macro to raise an exception.
 * 
 * This macro will return from the current function returning "r". The
 * remaining parameters are used to format the exception message.
 */
/*
#define interpreter_raise(r,fmt,...) { \
    _interpreter_raise(tp,interpreter_printf(tp,fmt,__VA_ARGS__)); \
    return r; \
}
*/
#define interpreter_raise(r,v) { \
    _interpreter_raise(tp,v); \
    return r; \
}

/* Function: interpreter_string
 * Creates a new string object from a C string.
 * 
 * Given a pointer to a C string, creates a interpreter object representing the
 * same string.
 * 
 * *Note* Only a reference to the string will be kept by interpreter, so make sure
 * it does not go out of scope, and don't de-allocate it. Also be aware that
 * interpreter will not delete the string for you. In many cases, it is best to
 * use <interpreter_string_t> or <interpreter_string_slice> to create a string where interpreter
 * manages storage for you.
 */
interpreter_inline static interpreter_obj interpreter_string(char const *v) {
    interpreter_obj val;
    interpreter_string_ s = {interpreter_STRING, 0, v, 0};
    s.len = strlen(v);
    val.string = s;
    return val;
}

#define interpreter_CSTR_LEN 256

interpreter_inline static void interpreter_cstr(TP,interpreter_obj v, char *s, int l) {
    if (v.type != interpreter_STRING) { 
        interpreter_raise(,interpreter_string("(interpreter_cstr) TypeError: value not a string"));
    }
    if (v.string.len >= l) {
        interpreter_raise(,interpreter_string("(interpreter_cstr) TypeError: value too long"));
    }
    memset(s,0,l);
    memcpy(s,v.string.val,v.string.len);
}


#define interpreter_OBJ() (interpreter_get(tp,tp->params,interpreter_None))
interpreter_inline static interpreter_obj interpreter_type(TP,int t,interpreter_obj v) {
    if (v.type != t) { interpreter_raise(interpreter_None,interpreter_string("(interpreter_type) TypeError: unexpected type")); }
    return v;
}



#define interpreter_NO_LIMIT 0
#define interpreter_TYPE(t) interpreter_type(tp,t,interpreter_OBJ())
#define interpreter_NUM() (interpreter_TYPE(interpreter_NUMBER).number.val)
/* #define interpreter_STR() (interpreter_CSTR(interpreter_TYPE(interpreter_STRING))) */
#define interpreter_STR() (interpreter_TYPE(interpreter_STRING))
#define interpreter_DEFAULT(d) (tp->params.list.val->len?interpreter_get(tp,tp->params,interpreter_None):(d))

/* Macro: interpreter_LOOP
 * Macro to iterate over all remaining arguments.
 *
 * If you have a function which takes a variable number of arguments, you can
 * iterate through all remaining arguments for example like this:
 *
 * > interpreter_obj *my_func(type_Vm *tp)
 * > {
 * >     // We retrieve the first argument like normal.
 * >     interpreter_obj first = interpreter_OBJ();
 * >     // Then we iterate over the remaining arguments.
 * >     interpreter_obj arg;
 * >     interpreter_LOOP(arg)
 * >         // do something with arg
 * >     interpreter_END
 * > }
 */
#define interpreter_LOOP(e) \
    int __l = tp->params.list.val->len; \
    int __i; for (__i=0; __i<__l; __i++) { \
    (e) = _interpreter_list_get(tp,tp->params.list.val,__i,"interpreter_LOOP");
#define interpreter_END \
    }

interpreter_inline static int _interpreter_min(int a, int b) { return (a<b?a:b); }
interpreter_inline static int _interpreter_max(int a, int b) { return (a>b?a:b); }
interpreter_inline static int _interpreter_sign(interpreter_num v) { return (v<0?-1:(v>0?1:0)); }

/* Function: interpreter_number
 * Creates a new numeric object.
 */
interpreter_inline static interpreter_obj interpreter_number(interpreter_num v) {
    interpreter_obj val = {interpreter_NUMBER};
    val.number.val = v;
    return val;
}

interpreter_inline static void interpreter_echo(TP,interpreter_obj e) {
    e = interpreter_str(tp,e);
    fwrite(e.string.val,1,e.string.len,stdout);
}

/* Function: interpreter_string_n
 * Creates a new string object from a partial C string.
 * 
 * Like <interpreter_string>, but you specify how many bytes of the given C string to
 * use for the string object. The *note* also applies for this function, as the
 * string reference and length are kept, but no actual substring is stored.
 */
interpreter_inline static interpreter_obj interpreter_string_n(char const *v,int n) {
    interpreter_obj val;
    interpreter_string_ s = {interpreter_STRING, 0,v,n};
    val.string = s;
    return val;
}

#endif
