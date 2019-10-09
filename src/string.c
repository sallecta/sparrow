/* File: String
 * String handling functions.
 */
 
/*
 * Create a new empty string of a certain size.
 * Does not put it in for GC tracking, since contents should be
 * filled after returning.
 */ 
type_vmObj vm_string_new(type_vm *tp, int n) {
    type_vmObj r = vm_string_n(0,n);
    r.string.info = (type_vmString*)calloc(sizeof(type_vmString)+n,1);/*calloc((x),1)*/
    r.string.info->len = n;
    r.string.val = r.string.info->s;
    return r;
}

/*
 * Create a new string which is a copy of some memory.
 * This is put into GC tracking for you.
 */
type_vmObj vm_string_copy(type_vm *tp, const char *s, int n) {
    type_vmObj r = vm_string_new(tp,n);
    memcpy(r.string.info->s,s,n);
    return vm_gc_track(tp,r);
}

/*
 * Create a new string which is a substring slice of another STRING.
 * Does not need to be put into GC tracking, as its parent is
 * already being tracked (supposedly).
 */
type_vmObj vm_string_substring(type_vm *tp, type_vmObj s, int a, int b) {
    int l = s.string.len;
    a = vm_max(0,(a<0?l+a:a)); b = vm_min(l,(b<0?l+b:b));
    type_vmObj r = s;
    r.string.val += a;
    r.string.len = b-a;
    return r;
}

type_vmObj vm_string_printf(type_vm *tp, char const *fmt,...) {
    int l;
    type_vmObj r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt,arg);
    r = vm_string_new(tp,l);
    s = r.string.info->s;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
    return vm_gc_track(tp,r);
}

int vm_string_index(type_vmObj s, type_vmObj k) {
    int i=0;
    while ((s.string.len - i) >= k.string.len) {
        if (memcmp(s.string.val+i,k.string.val,k.string.len) == 0) {
            return i;
        }
        i += 1;
    }
    return -1;
}

type_vmObj vm_string_join(type_vm *tp) {
    type_vmObj delim = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj val = vm_operations_get(tp,tp->params,vm_none);
    int l=0,i;
    type_vmObj r;
    char *s;
    for (i=0; i<val.list.val->len; i++) {
        if (i!=0) { l += delim.string.len; }
        l += vm_operations_str(tp,val.list.val->items[i]).string.len;
    }
    r = vm_string_new(tp,l);
    s = r.string.info->s;
    l = 0;
    for (i=0; i<val.list.val->len; i++) {
        type_vmObj e;
        if (i!=0) {
            memcpy(s+l,delim.string.val,delim.string.len); l += delim.string.len;
        }
        e = vm_operations_str(tp,val.list.val->items[i]);
        memcpy(s+l,e.string.val,e.string.len); l += e.string.len;
    }
    return vm_gc_track(tp,r);
}

type_vmObj vm_string_split(type_vm *tp) {
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj d = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj r = vm_list(tp);

    int i;
    while ((i=vm_string_index(v,d))!=-1) {
        vm_list_append(tp,r.list.val,vm_string_substring(tp,v,0,i));
        v.string.val += i + d.string.len; v.string.len -= i + d.string.len;
    }
    vm_list_append(tp,r.list.val,vm_string_substring(tp,v,0,v.string.len));
    return r;
}


type_vmObj vm_string_find(type_vm *tp) {
    type_vmObj s = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    return vm_create_numericObj(vm_string_index(s,v));
}

type_vmObj vm_string_obj_index(type_vm *tp) {
    type_vmObj s = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    int n = vm_string_index(s,v);
    if (n >= 0) { return vm_create_numericObj(n); }
    vm_raise(0,vm_string("(vm_string_obj_index) ValueError: substring not found"));
	return vm_none;
}

type_vmObj vm_string_str2(type_vm *tp) {
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    return vm_operations_str(tp,v);
}

type_vmObj vm_string_chr(type_vm *tp) {
    int v = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val;
    return vm_string_n(tp->chars[(unsigned char)v],1);
}
type_vmObj vm_string_ord(type_vm *tp) {
    type_vmObj s = vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none));
    if (s.string.len != 1) {
        vm_raise(0,vm_string("(vm_string_ord) TypeError: ord() expected a character"));
    }
    return vm_create_numericObj((unsigned char)s.string.val[0]);
}

type_vmObj vm_string_strip(type_vm *tp) {
    type_vmObj o = vm_typecheck(tp,vm_enum1_string,vm_operations_get(tp,tp->params,vm_none));
    char const *v = o.string.val; int l = o.string.len;
    int i; int a = l, b = 0;
    type_vmObj r;
    char *s;
    for (i=0; i<l; i++) {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r') {
            a = vm_min(a,i); b = vm_max(b,i+1);
        }
    }
    if ((b-a) < 0) { return vm_string(""); }
    r = vm_string_new(tp,b-a);
    s = r.string.info->s;
    memcpy(s,v+a,b-a);
    return vm_gc_track(tp,r);
}

type_vmObj vm_string_replace(type_vm *tp) {
    type_vmObj s = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj k = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj p = s;
    int i,n = 0;
    int c;
    int l;
    type_vmObj rr;
    char *r;
    char *d;
    type_vmObj z;
    while ((i = vm_string_index(p,k)) != -1) {
        n += 1;
        p.string.val += i + k.string.len; p.string.len -= i + k.string.len;
    }
/*     fprintf(stderr,"ns: %d\n",n); */
    l = s.string.len + n * (v.string.len-k.string.len);
    rr = vm_string_new(tp,l);
    r = rr.string.info->s;
    d = r;
    z = p = s;
    while ((i = vm_string_index(p,k)) != -1) {
        p.string.val += i; p.string.len -= i;
        memcpy(d,z.string.val,c=(p.string.val-z.string.val)); d += c;
        p.string.val += k.string.len; p.string.len -= k.string.len;
        memcpy(d,v.string.val,v.string.len); d += v.string.len;
        z = p;
    }
    memcpy(d,z.string.val,(s.string.val + s.string.len) - z.string.val);

    return vm_gc_track(tp,rr);
}

