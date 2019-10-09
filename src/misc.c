/* File: Miscellaneous
 * Various functions to help interface interpreter.
 */

type_vmObj vm_misc_dcall(type_vm *tp,type_vmObj fnc(type_vm *tp)) {
    return fnc(tp);
}
type_vmObj vm_misc_tcall(type_vm *tp,type_vmObj fnc) {
    if (fnc.fnc.ftype&2) {
        vm_list_insert(tp,tp->params.list.val,0,fnc.fnc.info->self);
    }
    return vm_misc_dcall(tp,(type_vmObj (*)(type_vm *))fnc.fnc.cfnc);
}

type_vmObj vm_misc_fnc_new(type_vm *tp,int t, void *v, type_vmObj c,type_vmObj s, type_vmObj g) {
    type_vmObj r = {vm_enum1_fnc};
    type_vmFnc *info = (type_vmFnc*)calloc(sizeof(type_vmFnc),1);/*calloc((x),1)*/
    info->code = c;
    info->self = s;
    info->globals = g;
    r.fnc.ftype = t;
    r.fnc.info = info;
    r.fnc.cfnc = v;
    return vm_gc_track(tp,r);
}

type_vmObj vm_misc_def(type_vm *tp,type_vmObj code, type_vmObj g) {
    type_vmObj r = vm_misc_fnc_new(tp,1,0,code,vm_none,g);
    return r;
}

/* Function: vm_misc_fnc
 * Creates a new interpreter function object.
 * 
 * This is how you can create a interpreter function object which, when called in
 * the script, calls the provided C function.
 */
type_vmObj vm_misc_fnc(type_vm *tp,type_vmObj v(type_vm *tp)) {
    return vm_misc_fnc_new(tp,0,v,vm_none,vm_none,vm_none);
}

type_vmObj vm_misc_method(type_vm *tp,type_vmObj self,type_vmObj v(type_vm *tp)) {
    return vm_misc_fnc_new(tp,2,v,vm_none,self,vm_none);
}

/* Function: vm_misc_dataObj
 * Creates a new data object.
 * 
 * Parameters:
 * magic - An integer number associated with the data type. This can be used
 *         to check the type of data objects.
 * v     - A pointer to user data. Only the pointer is stored in the object,
 *         you keep all responsibility for the data it points to.
 *
 * 
 * Returns:
 * The new data object.
 * 
 * Public fields:
 * The following fields can be access in a data object:
 * 
 * magic      - An integer number stored in the object.
 * val        - The data pointer of the object.
 * info->free - If not NULL, a callback function called when the object gets
 *              destroyed.
 * 
 * Example:
 * > void *__free__(type_vm *tp, type_vmObj self)
 * > {
 * >     free(self.data.val);
 * > }
 * >
 * > type_vmObj my_obj = vm_misc_dataObj(type_vm *tp, 0, my_ptr);
 * > my_obj.data.info->free = __free__;
 */
type_vmObj vm_misc_dataObj(type_vm *tp,int magic,void *v) {
    type_vmObj r = {vm_enum1_data};
    r.data.info = (vm_type_data*)calloc(sizeof(vm_type_data),1);/*calloc((x),1)*/
    r.data.val = v;
    r.data.magic = magic;
    return vm_gc_track(tp,r);
}

/* Function: vm_misc_params
 * Initialize the interpreter parameters.
 *
 * When you are calling a interpreter function, you can use this to initialize the
 * list of parameters getting passed to it. Usually, you may want to use
 * <vm_misc_params_n> or <vm_misc_params_v>.
 */
type_vmObj vm_misc_params(type_vm *tp) {
    type_vmObj r;
    tp->params = tp->params_sub.list.val->items[tp->curFrame];
    r = tp->params_sub.list.val->items[tp->curFrame];
    r.list.val->len = 0;
    return r;
}

/* Function: vm_misc_params_n
 * Specify a list of objects as function call parameters.
 *
 * See also: <vm_misc_params>, <vm_misc_params_v>
 *
 * Parameters:
 * n - The number of parameters.
 * argv - A list of n interpreter objects, which will be passed as parameters.
 *
 * Returns:
 * The parameters list. You may modify it before performing the function call.
 */
type_vmObj vm_misc_params_n(type_vm *tp,int n, type_vmObj argv[]) {
    type_vmObj r = vm_misc_params(tp);
    int i; for (i=0; i<n; i++) { vm_list_append(tp,r.list.val,argv[i]); }
    return r;
}

/* Function: vm_misc_params_v
 * Pass parameters for a interpreter function call.
 * 
 * When you want to call a interpreter method, then you use this to pass parameters
 * to it.
 * 
 * Parameters:
 * n   - The number of variable arguments following.
 * ... - Pass n interpreter objects, which a subsequently called interpreter method will
 *       receive as parameters.
 * 
 * Returns:
 * A interpreter list object representing the current call parameters. You can modify
 * the list before doing the function call.
 */
type_vmObj vm_misc_params_v(type_vm *tp,int n,...) {
    int i;
    type_vmObj r = vm_misc_params(tp);
    va_list a; va_start(a,n);
    for (i=0; i<n; i++) {
        vm_list_append(tp,r.list.val,va_arg(a,type_vmObj));
    }
    va_end(a);
    return r;
}
