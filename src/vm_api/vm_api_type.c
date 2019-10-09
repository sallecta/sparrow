/* 
 * Coerces any value to a boolean.
 */
type_vmObj vm_api_type_bool(type_vm *tp) {
    type_vmObj v = vm_operations_get(tp,tp->params,vm_none);
    return (vm_create_numericObj(vm_operations_bool(tp, v)));
}
type_vmObj vm_api_type_int(type_vm *tp) {
    return vm_create_numericObj((long)vm_api_type_float(tp).number.val);
}