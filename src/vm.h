/* File: General
 * Things defined in vm.h.
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
#define vm_inline __inline__
#endif



#ifndef vm_inline
#error "Unsuported compiler"
#endif


enum {
    vm_enum1_none,vm_enum1_number,vm_enum1_string,vm_enum1_dict,
    vm_enum1_list,vm_enum1_fnc,vm_enum1_data,
};
enum {
    vm_enum2_EOF,vm_enum2_ADD,vm_enum2_SUB,vm_enum2_MUL,vm_enum2_DIV,vm_enum2_POW,vm_enum2_BITAND,vm_enum2_BITOR,vm_enum2_CMP,vm_enum2_GET,vm_enum2_SET,
    vm_enum2_NUMBER,vm_enum2_STRING,vm_enum2_GGET,vm_enum2_GSET,vm_enum2_MOVE,vm_enum2_DEF,vm_enum2_PASS,vm_enum2_JUMP,vm_enum2_CALL,
    vm_enum2_RETURN,vm_enum2_IF,vm_enum2_DEBUG,vm_enum2_EQ,vm_enum2_LE,vm_enum2_LT,vm_enum2_DICT,vm_enum2_LIST,vm_enum2_NONE,vm_enum2_LEN,
    vm_enum2_LINE,vm_enum2_PARAMS,vm_enum2_IGET,vm_enum2_FILE,vm_enum2_NAME,vm_enum2_NE,vm_enum2_HAS,vm_enum2_RAISE,vm_enum2_SETJMP,
    vm_enum2_MOD,vm_enum2_LSH,vm_enum2_RSH,vm_enum2_ITER,vm_enum2_DEL,vm_enum2_REGS,vm_enum2_BITXOR, vm_enum2_IFN,
    vm_enum2_NOT, vm_enum2_BITNOT,
    vm_enum2_TOTAL
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
    struct vm_type_data *info;
    void *val;
    int magic;
} type_vmStructData;

/* Type: type_vmObj
 * interpreter's object representation.
 * 
 * Every object in interpreter is of this type in the C API.
 *
 * Fields:
 * type - This determines what kind of objects it is. It is either vm_enum1_none, in
 *        which case this is the none type and no other fields can be accessed.
 *        Or it has one of the values listed below, and the corresponding
 *        fields can be accessed.
 * number - vm_enum1_number
 * number.val - A double value with the numeric value.
 * string - vm_enum1_string
 * string.val - A pointer to the string data.
 * string.len - Length in bytes of the string data.
 * dict - vm_enum1_dict
 * list - vm_enum1_list
 * fnc - vm_enum1_fnc
 * data - vm_enum1_data
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

#define vm_def_GCMAX 4096
#define vm_def_FRAMES 256
#define vm_def_REGS_EXTRA 2
#define vm_def_REGS 16384

/* Type: type_vm
 * Representation of a interpreter virtual machine instance.
 * 
 * A new type_vm struct is created with <vm_init>, and will be passed to most
 * interpreter functions as first parameter. It contains all the data associated
 * with an instance of a interpreter virtual machine - so it is easy to have
 * multiple instances running at the same time. When you want to free up all
 * memory used by an instance, call <vm_deinit>.
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
    type_vmFrame frames[vm_def_FRAMES];
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

typedef struct vm_type_data {
    int gci;
    void (*free)(type_vm *tp,type_vmObj);
} vm_type_data;


extern type_vmObj vm_none;

void vm_sandbox(type_vm *tp, double, unsigned long);
void vm_time_update(type_vm *tp);
void vm_mem_update(type_vm *tp);

void vm_run(type_vm *tp,int cur);
void vm_operations_set(type_vm *tp,type_vmObj,type_vmObj,type_vmObj);
type_vmObj vm_operations_get(type_vm *tp,type_vmObj,type_vmObj);
type_vmObj vm_operations_haskey(type_vm *tp,type_vmObj self, type_vmObj k);
type_vmObj vm_operations_len(type_vm *tp,type_vmObj);
void vm_operations_dict_key_del(type_vm *tp,type_vmObj,type_vmObj);
type_vmObj vm_operations_str(type_vm *tp,type_vmObj);
int vm_operations_bool(type_vm *tp,type_vmObj);
int vm_operations_cmp(type_vm *tp,type_vmObj,type_vmObj);
void vm_raise(type_vm *tp,type_vmObj);
type_vmObj vm_string_printf(type_vm *tp,char const *fmt,...);
type_vmObj vm_gc_track(type_vm *tp,type_vmObj);
void vm_gc_grey(type_vm *tp,type_vmObj);
type_vmObj vm_call_sub(type_vm *tp, type_vmObj fnc, type_vmObj params);
type_vmObj vm_operations_add(type_vm *tp,type_vmObj a, type_vmObj b) ;

/* __func__ __VA_ARGS__ __FILE__ __LINE__ */


/* Function: vm_string
 * Creates a new string object from a C string.
 * 
 * Given a pointer to a C string, creates a interpreter object representing the
 * same string.
 * 
 * *Note* Only a reference to the string will be kept by interpreter, so make sure
 * it does not go out of scope, and don't de-allocate it. Also be aware that
 * interpreter will not delete the string for you. In many cases, it is best to
 * use <vm_string_new> or <interpreter_string_slice> to create a string where interpreter
 * manages storage for you.
 */
vm_inline static type_vmObj vm_string(char const *v) {
    type_vmObj val;
    type_vmStructString s = {vm_enum1_string, 0, v, 0};
    s.len = strlen(v);
    val.string = s;
    return val;
}

#define vm_def_CSTR_LEN 256

vm_inline static void interpreter_cstr(type_vm *tp,type_vmObj v, char *s, int l) {
    if (v.type != vm_enum1_string) { 
        vm_raise(tp,vm_string("(interpreter_cstr) TypeError: value not a string"));
    }
    if (v.string.len >= l) {
        vm_raise(tp,vm_string("(interpreter_cstr) TypeError: value too long"));
    }
    memset(s,0,l);
    memcpy(s,v.string.val,v.string.len);
}


vm_inline static type_vmObj vm_typecheck(type_vm *tp,int t,type_vmObj v) {
    if (v.type != t) { vm_raise(tp,vm_string("(vm_typecheck) TypeError: unexpected type")); }
    return v;
}



#define vm_def_NO_LIMIT 0


#define vm_macros_DEFAULT(d) (tp->params.list.val->len?vm_operations_get(tp,tp->params,vm_none):(d))

/* Macro: interpreter_LOOP
 * Macro to iterate over all remaining arguments.
 *
 * If you have a function which takes a variable number of arguments, you can
 * iterate through all remaining arguments for example like this:
 *
 * > type_vmObj *my_func(type_vm *tp)
 * > {
 * >     // We retrieve the first argument like normal.
 * >     type_vmObj first = vm_operations_get(tp,tp->params,vm_none);
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
    (e) = vm_list_get(tp,tp->params.list.val,__i,"interpreter_LOOP");
#define interpreter_END \
    }

vm_inline static int vm_min(int a, int b) { return (a<b?a:b); }
vm_inline static int vm_max(int a, int b) { return (a>b?a:b); }
vm_inline static int vm_sign(type_vmNum v) { return (v<0?-1:(v>0?1:0)); }

/* Function: vm_create_numericObj
 * Creates a new numeric object.
 */
vm_inline static type_vmObj vm_create_numericObj(type_vmNum v) {
    type_vmObj val = {vm_enum1_number};
    val.number.val = v;
    return val;
}

vm_inline static void vm_echo(type_vm *tp,type_vmObj e) {
    e = vm_operations_str(tp,e);
    fwrite(e.string.val,1,e.string.len,stdout);
}

/* Function: vm_string_n
 * Creates a new string object from a partial C string.
 * 
 * Like <vm_string>, but you specify how many bytes of the given C string to
 * use for the string object. The *note* also applies for this function, as the
 * string reference and length are kept, but no actual substring is stored.
 */
vm_inline static type_vmObj vm_string_n(char const *v,int n) {
    type_vmObj val;
    type_vmStructString s = {vm_enum1_string, 0,v,n};
    val.string = s;
    return val;
}

#endif
