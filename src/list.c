void _interpreter_list_realloc(TP, _interpreter_list *self,int len) {
    if (!len) { len=1; }
    self->items = (interpreter_obj*)interpreter_realloc(tp, self->items,len*sizeof(interpreter_obj));
    self->alloc = len;
}

void _interpreter_list_set(TP,_interpreter_list *self,int k, interpreter_obj v, const char *error) {
    if (k >= self->len) {
        interpreter_raise(,interpreter_string("(_interpreter_list_set) KeyError"));
    }
    self->items[k] = v;
    interpreter_grey(tp,v);
}
void _interpreter_list_free(TP, _interpreter_list *self) {
    interpreter_free(tp, self->items);
    interpreter_free(tp, self);
}

interpreter_obj _interpreter_list_get(TP,_interpreter_list *self,int k,const char *error) {
    if (k >= self->len) {
        interpreter_raise(interpreter_None,interpreter_string("(_interpreter_list_set) KeyError"));
    }
    return self->items[k];
}
void _interpreter_list_insertx(TP,_interpreter_list *self, int n, interpreter_obj v) {
    if (self->len >= self->alloc) {
        _interpreter_list_realloc(tp, self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(interpreter_obj)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void _interpreter_list_appendx(TP,_interpreter_list *self, interpreter_obj v) {
    _interpreter_list_insertx(tp,self,self->len,v);
}
void _interpreter_list_insert(TP,_interpreter_list *self, int n, interpreter_obj v) {
    _interpreter_list_insertx(tp,self,n,v);
    interpreter_grey(tp,v);
}
void _interpreter_list_append(TP,_interpreter_list *self, interpreter_obj v) {
    _interpreter_list_insert(tp,self,self->len,v);
}
interpreter_obj _interpreter_list_pop(TP,_interpreter_list *self, int n, const char *error) {
    interpreter_obj r = _interpreter_list_get(tp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(interpreter_obj)*(self->len-(n+1))); }
    self->len -= 1;
    return r;
}

int _interpreter_list_find(TP,_interpreter_list *self, interpreter_obj v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (interpreter_cmp(tp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

interpreter_obj interpreter_index(TP) {
    interpreter_obj self = interpreter_OBJ();
    interpreter_obj v = interpreter_OBJ();
    int i = _interpreter_list_find(tp,self.list.val,v);
    if (i < 0) {
        interpreter_raise(interpreter_None,interpreter_string("(interpreter_index) ValueError: list.index(x): x not in list"));
    }
    return interpreter_number(i);
}

_interpreter_list *_interpreter_list_new(TP) {
    return (_interpreter_list*)interpreter_malloc(tp, sizeof(_interpreter_list));
}

interpreter_obj _interpreter_list_copy(TP, interpreter_obj rr) {
    interpreter_obj val = {interpreter_LIST};
    _interpreter_list *o = rr.list.val;
    _interpreter_list *r = _interpreter_list_new(tp);
    *r = *o; r->gci = 0;
    r->items = (interpreter_obj*)interpreter_malloc(tp, sizeof(interpreter_obj)*o->len);
    memcpy(r->items,o->items,sizeof(interpreter_obj)*o->len);
    val.list.val = r;
    return interpreter_track(tp,val);
}

interpreter_obj interpreter_append(TP) {
    interpreter_obj self = interpreter_OBJ();
    interpreter_obj v = interpreter_OBJ();
    _interpreter_list_append(tp,self.list.val,v);
    return interpreter_None;
}

interpreter_obj interpreter_pop(TP) {
    interpreter_obj self = interpreter_OBJ();
    return _interpreter_list_pop(tp,self.list.val,self.list.val->len-1,"pop");
}

interpreter_obj interpreter_insert(TP) {
    interpreter_obj self = interpreter_OBJ();
    int n = interpreter_NUM();
    interpreter_obj v = interpreter_OBJ();
    _interpreter_list_insert(tp,self.list.val,n,v);
    return interpreter_None;
}

interpreter_obj interpreter_extend(TP) {
    interpreter_obj self = interpreter_OBJ();
    interpreter_obj v = interpreter_OBJ();
    int i;
    for (i=0; i<v.list.val->len; i++) {
        _interpreter_list_append(tp,self.list.val,v.list.val->items[i]);
    }
    return interpreter_None;
}

interpreter_obj interpreter_list_nt(TP) {
    interpreter_obj r = {interpreter_LIST};
    r.list.val = _interpreter_list_new(tp);
    return r;
}

interpreter_obj interpreter_list(TP) {
    interpreter_obj r = {interpreter_LIST};
    r.list.val = _interpreter_list_new(tp);
    return interpreter_track(tp,r);
}

interpreter_obj interpreter_list_n(TP,int n,interpreter_obj *argv) {
    int i;
    interpreter_obj r = interpreter_list(tp); _interpreter_list_realloc(tp, r.list.val,n);
    for (i=0; i<n; i++) {
        _interpreter_list_append(tp,r.list.val,argv[i]);
    }
    return r;
}

int _interpreter_sort_cmp(interpreter_obj *a,interpreter_obj *b) {
    return interpreter_cmp(0,*a,*b);
}

interpreter_obj interpreter_sort(TP) {
    interpreter_obj self = interpreter_OBJ();
    qsort(self.list.val->items, self.list.val->len, sizeof(interpreter_obj), (int(*)(const void*,const void*))_interpreter_sort_cmp);
    return interpreter_None;
}

