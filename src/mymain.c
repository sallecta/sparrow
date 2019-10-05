#include "interpreter.c"
#include "modules/math/init.c"

int main(int argc, char *argv[]) {
    type_Vm *iObj = interpreter_init(argc,argv);
    math_init(iObj);
    interpreter_ez_call(iObj,"obfuscatedDataType","interp",interpreter_None);
    interpreter_deinit(iObj);
    return(0);
}

/**/
