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


enum {
    interpreter_NONE,interpreter_NUMBER,interpreter_STRING,interpreter_DICT,
    interpreter_LIST,interpreter_FNC,interpreter_DATA,
};

typedef double type_vmNum;

typedef struct type_vmStructNum {
    int type;
    type_vmNum val;
} type_vmStructNum;
typedef struct type_vmStructString {
    int type;
    struct type_vmString *info;
    char const *val;
    int len;
} type_vmStructString;
typedef struct type_vmStructList {
    int type;
    struct type_vmList *val;
} type_vmStructList;
typedef struct type_vmStructDict {
    int type;
    struct type_vmDict *val;
    int dtype;
} type_vmStructDict;
typedef struct type_vmStructFnc {
    int type;
    struct type_vmFnc *info;
    int ftype;
    void *cfnc;
} type_vmStructFnc;
typedef struct type_vmStructData {
    int type;
    struct _interpreter_data *info;
    void *val;
    int magic;
} type_vmStructData;

/* Type: type_vmObj
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
typedef union type_vmObj {
    int type;
    type_vmStructNum number;
    struct { int type; int *data; } gci;
    type_vmStructString string;
    type_vmStructDict dict;
    type_vmStructList list;
    type_vmStructFnc fnc;
    type_vmStructData data;
} type_vmObj;

typedef struct type_vmString {
    int gci;
    int len;
    char s[1];
} type_vmString;
typedef struct type_vmList {
    int gci;
    type_vmObj *items;
    int len;
    int alloc;
} type_vmList;
typedef struct type_vmItem {
    int used;
    int hash;
    type_vmObj key;
    type_vmObj val;
} type_vmItem;
typedef struct type_vmDict {
    int gci;
    type_vmItem *items;
    int len;
    int alloc;
    int cur;
    int mask;
    int used;
    type_vmObj meta;
} type_vmDict;
typedef struct type_vmFnc {
    int gci;
    type_vmObj self;
    type_vmObj globals;
    type_vmObj code;
} type_vmFnc;


typedef union type_vmCode {
    unsigned char i;
    struct { unsigned char i,a,b,c; } regs;
    struct { char val[4]; } string;
    struct { float val; } number;
} type_vmCode;

typedef struct type_vmFrame {
/*    type_vmCode *codes; */
    type_vmObj code;
    type_vmCode *cur;
    type_vmCode *jmp;
    type_vmObj *regs;
    type_vmObj *ret_dest;
    type_vmObj fname;
    type_vmObj name;
    type_vmObj line;
    type_vmObj globals;
    int lineno;
    int cregs;
} type_vmFrame;

#define vm_GCMAX 4096
#define vm_FRAMES 256
#define vm_REGS_EXTRA 2
#define vm_REGS 16384

/* Type: type_vm
 * Representation of a interpreter virtual machine instance.
 * 
 * A new type_vm struct is created with <interpreter_init>, and will be passed to most
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
typedef struct type_vm {
    type_vmObj builtins;
    type_vmObj modules;
    type_vmFrame frames[vm_FRAMES];
    type_vmObj params_sub;
    type_vmObj params;
    type_vmObj regs_sub;
    type_vmObj *regs;
    type_vmObj root;
    jmp_buf buf;
    int jmp;
    type_vmObj ex;
    char chars[256][2];
    int cur;
    /* gc */
    type_vmList *white;
    type_vmList *grey;
    type_vmList *black;
    int steps;
    /* sandbox */
    clock_t clocks;
    double time_elapsed;
    double time_limit;
    unsigned long mem_limit;
    unsigned long mem_used;
    int mem_exceeded;
} type_vm;

/* #define TP type_vm *tp */

typedef struct _interpreter_data {
    int gci;
    void (*free)(type_vm *tp,type_vmObj);
} _interpreter_data;

#define interpreter_True interpreter_number(1)
#define interpreter_False interpreter_number(0)

extern type_vmObj interpreter_None;

/*#define interpreter_malloc(TP,x) calloc((x),1)*/
/* #define interpreter_realloc(TP,x,y) realloc(x,y) */
/* #define interpreter_free(TP,x) free(x) */

void interpreter_sandbox(type_vm *tp, double, unsigned long);
void interpreter_time_update(type_vm *tp);
void interpreter_mem_update(type_vm *tp);

void interpreter_run(type_vm *tp,int cur);
void interpreter_set(type_vm *tp,type_vmObj,type_vmObj,type_vmObj);
type_vmObj interpreter_get(type_vm *tp,type_vmObj,type_vmObj);
type_vmObj interpreter_has(type_vm *tp,type_vmObj self, type_vmObj k);
type_vmObj interpreter_len(type_vm *tp,type_vmObj);
void interpreter_del(type_vm *tp,type_vmObj,type_vmObj);
type_vmObj interpreter_str(type_vm *tp,type_vmObj);
int interpreter_bool(type_vm *tp,type_vmObj);
int interpreter_cmp(type_vm *tp,type_vmObj,type_vmObj);
void _interpreter_raise(type_vm *tp,type_vmObj);
type_vmObj interpreter_printf(type_vm *tp,char const *fmt,...);
type_vmObj interpreter_track(type_vm *tp,type_vmObj);
void interpreter_grey(type_vm *tp,type_vmObj);
type_vmObj interpreter_call(type_vm *tp, type_vmObj fnc, type_vmObj params);
type_vmObj interpreter_add(type_vm *tp,type_vmObj a, type_vmObj b) ;

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
interpreter_inline static type_vmObj interpreter_string(char const *v) {
    type_vmObj val;
    type_vmStructString s = {interpreter_STRING, 0, v, 0};
    s.len = strlen(v);
    val.string = s;
    return val;
}

#define interpreter_CSTR_LEN 256

interpreter_inline static void interpreter_cstr(type_vm *tp,type_vmObj v, char *s, int l) {
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
interpreter_inline static type_vmObj interpreter_type(type_vm *tp,int t,type_vmObj v) {
    if (v.type != t) { interpreter_raise(interpreter_None,interpreter_string("(interpreter_type) TypeError: unexpected type")); }
    return v;
}



#define interpreter_NO_LIMIT 0
#define interpreter_TYPE(t) interpreter_type(tp,t,interpreter_OBJ())
#define type_vmNum() (interpreter_TYPE(interpreter_NUMBER).number.val)
/* #define interpreter_STR() (interpreter_CSTR(interpreter_TYPE(interpreter_STRING))) */
#define interpreter_STR() (interpreter_TYPE(interpreter_STRING))
#define interpreter_DEFAULT(d) (tp->params.list.val->len?interpreter_get(tp,tp->params,interpreter_None):(d))

/* Macro: interpreter_LOOP
 * Macro to iterate over all remaining arguments.
 *
 * If you have a function which takes a variable number of arguments, you can
 * iterate through all remaining arguments for example like this:
 *
 * > type_vmObj *my_func(type_vm *tp)
 * > {
 * >     // We retrieve the first argument like normal.
 * >     type_vmObj first = interpreter_OBJ();
 * >     // Then we iterate over the remaining arguments.
 * >     type_vmObj arg;
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
interpreter_inline static int _interpreter_sign(type_vmNum v) { return (v<0?-1:(v>0?1:0)); }

/* Function: interpreter_number
 * Creates a new numeric object.
 */
interpreter_inline static type_vmObj interpreter_number(type_vmNum v) {
    type_vmObj val = {interpreter_NUMBER};
    val.number.val = v;
    return val;
}

interpreter_inline static void interpreter_echo(type_vm *tp,type_vmObj e) {
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
interpreter_inline static type_vmObj interpreter_string_n(char const *v,int n) {
    type_vmObj val;
    type_vmStructString s = {interpreter_STRING, 0,v,n};
    val.string = s;
    return val;
}

#endif
