/* File: Operations
 * Various interpreter operations.
 */

/* Function: interpreter_str
 * String representation of an object.
 *
 * Returns a string object representating self.
 */
interpreter_obj interpreter_str(TP,interpreter_obj self) {
    int type = self.type;
    if (type == interpreter_STRING) { return self; }
    if (type == interpreter_NUMBER) {
        interpreter_num v = self.number.val;
        if ((fabs(v)-fabs((long)v)) < 0.000001) { return interpreter_printf(tp,"%ld",(long)v); }
        return interpreter_printf(tp,"%f",v);
    } else if(type == interpreter_DICT) {
        return interpreter_printf(tp,"<dict 0x%x>",self.dict.val);
    } else if(type == interpreter_LIST) {
        return interpreter_printf(tp,"<list 0x%x>",self.list.val);
    } else if (type == interpreter_NONE) {
        return interpreter_string("None");
    } else if (type == interpreter_DATA) {
        return interpreter_printf(tp,"<data 0x%x>",self.data.val);
    } else if (type == interpreter_FNC) {
        return interpreter_printf(tp,"<fnc 0x%x>",self.fnc.info);
    }
    return interpreter_string("<?>");
}

/* Function: interpreter_bool
 * Check the truth value of an object
 *
 * Returns false if v is a numeric object with a value of exactly 0, v is of
 * type None or v is a string list or dictionary with a length of 0. Else true
 * is returned.
 */
int interpreter_bool(TP,interpreter_obj v) {
    switch(v.type) {
        case interpreter_NUMBER: return v.number.val != 0;
        case interpreter_NONE: return 0;
        case interpreter_STRING: return v.string.len != 0;
        case interpreter_LIST: return v.list.val->len != 0;
        case interpreter_DICT: return v.dict.val->len != 0;
    }
    return 1;
}


/* Function: interpreter_has
 * Checks if an object contains a key.
 *
 * Returns interpreter_True if self[k] exists, interpreter_False otherwise.
 */
interpreter_obj interpreter_has(TP,interpreter_obj self, interpreter_obj k) {
    int type = self.type;
    if (type == interpreter_DICT) {
        if (_interpreter_dict_find(tp,self.dict.val,k) != -1) { return interpreter_True; }
        return interpreter_False;
    } else if (type == interpreter_STRING && k.type == interpreter_STRING) {
        return interpreter_number(_interpreter_str_index(self,k)!=-1);
    } else if (type == interpreter_LIST) {
        return interpreter_number(_interpreter_list_find(tp,self.list.val,k)!=-1);
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_has) TypeError: iterable argument required"));
}

/* Function: interpreter_del
 * Remove a dictionary entry.
 *
 * Removes the key k from self. Also works on classes and objects.
 *
 * Note that unlike with Python, you cannot use this to remove list items.
 */
void interpreter_del(TP,interpreter_obj self, interpreter_obj k) {
    int type = self.type;
    if (type == interpreter_DICT) {
        _interpreter_dict_del(tp,self.dict.val,k,"interpreter_del");
        return;
    }
    interpreter_raise(,interpreter_string("(interpreter_del) TypeError: object does not support item deletion"));
}


/* Function: interpreter_iter
 * Iterate through a list or dict.
 *
 * If self is a list/string/dictionary, this will iterate over the
 * elements/characters/keys respectively, if k is an increasing index
 * starting with 0 up to the length of the object-1.
 *
 * In the case of a list of string, the returned items will correspond to the
 * item at index k. For a dictionary, no guarantees are made about the order.
 * You also cannot call the function with a specific k to get a specific
 * item -- it is only meant for iterating through all items, calling this
 * function len(self) times. Use <interpreter_get> to retrieve a specific item, and
 * <interpreter_len> to get the length.
 *
 * Parameters:
 * self - The object over which to iterate.
 * k - You must pass 0 on the first call, then increase it by 1 after each call,
 *     and don't call the function with k >= len(self).
 *
 * Returns:
 * The first (k = 0) or next (k = 1 .. len(self)-1) item in the iteration.
 */
interpreter_obj interpreter_iter(TP,interpreter_obj self, interpreter_obj k) {
    int type = self.type;
    if (type == interpreter_LIST || type == interpreter_STRING) { return interpreter_get(tp,self,k); }
    if (type == interpreter_DICT && k.type == interpreter_NUMBER) {
        return self.dict.val->items[_interpreter_dict_next(tp,self.dict.val)].key;
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_iter) TypeError: iteration over non-sequence"));
}


/* Function: interpreter_get
 * Attribute lookup.
 * 
 * This returns the result of using self[k] in actual code. It works for
 * dictionaries (including classes and instantiated objects), lists and strings.
 *
 * As a special case, if self is a list, self[None] will return the first
 * element in the list and subsequently remove it from the list.
 */
interpreter_obj interpreter_get(TP,interpreter_obj self, interpreter_obj k) {
    int type = self.type;
    interpreter_obj r;
    if (type == interpreter_DICT) {
        interpreter_META_BEGIN(self,"__get__");
            return interpreter_call(tp,meta,interpreter_params_v(tp,1,k));
        interpreter_META_END;
        if (self.dict.dtype && _interpreter_lookup(tp,self,k,&r)) { return r; }
        return _interpreter_dict_get(tp,self.dict.val,k,"interpreter_get");
    } else if (type == interpreter_LIST) {
        if (k.type == interpreter_NUMBER) {
            int l = interpreter_len(tp,self).number.val;
            int n = k.number.val;
            n = (n<0?l+n:n);
            return _interpreter_list_get(tp,self.list.val,n,"interpreter_get");
        } else if (k.type == interpreter_STRING) {
            if (interpreter_cmp(tp,interpreter_string("append"),k) == 0) {
                return interpreter_method(tp,self,interpreter_append);
            } else if (interpreter_cmp(tp,interpreter_string("pop"),k) == 0) {
                return interpreter_method(tp,self,interpreter_pop);
            } else if (interpreter_cmp(tp,interpreter_string("index"),k) == 0) {
                return interpreter_method(tp,self,interpreter_index);
            } else if (interpreter_cmp(tp,interpreter_string("sort"),k) == 0) {
                return interpreter_method(tp,self,interpreter_sort);
            } else if (interpreter_cmp(tp,interpreter_string("extend"),k) == 0) {
                return interpreter_method(tp,self,interpreter_extend);
            } else if (interpreter_cmp(tp,interpreter_string("*"),k) == 0) {
                interpreter_params_v(tp,1,self);
                r = interpreter_copy(tp);
                self.list.val->len=0;
                return r;
            }
        } else if (k.type == interpreter_NONE) {
            return _interpreter_list_pop(tp,self.list.val,0,"interpreter_get");
        }
    } else if (type == interpreter_STRING) {
        if (k.type == interpreter_NUMBER) {
            int l = self.string.len;
            int n = k.number.val;
            n = (n<0?l+n:n);
            if (n >= 0 && n < l) { return interpreter_string_n(tp->chars[(unsigned char)self.string.val[n]],1); }
        } else if (k.type == interpreter_STRING) {
            if (interpreter_cmp(tp,interpreter_string("join"),k) == 0) {
                return interpreter_method(tp,self,interpreter_join);
            } else if (interpreter_cmp(tp,interpreter_string("split"),k) == 0) {
                return interpreter_method(tp,self,interpreter_split);
            } else if (interpreter_cmp(tp,interpreter_string("index"),k) == 0) {
                return interpreter_method(tp,self,interpreter_str_index);
            } else if (interpreter_cmp(tp,interpreter_string("strip"),k) == 0) {
                return interpreter_method(tp,self,interpreter_strip);
            } else if (interpreter_cmp(tp,interpreter_string("replace"),k) == 0) {
                return interpreter_method(tp,self,interpreter_replace);
            }
        }
    }

    if (k.type == interpreter_LIST) {
        int a,b,l;
        interpreter_obj tmp;
        l = interpreter_len(tp,self).number.val;
        tmp = interpreter_get(tp,k,interpreter_number(0));
        if (tmp.type == interpreter_NUMBER) { a = tmp.number.val; }
        else if(tmp.type == interpreter_NONE) { a = 0; }
        else { interpreter_raise(interpreter_None,interpreter_string("(interpreter_get) TypeError: indices must be numbers")); }
        tmp = interpreter_get(tp,k,interpreter_number(1));
        if (tmp.type == interpreter_NUMBER) { b = tmp.number.val; }
        else if(tmp.type == interpreter_NONE) { b = l; }
        else { interpreter_raise(interpreter_None,interpreter_string("(interpreter_get) TypeError: indices must be numbers")); }
        a = _interpreter_max(0,(a<0?l+a:a)); b = _interpreter_min(l,(b<0?l+b:b));
        if (type == interpreter_LIST) {
            return interpreter_list_n(tp,b-a,&self.list.val->items[a]);
        } else if (type == interpreter_STRING) {
            return interpreter_string_sub(tp,self,a,b);
        }
    }

    interpreter_raise(interpreter_None,interpreter_string("(interpreter_get) TypeError: ?"));
}

/* Function: interpreter_iget
 * Failsafe attribute lookup.
 *
 * This is like <interpreter_get>, except it will return false if the attribute lookup
 * failed. Otherwise, it will return true, and the object will be returned
 * over the reference parameter r.
 */
int interpreter_iget(TP,interpreter_obj *r, interpreter_obj self, interpreter_obj k) {
    if (self.type == interpreter_DICT) {
        int n = _interpreter_dict_find(tp,self.dict.val,k);
        if (n == -1) { return 0; }
        *r = self.dict.val->items[n].val;
        interpreter_grey(tp,*r);
        return 1;
    }
    if (self.type == interpreter_LIST && !self.list.val->len) { return 0; }
    *r = interpreter_get(tp,self,k); interpreter_grey(tp,*r);
    return 1;
}

/* Function: interpreter_set
 * Attribute modification.
 * 
 * This is the counterpart of interpreter_get, it does the same as self[k] = v would do
 * in actual interpreter code.
 */
void interpreter_set(TP,interpreter_obj self, interpreter_obj k, interpreter_obj v) {
    int type = self.type;

    if (type == interpreter_DICT) {
        interpreter_META_BEGIN(self,"__set__");
            interpreter_call(tp,meta,interpreter_params_v(tp,2,k,v));
            return;
        interpreter_META_END;
        _interpreter_dict_set(tp,self.dict.val,k,v);
        return;
    } else if (type == interpreter_LIST) {
        if (k.type == interpreter_NUMBER) {
            _interpreter_list_set(tp,self.list.val,k.number.val,v,"interpreter_set");
            return;
        } else if (k.type == interpreter_NONE) {
            _interpreter_list_append(tp,self.list.val,v);
            return;
        } else if (k.type == interpreter_STRING) {
            if (interpreter_cmp(tp,interpreter_string("*"),k) == 0) {
                interpreter_params_v(tp,2,self,v); interpreter_extend(tp);
                return;
            }
        }
    }
    interpreter_raise(,interpreter_string("(interpreter_set) TypeError: object does not support item assignment"));
}

interpreter_obj interpreter_add(TP,interpreter_obj a, interpreter_obj b) {
    if (a.type == interpreter_NUMBER && a.type == b.type) {
        return interpreter_number(a.number.val+b.number.val);
    } else if (a.type == interpreter_STRING && a.type == b.type) {
        int al = a.string.len, bl = b.string.len;
        interpreter_obj r = interpreter_string_t(tp,al+bl);
        char *s = r.string.info->s;
        memcpy(s,a.string.val,al); memcpy(s+al,b.string.val,bl);
        return interpreter_track(tp,r);
    } else if (a.type == interpreter_LIST && a.type == b.type) {
        interpreter_obj r;
        interpreter_params_v(tp,1,a);
        r = interpreter_copy(tp);
        interpreter_params_v(tp,2,r,b);
        interpreter_extend(tp);
        return r;
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_add) TypeError: ?"));
}

interpreter_obj interpreter_mul(TP,interpreter_obj a, interpreter_obj b) {
    if (a.type == interpreter_NUMBER && a.type == b.type) {
        return interpreter_number(a.number.val*b.number.val);
    } else if ((a.type == interpreter_STRING && b.type == interpreter_NUMBER) || 
               (a.type == interpreter_NUMBER && b.type == interpreter_STRING)) {
        if(a.type == interpreter_NUMBER) {
            interpreter_obj c = a; a = b; b = c;
        }
        int al = a.string.len; int n = b.number.val;
        if(n <= 0) {
            interpreter_obj r = interpreter_string_t(tp,0);
            return interpreter_track(tp,r);
        }
        interpreter_obj r = interpreter_string_t(tp,al*n);
        char *s = r.string.info->s;
        int i; for (i=0; i<n; i++) { memcpy(s+al*i,a.string.val,al); }
        return interpreter_track(tp,r);
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_mul) TypeError: ?"));
}

/* Function: interpreter_len
 * Returns the length of an object.
 *
 * Returns the number of items in a list or dict, or the length of a string.
 */
interpreter_obj interpreter_len(TP,interpreter_obj self) {
    int type = self.type;
    if (type == interpreter_STRING) {
        return interpreter_number(self.string.len);
    } else if (type == interpreter_DICT) {
        return interpreter_number(self.dict.val->len);
    } else if (type == interpreter_LIST) {
        return interpreter_number(self.list.val->len);
    }
    
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_len) TypeError: len() of unsized object"));
}

int interpreter_cmp(TP,interpreter_obj a, interpreter_obj b) {
    if (a.type != b.type) { return a.type-b.type; }
    switch(a.type) {
        case interpreter_NONE: return 0;
        case interpreter_NUMBER: return _interpreter_sign(a.number.val-b.number.val);
        case interpreter_STRING: {
            int l = _interpreter_min(a.string.len,b.string.len);
            int v = memcmp(a.string.val,b.string.val,l);
            if (v == 0) {
                v = a.string.len-b.string.len;
            }
            return v;
        }
        case interpreter_LIST: {
            int n,v; for(n=0;n<_interpreter_min(a.list.val->len,b.list.val->len);n++) {
        interpreter_obj aa = a.list.val->items[n]; interpreter_obj bb = b.list.val->items[n];
            if (aa.type == interpreter_LIST && bb.type == interpreter_LIST) { v = aa.list.val-bb.list.val; } else { v = interpreter_cmp(tp,aa,bb); }
            if (v) { return v; } }
            return a.list.val->len-b.list.val->len;
        }
        case interpreter_DICT: return a.dict.val - b.dict.val;
        case interpreter_FNC: return a.fnc.info - b.fnc.info;
        case interpreter_DATA: return (char*)a.data.val - (char*)b.data.val;
    }
    interpreter_raise(0,interpreter_string("(interpreter_cmp) TypeError: ?"));
}

#define interpreter_OP(name,expr) \
    interpreter_obj name(TP,interpreter_obj _a,interpreter_obj _b) { \
    if (_a.type == interpreter_NUMBER && _a.type == _b.type) { \
        interpreter_num a = _a.number.val; interpreter_num b = _b.number.val; \
        return interpreter_number(expr); \
    } \
    interpreter_raise(interpreter_None,interpreter_string("(" #name ") TypeError: unsupported operand type(s)")); \
}

interpreter_OP(interpreter_bitwise_and,((long)a)&((long)b));
interpreter_OP(interpreter_bitwise_or,((long)a)|((long)b));
interpreter_OP(interpreter_bitwise_xor,((long)a)^((long)b));
interpreter_OP(interpreter_mod,((long)a)%((long)b));
interpreter_OP(interpreter_lsh,((long)a)<<((long)b));
interpreter_OP(interpreter_rsh,((long)a)>>((long)b));
interpreter_OP(interpreter_sub,a-b);
interpreter_OP(interpreter_div,a/b);
interpreter_OP(interpreter_pow,pow(a,b));

interpreter_obj interpreter_bitwise_not(TP, interpreter_obj a) {
    if (a.type == interpreter_NUMBER) {
        return interpreter_number(~(long)a.number.val);
    }
    interpreter_raise(interpreter_None,interpreter_string("(interpreter_bitwise_not) TypeError: unsupported operand type"));
}

/**/
