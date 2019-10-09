/* File: Operations
 * Various interpreter operations.
 */

/* Function: vm_operations_str
 * String representation of an object.
 *
 * Returns a string object representating self.
 */
type_vmObj vm_operations_str(type_vm *tp,type_vmObj self) {
    int type = self.type;
    if (type == vm_enum1_string) { return self; }
    if (type == vm_enum1_number) {
        type_vmNum v = self.number.val;
        if ((fabs(v)-fabs((long)v)) < 0.000001) { return vm_string_printf(tp,"%ld",(long)v); }
        return vm_string_printf(tp,"%f",v);
    } else if(type == vm_enum1_dict) {
        return vm_string_printf(tp,"<dict 0x%x>",self.dict.val);
    } else if(type == vm_enum1_list) {
        return vm_string_printf(tp,"<list 0x%x>",self.list.val);
    } else if (type == vm_enum1_none) {
        return vm_string("None");
    } else if (type == vm_enum1_data) {
        return vm_string_printf(tp,"<data 0x%x>",self.data.val);
    } else if (type == vm_enum1_fnc) {
        return vm_string_printf(tp,"<fnc 0x%x>",self.fnc.info);
    }
    return vm_string("<?>");
}

/* Function: vm_operations_bool
 * Check the truth value of an object
 *
 * Returns false if v is a numeric object with a value of exactly 0, v is of
 * type None or v is a string list or dictionary with a length of 0. Else true
 * is returned.
 */
int vm_operations_bool(type_vm *tp,type_vmObj v) {
    switch(v.type) {
        case vm_enum1_number: return v.number.val != 0;
        case vm_enum1_none: return 0;
        case vm_enum1_string: return v.string.len != 0;
        case vm_enum1_list: return v.list.val->len != 0;
        case vm_enum1_dict: return v.dict.val->len != 0;
    }
    return 1;
}


/* Function: vm_operations_haskey
 * Checks if an object contains a key.
 *
 * Returns vm_create_numericObj(1) if self[k] exists, vm_create_numericObj(0) otherwise.
 */
type_vmObj vm_operations_haskey(type_vm *tp,type_vmObj self, type_vmObj k) {
    int type = self.type;
    if (type == vm_enum1_dict) {
        if (vm_dict_find_sub(tp,self.dict.val,k) != -1) { return vm_create_numericObj(1); }
        return vm_create_numericObj(0);
    } else if (type == vm_enum1_string && k.type == vm_enum1_string) {
        return vm_create_numericObj(vm_string_index(self,k)!=-1);
    } else if (type == vm_enum1_list) {
        return vm_create_numericObj(vm_list_find(tp,self.list.val,k)!=-1);
    }
    vm_raise(0,vm_string("(vm_operations_haskey) TypeError: iterable argument required"));
	return vm_none;
}

/* Function: vm_operations_dict_key_del
 * Remove a dictionary entry.
 *
 * Removes the key k from self. Also works on classes and objects.
 *
 * Note that unlike with Python, you cannot use this to remove list items.
 */
void vm_operations_dict_key_del(type_vm *tp,type_vmObj self, type_vmObj k) {
    int type = self.type;
    if (type == vm_enum1_dict) {
        vm_dict_del(tp,self.dict.val,k,"vm_operations_dict_key_del");
        return;
    }
    vm_raise(tp,vm_string("(vm_operations_dict_key_del) TypeError: object does not support item deletion"));
}


/* Function: vm_operations_iterate
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
 * function len(self) times. Use <vm_operations_get> to retrieve a specific item, and
 * <vm_operations_len> to get the length.
 *
 * Parameters:
 * self - The object over which to iterate.
 * k - You must pass 0 on the first call, then increase it by 1 after each call,
 *     and don't call the function with k >= len(self).
 *
 * Returns:
 * The first (k = 0) or next (k = 1 .. len(self)-1) item in the iteration.
 */
type_vmObj vm_operations_iterate(type_vm *tp,type_vmObj self, type_vmObj k) {
    int type = self.type;
    if (type == vm_enum1_list || type == vm_enum1_string) { return vm_operations_get(tp,self,k); }
    if (type == vm_enum1_dict && k.type == vm_enum1_number) {
        return self.dict.val->items[vm_dict_next(tp,self.dict.val)].key;
    }
    vm_raise(0,vm_string("(vm_operations_iterate) TypeError: iteration over non-sequence"));
	return vm_none;
}


/* Function: vm_operations_get
 * Attribute lookup.
 * 
 * This returns the result of using self[k] in actual code. It works for
 * dictionaries (including classes and instantiated objects), lists and strings.
 *
 * As a special case, if self is a list, self[None] will return the first
 * element in the list and subsequently remove it from the list.
 */
type_vmObj vm_operations_get(type_vm *tp,type_vmObj self, type_vmObj k) {
    int type = self.type;
    type_vmObj r;
    if (type == vm_enum1_dict) {
			if (self.dict.dtype == 2) {
					type_vmObj meta; 
					if (vm_builtins_lookup(tp,self,vm_string("__get__"),&meta)) {
						return vm_call_sub(tp,meta,vm_misc_params_v(tp,1,k));
						}			
			}
        if (self.dict.dtype && vm_builtins_lookup(tp,self,k,&r)) { return r; }
        return vm_dict_get(tp,self.dict.val,k,"vm_operations_get");
    } else if (type == vm_enum1_list) {
        if (k.type == vm_enum1_number) {
            int l = vm_operations_len(tp,self).number.val;
            int n = k.number.val;
            n = (n<0?l+n:n);
            return vm_list_get(tp,self.list.val,n,"vm_operations_get");
        } else if (k.type == vm_enum1_string) {
            if (vm_operations_cmp(tp,vm_string("append"),k) == 0) {
                return vm_misc_method(tp,self,vm_list_append2);
            } else if (vm_operations_cmp(tp,vm_string("pop"),k) == 0) {
                return vm_misc_method(tp,self,vm_list_pop2);
            } else if (vm_operations_cmp(tp,vm_string("index"),k) == 0) {
                return vm_misc_method(tp,self,vm_list_index);
            } else if (vm_operations_cmp(tp,vm_string("sort"),k) == 0) {
                return vm_misc_method(tp,self,vm_list_sort);
            } else if (vm_operations_cmp(tp,vm_string("extend"),k) == 0) {
                return vm_misc_method(tp,self,vm_list_extend);
            } else if (vm_operations_cmp(tp,vm_string("*"),k) == 0) {
                vm_misc_params_v(tp,1,self);
                r = vm_builtins_copy(tp);
                self.list.val->len=0;
                return r;
            }
        } else if (k.type == vm_enum1_none) {
            return vm_list_pop(tp,self.list.val,0,"vm_operations_get");
        }
    } else if (type == vm_enum1_string) {
        if (k.type == vm_enum1_number) {
            int l = self.string.len;
            int n = k.number.val;
            n = (n<0?l+n:n);
            if (n >= 0 && n < l) { return vm_string_n(tp->chars[(unsigned char)self.string.val[n]],1); }
        } else if (k.type == vm_enum1_string) {
            if (vm_operations_cmp(tp,vm_string("join"),k) == 0) {
                return vm_misc_method(tp,self,vm_string_join);
            } else if (vm_operations_cmp(tp,vm_string("split"),k) == 0) {
                return vm_misc_method(tp,self,vm_string_split);
            } else if (vm_operations_cmp(tp,vm_string("index"),k) == 0) {
                return vm_misc_method(tp,self,vm_string_obj_index);
            } else if (vm_operations_cmp(tp,vm_string("strip"),k) == 0) {
                return vm_misc_method(tp,self,vm_string_strip);
            } else if (vm_operations_cmp(tp,vm_string("replace"),k) == 0) {
                return vm_misc_method(tp,self,vm_string_replace);
            }
        }
    }

    if (k.type == vm_enum1_list) {
        int a,b,l;
        type_vmObj tmp;
        l = vm_operations_len(tp,self).number.val;
        tmp = vm_operations_get(tp,k,vm_create_numericObj(0));
        if (tmp.type == vm_enum1_number) { a = tmp.number.val; }
        else if(tmp.type == vm_enum1_none) { a = 0; }
        else { vm_raise(0,vm_string("(vm_operations_get) TypeError: indices must be numbers")); }
        tmp = vm_operations_get(tp,k,vm_create_numericObj(1));
        if (tmp.type == vm_enum1_number) { b = tmp.number.val; }
        else if(tmp.type == vm_enum1_none) { b = l; }
        else { vm_raise(0,vm_string("(vm_operations_get) TypeError: indices must be numbers")); }
        a = vm_max(0,(a<0?l+a:a)); b = vm_min(l,(b<0?l+b:b));
        if (type == vm_enum1_list) {
            return vm_list_n(tp,b-a,&self.list.val->items[a]);
        } else if (type == vm_enum1_string) {
            return vm_string_substring(tp,self,a,b);
        }
    }

    vm_raise(0,vm_string("(vm_operations_get) TypeError: ?"));
	return vm_none;
}

/* Function: vm_operations_safeget
 * Failsafe attribute lookup.
 *
 * This is like <vm_operations_get>, except it will return false if the attribute lookup
 * failed. Otherwise, it will return true, and the object will be returned
 * over the reference parameter r.
 */
int vm_operations_safeget(type_vm *tp,type_vmObj *r, type_vmObj self, type_vmObj k) {
    if (self.type == vm_enum1_dict) {
        int n = vm_dict_find_sub(tp,self.dict.val,k);
        if (n == -1) { return 0; }
        *r = self.dict.val->items[n].val;
        vm_gc_grey(tp,*r);
        return 1;
    }
    if (self.type == vm_enum1_list && !self.list.val->len) { return 0; }
    *r = vm_operations_get(tp,self,k); vm_gc_grey(tp,*r);
    return 1;
}

/* Function: vm_operations_set
 * Attribute modification.
 * 
 * This is the counterpart of vm_operations_get, it does the same as self[k] = v would do
 * in actual interpreter code.
 */
void vm_operations_set(type_vm *tp,type_vmObj self, type_vmObj k, type_vmObj v) {
    int type = self.type;

    if (type == vm_enum1_dict) {
			if (self.dict.dtype == 2) {
					type_vmObj meta; 
					if (vm_builtins_lookup(tp,self,vm_string("__set__"),&meta)) {
						vm_call_sub(tp,meta,vm_misc_params_v(tp,2,k,v));
						return;
						}			
			}
        vm_dict_set_sub(tp,self.dict.val,k,v);
        return;
    } else if (type == vm_enum1_list) {
        if (k.type == vm_enum1_number) {
            vm_list_set(tp,self.list.val,k.number.val,v,"vm_operations_set");
            return;
        } else if (k.type == vm_enum1_none) {
            vm_list_append(tp,self.list.val,v);
            return;
        } else if (k.type == vm_enum1_string) {
            if (vm_operations_cmp(tp,vm_string("*"),k) == 0) {
                vm_misc_params_v(tp,2,self,v); vm_list_extend(tp);
                return;
            }
        }
    }
    vm_raise(tp,vm_string("(vm_operations_set) TypeError: object does not support item assignment"));
}

type_vmObj vm_operations_add(type_vm *tp,type_vmObj a, type_vmObj b) {
    if (a.type == vm_enum1_number && a.type == b.type) {
        return vm_create_numericObj(a.number.val+b.number.val);
    } else if (a.type == vm_enum1_string && a.type == b.type) {
        int al = a.string.len, bl = b.string.len;
        type_vmObj r = vm_string_new(tp,al+bl);
        char *s = r.string.info->s;
        memcpy(s,a.string.val,al); memcpy(s+al,b.string.val,bl);
        return vm_gc_track(tp,r);
    } else if (a.type == vm_enum1_list && a.type == b.type) {
        type_vmObj r;
        vm_misc_params_v(tp,1,a);
        r = vm_builtins_copy(tp);
        vm_misc_params_v(tp,2,r,b);
        vm_list_extend(tp);
        return r;
    }
    vm_raise(0,vm_string("(vm_operations_add) TypeError: ?"));
	return vm_none;
}

type_vmObj vm_operations_mul(type_vm *tp,type_vmObj a, type_vmObj b) {
    if (a.type == vm_enum1_number && a.type == b.type) {
        return vm_create_numericObj(a.number.val*b.number.val);
    } else if ((a.type == vm_enum1_string && b.type == vm_enum1_number) || 
               (a.type == vm_enum1_number && b.type == vm_enum1_string)) {
        if(a.type == vm_enum1_number) {
            type_vmObj c = a; a = b; b = c;
        }
        int al = a.string.len; int n = b.number.val;
        if(n <= 0) {
            type_vmObj r = vm_string_new(tp,0);
            return vm_gc_track(tp,r);
        }
        type_vmObj r = vm_string_new(tp,al*n);
        char *s = r.string.info->s;
        int i; for (i=0; i<n; i++) { memcpy(s+al*i,a.string.val,al); }
        return vm_gc_track(tp,r);
    }
    vm_raise(0,vm_string("(vm_operations_mul) TypeError: ?"));
	return vm_none;
}

/* Function: vm_operations_len
 * Returns the length of an object.
 *
 * Returns the number of items in a list or dict, or the length of a string.
 */
type_vmObj vm_operations_len(type_vm *tp,type_vmObj self) {
    int type = self.type;
    if (type == vm_enum1_string) {
        return vm_create_numericObj(self.string.len);
    } else if (type == vm_enum1_dict) {
        return vm_create_numericObj(self.dict.val->len);
    } else if (type == vm_enum1_list) {
        return vm_create_numericObj(self.list.val->len);
    }
    
    vm_raise(0,vm_string("(vm_operations_len) TypeError: len() of unsized object"));
	return vm_none;
}

int vm_operations_cmp(type_vm *tp,type_vmObj a, type_vmObj b) {
    if (a.type != b.type) { return a.type-b.type; }
    switch(a.type) {
        case vm_enum1_none: return 0;
        case vm_enum1_number: return vm_sign(a.number.val-b.number.val);
        case vm_enum1_string: {
            int l = vm_min(a.string.len,b.string.len);
            int v = memcmp(a.string.val,b.string.val,l);
            if (v == 0) {
                v = a.string.len-b.string.len;
            }
            return v;
        }
        case vm_enum1_list: {
            int n,v; for(n=0;n<vm_min(a.list.val->len,b.list.val->len);n++) {
        type_vmObj aa = a.list.val->items[n]; type_vmObj bb = b.list.val->items[n];
            if (aa.type == vm_enum1_list && bb.type == vm_enum1_list) { v = aa.list.val-bb.list.val; } else { v = vm_operations_cmp(tp,aa,bb); }
            if (v) { return v; } }
            return a.list.val->len-b.list.val->len;
        }
        case vm_enum1_dict: return a.dict.val - b.dict.val;
        case vm_enum1_fnc: return a.fnc.info - b.fnc.info;
        case vm_enum1_data: return (char*)a.data.val - (char*)b.data.val;
    }
    vm_raise(0,vm_string("(vm_operations_cmp) TypeError: ?"));
	return 0;
}


type_vmObj vm_operations_bitwise_and(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(((long)a)&((long)b)); 
    }
	return vm_none;
} /* interpreter_OP(vm_operations_bitwise_and,((long)a)&((long)b)); */
type_vmObj vm_operations_bitwise_or(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(((long)a)|((long)b)); 
    }
	return vm_none;
}
type_vmObj vm_operations_bitwise_xor(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(((long)a)^((long)b)); 
    }
	return vm_none;
}
type_vmObj vm_operations_mod(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(((long)a)%((long)b)); 
    }
	return vm_none;
}
type_vmObj vm_operations_lsh(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(((long)a)<<((long)b)); 
    }
	return vm_none;
}
type_vmObj vm_operations_rsh(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(((long)a)>>((long)b)); 
    }
	return vm_none;
}
type_vmObj vm_operations_sub(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(a-b); 
    }
	return vm_none;
}
type_vmObj vm_operations_div(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(a/b); 
    }
	return vm_none;
}
type_vmObj vm_operations_pow(type_vm *tp,type_vmObj _a,type_vmObj _b)
{
	    if (_a.type == vm_enum1_number && _a.type == _b.type) { 
        type_vmNum a = _a.number.val; type_vmNum b = _b.number.val; 
        return vm_create_numericObj(pow(a,b)); 
    }
	return vm_none;
}

type_vmObj vm_operations_bitwise_not(type_vm *tp, type_vmObj a) {
    if (a.type == vm_enum1_number) {
        return vm_create_numericObj(~(long)a.number.val);
    }
    vm_raise(0,vm_string("(vm_operations_bitwise_not) TypeError: unsupported operand type"));
	return vm_none;
}

/**/
