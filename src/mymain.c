#include "interpreter.c"
#include "modules/math/init.c"

int main(int argc, char *argv[]) {
    type_vm *vmObj = interpreter_init(argc,argv);
    math_init(vmObj);
    interpreter_ez_call(vmObj,"obfuscatedDataType","interp",interpreter_None);
    interpreter_deinit(vmObj);
    return(0);
}

/**/
