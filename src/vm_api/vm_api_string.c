type_vmObj vm_api_string_len(type_vm *tp) {
    type_vmObj e = vm_operations_get(tp,tp->params,vm_none);
    return vm_operations_len(tp,e);
}
