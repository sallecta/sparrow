void vm_list_realloc(type_vm *tp, type_vmList *self,int len) {
    if (!len) { len=1; }
    self->items = (type_vmObj*)realloc(self->items,len*sizeof(type_vmObj));/*realloc(x,y)*/
    self->alloc = len;
}

void vm_list_set(type_vm *tp,type_vmList *self,int k, type_vmObj v, const char *error) {
    if (k >= self->len) {
        vm_raise(tp,vm_string("(vm_list_set) KeyError"));
    }
    self->items[k] = v;
    vm_gc_grey(tp,v);
}
void vm_list_free(type_vm *tp, type_vmList *self) {
    free(self->items);/*free(x)*/
    free(self);
}

type_vmObj vm_list_get(type_vm *tp,type_vmList *self,int k,const char *error) {
    if (k >= self->len) {
        vm_raise(0,vm_string("(vm_list_set) KeyError"));
    }
    return self->items[k];
}
void vm_list_insertx(type_vm *tp,type_vmList *self, int n, type_vmObj v) {
    if (self->len >= self->alloc) {
        vm_list_realloc(tp, self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(type_vmObj)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void vm_list_appendx(type_vm *tp,type_vmList *self, type_vmObj v) {
    vm_list_insertx(tp,self,self->len,v);
}
void vm_list_insert(type_vm *tp,type_vmList *self, int n, type_vmObj v) {
    vm_list_insertx(tp,self,n,v);
    vm_gc_grey(tp,v);
}
void vm_list_append(type_vm *tp,type_vmList *self, type_vmObj v) {
    vm_list_insert(tp,self,self->len,v);
}
type_vmObj vm_list_pop(type_vm *tp,type_vmList *self, int n, const char *error) {
    type_vmObj r = vm_list_get(tp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(type_vmObj)*(self->len-(n+1))); }
    self->len -= 1;
    return r;
}

int vm_list_find(type_vm *tp,type_vmList *self, type_vmObj v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (vm_operations_cmp(tp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

type_vmObj vm_list_index(type_vm *tp) {
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    int i = vm_list_find(tp,self.list.val,v);
    if (i < 0) {
        vm_raise(tp,vm_string("(vm_list_index) ValueError: list.index(x): x not in list"));
    }
    return vm_create_numericObj(i);
}

type_vmList *vm_list_new(type_vm *tp) {
    return (type_vmList*)calloc(sizeof(type_vmList),1);
}

type_vmObj vm_list_copy(type_vm *tp, type_vmObj rr) {
    type_vmObj val = {vm_enum1_list};
    type_vmList *o = rr.list.val;
    type_vmList *r = vm_list_new(tp);
    *r = *o; r->gci = 0;
    r->items = (type_vmObj*)calloc(sizeof(type_vmObj)*o->len,1);
    memcpy(r->items,o->items,sizeof(type_vmObj)*o->len);
    val.list.val = r;
    return vm_gc_track(tp,val);
}

type_vmObj vm_list_append2(type_vm *tp) {
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    vm_list_append(tp,self.list.val,v);
    return vm_none;
}

type_vmObj vm_list_pop2(type_vm *tp) {
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    return vm_list_pop(tp,self.list.val,self.list.val->len-1,"pop");
}

type_vmObj vm_list_insert2(type_vm *tp) {
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    int n = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val;
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    vm_list_insert(tp,self.list.val,n,v);
    return vm_none;
}

type_vmObj vm_list_extend(type_vm *tp) {
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    int i;
    for (i=0; i<v.list.val->len; i++) {
        vm_list_append(tp,self.list.val,v.list.val->items[i]);
    }
    return vm_none;
}

type_vmObj vm_list_nt(type_vm *tp) {
    type_vmObj r = {vm_enum1_list};
    r.list.val = vm_list_new(tp);
    return r;
}

type_vmObj vm_list(type_vm *tp) {
    type_vmObj r = {vm_enum1_list};
    r.list.val = vm_list_new(tp);
    return vm_gc_track(tp,r);
}

type_vmObj vm_list_n(type_vm *tp,int n,type_vmObj *argv) {
    int i;
    type_vmObj r = vm_list(tp); vm_list_realloc(tp, r.list.val,n);
    for (i=0; i<n; i++) {
        vm_list_append(tp,r.list.val,argv[i]);
    }
    return r;
}

int vm_list_sort_cmp(type_vmObj *a,type_vmObj *b) {
    return vm_operations_cmp(0,*a,*b);
}

type_vmObj vm_list_sort(type_vm *tp) {
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    qsort(self.list.val->items, self.list.val->len, sizeof(type_vmObj), (int(*)(const void*,const void*))vm_list_sort_cmp);
    return vm_none;
}

