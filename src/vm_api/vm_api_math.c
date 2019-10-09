
type_vmObj vm_api_type_float(type_vm *tp) {
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    int ord = vm_macros_DEFAULT(vm_create_numericObj(0)).number.val;
    int type = v.type;
    if (type == vm_enum1_number) { return v; }
    if (type == vm_enum1_string && v.string.len < 32) {
        char s[32]; memset(s,0,v.string.len+1);
        memcpy(s,v.string.val,v.string.len);
        if (strchr(s,'.')) { return vm_create_numericObj(atof(s)); }
        return(vm_create_numericObj(strtol(s,0,ord)));
    }
    vm_raise(tp,vm_string("(vm_api_type_float) TypeError: ?"));
	return vm_none;
}

type_vmObj vm_api_math_abs(type_vm *tp) {
    return vm_create_numericObj(fabs(vm_api_type_float(tp).number.val));
}

type_vmNum roundf_sub(type_vmNum v) {
    type_vmNum av = fabs(v); type_vmNum iv = (long)av;
    av = (av-iv < 0.5?iv:iv+1);
    return (v<0?-av:av);
}
type_vmObj vm_api_math_round(type_vm *tp) {
    return vm_create_numericObj(roundf_sub(vm_api_type_float(tp).number.val));
}

type_vmObj vm_api_math_range(type_vm *tp) {
    int a,b,c,i;
    type_vmObj r = vm_list(tp);
    switch (tp->params.list.val->len) {
        case 1: a = 0; b = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val; c = 1; break;
        case 2:
        case 3: a = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val; \
		b = vm_typecheck(tp,vm_enum1_number,vm_operations_get(tp,tp->params,vm_none)).number.val; \
		c = vm_macros_DEFAULT(vm_create_numericObj(1)).number.val; break;
        default: return r;
    }
    if (c != 0) {
        for (i=a; (c>0) ? i<b : i>b; i+=c) {
            vm_list_append(tp,r.list.val,vm_create_numericObj(i));
        }
    }
    return r;
}


