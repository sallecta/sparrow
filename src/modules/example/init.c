#include "example.c"

/*
 * init example module, namely, set its dictionary
 */
void example_init(type_vm *tp)
{
    /*
     * new a module dict for example
     */
    type_vmObj example_mod = vm_dict_create(tp);

    /*
     * bind example functions to example module
     */
    vm_operations_set(tp, example_mod, vm_string("myfunction"), vm_misc_fnc(tp, example_myfunction));

    /*
     * bind special attributes to example module
     */
    vm_operations_set(tp, example_mod, vm_string("__doc__"), 
            vm_string(
                "This module is always available.  It provides access to the\n"
                "exampleematical functions defined by the C standard."));
    vm_operations_set(tp, example_mod, vm_string("__name__"), vm_string("example"));
    vm_operations_set(tp, example_mod, vm_string("__file__"), vm_string(__FILE__));

    /*
     * bind to tiny modules[]
     */
    vm_operations_set(tp, tp->modules, vm_string("example"), example_mod);
}

