#include "example.c"

/*
 * init example module, namely, set its dictionary
 */
void example_init(type_vm *tp)
{
    /*
     * new a module dict for example
     */
    type_vmObj example_mod = interpreter_dict(tp);

    /*
     * bind example functions to example module
     */
    interpreter_set(tp, example_mod, interpreter_string("myfunction"), interpreter_fnc(tp, example_myfunction));

    /*
     * bind special attributes to example module
     */
    interpreter_set(tp, example_mod, interpreter_string("__doc__"), 
            interpreter_string(
                "This module is always available.  It provides access to the\n"
                "exampleematical functions defined by the C standard."));
    interpreter_set(tp, example_mod, interpreter_string("__name__"), interpreter_string("example"));
    interpreter_set(tp, example_mod, interpreter_string("__file__"), interpreter_string(__FILE__));

    /*
     * bind to tiny modules[]
     */
    interpreter_set(tp, tp->modules, interpreter_string("example"), example_mod);
}

