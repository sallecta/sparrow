/* File: VM
 * Functionality pertaining to the virtual machine.
 */

 #include "defines/dict.sys.h"

type_vm *_interpreter_init(void) {
    int i;
    type_vm *tp = (type_vm*)calloc(sizeof(type_vm),1);
    tp->time_limit = interpreter_NO_LIMIT;
    tp->clocks = clock();
    tp->time_elapsed = 0.0;
    tp->mem_limit = interpreter_NO_LIMIT;
    tp->mem_exceeded = 0;
    tp->mem_used = sizeof(type_vm);
    tp->cur = 0;
    tp->jmp = 0;
    tp->ex = interpreter_None;
    tp->root = interpreter_list_nt(tp);
    for (i=0; i<256; i++) { tp->chars[i][0]=i; }
    interpreter_gc_init(tp);
    tp->_regs = interpreter_list(tp);
    for (i=0; i<interpreter_REGS; i++) { interpreter_set(tp,tp->_regs,interpreter_None,interpreter_None); }
    tp->builtins = interpreter_dict(tp);
    tp->modules = interpreter_dict(tp);
    tp->_params = interpreter_list(tp);
    for (i=0; i<interpreter_FRAMES; i++) { interpreter_set(tp,tp->_params,interpreter_None,interpreter_list(tp)); }
    interpreter_set(tp,tp->root,interpreter_None,tp->builtins);
    interpreter_set(tp,tp->root,interpreter_None,tp->modules);
    interpreter_set(tp,tp->root,interpreter_None,tp->_regs);
    interpreter_set(tp,tp->root,interpreter_None,tp->_params);
    interpreter_set(tp,tp->builtins,interpreter_string("MODULES"),tp->modules);
    interpreter_set(tp,tp->modules,interpreter_string("BUILTINS"),tp->builtins);
    interpreter_set(tp,tp->builtins,interpreter_string("BUILTINS"),tp->builtins);
    interpreter_obj sys = interpreter_dict(tp);
    /*defines from interpreter/defines*/
    /*interpreter_set(tp, sys, interpreter_string("version"), interpreter_string("interpreter 1.2+SVN"));*/
    interpreter_set(tp, sys, interpreter_string("version"), interpreter_string(config_sys_version));
    interpreter_set(tp, sys, interpreter_string("flags"), interpreter_string(config_sys_flags));
    interpreter_set(tp, sys, interpreter_string("platform"), interpreter_string(config_sys_platform));
    /*end defines from interpreter/defines*/
    interpreter_set(tp,tp->modules, interpreter_string("sys"), sys);
    tp->regs = tp->_regs.list.val->items;
    interpreter_full(tp);
    return tp;
}


/* Function: interpreter_deinit
 * Destroys a VM instance.
 *
 * When you no longer need an instance of interpreter, you can use this to free all
 * memory used by it. Even when you are using only a single interpreter instance, it
 * may be good practice to call this function on shutdown.
 */
void interpreter_deinit(TP) {
    while (tp->root.list.val->len) {
        _interpreter_list_pop(tp,tp->root.list.val,0,"interpreter_deinit");
    }
    interpreter_full(tp); interpreter_full(tp);
    interpreter_delete(tp,tp->root);
    interpreter_gc_deinit(tp);
    tp->mem_used -= sizeof(type_vm);
    free(tp);
}

/* interpreter_frame_*/
void interpreter_frame(TP,interpreter_obj globals,interpreter_obj code,interpreter_obj *ret_dest) {
    interpreter_frame_ f;
    f.globals = globals;
    f.code = code;
    f.cur = (interpreter_code*)f.code.string.val;
    f.jmp = 0;
/*     fprintf(stderr,"tp->cur: %d\n",tp->cur);*/
    f.regs = (tp->cur <= 0?tp->regs:tp->frames[tp->cur].regs+tp->frames[tp->cur].cregs);

    f.regs[0] = f.globals;
    f.regs[1] = f.code;
    f.regs += interpreter_REGS_EXTRA;

    f.ret_dest = ret_dest;
    f.lineno = 0;
    f.line = interpreter_string("");
    f.name = interpreter_string("?");
    f.fname = interpreter_string("?");
    f.cregs = 0;
/*     return f;*/
    if (f.regs+(256+interpreter_REGS_EXTRA) >= tp->regs+interpreter_REGS || tp->cur >= interpreter_FRAMES-1) {
        interpreter_raise(,interpreter_string("(interpreter_frame) RuntimeError: stack overflow"));
    }
    tp->cur += 1;
    tp->frames[tp->cur] = f;
}

void _interpreter_raise(TP,interpreter_obj e) {
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

void interpreter_print_stack(TP) {
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

void interpreter_handle(TP) {
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
interpreter_obj interpreter_call(TP,interpreter_obj self, interpreter_obj params) {
    /* I'm not sure we should have to do this, but
    just for giggles we will. */
    tp->params = params;

    if (self.type == interpreter_DICT) {
        if (self.dict.dtype == 1) {
            interpreter_obj meta; if (_interpreter_lookup(tp,self,interpreter_string("__new__"),&meta)) {
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
        interpreter_obj r = _interpreter_tcall(tp,self);
        interpreter_grey(tp,r);
        return r;
    }
    if (self.type == interpreter_FNC) {
        interpreter_obj dest = interpreter_None;
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


void interpreter_return(TP, interpreter_obj v) {
    interpreter_obj *dest = tp->frames[tp->cur].ret_dest;
    if (dest) { *dest = v; interpreter_grey(tp,v); }
/*     memset(tp->frames[tp->cur].regs,0,interpreter_REGS_PER_FRAME*sizeof(interpreter_obj));
       fprintf(stderr,"regs:%d\n",(tp->frames[tp->cur].cregs+1));*/
    memset(tp->frames[tp->cur].regs-interpreter_REGS_EXTRA,0,(interpreter_REGS_EXTRA+tp->frames[tp->cur].cregs)*sizeof(interpreter_obj));
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


int interpreter_step(TP) {
    interpreter_frame_ *f = &tp->frames[tp->cur];
    interpreter_obj *regs = f->regs;
    interpreter_code *cur = f->cur;
    while(1) {
    interpreter_code e = *cur;
    /*
     fprintf(stderr,"%2d.%4d: %-6s %3d %3d %3d\n",tp->cur,cur - (interpreter_code*)f->code.string.val,interpreter_strings[e.i],VA,VB,VC);
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
            RA = interpreter_number(*(interpreter_num*)(*++cur).string.val);
            cur += sizeof(interpreter_num)/4;
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

void _interpreter_run(TP,int cur) {
    tp->jmp += 1; if (setjmp(tp->buf)) { interpreter_handle(tp); }
    while (tp->cur >= cur && interpreter_step(tp) != -1);
    tp->jmp -= 1;
}

void interpreter_run(TP,int cur) {
    jmp_buf tmp;
    memcpy(tmp,tp->buf,sizeof(jmp_buf));
    _interpreter_run(tp,cur);
    memcpy(tp->buf,tmp,sizeof(jmp_buf));
}


interpreter_obj interpreter_ez_call(TP, const char *mod, const char *fnc, interpreter_obj params) {
    interpreter_obj tmp;
    tmp = interpreter_get(tp,tp->modules,interpreter_string(mod));
    tmp = interpreter_get(tp,tmp,interpreter_string(fnc));
    return interpreter_call(tp,tmp,params);
}

interpreter_obj _interpreter_import(TP, interpreter_obj fname, interpreter_obj name, interpreter_obj code) {
    interpreter_obj g;

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
interpreter_obj interpreter_import(TP, const char * fname, const char * name, void *codes, int len) {
    interpreter_obj f = fname?interpreter_string(fname):interpreter_None;
    interpreter_obj bc = codes?interpreter_string_n((const char*)codes,len):interpreter_None;
    return _interpreter_import(tp,f,interpreter_string(name),bc);
}



interpreter_obj interpreter_exec_(TP) {
    interpreter_obj code = interpreter_OBJ();
    interpreter_obj globals = interpreter_OBJ();
    interpreter_obj r = interpreter_None;
    interpreter_frame(tp,globals,code,&r);
    interpreter_run(tp,tp->cur);
    return r;
}


interpreter_obj interpreter_import_(TP) {
    interpreter_obj mod = interpreter_OBJ();
    interpreter_obj r;

    if (interpreter_has(tp,tp->modules,mod).number.val) {
        return interpreter_get(tp,tp->modules,mod);
    }

    r = _interpreter_import(tp,interpreter_add(tp,mod,interpreter_string(".tpc")),mod,interpreter_None);
    return r;
}

void interpreter_builtins(TP) {
    interpreter_obj o;
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
        interpreter_set(tp,tp->builtins,interpreter_string(b[i].s),interpreter_fnc(tp,(interpreter_obj (*)(type_vm *))b[i].f));
    }

    o = interpreter_object(tp);
    interpreter_set(tp,o,interpreter_string("__call__"),interpreter_fnc(tp,interpreter_object_call));
    interpreter_set(tp,o,interpreter_string("__new__"),interpreter_fnc(tp,interpreter_object_new));
    interpreter_set(tp,tp->builtins,interpreter_string("object"),o);
}


void interpreter_args(TP,int argc, char *argv[]) {
    interpreter_obj self = interpreter_list(tp);
    int i;
    for (i=1; i<argc; i++) { _interpreter_list_append(tp,self.list.val,interpreter_string(argv[i])); }
    interpreter_set(tp,tp->builtins,interpreter_string("ARGV"),self);
}

interpreter_obj interpreter_main(TP,char *fname, void *code, int len) {
    return interpreter_import(tp,fname,"__main__",code, len);
}

/* Function: interpreter_compile
 * Compile some interpreter code.
 *
 */
interpreter_obj interpreter_compile(TP, interpreter_obj text, interpreter_obj fname) {
    return interpreter_ez_call(tp,"BUILTINS","compile",interpreter_params_v(tp,2,text,fname));
}

/* Function: interpreter_exec
 * Execute VM code.
 */
interpreter_obj interpreter_exec(TP, interpreter_obj code, interpreter_obj globals) {
    interpreter_obj r=interpreter_None;
    interpreter_frame(tp,globals,code,&r);
    interpreter_run(tp,tp->cur);
    return r;
}

interpreter_obj interpreter_eval(TP, const char *text, interpreter_obj globals) {
    interpreter_obj code = interpreter_compile(tp,interpreter_string(text),interpreter_string("<eval>"));
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
    type_vm *tp = _interpreter_init();
    interpreter_builtins(tp);
    interpreter_args(tp,argc,argv);
    interpreter_compiler(tp);
    return tp;
}

/**/
