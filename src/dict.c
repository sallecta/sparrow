/* File: Dict
 * Functions for dealing with dictionaries.
 */
int vm_dict_lua_hash(void const *v,int l) {
    int i,step = (l>>5)+1;
    int h = l + (l >= 4?*(int*)v:0);
    for (i=l; i>=step; i-=step) {
        h = h^((h<<5)+(h>>2)+((unsigned char *)v)[i-1]);
    }
    return h;
}
void vm_dict_free(type_vm *tp, type_vmDict *self) {
    free(self->items);
    free(self);
}


int vm_dict_hash(type_vm *tp,type_vmObj v) {
    switch (v.type) {
        case vm_enum1_none: return 0;
        case vm_enum1_number: return vm_dict_lua_hash(&v.number.val,sizeof(type_vmNum));
        case vm_enum1_string: return vm_dict_lua_hash(v.string.val,v.string.len);
        case vm_enum1_dict: return vm_dict_lua_hash(&v.dict.val,sizeof(void*));
        case vm_enum1_list: {
            int r = v.list.val->len; int n; for(n=0; n<v.list.val->len; n++) {
            type_vmObj vv = v.list.val->items[n]; r += vv.type != vm_enum1_list?vm_dict_hash(tp,v.list.val->items[n]):vm_dict_lua_hash(&vv.list.val,sizeof(void*)); } return r;
        }
        case vm_enum1_fnc: return vm_dict_lua_hash(&v.fnc.info,sizeof(void*));
        case vm_enum1_data: return vm_dict_lua_hash(&v.data.val,sizeof(void*));
    }
    vm_raise(0,vm_string("(vm_dict_hash) TypeError: value unhashable"));
	return 0;
}

void vm_dict_hash_set_sub(type_vm *tp,type_vmDict *self, int hash, type_vmObj k, type_vmObj v) {
    type_vmItem item;
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used > 0) { continue; }
        if (self->items[n].used == 0) { self->used += 1; }
        item.used = 1;
        item.hash = hash;
        item.key = k;
        item.val = v;
        self->items[n] = item;
        self->len += 1;
        return;
    }
    vm_raise(tp,vm_string("(vm_dict_hash_set_sub) RuntimeError: ?"));
}

void vm_dict_realloc_sub(type_vm *tp,type_vmDict *self,int len) {
    type_vmItem *items = self->items;
    int i,alloc = self->alloc;
    len = vm_max(8,len);

    self->items = (type_vmItem*)calloc(len*sizeof(type_vmItem),1);/*calloc((x),1)*/
    self->alloc = len; self->mask = len-1;
    self->len = 0; self->used = 0;

    for (i=0; i<alloc; i++) {
        if (items[i].used != 1) { continue; }
        vm_dict_hash_set_sub(tp,self,items[i].hash,items[i].key,items[i].val);
    }
    free(items);
}

int vm_dict_hash_find_sub(type_vm *tp,type_vmDict *self, int hash, type_vmObj k) {
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used == 0) { break; }
        if (self->items[n].used < 0) { continue; }
        if (self->items[n].hash != hash) { continue; }
        if (vm_operations_cmp(tp,self->items[n].key,k) != 0) { continue; }
        return n;
    }
    return -1;
}
int vm_dict_find_sub(type_vm *tp,type_vmDict *self,type_vmObj k) {
    return vm_dict_hash_find_sub(tp,self,vm_dict_hash(tp,k),k);
}

void vm_dict_setx_sub(type_vm *tp,type_vmDict *self,type_vmObj k, type_vmObj v) {
    int hash = vm_dict_hash(tp,k); int n = vm_dict_hash_find_sub(tp,self,hash,k);
    if (n == -1) {
        if (self->len >= (self->alloc/2)) {
            vm_dict_realloc_sub(tp,self,self->alloc*2);
        } else if (self->used >= (self->alloc*3/4)) {
            vm_dict_realloc_sub(tp,self,self->alloc);
        }
        vm_dict_hash_set_sub(tp,self,hash,k,v);
    } else {
        self->items[n].val = v;
    }
}

void vm_dict_set_sub(type_vm *tp,type_vmDict *self,type_vmObj k, type_vmObj v) {
    vm_dict_setx_sub(tp,self,k,v);
    vm_gc_grey(tp,k); vm_gc_grey(tp,v);
}

type_vmObj vm_dict_get(type_vm *tp,type_vmDict *self,type_vmObj k, const char *error) {
    int n = vm_dict_find_sub(tp,self,k);
    if (n < 0) {
        vm_raise(tp,vm_operations_add(tp,vm_string("(vm_dict_get) KeyError: "),vm_operations_str(tp,k)));
    }
    return self->items[n].val;
}

void vm_dict_del(type_vm *tp,type_vmDict *self,type_vmObj k, const char *error) {
    int n = vm_dict_find_sub(tp,self,k);
    if (n < 0) {
        vm_raise(tp,vm_operations_add(tp,vm_string("(vm_dict_del) KeyError: "),vm_operations_str(tp,k)));
    }
    self->items[n].used = -1;
    self->len -= 1;
}

type_vmDict *vm_dict_new(type_vm *tp) {
    type_vmDict *self = (type_vmDict*)calloc(sizeof(type_vmDict),1);/*calloc((x),1)*/
    return self;
}
type_vmObj vm_dict_copy(type_vm *tp,type_vmObj rr) {
    type_vmObj obj = {vm_enum1_dict};
    type_vmDict *o = rr.dict.val;
    type_vmDict *r = vm_dict_new(tp);
    *r = *o; r->gci = 0;
    r->items = (type_vmItem*)calloc(sizeof(type_vmItem)*o->alloc,1);/*calloc((x),1)*/
    memcpy(r->items,o->items,sizeof(type_vmItem)*o->alloc);
    obj.dict.val = r;
    obj.dict.dtype = 1;
    return vm_gc_track(tp,obj);
}

int vm_dict_next(type_vm *tp,type_vmDict *self) {
    if (!self->len) {
        vm_raise(0,vm_string("(vm_dict_next) RuntimeError"));
    }
    while (1) {
        self->cur = ((self->cur + 1) & self->mask);
        if (self->items[self->cur].used > 0) {
            return self->cur;
        }
    }

}

type_vmObj vm_dict_merge(type_vm *tp) {
    type_vmObj self = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    int i; for (i=0; i<v.dict.val->len; i++) {
        int n = vm_dict_next(tp,v.dict.val);
        vm_dict_set_sub(tp,self.dict.val,
            v.dict.val->items[n].key,v.dict.val->items[n].val);
    }
    return vm_none;
}

/* Function: vm_dict_create
 *
 * Creates a new dictionary object.
 *
 * *Note* If you use <vm_builtins_setmeta> on the dictionary, you have to use <vm_builtins_getraw> to
 * access the "raw" dictionary again.
 *
 * Returns:
 * The newly created dictionary.
 */
type_vmObj vm_dict_create(type_vm *tp) {
    type_vmObj r = {vm_enum1_dict};
    r.dict.val = vm_dict_new(tp);
    r.dict.dtype = 1;
    return tp ? vm_gc_track(tp,r) : r;
}

type_vmObj interpreter_dict_n(type_vm *tp,int n, type_vmObj* argv) {
    type_vmObj r = vm_dict_create(tp);
    int i; for (i=0; i<n; i++) { vm_operations_set(tp,r,argv[i*2],argv[i*2+1]); }
    return r;
}


