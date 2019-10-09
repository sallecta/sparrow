type_vmObj vm_api_stat_min(type_vm *tp) {
    type_vmObj r = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj e;
    interpreter_LOOP(e)
        if (vm_operations_cmp(tp,r,e) > 0) { r = e; }
    interpreter_END;
    return r;
}
type_vmObj vm_api_stat_max(type_vm *tp) {
    type_vmObj r = vm_operations_get(tp,tp->params,vm_none);
    type_vmObj e;
    interpreter_LOOP(e)
        if (vm_operations_cmp(tp,r,e) < 0) { r = e; }
    interpreter_END;
    return r;
}
