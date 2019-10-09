

#include "vm.h"
#include "list.c"
#include "dict.c"
#include "misc.c"
#include "string.c"
#include "builtins.c"
#include "gc.c"
#include "operations.c"

void vm_compiler(type_vm *tp);
/* File: VM
 * Functionality pertaining to the virtual machine.
 */

 #include "defines/dict.sys.h"

type_vm *sub_vm_init(void) {
    int i;
    type_vm *vm = (type_vm*)calloc(sizeof(type_vm),1);
    vm->time_limit = vm_def_NO_LIMIT;
    vm->clocks = clock();
    vm->time_elapsed = 0.0;
    vm->mem_limit = vm_def_NO_LIMIT;
    vm->mem_exceeded = 0;
    vm->mem_used = sizeof(type_vm);
    vm->cur = 0;
    vm->jmp = 0;
    vm->ex = vm_none;
    vm->root = vm_list_nt(vm);
    for (i=0; i<256; i++) { vm->chars[i][0]=i; }
    vm_gc_init(vm);
    vm->regs_sub = vm_list(vm);
    for (i=0; i<vm_def_REGS; i++) { vm_operations_set(vm,vm->regs_sub,vm_none,vm_none); }
    vm->builtins = vm_dict_create(vm);
    vm->modules = vm_dict_create(vm);
    vm->params_sub = vm_list(vm);
    for (i=0; i<vm_def_FRAMES; i++) { vm_operations_set(vm,vm->params_sub,vm_none,vm_list(vm)); }
    vm_operations_set(vm,vm->root,vm_none,vm->builtins);
    vm_operations_set(vm,vm->root,vm_none,vm->modules);
    vm_operations_set(vm,vm->root,vm_none,vm->regs_sub);
    vm_operations_set(vm,vm->root,vm_none,vm->params_sub);
    vm_operations_set(vm,vm->builtins,vm_string("MODULES"),vm->modules);
    vm_operations_set(vm,vm->modules,vm_string("BUILTINS"),vm->builtins);
    vm_operations_set(vm,vm->builtins,vm_string("BUILTINS"),vm->builtins);
    type_vmObj sys = vm_dict_create(vm);
    /*defines from src/defines*/
    vm_operations_set(vm, sys, vm_string("version"), vm_string(config_sys_version));
    vm_operations_set(vm, sys, vm_string("flags"), vm_string(config_sys_flags));
    vm_operations_set(vm, sys, vm_string("platform"), vm_string(config_sys_platform));
    /*end defines from src/defines*/
    vm_operations_set(vm,vm->modules, vm_string("sys"), sys);
    vm->regs = vm->regs_sub.list.val->items;
    vm_gc_full(vm);
    return vm;
}


/* Function: vm_deinit
 * Destroys a VM instance.
 *
 * When you no longer need an instance of interpreter, you can use this to free all
 * memory used by it. Even when you are using only a single interpreter instance, it
 * may be good practice to call this function on shutdown.
 */
void vm_deinit(type_vm *tp) {
    while (tp->root.list.val->len) {
        vm_list_pop(tp,tp->root.list.val,0,"vm_deinit");
    }
    vm_gc_full(tp); vm_gc_full(tp);
    vm_gc_delete(tp,tp->root);
    vm_gc_deinit(tp);
    tp->mem_used -= sizeof(type_vm);
    free(tp);
}

/* type_vmFrame*/
void vm_frame(type_vm *tp,type_vmObj globals,type_vmObj code,type_vmObj *ret_dest) {
    type_vmFrame f;
    f.globals = globals;
    f.code = code;
    f.cur = (type_vmCode*)f.code.string.val;
    f.jmp = 0;
/*     fprintf(stderr,"tp->cur: %d\n",tp->cur);*/
    f.regs = (tp->cur <= 0?tp->regs:tp->frames[tp->cur].regs+tp->frames[tp->cur].cregs);

    f.regs[0] = f.globals;
    f.regs[1] = f.code;
    f.regs += vm_def_REGS_EXTRA;

    f.ret_dest = ret_dest;
    f.lineno = 0;
    f.line = vm_string("");
    f.name = vm_string("?");
    f.fname = vm_string("?");
    f.cregs = 0;
/*     return f;*/
    if (f.regs+(256+vm_def_REGS_EXTRA) >= tp->regs+vm_def_REGS || tp->cur >= vm_def_FRAMES-1) {
        vm_raise(tp,vm_string("(vm_frame) RuntimeError: stack overflow"));
    }
    tp->cur += 1;
    tp->frames[tp->cur] = f;
}

void vm_raise(type_vm *tp,type_vmObj e) {
    if (!tp || !tp->jmp) {
        printf("\nException:\n"); vm_echo(tp,e); printf("\n");
        exit(-1);
    }
    if (e.type != vm_enum1_none) { tp->ex = e; }
    vm_gc_grey(tp,e);
    longjmp(tp->buf,1);
}

void vm_print_stack(type_vm *tp) {
    int i;
    printf("\n");
    for (i=0; i<=tp->cur; i++) {
        if (!tp->frames[i].lineno) { continue; }
        printf("File \""); vm_echo(tp,tp->frames[i].fname); printf("\", ");
        printf("line %d, in ",tp->frames[i].lineno);
        vm_echo(tp,tp->frames[i].name); printf("\n ");
        vm_echo(tp,tp->frames[i].line); printf("\n");
    }
    printf("\nException:\n"); vm_echo(tp,tp->ex); printf("\n");
}

void vm_handle(type_vm *tp) {
    int i;
    for (i=tp->cur; i>=0; i--) {
        if (tp->frames[i].jmp) { break; }
    }
    if (i >= 0) {
        tp->cur = i;
        tp->frames[i].cur = tp->frames[i].jmp;
        tp->frames[i].jmp = 0;
        return;
    }
    vm_print_stack(tp);
    exit(-1);
}

/* Function: vm_call_sub
 * Calls a interpreter function.
 *
 * Use this to call a interpreter function.
 *
 * Parameters:
 * tp - The VM instance.
 * self - The object to call.
 * params - Parameters to pass.
 *
 * Example:
 * > vm_call_sub(tp,
 * >     vm_operations_get(tp, tp->builtins, vm_string("foo")),
 * >     vm_misc_params_v(tp, vm_string("hello")))
 * This will look for a global function named "foo", then call it with a single
 * positional parameter containing the string "hello".
 */
type_vmObj vm_call_sub(type_vm *tp,type_vmObj self, type_vmObj params) {
    /* I'm not sure we should have to do this, but
    just for giggles we will. */
    tp->params = params;

    if (self.type == vm_enum1_dict) {
        if (self.dict.dtype == 1) {
            type_vmObj meta; if (vm_builtins_lookup(tp,self,vm_string("__new__"),&meta)) {
                vm_list_insert(tp,params.list.val,0,self);
                return vm_call_sub(tp,meta,params);
            }
        } else if (self.dict.dtype == 2) {
			if (self.dict.dtype == 2) {
					type_vmObj meta; 
					if (vm_builtins_lookup(tp,self,vm_string("__call__"),&meta)) {
						return vm_call_sub(tp,meta,params);
						}			
			}
        }
    }
    if (self.type == vm_enum1_fnc && !(self.fnc.ftype&1)) {
        type_vmObj r = vm_misc_tcall(tp,self);
        vm_gc_grey(tp,r);
        return r;
    }
    if (self.type == vm_enum1_fnc) {
        type_vmObj dest = vm_none;
        vm_frame(tp,self.fnc.info->globals,self.fnc.info->code,&dest);
        if ((self.fnc.ftype&2)) {
            tp->frames[tp->cur].regs[0] = params;
            vm_list_insert(tp,params.list.val,0,self.fnc.info->self);
        } else {
            tp->frames[tp->cur].regs[0] = params;
        }
        vm_run(tp,tp->cur);
        return dest;
    }
    vm_misc_params_v(tp,1,self); vm_builtins_print(tp);
    vm_raise(0,vm_string("(vm_call_sub) TypeError: object is not callable"));
	return vm_none;
}


void vm_return(type_vm *tp, type_vmObj v) {
    type_vmObj *dest = tp->frames[tp->cur].ret_dest;
    if (dest) { *dest = v; vm_gc_grey(tp,v); }
    memset(tp->frames[tp->cur].regs-vm_def_REGS_EXTRA,0,(vm_def_REGS_EXTRA+tp->frames[tp->cur].cregs)*sizeof(type_vmObj));
    tp->cur -= 1;
}





#define vm_macro_UVBC (unsigned short)(((e.regs.b<<8)+e.regs.c))
#define vm_macro_SVBC (short)(((e.regs.b<<8)+e.regs.c))
#define vm_macro_GA vm_gc_grey(tp,regs[e.regs.a])
#define vm_macro_SR(v) f->cur = cur; return(v);


int vm_step(type_vm *tp) {
    type_vmFrame *f = &tp->frames[tp->cur];
    type_vmObj *regs = f->regs;
    type_vmCode *cur = f->cur;
    while(1) {
    type_vmCode e = *cur;
    switch (e.i) {
        case vm_enum2_EOF: vm_return(tp,vm_none); vm_macro_SR(0); break;
        case vm_enum2_ADD: regs[e.regs.a] = vm_operations_add(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_SUB: regs[e.regs.a] = vm_operations_sub(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_MUL: regs[e.regs.a] = vm_operations_mul(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_DIV: regs[e.regs.a] = vm_operations_div(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_POW: regs[e.regs.a] = vm_operations_pow(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_BITAND: regs[e.regs.a] = vm_operations_bitwise_and(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_BITOR:  regs[e.regs.a] = vm_operations_bitwise_or(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_BITXOR:  regs[e.regs.a] = vm_operations_bitwise_xor(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_MOD:  regs[e.regs.a] = vm_operations_mod(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_LSH:  regs[e.regs.a] = vm_operations_lsh(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_RSH:  regs[e.regs.a] = vm_operations_rsh(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_CMP: regs[e.regs.a] = vm_create_numericObj(vm_operations_cmp(tp,regs[e.regs.b],regs[e.regs.c])); break;
        case vm_enum2_NE: regs[e.regs.a] = vm_create_numericObj(vm_operations_cmp(tp,regs[e.regs.b],regs[e.regs.c])!=0); break;
        case vm_enum2_EQ: regs[e.regs.a] = vm_create_numericObj(vm_operations_cmp(tp,regs[e.regs.b],regs[e.regs.c])==0); break;
        case vm_enum2_LE: regs[e.regs.a] = vm_create_numericObj(vm_operations_cmp(tp,regs[e.regs.b],regs[e.regs.c])<=0); break;
        case vm_enum2_LT: regs[e.regs.a] = vm_create_numericObj(vm_operations_cmp(tp,regs[e.regs.b],regs[e.regs.c])<0); break;
        case vm_enum2_BITNOT:  regs[e.regs.a] = vm_operations_bitwise_not(tp,regs[e.regs.b]); break;
        case vm_enum2_NOT: regs[e.regs.a] = vm_create_numericObj(!vm_operations_bool(tp,regs[e.regs.b])); break;
        case vm_enum2_PASS: break;
        case vm_enum2_IF: if (vm_operations_bool(tp,regs[e.regs.a])) { cur += 1; } break;
        case vm_enum2_IFN: if (!vm_operations_bool(tp,regs[e.regs.a])) { cur += 1; } break;
        case vm_enum2_GET: regs[e.regs.a] = vm_operations_get(tp,regs[e.regs.b],regs[e.regs.c]); vm_macro_GA; break;
        case vm_enum2_ITER:
            if (regs[e.regs.c].number.val < vm_operations_len(tp,regs[e.regs.b]).number.val) {
                regs[e.regs.a] = vm_operations_iterate(tp,regs[e.regs.b],regs[e.regs.c]); vm_macro_GA;
                regs[e.regs.c].number.val += 1;
                cur += 1;
            }
            break;
        case vm_enum2_HAS: regs[e.regs.a] = vm_operations_haskey(tp,regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_IGET: vm_operations_safeget(tp,&regs[e.regs.a],regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_SET: vm_operations_set(tp,regs[e.regs.a],regs[e.regs.b],regs[e.regs.c]); break;
        case vm_enum2_DEL: vm_operations_dict_key_del(tp,regs[e.regs.a],regs[e.regs.b]); break;
        case vm_enum2_MOVE: regs[e.regs.a] = regs[e.regs.b]; break;
        case vm_enum2_NUMBER:
            regs[e.regs.a] = vm_create_numericObj(*(type_vmNum*)(*++cur).string.val);
            cur += sizeof(type_vmNum)/4;
            continue;
        case vm_enum2_STRING: {
            /* regs[e.regs.a] = vm_string_n((*(cur+1)).string.val,vm_macro_UVBC); */
            int a = (*(cur+1)).string.val-f->code.string.val;
            regs[e.regs.a] = vm_string_substring(tp,f->code,a,a+vm_macro_UVBC),
            cur += (vm_macro_UVBC/4)+1;
            }
            break;
        case vm_enum2_DICT: regs[e.regs.a] = interpreter_dict_n(tp,e.regs.c/2,&regs[e.regs.b]); break;
        case vm_enum2_LIST: regs[e.regs.a] = vm_list_n(tp,e.regs.c,&regs[e.regs.b]); break;
        case vm_enum2_PARAMS: regs[e.regs.a] = vm_misc_params_n(tp,e.regs.c,&regs[e.regs.b]); break;
        case vm_enum2_LEN: regs[e.regs.a] = vm_operations_len(tp,regs[e.regs.b]); break;
        case vm_enum2_JUMP: cur += vm_macro_SVBC; continue; break;
        case vm_enum2_SETJMP: f->jmp = vm_macro_SVBC?cur+vm_macro_SVBC:0; break;
        case vm_enum2_CALL:
            f->cur = cur + 1;  regs[e.regs.a] = vm_call_sub(tp,regs[e.regs.b],regs[e.regs.c]); vm_macro_GA;
            return 0; break;
        case vm_enum2_GGET:
            if (!vm_operations_safeget(tp,&regs[e.regs.a],f->globals,regs[e.regs.b])) {
                regs[e.regs.a] = vm_operations_get(tp,tp->builtins,regs[e.regs.b]); vm_macro_GA;
            }
            break;
        case vm_enum2_GSET: vm_operations_set(tp,f->globals,regs[e.regs.a],regs[e.regs.b]); break;
        case vm_enum2_DEF: {
            int a = (*(cur+1)).string.val-f->code.string.val;
            regs[e.regs.a] = vm_misc_def(tp,
                /*vm_string_n((*(cur+1)).string.val,(vm_macro_SVBC-1)*4),*/
                vm_string_substring(tp,f->code,a,a+(vm_macro_SVBC-1)*4),
                f->globals);
            cur += vm_macro_SVBC; continue;
            }
            break;

        case vm_enum2_RETURN: vm_return(tp,regs[e.regs.a]); vm_macro_SR(0); break;
        case vm_enum2_RAISE: vm_raise(tp,regs[e.regs.a]); vm_macro_SR(0); break;
        case vm_enum2_DEBUG:
            vm_misc_params_v(tp,3,vm_string("DEBUG:"),vm_create_numericObj(e.regs.a),regs[e.regs.a]); vm_builtins_print(tp);
            break;
        case vm_enum2_NONE: regs[e.regs.a] = vm_none; break;
        case vm_enum2_LINE:
            ;
            int a = (*(cur+1)).string.val-f->code.string.val;
            f->line = vm_string_substring(tp,f->code,a,a+e.regs.a*4-1);
            cur += e.regs.a; f->lineno = vm_macro_UVBC;
            break;
        case vm_enum2_FILE: f->fname = regs[e.regs.a]; break;
        case vm_enum2_NAME: f->name = regs[e.regs.a]; break;
        case vm_enum2_REGS: f->cregs = e.regs.a; break;
        default:
            vm_raise(0,vm_string("(vm_step) RuntimeError: invalid instruction"));
            break;
    }
    cur += 1;
    }
    vm_macro_SR(0);
}

void _vm_run(type_vm *tp,int cur) {
    tp->jmp += 1; if (setjmp(tp->buf)) { vm_handle(tp); }
    while (tp->cur >= cur && vm_step(tp) != -1);
    tp->jmp -= 1;
}

void vm_run(type_vm *tp,int cur) {
    jmp_buf tmp;
    memcpy(tmp,tp->buf,sizeof(jmp_buf));
    _vm_run(tp,cur);
    memcpy(tp->buf,tmp,sizeof(jmp_buf));
}


type_vmObj vm_call(type_vm *tp, const char *mod, const char *fnc, type_vmObj params) {
    type_vmObj tmp;
    tmp = vm_operations_get(tp,tp->modules,vm_string(mod));
    tmp = vm_operations_get(tp,tmp,vm_string(fnc));
    return vm_call_sub(tp,tmp,params);
}

type_vmObj vm_import_sub(type_vm *tp, type_vmObj fname, type_vmObj name, type_vmObj code) {
    type_vmObj g;

    if (!((fname.type != vm_enum1_none && vm_string_index(fname,vm_string(".tpc"))!=-1) || code.type != vm_enum1_none)) {
        return vm_call(tp,"obfuscatedDataType","import_fname",vm_misc_params_v(tp,2,fname,name));
    }

    if (code.type == vm_enum1_none) {
        vm_misc_params_v(tp,1,fname);
        code = vm_builtins_load(tp);
    }

    g = vm_dict_create(tp);
    vm_operations_set(tp,g,vm_string("__name__"),name);
    vm_operations_set(tp,g,vm_string("__code__"),code);
    vm_operations_set(tp,g,vm_string("__dict__"),g);
    vm_frame(tp,g,code,0);
    vm_operations_set(tp,tp->modules,name,g);

    if (!tp->jmp) { vm_run(tp,tp->cur); }

    return g;
}


/* Function: vm_import_module
 * Imports a module.
 *
 * Parameters:
 * fname - The filename of a file containing the module's code.
 * name - The name of the module.
 * codes - The module's code.  If this is given, fname is ignored.
 * len - The length of the bytecode.
 *
 * Returns:
 * The module object.
 */
type_vmObj vm_import_module(type_vm *tp, const char * fname, const char * name, void *codes, int len) {
    type_vmObj f = fname?vm_string(fname):vm_none;
    type_vmObj bc = codes?vm_string_n((const char*)codes,len):vm_none;
    return vm_import_sub(tp,f,vm_string(name),bc);
}



type_vmObj vm_exec_sub(type_vm *tp) {
    type_vmObj code = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj globals = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj r = vm_none;
    vm_frame(tp,globals,code,&r);
    vm_run(tp,tp->cur);
    return r;
}


type_vmObj vm_import(type_vm *tp) {
    type_vmObj mod = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj r;

    if (vm_operations_haskey(tp,tp->modules,mod).number.val) {
        return vm_operations_get(tp,tp->modules,mod);
    }

    r = vm_import_sub(tp,vm_operations_add(tp,mod,vm_string(".tpc")),mod,vm_none);
    return r;
}

void vm_regbuiltins(type_vm *tp) {
    type_vmObj o;
    struct {const char *s;void *f;} b[] = {
    {"print",vm_builtins_print}, {"range",vm_builtins_range}, {"min",vm_builtins_min},
    {"max",vm_builtins_max}, {"bind",vm_builtins_bind}, {"copy",vm_builtins_copy},
    {"import",vm_import}, {"len",vm_builtins_len}, {"assert",vm_builtins_assert},
    {"str",vm_string_str2}, {"float",vm_builtins_float}, {"system",vm_builtins_system},
    {"istype",vm_builtins_istype}, {"chr",vm_string_chr}, {"save",vm_builtins_save},
    {"load",vm_builtins_load}, {"fpack",vm_builtins_fpack}, {"abs",vm_builtins_abs},
    {"int",vm_builtins_int}, {"exec",vm_exec_sub}, {"exists",vm_builtins_exists},
    {"mtime",vm_builtins_mtime}, {"number",vm_builtins_float}, {"round",vm_builtins_round},
    {"ord",vm_string_ord}, {"merge",vm_dict_merge}, {"getraw",vm_builtins_getraw},
    {"setmeta",vm_builtins_setmeta}, {"getmeta",vm_builtins_getmeta},
    {"bool", vm_builtins_bool},
    {0,0},
    };
    int i; for(i=0; b[i].s; i++) {
        vm_operations_set(tp,tp->builtins,vm_string(b[i].s),vm_misc_fnc(tp,(type_vmObj (*)(type_vm *))b[i].f));
    }

    o = vm_builtins_object(tp);
    vm_operations_set(tp,o,vm_string("__call__"),vm_misc_fnc(tp,vm_builtins_object_call));
    vm_operations_set(tp,o,vm_string("__new__"),vm_misc_fnc(tp,vm_builtins_object_new));
    vm_operations_set(tp,tp->builtins,vm_string("object"),o);
}


void vm_args(type_vm *tp,int argc, char *argv[]) {
    type_vmObj self = vm_list(tp);
    int i;
    for (i=1; i<argc; i++) { vm_list_append(tp,self.list.val,vm_string(argv[i])); }
    vm_operations_set(tp,tp->builtins,vm_string("ARGV"),self);
}

type_vmObj interpreter_main(type_vm *tp,char *fname, void *code, int len) {
    return vm_import_module(tp,fname,"__main__",code, len);
}

/* Function: interpreter_compile
 * Compile some interpreter code.
 *
 */
type_vmObj interpreter_compile(type_vm *tp, type_vmObj text, type_vmObj fname) {
    return vm_call(tp,"BUILTINS","compile",vm_misc_params_v(tp,2,text,fname));
}

/* Function: interpreter_exec
 * Execute VM code.
 */
type_vmObj interpreter_exec(type_vm *tp, type_vmObj code, type_vmObj globals) {
    type_vmObj r=vm_none;
    vm_frame(tp,globals,code,&r);
    vm_run(tp,tp->cur);
    return r;
}

type_vmObj interpreter_eval(type_vm *tp, const char *text, type_vmObj globals) {
    type_vmObj code = interpreter_compile(tp,vm_string(text),vm_string("<eval>"));
    return interpreter_exec(tp,code,globals);
}

/* Function: vm_init
 * Initializes a new virtual machine.
 *
 * The given parameters have the same format as the parameters to main, and
 * allow passing arguments to your interpreter scripts.
 *
 * Returns:
 * The newly created interpreter instance.
 */
type_vm *vm_init(int argc, char *argv[]) {
    type_vm *vm = sub_vm_init();
    vm_regbuiltins(vm);
    vm_args(vm,argc,argv);
    vm_compiler(vm);
    return vm;
}

/**/




type_vmObj vm_none = {vm_enum1_none};

#include "tokens.c"
void vm_compiler(type_vm *tp) {
    vm_import_module(tp,0,"tokenize",vm_tokens_tokenize,sizeof(vm_tokens_tokenize));
    vm_import_module(tp,0,"parse",interpreter_parse,sizeof(interpreter_parse));
    vm_import_module(tp,0,"encode",vm_tokens_encode,sizeof(vm_tokens_encode));
    vm_import_module(tp,0,"obfuscatedDataType",interpreter_obfuscatedArray,sizeof(interpreter_obfuscatedArray));
    vm_call(tp,"obfuscatedDataType","_init",vm_none);
}

/**/
