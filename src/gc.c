/* interpreter_obj interpreter_track(TP,interpreter_obj v) { return v; }
   void interpreter_grey(TP,interpreter_obj v) { }
   void interpreter_full(TP) { }
   void interpreter_gc_init(TP) { }
   void interpreter_gc_deinit(TP) { }
   void interpreter_delete(TP,interpreter_obj v) { }*/

void interpreter_grey(TP,interpreter_obj v) {
    if (v.type < interpreter_STRING || (!v.gci.data) || *v.gci.data) { return; }
    *v.gci.data = 1;
    if (v.type == interpreter_STRING || v.type == interpreter_DATA) {
        _interpreter_list_appendx(tp,tp->black,v);
        return;
    }
    _interpreter_list_appendx(tp,tp->grey,v);
}

void interpreter_follow(TP,interpreter_obj v) {
    int type = v.type;
    if (type == interpreter_LIST) {
        int n;
        for (n=0; n<v.list.val->len; n++) {
            interpreter_grey(tp,v.list.val->items[n]);
        }
    }
    if (type == interpreter_DICT) {
        int i;
        for (i=0; i<v.dict.val->len; i++) {
            int n = _interpreter_dict_next(tp,v.dict.val);
            interpreter_grey(tp,v.dict.val->items[n].key);
            interpreter_grey(tp,v.dict.val->items[n].val);
        }
        interpreter_grey(tp,v.dict.val->meta); 
    }
    if (type == interpreter_FNC) {
        interpreter_grey(tp,v.fnc.info->self);
        interpreter_grey(tp,v.fnc.info->globals);
        interpreter_grey(tp,v.fnc.info->code);
    }
}

void interpreter_reset(TP) {
    int n;
    _interpreter_list *tmp;
    for (n=0; n<tp->black->len; n++) {
        *tp->black->items[n].gci.data = 0;
    }
    tmp = tp->white;
    tp->white = tp->black;
    tp->black = tmp;
}

void interpreter_gc_init(TP) {
    tp->white = _interpreter_list_new(tp);
    tp->grey = _interpreter_list_new(tp);
    tp->black = _interpreter_list_new(tp);
    tp->steps = 0;
}

void interpreter_gc_deinit(TP) {
    _interpreter_list_free(tp, tp->white);
    _interpreter_list_free(tp, tp->grey);
    _interpreter_list_free(tp, tp->black);
}

void interpreter_delete(TP,interpreter_obj v) {
    int type = v.type;
    if (type == interpreter_LIST) {
        _interpreter_list_free(tp, v.list.val);
        return;
    } else if (type == interpreter_DICT) {
        _interpreter_dict_free(tp, v.dict.val);
        return;
    } else if (type == interpreter_STRING) {
        interpreter_free(tp, v.string.info);
        return;
    } else if (type == interpreter_DATA) {
        if (v.data.info->free) {
            v.data.info->free(tp,v);
        }
        interpreter_free(tp, v.data.info);
        return;
    } else if (type == interpreter_FNC) {
        interpreter_free(tp, v.fnc.info);
        return;
    }
    interpreter_raise(,interpreter_string("(interpreter_delete) TypeError: ?"));
}

void interpreter_collect(TP) {
    int n;
    for (n=0; n<tp->white->len; n++) {
        interpreter_obj r = tp->white->items[n];
        if (*r.gci.data) { continue; }
        interpreter_delete(tp,r);
    }
    tp->white->len = 0;
    interpreter_reset(tp);
}

void _interpreter_gcinc(TP) {
    interpreter_obj v;
    if (!tp->grey->len) {
        return;
    }
    v = _interpreter_list_pop(tp,tp->grey,tp->grey->len-1,"_interpreter_gcinc");
    interpreter_follow(tp,v);
    _interpreter_list_appendx(tp,tp->black,v);
}

void interpreter_full(TP) {
    while (tp->grey->len) {
        _interpreter_gcinc(tp);
    }
    interpreter_collect(tp);
    interpreter_follow(tp,tp->root);
}

void interpreter_gcinc(TP) {
    tp->steps += 1;
    if (tp->steps < interpreter_GCMAX || tp->grey->len > 0) {
        _interpreter_gcinc(tp); _interpreter_gcinc(tp); 
    }
    if (tp->steps < interpreter_GCMAX || tp->grey->len > 0) { return; }
    tp->steps = 0;
    interpreter_full(tp);
    return;
}

interpreter_obj interpreter_track(TP,interpreter_obj v) {
    interpreter_gcinc(tp);
    interpreter_grey(tp,v);
    return v;
}

/**/

