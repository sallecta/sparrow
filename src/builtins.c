/* File: Builtins
 * Builtin interpreter functions.
 */

interpreter_obj interpreter_print(TP) {
    int n = 0;
    interpreter_obj e;
    interpreter_LOOP(e)
        if (n) { printf(" "); }
        interpreter_echo(tp,e);
        n += 1;
    interpreter_END;
    printf("\n");
    return interpreter_None;
}

interpreter_obj interpreter_bind(TP) {
    interpreter_obj r = interpreter_TYPE(interpreter_FNC);
    interpreter_obj self = interpreter_OBJ();
    return interpreter_fnc_new(tp,
        r.fnc.ftype|2,r.fnc.cfnc,r.fnc.info->code,
        self,r.fnc.info->globals);
}

interpreter_obj interpreter_min(TP) {
    interpreter_obj r = interpreter_OBJ();
    interpreter_obj e;
    interpreter_LOOP(e)
        if (interpreter_cmp(tp,r,e) > 0) { r = e; }
    interpreter_END;
    return r;
}

interpreter_obj interpreter_max(TP) {
    interpreter_obj r = interpreter_OBJ();
    interpreter_obj e;
    interpreter_LOOP(e)
        if (interpreter_cmp(tp,r,e) < 0) { r = e; }
    interpreter_END;
    return r;
}

interpreter_obj interpreter_copy(TP) {
    interpreter_obj r = interpreter_OBJ();
    int type = r.type;
    if (type == interpreter_LIST) {
        return _interpreter_list_copy(tp,r);
    } else if (type == interpreter_DICT) {
        return _interpreter_dict_copy(tp,r);
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_copy) TypeError: ?"));
}


interpreter_obj interpreter_len_(TP) {
    interpreter_obj e = interpreter_OBJ();
    return interpreter_len(tp,e);
}

interpreter_obj interpreter_assert(TP) {
    int a = interpreter_NUM();
    if (a) { return interpreter_None; }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_assert) AssertionError"));
}

interpreter_obj interpreter_range(TP) {
    int a,b,c,i;
    interpreter_obj r = interpreter_list(tp);
    switch (tp->params.list.val->len) {
        case 1: a = 0; b = interpreter_NUM(); c = 1; break;
        case 2:
        case 3: a = interpreter_NUM(); b = interpreter_NUM(); c = interpreter_DEFAULT(interpreter_number(1)).number.val; break;
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
interpreter_obj interpreter_system(TP) {
    char s[interpreter_CSTR_LEN]; interpreter_cstr(tp,interpreter_STR(),s,interpreter_CSTR_LEN);
    int r = system(s);
    return interpreter_number(r);
}

interpreter_obj interpreter_istype(TP) {
    interpreter_obj v = interpreter_OBJ();
    interpreter_obj t = interpreter_STR();
    if (interpreter_cmp(tp,t,interpreter_string("string")) == 0) { return interpreter_number(v.type == interpreter_STRING); }
    if (interpreter_cmp(tp,t,interpreter_string("list")) == 0) { return interpreter_number(v.type == interpreter_LIST); }
    if (interpreter_cmp(tp,t,interpreter_string("dict")) == 0) { return interpreter_number(v.type == interpreter_DICT); }
    if (interpreter_cmp(tp,t,interpreter_string("number")) == 0) { return interpreter_number(v.type == interpreter_NUMBER); }
    if (interpreter_cmp(tp,t,interpreter_string("fnc")) == 0) { return interpreter_number(v.type == interpreter_FNC && (v.fnc.ftype&2) == 0); }
    if (interpreter_cmp(tp,t,interpreter_string("method")) == 0) { return interpreter_number(v.type == interpreter_FNC && (v.fnc.ftype&2) != 0); }
    interpreter_raise(interpreter_None,interpreter_string("(is_type) TypeError: ?"));
}


interpreter_obj interpreter_float(TP) {
    interpreter_obj v = interpreter_OBJ();
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


interpreter_obj interpreter_save(TP) {
    char fname[256]; interpreter_cstr(tp,interpreter_STR(),fname,256);
    interpreter_obj v = interpreter_OBJ();
    FILE *f;
    f = fopen(fname,"wb");
    if (!f) { interpreter_raise(interpreter_None,interpreter_string("(interpreter_save) IOError: ?")); }
    fwrite(v.string.val,v.string.len,1,f);
    fclose(f);
    return interpreter_None;
}

interpreter_obj interpreter_load(TP) {
    FILE *f;
    long l;
    interpreter_obj r;
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


interpreter_obj interpreter_fpack(TP) {
    interpreter_num v = interpreter_NUM();
    interpreter_obj r = interpreter_string_t(tp,sizeof(interpreter_num));
    *(interpreter_num*)r.string.val = v;
    return interpreter_track(tp,r);
}

interpreter_obj interpreter_abs(TP) {
    return interpreter_number(fabs(interpreter_float(tp).number.val));
}
interpreter_obj interpreter_int(TP) {
    return interpreter_number((long)interpreter_float(tp).number.val);
}
interpreter_num _roundf(interpreter_num v) {
    interpreter_num av = fabs(v); interpreter_num iv = (long)av;
    av = (av-iv < 0.5?iv:iv+1);
    return (v<0?-av:av);
}
interpreter_obj interpreter_round(TP) {
    return interpreter_number(_roundf(interpreter_float(tp).number.val));
}

interpreter_obj interpreter_exists(TP) {
    char fname[interpreter_CSTR_LEN]; interpreter_cstr(tp,interpreter_STR(),fname,interpreter_CSTR_LEN);
    struct stat stbuf;
    return interpreter_number(!stat(fname,&stbuf));
}
interpreter_obj interpreter_mtime(TP) {
    char fname[interpreter_CSTR_LEN]; interpreter_cstr(tp,interpreter_STR(),fname,interpreter_CSTR_LEN);
    struct stat stbuf;
    if (!stat(fname,&stbuf)) { return interpreter_number(stbuf.st_mtime); }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_mtime) IOError: ?"));
}

int _interpreter_lookup_(TP,interpreter_obj self, interpreter_obj k, interpreter_obj *meta, int depth) {
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

int _interpreter_lookup(TP,interpreter_obj self, interpreter_obj k, interpreter_obj *meta) {
    return _interpreter_lookup_(tp,self,k,meta,8);
}

#define interpreter_META_BEGIN(self,name) \
    if (self.dict.dtype == 2) { \
        interpreter_obj meta; if (_interpreter_lookup(tp,self,interpreter_string(name),&meta)) {

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
interpreter_obj interpreter_setmeta(TP) {
    interpreter_obj self = interpreter_TYPE(interpreter_DICT);
    interpreter_obj meta = interpreter_TYPE(interpreter_DICT);
    self.dict.val->meta = meta;
    return interpreter_None;
}

interpreter_obj interpreter_getmeta(TP) {
    interpreter_obj self = interpreter_TYPE(interpreter_DICT);
    return self.dict.val->meta;
}

/* Function: interpreter_object
 * Creates a new object.
 *
 * Returns:
 * The newly created object. The object initially has no parent class, use
 * <interpreter_setmeta> to set a class. Also see <interpreter_object_new>.
 */
interpreter_obj interpreter_object(TP) {
    interpreter_obj self = interpreter_dict(tp);
    self.dict.dtype = 2;
    return self;
}

interpreter_obj interpreter_object_new(TP) {
    interpreter_obj klass = interpreter_TYPE(interpreter_DICT);
    interpreter_obj self = interpreter_object(tp);
    self.dict.val->meta = klass;
    interpreter_META_BEGIN(self,"__init__");
        interpreter_call(tp,meta,tp->params);
    interpreter_META_END;
    return self;
}

interpreter_obj interpreter_object_call(TP) {
    interpreter_obj self;
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
interpreter_obj interpreter_getraw(TP) {
    interpreter_obj self = interpreter_TYPE(interpreter_DICT);
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
interpreter_obj interpreter_class(TP) {
    interpreter_obj klass = interpreter_dict(tp);
    klass.dict.val->meta = interpreter_get(tp,tp->builtins,interpreter_string("object")); 
    return klass;
}

/* Function: interpreter_builtins_bool
 * Coerces any value to a boolean.
 */
interpreter_obj interpreter_builtins_bool(TP) {
    interpreter_obj v = interpreter_OBJ();
    return (interpreter_number(interpreter_bool(tp, v)));
}
