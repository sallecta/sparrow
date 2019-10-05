/* File: Dict
 * Functions for dealing with dictionaries.
 */
int interpreter_lua_hash(void const *v,int l) {
    int i,step = (l>>5)+1;
    int h = l + (l >= 4?*(int*)v:0);
    for (i=l; i>=step; i-=step) {
        h = h^((h<<5)+(h>>2)+((unsigned char *)v)[i-1]);
    }
    return h;
}
void _interpreter_dict_free(type_vm *tp, type_vmDict *self) {
    free(self->items);
    free(self);
}


int interpreter_hash(type_vm *tp,type_vmObj v) {
    switch (v.type) {
        case interpreter_NONE: return 0;
        case interpreter_NUMBER: return interpreter_lua_hash(&v.number.val,sizeof(type_vmNum));
        case interpreter_STRING: return interpreter_lua_hash(v.string.val,v.string.len);
        case interpreter_DICT: return interpreter_lua_hash(&v.dict.val,sizeof(void*));
        case interpreter_LIST: {
            int r = v.list.val->len; int n; for(n=0; n<v.list.val->len; n++) {
            type_vmObj vv = v.list.val->items[n]; r += vv.type != interpreter_LIST?interpreter_hash(tp,v.list.val->items[n]):interpreter_lua_hash(&vv.list.val,sizeof(void*)); } return r;
        }
        case interpreter_FNC: return interpreter_lua_hash(&v.fnc.info,sizeof(void*));
        case interpreter_DATA: return interpreter_lua_hash(&v.data.val,sizeof(void*));
    }
    interpreter_raise(0,interpreter_string("(interpreter_hash) TypeError: value unhashable"));
}

void _interpreter_dict_hash_set(type_vm *tp,type_vmDict *self, int hash, type_vmObj k, type_vmObj v) {
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
    interpreter_raise(,interpreter_string("(_interpreter_dict_hash_set) RuntimeError: ?"));
}

void _interpreter_dict_interpreter_realloc(type_vm *tp,type_vmDict *self,int len) {
    type_vmItem *items = self->items;
    int i,alloc = self->alloc;
    len = _interpreter_max(8,len);

    self->items = (type_vmItem*)calloc(len*sizeof(type_vmItem),1);/*calloc((x),1)*/
    self->alloc = len; self->mask = len-1;
    self->len = 0; self->used = 0;

    for (i=0; i<alloc; i++) {
        if (items[i].used != 1) { continue; }
        _interpreter_dict_hash_set(tp,self,items[i].hash,items[i].key,items[i].val);
    }
    free(items);
}

int _interpreter_dict_hash_find(type_vm *tp,type_vmDict *self, int hash, type_vmObj k) {
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used == 0) { break; }
        if (self->items[n].used < 0) { continue; }
        if (self->items[n].hash != hash) { continue; }
        if (interpreter_cmp(tp,self->items[n].key,k) != 0) { continue; }
        return n;
    }
    return -1;
}
int _interpreter_dict_find(type_vm *tp,type_vmDict *self,type_vmObj k) {
    return _interpreter_dict_hash_find(tp,self,interpreter_hash(tp,k),k);
}

void _interpreter_dict_setx(type_vm *tp,type_vmDict *self,type_vmObj k, type_vmObj v) {
    int hash = interpreter_hash(tp,k); int n = _interpreter_dict_hash_find(tp,self,hash,k);
    if (n == -1) {
        if (self->len >= (self->alloc/2)) {
            _interpreter_dict_interpreter_realloc(tp,self,self->alloc*2);
        } else if (self->used >= (self->alloc*3/4)) {
            _interpreter_dict_interpreter_realloc(tp,self,self->alloc);
        }
        _interpreter_dict_hash_set(tp,self,hash,k,v);
    } else {
        self->items[n].val = v;
    }
}

void _interpreter_dict_set(type_vm *tp,type_vmDict *self,type_vmObj k, type_vmObj v) {
    _interpreter_dict_setx(tp,self,k,v);
    interpreter_grey(tp,k); interpreter_grey(tp,v);
}

type_vmObj _interpreter_dict_get(type_vm *tp,type_vmDict *self,type_vmObj k, const char *error) {
    int n = _interpreter_dict_find(tp,self,k);
    if (n < 0) {
        interpreter_raise(interpreter_None,interpreter_add(tp,interpreter_string("(_interpreter_dict_get) KeyError: "),interpreter_str(tp,k)));
    }
    return self->items[n].val;
}

void _interpreter_dict_del(type_vm *tp,type_vmDict *self,type_vmObj k, const char *error) {
    int n = _interpreter_dict_find(tp,self,k);
    if (n < 0) {
        interpreter_raise(,interpreter_add(tp,interpreter_string("(_interpreter_dict_del) KeyError: "),interpreter_str(tp,k)));
    }
    self->items[n].used = -1;
    self->len -= 1;
}

type_vmDict *_interpreter_dict_new(type_vm *tp) {
    type_vmDict *self = (type_vmDict*)calloc(sizeof(type_vmDict),1);/*calloc((x),1)*/
    return self;
}
type_vmObj _interpreter_dict_copy(type_vm *tp,type_vmObj rr) {
    type_vmObj obj = {interpreter_DICT};
    type_vmDict *o = rr.dict.val;
    type_vmDict *r = _interpreter_dict_new(tp);
    *r = *o; r->gci = 0;
    r->items = (type_vmItem*)calloc(sizeof(type_vmItem)*o->alloc,1);/*calloc((x),1)*/
    memcpy(r->items,o->items,sizeof(type_vmItem)*o->alloc);
    obj.dict.val = r;
    obj.dict.dtype = 1;
    return interpreter_track(tp,obj);
}

int _interpreter_dict_next(type_vm *tp,type_vmDict *self) {
    if (!self->len) {
        interpreter_raise(0,interpreter_string("(_interpreter_dict_next) RuntimeError"));
    }
    while (1) {
        self->cur = ((self->cur + 1) & self->mask);
        if (self->items[self->cur].used > 0) {
            return self->cur;
        }
    }
}

type_vmObj interpreter_merge(type_vm *tp) {
    type_vmObj self = interpreter_OBJ();
    type_vmObj v = interpreter_OBJ();
    int i; for (i=0; i<v.dict.val->len; i++) {
        int n = _interpreter_dict_next(tp,v.dict.val);
        _interpreter_dict_set(tp,self.dict.val,
            v.dict.val->items[n].key,v.dict.val->items[n].val);
    }
    return interpreter_None;
}

/* Function: interpreter_dict
 *
 * Creates a new dictionary object.
 *
 * *Note* If you use <interpreter_setmeta> on the dictionary, you have to use <interpreter_getraw> to
 * access the "raw" dictionary again.
 *
 * Returns:
 * The newly created dictionary.
 */
type_vmObj interpreter_dict(type_vm *tp) {
    type_vmObj r = {interpreter_DICT};
    r.dict.val = _interpreter_dict_new(tp);
    r.dict.dtype = 1;
    return tp ? interpreter_track(tp,r) : r;
}

type_vmObj interpreter_dict_n(type_vm *tp,int n, type_vmObj* argv) {
    type_vmObj r = interpreter_dict(tp);
    int i; for (i=0; i<n; i++) { interpreter_set(tp,r,argv[i*2],argv[i*2+1]); }
    return r;
}


