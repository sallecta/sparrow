#include "os.c"


/* os */


void os_init(type_vm *tp)
{
    type_vmObj module,submodule;
    module = vm_dict_create(tp);
    vm_operations_set(tp,tp->modules,vm_string("os"),module);
/*    vm_operations_set(tp,module,vm_string("init"),vm_misc_fnc(tp,_os_init));*/

    /* path */
    submodule = vm_dict_create(tp);
    vm_operations_set(tp,module,vm_string("path"),submodule);
    vm_operations_set(tp,submodule,vm_string("abspath"),vm_misc_fnc(tp,os_path_abspath));

    /*
     * bind special attributes to os module
     */
    vm_operations_set(tp, module, vm_string("__doc__"),
           vm_string(
               "OS module"));
    vm_operations_set(tp, module, vm_string("__name__"), vm_string("os"));
    vm_operations_set(tp, module, vm_string("__file__"), vm_string(__FILE__));
}