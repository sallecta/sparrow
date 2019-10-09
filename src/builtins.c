/* File: Builtins
 * Builtin interpreter functions.
 */

type_vmObj vm_builtins_print(type_vm *tp) {
    int n = 0;
    type_vmObj e;
    interpreter_LOOP(e)
        if (n) { printf(" "); }
        vm_echo(tp,e);
        n += 1;
    interpreter_END;
    printf("\n");
    return vm_none;
}

type_vmObj vm_builtins_bind(type_vm *tp) {
    type_vmObj r = vm_typecheck(tp,vm_enum1_fnc,vm_operations_get(tp,tp->params,vm_none));
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    return vm_misc_fnc_new(tp,
        r.fnc.ftype|2,r.fnc.cfnc,r.fnc.info->code,
        self,r.fnc.info->globals);
}

type_vmObj vm_builtins_min(type_vm *tp) {
    type_vmObj r = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj e;
    interpreter_LOOP(e)
        if (vm_operations_cmp(tp,r,e) > 0) { r = e; }
    interpreter_END;
    return r;
}

type_vmObj vm_builtins_max(type_vm *tp) {
    type_vmObj r = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj e;
    interpreter_LOOP(e)
        if (vm_operations_cmp(tp,r,e) < 0) { r = e; }
    interpreter_END;
    return r;
}

type_vmObj vm_builtins_copy(type_vm *tp) {
    type_vmObj r = vm_operations_get(tp,tp->params,vm_none);
    int type = r.type;
    if (type == vm_enum1_list) {
        return vm_list_copy(tp,r);
    } else if (type == vm_enum1_dict) {
        return vm_dict_copy(tp,r);
    }
    vm_raise(tp,vm_string("(vm_builtins_copy) TypeError: ?"));
	return vm_none;
}


type_vmObj vm_builtins_len(type_vm *tp) {
    type_vmObj e = vm_operations_get(tp,tp->params,vm_none);
    return vm_operations_len(tp,e);
}

type_vmObj vm_builtins_assert(type_vm *tp) {
    int a = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val;
    if (a) { return vm_none; }
    vm_raise(tp,vm_string("(vm_builtins_assert) AssertionError"));
	return vm_none;
}

type_vmObj vm_builtins_range(type_vm *tp) {
    int a,b,c,i;
    type_vmObj r = vm_list(tp);
    switch (tp->params.list.val->len) {
        case 1: a = 0; b = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val; c = 1; break;
        case 2:
        case 3: a = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val; \
		b = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val; \
		c = vm_macros_DEFAULT(vm_create_numericObj(1)).number.val; break;
        default: return r;
    }
    if (c != 0) {
        for (i=a; (c>0) ? i<b : i>b; i+=c) {
            vm_list_append(tp,r.list.val,vm_create_numericObj(i));
        }
    }
    return r;
}

/* Function: vm_builtins_system
 *
 * The system builtin. A grave security flaw. If your version of interpreter
 * enables this, you better remove it before deploying your app :P
 */
type_vmObj vm_builtins_system(type_vm *tp) {
    char s[vm_def_CSTR_LEN]; interpreter_cstr(tp,vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none)),s,vm_def_CSTR_LEN);
    int r = system(s);
    return vm_create_numericObj(r);
}

type_vmObj vm_builtins_istype(type_vm *tp) {
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj t = vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none));
    if (vm_operations_cmp(tp,t,vm_string("string")) == 0) { return vm_create_numericObj(v.type == vm_enum1_string); }
    if (vm_operations_cmp(tp,t,vm_string("list")) == 0) { return vm_create_numericObj(v.type == vm_enum1_list); }
    if (vm_operations_cmp(tp,t,vm_string("dict")) == 0) { return vm_create_numericObj(v.type == vm_enum1_dict); }
    if (vm_operations_cmp(tp,t,vm_string("number")) == 0) { return vm_create_numericObj(v.type == vm_enum1_number); }
    if (vm_operations_cmp(tp,t,vm_string("fnc")) == 0) { return vm_create_numericObj(v.type == vm_enum1_fnc && (v.fnc.ftype&2) == 0); }
    if (vm_operations_cmp(tp,t,vm_string("method")) == 0) { return vm_create_numericObj(v.type == vm_enum1_fnc && (v.fnc.ftype&2) != 0); }
    vm_raise(tp,vm_string("(is_type) TypeError: ?"));
	return vm_none;
}


type_vmObj vm_builtins_float(type_vm *tp) {
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    int ord = vm_macros_DEFAULT(vm_create_numericObj(0)).number.val;
    int type = v.type;
    if (type == vm_enum1_number) { return v; }
    if (type == vm_enum1_string && v.string.len < 32) {
        char s[32]; memset(s,0,v.string.len+1);
        memcpy(s,v.string.val,v.string.len);
        if (strchr(s,'.')) { return vm_create_numericObj(atof(s)); }
        return(vm_create_numericObj(strtol(s,0,ord)));
    }
    vm_raise(tp,vm_string("(vm_builtins_float) TypeError: ?"));
	return vm_none;
}


type_vmObj vm_builtins_save(type_vm *tp) {
    char fname[256]; interpreter_cstr(tp,vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none)),fname,256);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    FILE *f;
    f = fopen(fname,"wb");
    if (!f) { vm_raise(tp,vm_string("(vm_builtins_save) IOError: ?")); }
    fwrite(v.string.val,v.string.len,1,f);
    fclose(f);
    return vm_none;
}

type_vmObj vm_builtins_load(type_vm *tp) {
    FILE *f;
    long l;
    type_vmObj r;
    char *s;
    char fname[256]; interpreter_cstr(tp,vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none)),fname,256);
    struct stat stbuf;
    stat(fname, &stbuf);
    l = stbuf.st_size;
    f = fopen(fname,"rb");
    if (!f) {
        vm_raise(tp,vm_string("(vm_builtins_load) IOError: ?"));
    }
    r = vm_string_new(tp,l);
    s = r.string.info->s;
    /*fread(s,1,l,f);*/
    if (fread(s,1,l,f)==0) { printf("fread is 0"); }
    fclose(f);
    return vm_gc_track(tp,r);
}


type_vmObj vm_builtins_fpack(type_vm *tp) {
    type_vmNum v = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val;
    type_vmObj r = vm_string_new(tp,sizeof(type_vmNum));
    *(type_vmNum*)r.string.val = v;
    return vm_gc_track(tp,r);
}

type_vmObj vm_builtins_abs(type_vm *tp) {
    return vm_create_numericObj(fabs(vm_builtins_float(tp).number.val));
}
type_vmObj vm_builtins_int(type_vm *tp) {
    return vm_create_numericObj((long)vm_builtins_float(tp).number.val);
}
type_vmNum roundf_sub(type_vmNum v) {
    type_vmNum av = fabs(v); type_vmNum iv = (long)av;
    av = (av-iv < 0.5?iv:iv+1);
    return (v<0?-av:av);
}
type_vmObj vm_builtins_round(type_vm *tp) {
    return vm_create_numericObj(roundf_sub(vm_builtins_float(tp).number.val));
}

type_vmObj vm_builtins_exists(type_vm *tp) {
    char fname[vm_def_CSTR_LEN]; interpreter_cstr(tp,vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none)),fname,vm_def_CSTR_LEN);
    struct stat stbuf;
    return vm_create_numericObj(!stat(fname,&stbuf));
}
type_vmObj vm_builtins_mtime(type_vm *tp) {
    char fname[vm_def_CSTR_LEN]; interpreter_cstr(tp,vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none)),fname,vm_def_CSTR_LEN);
    struct stat stbuf;
    if (!stat(fname,&stbuf)) { return vm_create_numericObj(stbuf.st_mtime); }
    vm_raise(tp,vm_string("(vm_builtins_mtime) IOError: ?"));
	return vm_none;
}

int vm_lookup_sub(type_vm *tp,type_vmObj self, type_vmObj k, type_vmObj *meta, int depth) {
    int n = vm_dict_find_sub(tp,self.dict.val,k);
    if (n != -1) {
        *meta = self.dict.val->items[n].val;
        return 1;
    }
    depth--; if (!depth) { vm_raise(0,vm_string("(interpreter_lookup) RuntimeError: maximum lookup depth exceeded")); }
    if (self.dict.dtype && self.dict.val->meta.type == vm_enum1_dict && vm_lookup_sub(tp,self.dict.val->meta,k,meta,depth)) {
        if (self.dict.dtype == 2 && meta->type == vm_enum1_fnc) {
            *meta = vm_misc_fnc_new(tp,meta->fnc.ftype|2,
                meta->fnc.cfnc,meta->fnc.info->code,
                self,meta->fnc.info->globals);
        }
        return 1;
    }
    return 0;
}

int vm_builtins_lookup(type_vm *tp,type_vmObj self, type_vmObj k, type_vmObj *meta) {
    return vm_lookup_sub(tp,self,k,meta,8);
}


/* Function: vm_builtins_setmeta
 * Set a "dict's meta".
 *
 * This is a builtin function, so you need to use <vm_misc_params> to provide the
 * parameters.
 *
 * In interpreter, each dictionary can have a so-called "meta" dictionary attached
 * to it. When dictionary attributes are accessed, but not present in the
 * dictionary, they instead are looked up in the meta dictionary. To get the
 * raw dictionary, you can use <vm_builtins_getraw>.
 *
 * This function is particulary useful for objects and classes, which are just
 * special dictionaries created with <vm_builtins_object> and <vm_builtins_class>. There you can
 * use vm_builtins_setmeta to change the class of the object or parent class of a class.
 *
 * Parameters:
 * self - The dictionary for which to set a meta.
 * meta - The meta dictionary.
 *
 * Returns:
 * None
 */
type_vmObj vm_builtins_setmeta(type_vm *tp) {
    type_vmObj self = vm_typecheck(tp,vm_enum1_dict,vm_operations_get(tp,tp->params,vm_none));
    type_vmObj meta = vm_typecheck(tp,vm_enum1_dict,vm_operations_get(tp,tp->params,vm_none));
    self.dict.val->meta = meta;
    return vm_none;
}

type_vmObj vm_builtins_getmeta(type_vm *tp) {
    type_vmObj self = vm_typecheck(tp,vm_enum1_dict,vm_operations_get(tp,tp->params,vm_none));
    return self.dict.val->meta;
}

/* Function: vm_builtins_object
 * Creates a new object.
 *
 * Returns:
 * The newly created object. The object initially has no parent class, use
 * <vm_builtins_setmeta> to set a class. Also see <vm_builtins_object_new>.
 */
type_vmObj vm_builtins_object(type_vm *tp) {
    type_vmObj self = vm_dict_create(tp);
    self.dict.dtype = 2;
    return self;
}

type_vmObj vm_builtins_object_new(type_vm *tp) {
    type_vmObj klass = vm_typecheck(tp,vm_enum1_dict,vm_operations_get(tp,tp->params,vm_none));
    type_vmObj self = vm_builtins_object(tp);
    self.dict.val->meta = klass;
			if (self.dict.dtype == 2) {
					type_vmObj meta; 
					if (vm_builtins_lookup(tp,self,vm_string("__init__"),&meta)) {
						vm_call_sub(tp,meta,tp->params);
						}			
			}
    return self;
}

type_vmObj vm_builtins_object_call(type_vm *tp) {
    type_vmObj self;
    if (tp->params.list.val->len) {
        self = vm_typecheck(tp,vm_enum1_dict,vm_operations_get(tp,tp->params,vm_none));
        self.dict.dtype = 2;
    } else {
        self = vm_builtins_object(tp);
    }
    return self;
}

/* Function: vm_builtins_getraw
 * Retrieve the raw dict of a dict.
 *
 * This builtin retrieves one dict parameter from interpreter, and returns its raw
 * dict. This is very useful when implementing your own __get__ and __set__
 * functions, as it allows you to directly access the attributes stored in the
 * dict.
 */
type_vmObj vm_builtins_getraw(type_vm *tp) {
    type_vmObj self = vm_typecheck(tp,vm_enum1_dict,vm_operations_get(tp,tp->params,vm_none));
    self.dict.dtype = 0;
    return self;
}

/* Function: vm_builtins_class
 * Creates a new base class.
 *
 * Parameters:
 * none
 *
 * Returns:
 * A new, empty class (derived from interpreter's builtin "object" class).
 */
type_vmObj vm_builtins_class(type_vm *tp) {
    type_vmObj klass = vm_dict_create(tp);
    klass.dict.val->meta = vm_operations_get(tp,tp->builtins,vm_string("object")); 
    return klass;
}

/* Function: vm_builtins_bool
 * Coerces any value to a boolean.
 */
type_vmObj vm_builtins_bool(type_vm *tp) {
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    return (vm_create_numericObj(vm_operations_bool(tp, v)));
}
