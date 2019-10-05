#include "math.c"

/*
 * init math module, namely, set its dictionary
 */
void math_init(type_vm *tp)
{
    /*
     * new a module dict for math
     */
    type_vmObj math_mod = interpreter_dict(tp);

    /*
     * initialize pi and e
     */
    math_pi = interpreter_number(M_PI);
    math_e  = interpreter_number(M_E);

    /*
     * bind math functions to math module
     */
    interpreter_set(tp, math_mod, interpreter_string("pi"), math_pi);
    interpreter_set(tp, math_mod, interpreter_string("e"), math_e);
    interpreter_set(tp, math_mod, interpreter_string("acos"), interpreter_fnc(tp, math_acos));
    interpreter_set(tp, math_mod, interpreter_string("asin"), interpreter_fnc(tp, math_asin));
    interpreter_set(tp, math_mod, interpreter_string("atan"), interpreter_fnc(tp, math_atan));
    interpreter_set(tp, math_mod, interpreter_string("atan2"), interpreter_fnc(tp, math_atan2));
    interpreter_set(tp, math_mod, interpreter_string("ceil"), interpreter_fnc(tp, math_ceil));
    interpreter_set(tp, math_mod, interpreter_string("cos"), interpreter_fnc(tp, math_cos));
    interpreter_set(tp, math_mod, interpreter_string("cosh"), interpreter_fnc(tp, math_cosh));
    interpreter_set(tp, math_mod, interpreter_string("degrees"), interpreter_fnc(tp, math_degrees));
    interpreter_set(tp, math_mod, interpreter_string("exp"), interpreter_fnc(tp, math_exp));
    interpreter_set(tp, math_mod, interpreter_string("fabs"), interpreter_fnc(tp, math_fabs));
    interpreter_set(tp, math_mod, interpreter_string("floor"), interpreter_fnc(tp, math_floor));
    interpreter_set(tp, math_mod, interpreter_string("fmod"), interpreter_fnc(tp, math_fmod));
    interpreter_set(tp, math_mod, interpreter_string("frexp"), interpreter_fnc(tp, math_frexp));
    interpreter_set(tp, math_mod, interpreter_string("hypot"), interpreter_fnc(tp, math_hypot));
    interpreter_set(tp, math_mod, interpreter_string("ldexp"), interpreter_fnc(tp, math_ldexp));
    interpreter_set(tp, math_mod, interpreter_string("log"), interpreter_fnc(tp, math_log));
    interpreter_set(tp, math_mod, interpreter_string("log10"), interpreter_fnc(tp, math_log10));
    interpreter_set(tp, math_mod, interpreter_string("modf"), interpreter_fnc(tp, math_modf));
    interpreter_set(tp, math_mod, interpreter_string("pow"), interpreter_fnc(tp, math_pow));
    interpreter_set(tp, math_mod, interpreter_string("radians"), interpreter_fnc(tp, math_radians));
    interpreter_set(tp, math_mod, interpreter_string("sin"), interpreter_fnc(tp, math_sin));
    interpreter_set(tp, math_mod, interpreter_string("sinh"), interpreter_fnc(tp, math_sinh));
    interpreter_set(tp, math_mod, interpreter_string("sqrt"), interpreter_fnc(tp, math_sqrt));
    interpreter_set(tp, math_mod, interpreter_string("tan"), interpreter_fnc(tp, math_tan));
    interpreter_set(tp, math_mod, interpreter_string("tanh"), interpreter_fnc(tp, math_tanh));

    /*
     * bind special attributes to math module
     */
    interpreter_set(tp, math_mod, interpreter_string("__doc__"), 
            interpreter_string(
                "This module is always available.  It provides access to the\n"
                "mathematical functions defined by the C standard."));
    interpreter_set(tp, math_mod, interpreter_string("__name__"), interpreter_string("math"));
    interpreter_set(tp, math_mod, interpreter_string("__file__"), interpreter_string(__FILE__));

    /*
     * bind to tiny modules[]
     */
    interpreter_set(tp, tp->modules, interpreter_string("math"), math_mod);
}

