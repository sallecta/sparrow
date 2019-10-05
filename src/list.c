void _interpreter_list_realloc(type_vm *tp, type_vmList *self,int len) {
    if (!len) { len=1; }
    self->items = (type_vmObj*)realloc(self->items,len*sizeof(type_vmObj));/*realloc(x,y)*/
    self->alloc = len;
}

void _interpreter_list_set(type_vm *tp,type_vmList *self,int k, type_vmObj v, const char *error) {
    if (k >= self->len) {
        interpreter_raise(,interpreter_string("(_interpreter_list_set) KeyError"));
    }
    self->items[k] = v;
    interpreter_grey(tp,v);
}
void _interpreter_list_free(type_vm *tp, type_vmList *self) {
    free(self->items);/*free(x)*/
    free(self);
}

type_vmObj _interpreter_list_get(type_vm *tp,type_vmList *self,int k,const char *error) {
    if (k >= self->len) {
        interpreter_raise(interpreter_None,interpreter_string("(_interpreter_list_set) KeyError"));
    }
    return self->items[k];
}
void _interpreter_list_insertx(type_vm *tp,type_vmList *self, int n, type_vmObj v) {
    if (self->len >= self->alloc) {
        _interpreter_list_realloc(tp, self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(type_vmObj)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void _interpreter_list_appendx(type_vm *tp,type_vmList *self, type_vmObj v) {
    _interpreter_list_insertx(tp,self,self->len,v);
}
void _interpreter_list_insert(type_vm *tp,type_vmList *self, int n, type_vmObj v) {
    _interpreter_list_insertx(tp,self,n,v);
    interpreter_grey(tp,v);
}
void _interpreter_list_append(type_vm *tp,type_vmList *self, type_vmObj v) {
    _interpreter_list_insert(tp,self,self->len,v);
}
type_vmObj _interpreter_list_pop(type_vm *tp,type_vmList *self, int n, const char *error) {
    type_vmObj r = _interpreter_list_get(tp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(type_vmObj)*(self->len-(n+1))); }
    self->len -= 1;
    return r;
}

int _interpreter_list_find(type_vm *tp,type_vmList *self, type_vmObj v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (interpreter_cmp(tp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

type_vmObj interpreter_index(type_vm *tp) {
    type_vmObj self = interpreter_OBJ();
    type_vmObj v = interpreter_OBJ();
    int i = _interpreter_list_find(tp,self.list.val,v);
    if (i < 0) {
        interpreter_raise(interpreter_None,interpreter_string("(interpreter_index) ValueError: list.index(x): x not in list"));
    }
    return interpreter_number(i);
}

type_vmList *_interpreter_list_new(type_vm *tp) {
    return (type_vmList*)calloc(sizeof(type_vmList),1);/*calloc((x),1)*/
}

type_vmObj _interpreter_list_copy(type_vm *tp, type_vmObj rr) {
    type_vmObj val = {interpreter_LIST};
    type_vmList *o = rr.list.val;
    type_vmList *r = _interpreter_list_new(tp);
    *r = *o; r->gci = 0;
    r->items = (type_vmObj*)calloc(sizeof(type_vmObj)*o->len,1);/*calloc((x),1)*/
    memcpy(r->items,o->items,sizeof(type_vmObj)*o->len);
    val.list.val = r;
    return interpreter_track(tp,val);
}

type_vmObj interpreter_append(type_vm *tp) {
    type_vmObj self = interpreter_OBJ();
    type_vmObj v = interpreter_OBJ();
    _interpreter_list_append(tp,self.list.val,v);
    return interpreter_None;
}

type_vmObj interpreter_pop(type_vm *tp) {
    type_vmObj self = interpreter_OBJ();
    return _interpreter_list_pop(tp,self.list.val,self.list.val->len-1,"pop");
}

type_vmObj interpreter_insert(type_vm *tp) {
    type_vmObj self = interpreter_OBJ();
    int n = type_vmNum();
    type_vmObj v = interpreter_OBJ();
    _interpreter_list_insert(tp,self.list.val,n,v);
    return interpreter_None;
}

type_vmObj interpreter_extend(type_vm *tp) {
    type_vmObj self = interpreter_OBJ();
    type_vmObj v = interpreter_OBJ();
    int i;
    for (i=0; i<v.list.val->len; i++) {
        _interpreter_list_append(tp,self.list.val,v.list.val->items[i]);
    }
    return interpreter_None;
}

type_vmObj interpreter_list_nt(type_vm *tp) {
    type_vmObj r = {interpreter_LIST};
    r.list.val = _interpreter_list_new(tp);
    return r;
}

type_vmObj interpreter_list(type_vm *tp) {
    type_vmObj r = {interpreter_LIST};
    r.list.val = _interpreter_list_new(tp);
    return interpreter_track(tp,r);
}

type_vmObj interpreter_list_n(type_vm *tp,int n,type_vmObj *argv) {
    int i;
    type_vmObj r = interpreter_list(tp); _interpreter_list_realloc(tp, r.list.val,n);
    for (i=0; i<n; i++) {
        _interpreter_list_append(tp,r.list.val,argv[i]);
    }
    return r;
}

int _interpreter_sort_cmp(type_vmObj *a,type_vmObj *b) {
    return interpreter_cmp(0,*a,*b);
}

type_vmObj interpreter_sort(type_vm *tp) {
    type_vmObj self = interpreter_OBJ();
    qsort(self.list.val->items, self.list.val->len, sizeof(type_vmObj), (int(*)(const void*,const void*))_interpreter_sort_cmp);
    return interpreter_None;
}

