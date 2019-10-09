#include "math.c"

/*
 * init math module, namely, set its dictionary
 */
void math_init(type_vm *tp)
{
    /*
     * new a module dict for math
     */
    type_vmObj math_mod = vm_dict_create(tp);

    /*
     * initialize pi and e
     */
    math_pi = vm_create_numericObj(M_PI);
    math_e  = vm_create_numericObj(M_E);

    /*
     * bind math functions to math module
     */
    vm_operations_set(tp, math_mod, vm_string("pi"), math_pi);
    vm_operations_set(tp, math_mod, vm_string("e"), math_e);
    vm_operations_set(tp, math_mod, vm_string("acos"), vm_misc_fnc(tp, math_acos));
    vm_operations_set(tp, math_mod, vm_string("asin"), vm_misc_fnc(tp, math_asin));
    vm_operations_set(tp, math_mod, vm_string("atan"), vm_misc_fnc(tp, math_atan));
    vm_operations_set(tp, math_mod, vm_string("atan2"), vm_misc_fnc(tp, math_atan2));
    vm_operations_set(tp, math_mod, vm_string("ceil"), vm_misc_fnc(tp, math_ceil));
    vm_operations_set(tp, math_mod, vm_string("cos"), vm_misc_fnc(tp, math_cos));
    vm_operations_set(tp, math_mod, vm_string("cosh"), vm_misc_fnc(tp, math_cosh));
    vm_operations_set(tp, math_mod, vm_string("degrees"), vm_misc_fnc(tp, math_degrees));
    vm_operations_set(tp, math_mod, vm_string("exp"), vm_misc_fnc(tp, math_exp));
    vm_operations_set(tp, math_mod, vm_string("fabs"), vm_misc_fnc(tp, math_fabs));
    vm_operations_set(tp, math_mod, vm_string("floor"), vm_misc_fnc(tp, math_floor));
    vm_operations_set(tp, math_mod, vm_string("fmod"), vm_misc_fnc(tp, math_fmod));
    vm_operations_set(tp, math_mod, vm_string("frexp"), vm_misc_fnc(tp, math_frexp));
    vm_operations_set(tp, math_mod, vm_string("hypot"), vm_misc_fnc(tp, math_hypot));
    vm_operations_set(tp, math_mod, vm_string("ldexp"), vm_misc_fnc(tp, math_ldexp));
    vm_operations_set(tp, math_mod, vm_string("log"), vm_misc_fnc(tp, math_log));
    vm_operations_set(tp, math_mod, vm_string("log10"), vm_misc_fnc(tp, math_log10));
    vm_operations_set(tp, math_mod, vm_string("modf"), vm_misc_fnc(tp, math_modf));
    vm_operations_set(tp, math_mod, vm_string("pow"), vm_misc_fnc(tp, math_pow));
    vm_operations_set(tp, math_mod, vm_string("radians"), vm_misc_fnc(tp, math_radians));
    vm_operations_set(tp, math_mod, vm_string("sin"), vm_misc_fnc(tp, math_sin));
    vm_operations_set(tp, math_mod, vm_string("sinh"), vm_misc_fnc(tp, math_sinh));
    vm_operations_set(tp, math_mod, vm_string("sqrt"), vm_misc_fnc(tp, math_sqrt));
    vm_operations_set(tp, math_mod, vm_string("tan"), vm_misc_fnc(tp, math_tan));
    vm_operations_set(tp, math_mod, vm_string("tanh"), vm_misc_fnc(tp, math_tanh));

    /*
     * bind special attributes to math module
     */
    vm_operations_set(tp, math_mod, vm_string("__doc__"), 
            vm_string(
                "This module is always available.  It provides access to the\n"
                "mathematical functions defined by the C standard."));
    vm_operations_set(tp, math_mod, vm_string("__name__"), vm_string("math"));
    vm_operations_set(tp, math_mod, vm_string("__file__"), vm_string(__FILE__));

    /*
     * bind to tiny modules[]
     */
    vm_operations_set(tp, tp->modules, vm_string("math"), math_mod);
}

