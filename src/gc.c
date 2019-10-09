void vm_gc_grey(type_vm *tp,type_vmObj v) {
    if (v.type < vm_enum1_string || (!v.gci.data) || *v.gci.data) { return; }
    *v.gci.data = 1;
    if (v.type == vm_enum1_string || v.type == vm_enum1_data) {
        vm_list_appendx(tp,tp->black,v);
        return;
    }
    vm_list_appendx(tp,tp->grey,v);
}

void vm_gc_follow(type_vm *tp,type_vmObj v) {
    int type = v.type;
    if (type == vm_enum1_list) {
        int n;
        for (n=0; n<v.list.val->len; n++) {
            vm_gc_grey(tp,v.list.val->items[n]);
        }
    }
    if (type == vm_enum1_dict) {
        int i;
        for (i=0; i<v.dict.val->len; i++) {
            int n = vm_dict_next(tp,v.dict.val);
            vm_gc_grey(tp,v.dict.val->items[n].key);
            vm_gc_grey(tp,v.dict.val->items[n].val);
        }
        vm_gc_grey(tp,v.dict.val->meta); 
    }
    if (type == vm_enum1_fnc) {
        vm_gc_grey(tp,v.fnc.info->self);
        vm_gc_grey(tp,v.fnc.info->globals);
        vm_gc_grey(tp,v.fnc.info->code);
    }
}

void vm_gc_reset(type_vm *tp) {
    int n;
    type_vmList *tmp;
    for (n=0; n<tp->black->len; n++) {
        *tp->black->items[n].gci.data = 0;
    }
    tmp = tp->white;
    tp->white = tp->black;
    tp->black = tmp;
}

void vm_gc_init(type_vm *tp) {
    tp->white = vm_list_new(tp);
    tp->grey = vm_list_new(tp);
    tp->black = vm_list_new(tp);
    tp->steps = 0;
}

void vm_gc_deinit(type_vm *tp) {
    vm_list_free(tp, tp->white);
    vm_list_free(tp, tp->grey);
    vm_list_free(tp, tp->black);
}

void vm_gc_delete(type_vm *tp,type_vmObj v) {
    int type = v.type;
    if (type == vm_enum1_list) {
        vm_list_free(tp, v.list.val);
        return;
    } else if (type == vm_enum1_dict) {
        vm_dict_free(tp, v.dict.val);
        return;
    } else if (type == vm_enum1_string) {
        free(v.string.info);
        return;
    } else if (type == vm_enum1_data) {
        if (v.data.info->free) {
            v.data.info->free(tp,v);
        }
        free(v.data.info);
        return;
    } else if (type == vm_enum1_fnc) {
        free(v.fnc.info);
        return;
    }
    vm_raise(tp,vm_string("(vm_gc_delete) TypeError: ?"));
}

void vm_gc_collect(type_vm *tp) {
    int n;
    for (n=0; n<tp->white->len; n++) {
        type_vmObj r = tp->white->items[n];
        if (*r.gci.data) { continue; }
        vm_gc_delete(tp,r);
    }
    tp->white->len = 0;
    vm_gc_reset(tp);
}

void vm_gc_inc_sub(type_vm *tp) {
    type_vmObj v;
    if (!tp->grey->len) {
        return;
    }
    v = vm_list_pop(tp,tp->grey,tp->grey->len-1,"vm_gc_inc_sub");
    vm_gc_follow(tp,v);
    vm_list_appendx(tp,tp->black,v);
}

void vm_gc_full(type_vm *tp) {
    while (tp->grey->len) {
        vm_gc_inc_sub(tp);
    }
    vm_gc_collect(tp);
    vm_gc_follow(tp,tp->root);
}

void vm_gc_inc(type_vm *tp) {
    tp->steps += 1;
    if (tp->steps < vm_def_GCMAX || tp->grey->len > 0) {
        vm_gc_inc_sub(tp); vm_gc_inc_sub(tp); 
    }
    if (tp->steps < vm_def_GCMAX || tp->grey->len > 0) { return; }
    tp->steps = 0;
    vm_gc_full(tp);
    return;
}

type_vmObj vm_gc_track(type_vm *tp,type_vmObj v) {
    vm_gc_inc(tp);
    vm_gc_grey(tp,v);
    return v;
}

/**/

