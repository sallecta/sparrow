/* File: String
 * String handling functions.
 */
 
/*
 * Create a new empty string of a certain size.
 * Does not put it in for GC tracking, since contents should be
 * filled after returning.
 */ 
type_vmObj interpreter_string_t(type_vm *tp, int n) {
    type_vmObj r = interpreter_string_n(0,n);
    r.string.info = (type_vmString*)calloc(sizeof(type_vmString)+n,1);/*calloc((x),1)*/
    r.string.info->len = n;
    r.string.val = r.string.info->s;
    return r;
}

/*
 * Create a new string which is a copy of some memory.
 * This is put into GC tracking for you.
 */
type_vmObj interpreter_string_copy(type_vm *tp, const char *s, int n) {
    type_vmObj r = interpreter_string_t(tp,n);
    memcpy(r.string.info->s,s,n);
    return interpreter_track(tp,r);
}

/*
 * Create a new string which is a substring slice of another STRING.
 * Does not need to be put into GC tracking, as its parent is
 * already being tracked (supposedly).
 */
type_vmObj interpreter_string_sub(type_vm *tp, type_vmObj s, int a, int b) {
    int l = s.string.len;
    a = _interpreter_max(0,(a<0?l+a:a)); b = _interpreter_min(l,(b<0?l+b:b));
    type_vmObj r = s;
    r.string.val += a;
    r.string.len = b-a;
    return r;
}

type_vmObj interpreter_printf(type_vm *tp, char const *fmt,...) {
    int l;
    type_vmObj r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt,arg);
    r = interpreter_string_t(tp,l);
    s = r.string.info->s;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
    return interpreter_track(tp,r);
}

int _interpreter_str_index(type_vmObj s, type_vmObj k) {
    int i=0;
    while ((s.string.len - i) >= k.string.len) {
        if (memcmp(s.string.val+i,k.string.val,k.string.len) == 0) {
            return i;
        }
        i += 1;
    }
    return -1;
}

type_vmObj interpreter_join(type_vm *tp) {
    type_vmObj delim = interpreter_OBJ();
    type_vmObj val = interpreter_OBJ();
    int l=0,i;
    type_vmObj r;
    char *s;
    for (i=0; i<val.list.val->len; i++) {
        if (i!=0) { l += delim.string.len; }
        l += interpreter_str(tp,val.list.val->items[i]).string.len;
    }
    r = interpreter_string_t(tp,l);
    s = r.string.info->s;
    l = 0;
    for (i=0; i<val.list.val->len; i++) {
        type_vmObj e;
        if (i!=0) {
            memcpy(s+l,delim.string.val,delim.string.len); l += delim.string.len;
        }
        e = interpreter_str(tp,val.list.val->items[i]);
        memcpy(s+l,e.string.val,e.string.len); l += e.string.len;
    }
    return interpreter_track(tp,r);
}

type_vmObj interpreter_split(type_vm *tp) {
    type_vmObj v = interpreter_OBJ();
    type_vmObj d = interpreter_OBJ();
    type_vmObj r = interpreter_list(tp);

    int i;
    while ((i=_interpreter_str_index(v,d))!=-1) {
        _interpreter_list_append(tp,r.list.val,interpreter_string_sub(tp,v,0,i));
        v.string.val += i + d.string.len; v.string.len -= i + d.string.len;
    }
    _interpreter_list_append(tp,r.list.val,interpreter_string_sub(tp,v,0,v.string.len));
    return r;
}


type_vmObj interpreter_find(type_vm *tp) {
    type_vmObj s = interpreter_OBJ();
    type_vmObj v = interpreter_OBJ();
    return interpreter_number(_interpreter_str_index(s,v));
}

type_vmObj interpreter_str_index(type_vm *tp) {
    type_vmObj s = interpreter_OBJ();
    type_vmObj v = interpreter_OBJ();
    int n = _interpreter_str_index(s,v);
    if (n >= 0) { return interpreter_number(n); }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_str_index) ValueError: substring not found"));
}

type_vmObj interpreter_str2(type_vm *tp) {
    type_vmObj v = interpreter_OBJ();
    return interpreter_str(tp,v);
}

type_vmObj interpreter_chr(type_vm *tp) {
    int v = type_vmNum();
    return interpreter_string_n(tp->chars[(unsigned char)v],1);
}
type_vmObj interpreter_ord(type_vm *tp) {
    type_vmObj s = interpreter_STR();
    if (s.string.len != 1) {
        interpreter_raise(interpreter_None,interpreter_string("(interpreter_ord) TypeError: ord() expected a character"));
    }
    return interpreter_number((unsigned char)s.string.val[0]);
}

type_vmObj interpreter_strip(type_vm *tp) {
    type_vmObj o = interpreter_TYPE(interpreter_STRING);
    char const *v = o.string.val; int l = o.string.len;
    int i; int a = l, b = 0;
    type_vmObj r;
    char *s;
    for (i=0; i<l; i++) {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r') {
            a = _interpreter_min(a,i); b = _interpreter_max(b,i+1);
        }
    }
    if ((b-a) < 0) { return interpreter_string(""); }
    r = interpreter_string_t(tp,b-a);
    s = r.string.info->s;
    memcpy(s,v+a,b-a);
    return interpreter_track(tp,r);
}

type_vmObj interpreter_replace(type_vm *tp) {
    type_vmObj s = interpreter_OBJ();
    type_vmObj k = interpreter_OBJ();
    type_vmObj v = interpreter_OBJ();
    type_vmObj p = s;
    int i,n = 0;
    int c;
    int l;
    type_vmObj rr;
    char *r;
    char *d;
    type_vmObj z;
    while ((i = _interpreter_str_index(p,k)) != -1) {
        n += 1;
        p.string.val += i + k.string.len; p.string.len -= i + k.string.len;
    }
/*     fprintf(stderr,"ns: %d\n",n); */
    l = s.string.len + n * (v.string.len-k.string.len);
    rr = interpreter_string_t(tp,l);
    r = rr.string.info->s;
    d = r;
    z = p = s;
    while ((i = _interpreter_str_index(p,k)) != -1) {
        p.string.val += i; p.string.len -= i;
        memcpy(d,z.string.val,c=(p.string.val-z.string.val)); d += c;
        p.string.val += k.string.len; p.string.len -= k.string.len;
        memcpy(d,v.string.val,v.string.len); d += v.string.len;
        z = p;
    }
    memcpy(d,z.string.val,(s.string.val + s.string.len) - z.string.val);

    return interpreter_track(tp,rr);
}

