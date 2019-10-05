/* File: Builtins
 * Builtin interpreter functions.
 */

type_vmObj interpreter_print(type_vm *tp) {
    int n = 0;
    type_vmObj e;
    interpreter_LOOP(e)
        if (n) { printf(" "); }
        interpreter_echo(tp,e);
        n += 1;
    interpreter_END;
    printf("\n");
    return interpreter_None;
}

type_vmObj interpreter_bind(type_vm *tp) {
    type_vmObj r = interpreter_TYPE(interpreter_FNC);
    type_vmObj self = interpreter_OBJ();
    return interpreter_fnc_new(tp,
        r.fnc.ftype|2,r.fnc.cfnc,r.fnc.info->code,
        self,r.fnc.info->globals);
}

type_vmObj interpreter_min(type_vm *tp) {
    type_vmObj r = interpreter_OBJ();
    type_vmObj e;
    interpreter_LOOP(e)
        if (interpreter_cmp(tp,r,e) > 0) { r = e; }
    interpreter_END;
    return r;
}

type_vmObj interpreter_max(type_vm *tp) {
    type_vmObj r = interpreter_OBJ();
    type_vmObj e;
    interpreter_LOOP(e)
        if (interpreter_cmp(tp,r,e) < 0) { r = e; }
    interpreter_END;
    return r;
}

type_vmObj interpreter_copy(type_vm *tp) {
    type_vmObj r = interpreter_OBJ();
    int type = r.type;
    if (type == interpreter_LIST) {
        return _interpreter_list_copy(tp,r);
    } else if (type == interpreter_DICT) {
        return _interpreter_dict_copy(tp,r);
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_copy) TypeError: ?"));
}


type_vmObj interpreter_len_(type_vm *tp) {
    type_vmObj e = interpreter_OBJ();
    return interpreter_len(tp,e);
}

type_vmObj interpreter_assert(type_vm *tp) {
    int a = type_vmNum();
    if (a) { return interpreter_None; }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_assert) AssertionError"));
}

type_vmObj interpreter_range(type_vm *tp) {
    int a,b,c,i;
    type_vmObj r = interpreter_list(tp);
    switch (tp->params.list.val->len) {
        case 1: a = 0; b = type_vmNum(); c = 1; break;
        case 2:
        case 3: a = type_vmNum(); b = type_vmNum(); c = interpreter_DEFAULT(interpreter_number(1)).number.val; break;
        default: return r;
    }
    if (c != 0) {
        for (i=a; (c>0) ? i<b : i>b; i+=c) {
            _interpreter_list_append(tp,r.list.val,interpreter_number(i));
        }
    }
    return r;
}

/* Function: interpreter_system
 *
 * The system builtin. A grave security flaw. If your version of interpreter
 * enables this, you better remove it before deploying your app :P
 */
type_vmObj interpreter_system(type_vm *tp) {
    char s[interpreter_CSTR_LEN]; interpreter_cstr(tp,interpreter_STR(),s,interpreter_CSTR_LEN);
    int r = system(s);
    return interpreter_number(r);
}

type_vmObj interpreter_istype(type_vm *tp) {
    type_vmObj v = interpreter_OBJ();
    type_vmObj t = interpreter_STR();
    if (interpreter_cmp(tp,t,interpreter_string("string")) == 0) { return interpreter_number(v.type == interpreter_STRING); }
    if (interpreter_cmp(tp,t,interpreter_string("list")) == 0) { return interpreter_number(v.type == interpreter_LIST); }
    if (interpreter_cmp(tp,t,interpreter_string("dict")) == 0) { return interpreter_number(v.type == interpreter_DICT); }
    if (interpreter_cmp(tp,t,interpreter_string("number")) == 0) { return interpreter_number(v.type == interpreter_NUMBER); }
    if (interpreter_cmp(tp,t,interpreter_string("fnc")) == 0) { return interpreter_number(v.type == interpreter_FNC && (v.fnc.ftype&2) == 0); }
    if (interpreter_cmp(tp,t,interpreter_string("method")) == 0) { return interpreter_number(v.type == interpreter_FNC && (v.fnc.ftype&2) != 0); }
    interpreter_raise(interpreter_None,interpreter_string("(is_type) TypeError: ?"));
}


type_vmObj interpreter_float(type_vm *tp) {
    type_vmObj v = interpreter_OBJ();
    int ord = interpreter_DEFAULT(interpreter_number(0)).number.val;
    int type = v.type;
    if (type == interpreter_NUMBER) { return v; }
    if (type == interpreter_STRING && v.string.len < 32) {
        char s[32]; memset(s,0,v.string.len+1);
        memcpy(s,v.string.val,v.string.len);
        if (strchr(s,'.')) { return interpreter_number(atof(s)); }
        return(interpreter_number(strtol(s,0,ord)));
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_float) TypeError: ?"));
}


type_vmObj interpreter_save(type_vm *tp) {
    char fname[256]; interpreter_cstr(tp,interpreter_STR(),fname,256);
    type_vmObj v = interpreter_OBJ();
    FILE *f;
    f = fopen(fname,"wb");
    if (!f) { interpreter_raise(interpreter_None,interpreter_string("(interpreter_save) IOError: ?")); }
    fwrite(v.string.val,v.string.len,1,f);
    fclose(f);
    return interpreter_None;
}

type_vmObj interpreter_load(type_vm *tp) {
    FILE *f;
    long l;
    type_vmObj r;
    char *s;
    char fname[256]; interpreter_cstr(tp,interpreter_STR(),fname,256);
    struct stat stbuf;
    stat(fname, &stbuf);
    l = stbuf.st_size;
    f = fopen(fname,"rb");
    if (!f) {
        interpreter_raise(interpreter_None,interpreter_string("(interpreter_load) IOError: ?"));
    }
    r = interpreter_string_t(tp,l);
    s = r.string.info->s;
    /*fread(s,1,l,f);*/
    if (fread(s,1,l,f)==0) { printf("fread is 0"); }
    fclose(f);
    return interpreter_track(tp,r);
}


type_vmObj interpreter_fpack(type_vm *tp) {
    type_vmNum v = type_vmNum();
    type_vmObj r = interpreter_string_t(tp,sizeof(type_vmNum));
    *(type_vmNum*)r.string.val = v;
    return interpreter_track(tp,r);
}

type_vmObj interpreter_abs(type_vm *tp) {
    return interpreter_number(fabs(interpreter_float(tp).number.val));
}
type_vmObj interpreter_int(type_vm *tp) {
    return interpreter_number((long)interpreter_float(tp).number.val);
}
type_vmNum _roundf(type_vmNum v) {
    type_vmNum av = fabs(v); type_vmNum iv = (long)av;
    av = (av-iv < 0.5?iv:iv+1);
    return (v<0?-av:av);
}
type_vmObj interpreter_round(type_vm *tp) {
    return interpreter_number(_roundf(interpreter_float(tp).number.val));
}

type_vmObj interpreter_exists(type_vm *tp) {
    char fname[interpreter_CSTR_LEN]; interpreter_cstr(tp,interpreter_STR(),fname,interpreter_CSTR_LEN);
    struct stat stbuf;
    return interpreter_number(!stat(fname,&stbuf));
}
type_vmObj interpreter_mtime(type_vm *tp) {
    char fname[interpreter_CSTR_LEN]; interpreter_cstr(tp,interpreter_STR(),fname,interpreter_CSTR_LEN);
    struct stat stbuf;
    if (!stat(fname,&stbuf)) { return interpreter_number(stbuf.st_mtime); }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_mtime) IOError: ?"));
}

int _interpreter_lookup_(type_vm *tp,type_vmObj self, type_vmObj k, type_vmObj *meta, int depth) {
    int n = _interpreter_dict_find(tp,self.dict.val,k);
    if (n != -1) {
        *meta = self.dict.val->items[n].val;
        return 1;
    }
    depth--; if (!depth) { interpreter_raise(0,interpreter_string("(interpreter_lookup) RuntimeError: maximum lookup depth exceeded")); }
    if (self.dict.dtype && self.dict.val->meta.type == interpreter_DICT && _interpreter_lookup_(tp,self.dict.val->meta,k,meta,depth)) {
        if (self.dict.dtype == 2 && meta->type == interpreter_FNC) {
            *meta = interpreter_fnc_new(tp,meta->fnc.ftype|2,
                meta->fnc.cfnc,meta->fnc.info->code,
                self,meta->fnc.info->globals);
        }
        return 1;
    }
    return 0;
}

int _interpreter_lookup(type_vm *tp,type_vmObj self, type_vmObj k, type_vmObj *meta) {
    return _interpreter_lookup_(tp,self,k,meta,8);
}

#define interpreter_META_BEGIN(self,name) \
    if (self.dict.dtype == 2) { \
        type_vmObj meta; if (_interpreter_lookup(tp,self,interpreter_string(name),&meta)) {

#define interpreter_META_END \
        } \
    }

/* Function: interpreter_setmeta
 * Set a "dict's meta".
 *
 * This is a builtin function, so you need to use <interpreter_params> to provide the
 * parameters.
 *
 * In interpreter, each dictionary can have a so-called "meta" dictionary attached
 * to it. When dictionary attributes are accessed, but not present in the
 * dictionary, they instead are looked up in the meta dictionary. To get the
 * raw dictionary, you can use <interpreter_getraw>.
 *
 * This function is particulary useful for objects and classes, which are just
 * special dictionaries created with <interpreter_object> and <interpreter_class>. There you can
 * use interpreter_setmeta to change the class of the object or parent class of a class.
 *
 * Parameters:
 * self - The dictionary for which to set a meta.
 * meta - The meta dictionary.
 *
 * Returns:
 * None
 */
type_vmObj interpreter_setmeta(type_vm *tp) {
    type_vmObj self = interpreter_TYPE(interpreter_DICT);
    type_vmObj meta = interpreter_TYPE(interpreter_DICT);
    self.dict.val->meta = meta;
    return interpreter_None;
}

type_vmObj interpreter_getmeta(type_vm *tp) {
    type_vmObj self = interpreter_TYPE(interpreter_DICT);
    return self.dict.val->meta;
}

/* Function: interpreter_object
 * Creates a new object.
 *
 * Returns:
 * The newly created object. The object initially has no parent class, use
 * <interpreter_setmeta> to set a class. Also see <interpreter_object_new>.
 */
type_vmObj interpreter_object(type_vm *tp) {
    type_vmObj self = interpreter_dict(tp);
    self.dict.dtype = 2;
    return self;
}

type_vmObj interpreter_object_new(type_vm *tp) {
    type_vmObj klass = interpreter_TYPE(interpreter_DICT);
    type_vmObj self = interpreter_object(tp);
    self.dict.val->meta = klass;
    interpreter_META_BEGIN(self,"__init__");
        interpreter_call(tp,meta,tp->params);
    interpreter_META_END;
    return self;
}

type_vmObj interpreter_object_call(type_vm *tp) {
    type_vmObj self;
    if (tp->params.list.val->len) {
        self = interpreter_TYPE(interpreter_DICT);
        self.dict.dtype = 2;
    } else {
        self = interpreter_object(tp);
    }
    return self;
}

/* Function: interpreter_getraw
 * Retrieve the raw dict of a dict.
 *
 * This builtin retrieves one dict parameter from interpreter, and returns its raw
 * dict. This is very useful when implementing your own __get__ and __set__
 * functions, as it allows you to directly access the attributes stored in the
 * dict.
 */
type_vmObj interpreter_getraw(type_vm *tp) {
    type_vmObj self = interpreter_TYPE(interpreter_DICT);
    self.dict.dtype = 0;
    return self;
}

/* Function: interpreter_class
 * Creates a new base class.
 *
 * Parameters:
 * none
 *
 * Returns:
 * A new, empty class (derived from interpreter's builtin "object" class).
 */
type_vmObj interpreter_class(type_vm *tp) {
    type_vmObj klass = interpreter_dict(tp);
    klass.dict.val->meta = interpreter_get(tp,tp->builtins,interpreter_string("object")); 
    return klass;
}

/* Function: interpreter_builtins_bool
 * Coerces any value to a boolean.
 */
type_vmObj interpreter_builtins_bool(type_vm *tp) {
    type_vmObj v = interpreter_OBJ();
    return (interpreter_number(interpreter_bool(tp, v)));
}
