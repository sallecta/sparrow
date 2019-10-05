/* File: VM
 * Functionality pertaining to the virtual machine.
 */

 #include "defines/dict.sys.h"

type_vm *sub_interpreterInit(void) {
    int i;
    type_vm *vm = (type_vm*)calloc(sizeof(type_vm),1);
    vm->time_limit = interpreter_NO_LIMIT;
    vm->clocks = clock();
    vm->time_elapsed = 0.0;
    vm->mem_limit = interpreter_NO_LIMIT;
    vm->mem_exceeded = 0;
    vm->mem_used = sizeof(type_vm);
    vm->cur = 0;
    vm->jmp = 0;
    vm->ex = interpreter_None;
    vm->root = interpreter_list_nt(vm);
    for (i=0; i<256; i++) { vm->chars[i][0]=i; }
    interpreter_gc_init(vm);
    vm->regs_sub = interpreter_list(vm);
    for (i=0; i<vm_REGS; i++) { interpreter_set(vm,vm->regs_sub,interpreter_None,interpreter_None); }
    vm->builtins = interpreter_dict(vm);
    vm->modules = interpreter_dict(vm);
    vm->params_sub = interpreter_list(vm);
    for (i=0; i<vm_FRAMES; i++) { interpreter_set(vm,vm->params_sub,interpreter_None,interpreter_list(vm)); }
    interpreter_set(vm,vm->root,interpreter_None,vm->builtins);
    interpreter_set(vm,vm->root,interpreter_None,vm->modules);
    interpreter_set(vm,vm->root,interpreter_None,vm->regs_sub);
    interpreter_set(vm,vm->root,interpreter_None,vm->params_sub);
    interpreter_set(vm,vm->builtins,interpreter_string("MODULES"),vm->modules);
    interpreter_set(vm,vm->modules,interpreter_string("BUILTINS"),vm->builtins);
    interpreter_set(vm,vm->builtins,interpreter_string("BUILTINS"),vm->builtins);
    type_vmObj sys = interpreter_dict(vm);
    /*defines from interpreter/defines*/
    /*interpreter_set(vm, sys, interpreter_string("version"), interpreter_string("interpreter 1.2+SVN"));*/
    interpreter_set(vm, sys, interpreter_string("version"), interpreter_string(config_sys_version));
    interpreter_set(vm, sys, interpreter_string("flags"), interpreter_string(config_sys_flags));
    interpreter_set(vm, sys, interpreter_string("platform"), interpreter_string(config_sys_platform));
    /*end defines from interpreter/defines*/
    interpreter_set(vm,vm->modules, interpreter_string("sys"), sys);
    vm->regs = vm->regs_sub.list.val->items;
    interpreter_full(vm);
    return vm;
}


/* Function: interpreter_deinit
 * Destroys a VM instance.
 *
 * When you no longer need an instance of interpreter, you can use this to free all
 * memory used by it. Even when you are using only a single interpreter instance, it
 * may be good practice to call this function on shutdown.
 */
void interpreter_deinit(type_vm *tp) {
    while (tp->root.list.val->len) {
        _interpreter_list_pop(tp,tp->root.list.val,0,"interpreter_deinit");
    }
    interpreter_full(tp); interpreter_full(tp);
    interpreter_delete(tp,tp->root);
    interpreter_gc_deinit(tp);
    tp->mem_used -= sizeof(type_vm);
    free(tp);
}

/* type_vmFrame*/
void interpreter_frame(type_vm *tp,type_vmObj globals,type_vmObj code,type_vmObj *ret_dest) {
    type_vmFrame f;
    f.globals = globals;
    f.code = code;
    f.cur = (type_vmCode*)f.code.string.val;
    f.jmp = 0;
/*     fprintf(stderr,"tp->cur: %d\n",tp->cur);*/
    f.regs = (tp->cur <= 0?tp->regs:tp->frames[tp->cur].regs+tp->frames[tp->cur].cregs);

    f.regs[0] = f.globals;
    f.regs[1] = f.code;
    f.regs += vm_REGS_EXTRA;

    f.ret_dest = ret_dest;
    f.lineno = 0;
    f.line = interpreter_string("");
    f.name = interpreter_string("?");
    f.fname = interpreter_string("?");
    f.cregs = 0;
/*     return f;*/
    if (f.regs+(256+vm_REGS_EXTRA) >= tp->regs+vm_REGS || tp->cur >= vm_FRAMES-1) {
        interpreter_raise(,interpreter_string("(interpreter_frame) RuntimeError: stack overflow"));
    }
    tp->cur += 1;
    tp->frames[tp->cur] = f;
}

void _interpreter_raise(type_vm *tp,type_vmObj e) {
    /*char *x = 0; x[0]=0;*/
    if (!tp || !tp->jmp) {
#ifndef CPYTHON_MOD
        printf("\nException:\n"); interpreter_echo(tp,e); printf("\n");
        exit(-1);
#else
        tp->ex = e;
        longjmp(tp->nextexpr,1);
#endif
    }
    if (e.type != interpreter_NONE) { tp->ex = e; }
    interpreter_grey(tp,e);
    longjmp(tp->buf,1);
}

void interpreter_print_stack(type_vm *tp) {
    int i;
    printf("\n");
    for (i=0; i<=tp->cur; i++) {
        if (!tp->frames[i].lineno) { continue; }
        printf("File \""); interpreter_echo(tp,tp->frames[i].fname); printf("\", ");
        printf("line %d, in ",tp->frames[i].lineno);
        interpreter_echo(tp,tp->frames[i].name); printf("\n ");
        interpreter_echo(tp,tp->frames[i].line); printf("\n");
    }
    printf("\nException:\n"); interpreter_echo(tp,tp->ex); printf("\n");
}

void interpreter_handle(type_vm *tp) {
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
#ifndef CPYTHON_MOD
    interpreter_print_stack(tp);
    exit(-1);
#else
    longjmp(tp->nextexpr,1);
#endif
}

/* Function: interpreter_call
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
 * > interpreter_call(tp,
 * >     interpreter_get(tp, tp->builtins, interpreter_string("foo")),
 * >     interpreter_params_v(tp, interpreter_string("hello")))
 * This will look for a global function named "foo", then call it with a single
 * positional parameter containing the string "hello".
 */
type_vmObj interpreter_call(type_vm *tp,type_vmObj self, type_vmObj params) {
    /* I'm not sure we should have to do this, but
    just for giggles we will. */
    tp->params = params;

    if (self.type == interpreter_DICT) {
        if (self.dict.dtype == 1) {
            type_vmObj meta; if (_interpreter_lookup(tp,self,interpreter_string("__new__"),&meta)) {
                _interpreter_list_insert(tp,params.list.val,0,self);
                return interpreter_call(tp,meta,params);
            }
        } else if (self.dict.dtype == 2) {
            interpreter_META_BEGIN(self,"__call__");
                return interpreter_call(tp,meta,params);
            interpreter_META_END;
        }
    }
    if (self.type == interpreter_FNC && !(self.fnc.ftype&1)) {
        type_vmObj r = _interpreter_tcall(tp,self);
        interpreter_grey(tp,r);
        return r;
    }
    if (self.type == interpreter_FNC) {
        type_vmObj dest = interpreter_None;
        interpreter_frame(tp,self.fnc.info->globals,self.fnc.info->code,&dest);
        if ((self.fnc.ftype&2)) {
            tp->frames[tp->cur].regs[0] = params;
            _interpreter_list_insert(tp,params.list.val,0,self.fnc.info->self);
        } else {
            tp->frames[tp->cur].regs[0] = params;
        }
        interpreter_run(tp,tp->cur);
        return dest;
    }
    interpreter_params_v(tp,1,self); interpreter_print(tp);
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_call) TypeError: object is not callable"));
}


void interpreter_return(type_vm *tp, type_vmObj v) {
    type_vmObj *dest = tp->frames[tp->cur].ret_dest;
    if (dest) { *dest = v; interpreter_grey(tp,v); }
/*     memset(tp->frames[tp->cur].regs,0,interpreter_REGS_PER_FRAME*sizeof(type_vmObj));
       fprintf(stderr,"regs:%d\n",(tp->frames[tp->cur].cregs+1));*/
    memset(tp->frames[tp->cur].regs-vm_REGS_EXTRA,0,(vm_REGS_EXTRA+tp->frames[tp->cur].cregs)*sizeof(type_vmObj));
    tp->cur -= 1;
}

enum {
    interpreter_IEOF,interpreter_IADD,interpreter_ISUB,interpreter_IMUL,interpreter_IDIV,interpreter_IPOW,interpreter_IBITAND,interpreter_IBITOR,interpreter_ICMP,interpreter_IGET,interpreter_ISET,
    interpreter_INUMBER,interpreter_ISTRING,interpreter_IGGET,interpreter_IGSET,interpreter_IMOVE,interpreter_IDEF,interpreter_IPASS,interpreter_IJUMP,interpreter_ICALL,
    interpreter_IRETURN,interpreter_IIF,interpreter_IDEBUG,interpreter_IEQ,interpreter_ILE,interpreter_ILT,interpreter_IDICT,interpreter_ILIST,interpreter_INONE,interpreter_ILEN,
    interpreter_ILINE,interpreter_IPARAMS,interpreter_IIGET,interpreter_IFILE,interpreter_INAME,interpreter_INE,interpreter_IHAS,interpreter_IRAISE,interpreter_ISETJMP,
    interpreter_IMOD,interpreter_ILSH,interpreter_IRSH,interpreter_IITER,interpreter_IDEL,interpreter_IREGS,interpreter_IBITXOR, interpreter_IIFN,
    interpreter_INOT, interpreter_IBITNOT,
    interpreter_ITOTAL
};

/* char *interpreter_strings[interpreter_ITOTAL] = {
       "EOF","ADD","SUB","MUL","DIV","POW","BITAND","BITOR","CMP","GET","SET","NUM",
       "STR","GGET","GSET","MOVE","DEF","PASS","JUMP","CALL","RETURN","IF","DEBUG",
       "EQ","LE","LT","DICT","LIST","NONE","LEN","LINE","PARAMS","IGET","FILE",
       "NAME","NE","HAS","RAISE","SETJMP","MOD","LSH","RSH","ITER","DEL","REGS",
       "BITXOR", "IFN", "NOT", "BITNOT",
   };*/

#define VA ((int)e.regs.a)
#define VB ((int)e.regs.b)
#define VC ((int)e.regs.c)
#define RA regs[e.regs.a]
#define RB regs[e.regs.b]
#define RC regs[e.regs.c]
#define UVBC (unsigned short)(((VB<<8)+VC))
#define SVBC (short)(((VB<<8)+VC))
#define GA interpreter_grey(tp,RA)
#define SR(v) f->cur = cur; return(v);


int interpreter_step(type_vm *tp) {
    type_vmFrame *f = &tp->frames[tp->cur];
    type_vmObj *regs = f->regs;
    type_vmCode *cur = f->cur;
    while(1) {
    type_vmCode e = *cur;
    /*
     fprintf(stderr,"%2d.%4d: %-6s %3d %3d %3d\n",tp->cur,cur - (type_vmCode*)f->code.string.val,interpreter_strings[e.i],VA,VB,VC);
       int i; for(i=0;i<16;i++) { fprintf(stderr,"%d: %s\n",i,interpreter_xSTR(regs[i])); }
    */
    switch (e.i) {
        case interpreter_IEOF: interpreter_return(tp,interpreter_None); SR(0); break;
        case interpreter_IADD: RA = interpreter_add(tp,RB,RC); break;
        case interpreter_ISUB: RA = interpreter_sub(tp,RB,RC); break;
        case interpreter_IMUL: RA = interpreter_mul(tp,RB,RC); break;
        case interpreter_IDIV: RA = interpreter_div(tp,RB,RC); break;
        case interpreter_IPOW: RA = interpreter_pow(tp,RB,RC); break;
        case interpreter_IBITAND: RA = interpreter_bitwise_and(tp,RB,RC); break;
        case interpreter_IBITOR:  RA = interpreter_bitwise_or(tp,RB,RC); break;
        case interpreter_IBITXOR:  RA = interpreter_bitwise_xor(tp,RB,RC); break;
        case interpreter_IMOD:  RA = interpreter_mod(tp,RB,RC); break;
        case interpreter_ILSH:  RA = interpreter_lsh(tp,RB,RC); break;
        case interpreter_IRSH:  RA = interpreter_rsh(tp,RB,RC); break;
        case interpreter_ICMP: RA = interpreter_number(interpreter_cmp(tp,RB,RC)); break;
        case interpreter_INE: RA = interpreter_number(interpreter_cmp(tp,RB,RC)!=0); break;
        case interpreter_IEQ: RA = interpreter_number(interpreter_cmp(tp,RB,RC)==0); break;
        case interpreter_ILE: RA = interpreter_number(interpreter_cmp(tp,RB,RC)<=0); break;
        case interpreter_ILT: RA = interpreter_number(interpreter_cmp(tp,RB,RC)<0); break;
        case interpreter_IBITNOT:  RA = interpreter_bitwise_not(tp,RB); break;
        case interpreter_INOT: RA = interpreter_number(!interpreter_bool(tp,RB)); break;
        case interpreter_IPASS: break;
        case interpreter_IIF: if (interpreter_bool(tp,RA)) { cur += 1; } break;
        case interpreter_IIFN: if (!interpreter_bool(tp,RA)) { cur += 1; } break;
        case interpreter_IGET: RA = interpreter_get(tp,RB,RC); GA; break;
        case interpreter_IITER:
            if (RC.number.val < interpreter_len(tp,RB).number.val) {
                RA = interpreter_iter(tp,RB,RC); GA;
                RC.number.val += 1;
                cur += 1;
            }
            break;
        case interpreter_IHAS: RA = interpreter_has(tp,RB,RC); break;
        case interpreter_IIGET: interpreter_iget(tp,&RA,RB,RC); break;
        case interpreter_ISET: interpreter_set(tp,RA,RB,RC); break;
        case interpreter_IDEL: interpreter_del(tp,RA,RB); break;
        case interpreter_IMOVE: RA = RB; break;
        case interpreter_INUMBER:
            RA = interpreter_number(*(type_vmNum*)(*++cur).string.val);
            cur += sizeof(type_vmNum)/4;
            continue;
        case interpreter_ISTRING: {
            /* RA = interpreter_string_n((*(cur+1)).string.val,UVBC); */
            int a = (*(cur+1)).string.val-f->code.string.val;
            RA = interpreter_string_sub(tp,f->code,a,a+UVBC),
            cur += (UVBC/4)+1;
            }
            break;
        case interpreter_IDICT: RA = interpreter_dict_n(tp,VC/2,&RB); break;
        case interpreter_ILIST: RA = interpreter_list_n(tp,VC,&RB); break;
        case interpreter_IPARAMS: RA = interpreter_params_n(tp,VC,&RB); break;
        case interpreter_ILEN: RA = interpreter_len(tp,RB); break;
        case interpreter_IJUMP: cur += SVBC; continue; break;
        case interpreter_ISETJMP: f->jmp = SVBC?cur+SVBC:0; break;
        case interpreter_ICALL:
            f->cur = cur + 1;  RA = interpreter_call(tp,RB,RC); GA;
            return 0; break;
        case interpreter_IGGET:
            if (!interpreter_iget(tp,&RA,f->globals,RB)) {
                RA = interpreter_get(tp,tp->builtins,RB); GA;
            }
            break;
        case interpreter_IGSET: interpreter_set(tp,f->globals,RA,RB); break;
        case interpreter_IDEF: {
            int a = (*(cur+1)).string.val-f->code.string.val;
            RA = interpreter_def(tp,
                /*interpreter_string_n((*(cur+1)).string.val,(SVBC-1)*4),*/
                interpreter_string_sub(tp,f->code,a,a+(SVBC-1)*4),
                f->globals);
            cur += SVBC; continue;
            }
            break;

        case interpreter_IRETURN: interpreter_return(tp,RA); SR(0); break;
        case interpreter_IRAISE: _interpreter_raise(tp,RA); SR(0); break;
        case interpreter_IDEBUG:
            interpreter_params_v(tp,3,interpreter_string("DEBUG:"),interpreter_number(VA),RA); interpreter_print(tp);
            break;
        case interpreter_INONE: RA = interpreter_None; break;
        case interpreter_ILINE:
            ;
            int a = (*(cur+1)).string.val-f->code.string.val;
            f->line = interpreter_string_sub(tp,f->code,a,a+VA*4-1);
            cur += VA; f->lineno = UVBC;
            break;
        case interpreter_IFILE: f->fname = RA; break;
        case interpreter_INAME: f->name = RA; break;
        case interpreter_IREGS: f->cregs = VA; break;
        default:
            interpreter_raise(0,interpreter_string("(interpreter_step) RuntimeError: invalid instruction"));
            break;
    }
    cur += 1;
    }
    SR(0);
}

void _interpreter_run(type_vm *tp,int cur) {
    tp->jmp += 1; if (setjmp(tp->buf)) { interpreter_handle(tp); }
    while (tp->cur >= cur && interpreter_step(tp) != -1);
    tp->jmp -= 1;
}

void interpreter_run(type_vm *tp,int cur) {
    jmp_buf tmp;
    memcpy(tmp,tp->buf,sizeof(jmp_buf));
    _interpreter_run(tp,cur);
    memcpy(tp->buf,tmp,sizeof(jmp_buf));
}


type_vmObj interpreter_ez_call(type_vm *tp, const char *mod, const char *fnc, type_vmObj params) {
    type_vmObj tmp;
    tmp = interpreter_get(tp,tp->modules,interpreter_string(mod));
    tmp = interpreter_get(tp,tmp,interpreter_string(fnc));
    return interpreter_call(tp,tmp,params);
}

type_vmObj _interpreter_import(type_vm *tp, type_vmObj fname, type_vmObj name, type_vmObj code) {
    type_vmObj g;

    if (!((fname.type != interpreter_NONE && _interpreter_str_index(fname,interpreter_string(".tpc"))!=-1) || code.type != interpreter_NONE)) {
        return interpreter_ez_call(tp,"obfuscatedDataType","import_fname",interpreter_params_v(tp,2,fname,name));
    }

    if (code.type == interpreter_NONE) {
        interpreter_params_v(tp,1,fname);
        code = interpreter_load(tp);
    }

    g = interpreter_dict(tp);
    interpreter_set(tp,g,interpreter_string("__name__"),name);
    interpreter_set(tp,g,interpreter_string("__code__"),code);
    interpreter_set(tp,g,interpreter_string("__dict__"),g);
    interpreter_frame(tp,g,code,0);
    interpreter_set(tp,tp->modules,name,g);

    if (!tp->jmp) { interpreter_run(tp,tp->cur); }

    return g;
}


/* Function: interpreter_import
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
type_vmObj interpreter_import(type_vm *tp, const char * fname, const char * name, void *codes, int len) {
    type_vmObj f = fname?interpreter_string(fname):interpreter_None;
    type_vmObj bc = codes?interpreter_string_n((const char*)codes,len):interpreter_None;
    return _interpreter_import(tp,f,interpreter_string(name),bc);
}



type_vmObj interpreter_exec_(type_vm *tp) {
    type_vmObj code = interpreter_OBJ();
    type_vmObj globals = interpreter_OBJ();
    type_vmObj r = interpreter_None;
    interpreter_frame(tp,globals,code,&r);
    interpreter_run(tp,tp->cur);
    return r;
}


type_vmObj interpreter_import_(type_vm *tp) {
    type_vmObj mod = interpreter_OBJ();
    type_vmObj r;

    if (interpreter_has(tp,tp->modules,mod).number.val) {
        return interpreter_get(tp,tp->modules,mod);
    }

    r = _interpreter_import(tp,interpreter_add(tp,mod,interpreter_string(".tpc")),mod,interpreter_None);
    return r;
}

void interpreter_builtins(type_vm *tp) {
    type_vmObj o;
    struct {const char *s;void *f;} b[] = {
    {"print",interpreter_print}, {"range",interpreter_range}, {"min",interpreter_min},
    {"max",interpreter_max}, {"bind",interpreter_bind}, {"copy",interpreter_copy},
    {"import",interpreter_import_}, {"len",interpreter_len_}, {"assert",interpreter_assert},
    {"str",interpreter_str2}, {"float",interpreter_float}, {"system",interpreter_system},
    {"istype",interpreter_istype}, {"chr",interpreter_chr}, {"save",interpreter_save},
    {"load",interpreter_load}, {"fpack",interpreter_fpack}, {"abs",interpreter_abs},
    {"int",interpreter_int}, {"exec",interpreter_exec_}, {"exists",interpreter_exists},
    {"mtime",interpreter_mtime}, {"number",interpreter_float}, {"round",interpreter_round},
    {"ord",interpreter_ord}, {"merge",interpreter_merge}, {"getraw",interpreter_getraw},
    {"setmeta",interpreter_setmeta}, {"getmeta",interpreter_getmeta},
    {"bool", interpreter_builtins_bool},
    {0,0},
    };
    int i; for(i=0; b[i].s; i++) {
        interpreter_set(tp,tp->builtins,interpreter_string(b[i].s),interpreter_fnc(tp,(type_vmObj (*)(type_vm *))b[i].f));
    }

    o = interpreter_object(tp);
    interpreter_set(tp,o,interpreter_string("__call__"),interpreter_fnc(tp,interpreter_object_call));
    interpreter_set(tp,o,interpreter_string("__new__"),interpreter_fnc(tp,interpreter_object_new));
    interpreter_set(tp,tp->builtins,interpreter_string("object"),o);
}


void interpreter_args(type_vm *tp,int argc, char *argv[]) {
    type_vmObj self = interpreter_list(tp);
    int i;
    for (i=1; i<argc; i++) { _interpreter_list_append(tp,self.list.val,interpreter_string(argv[i])); }
    interpreter_set(tp,tp->builtins,interpreter_string("ARGV"),self);
}

type_vmObj interpreter_main(type_vm *tp,char *fname, void *code, int len) {
    return interpreter_import(tp,fname,"__main__",code, len);
}

/* Function: interpreter_compile
 * Compile some interpreter code.
 *
 */
type_vmObj interpreter_compile(type_vm *tp, type_vmObj text, type_vmObj fname) {
    return interpreter_ez_call(tp,"BUILTINS","compile",interpreter_params_v(tp,2,text,fname));
}

/* Function: interpreter_exec
 * Execute VM code.
 */
type_vmObj interpreter_exec(type_vm *tp, type_vmObj code, type_vmObj globals) {
    type_vmObj r=interpreter_None;
    interpreter_frame(tp,globals,code,&r);
    interpreter_run(tp,tp->cur);
    return r;
}

type_vmObj interpreter_eval(type_vm *tp, const char *text, type_vmObj globals) {
    type_vmObj code = interpreter_compile(tp,interpreter_string(text),interpreter_string("<eval>"));
    return interpreter_exec(tp,code,globals);
}

/* Function: interpreter_init
 * Initializes a new virtual machine.
 *
 * The given parameters have the same format as the parameters to main, and
 * allow passing arguments to your interpreter scripts.
 *
 * Returns:
 * The newly created interpreter instance.
 */
type_vm *interpreter_init(int argc, char *argv[]) {
    type_vm *vm = sub_interpreterInit();
    interpreter_builtins(vm);
    interpreter_args(vm,argc,argv);
    interpreter_compiler(vm);
    return vm;
}

/**/
