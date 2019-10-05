#include "os.c"


/* os */


void os_init(type_vm *tp)
{
    type_vmObj module,submodule;
    module = interpreter_dict(tp);
    interpreter_set(tp,tp->modules,interpreter_string("os"),module);
/*    interpreter_set(tp,module,interpreter_string("init"),interpreter_fnc(tp,_os_init));*/

    /* path */
    submodule = interpreter_dict(tp);
    interpreter_set(tp,module,interpreter_string("path"),submodule);
    interpreter_set(tp,submodule,interpreter_string("abspath"),interpreter_fnc(tp,os_path_abspath));

    /*
     * bind special attributes to os module
     */
    interpreter_set(tp, module, interpreter_string("__doc__"),
           interpreter_string(
               "OS module"));
    interpreter_set(tp, module, interpreter_string("__name__"), interpreter_string("os"));
    interpreter_set(tp, module, interpreter_string("__file__"), interpreter_string(__FILE__));
}