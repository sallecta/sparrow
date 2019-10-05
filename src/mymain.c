#include "interpreter.c"
#include "modules/math/init.c"

int main(int argc, char *argv[]) {
    type_vm *vm = interpreter_init(argc,argv);
    math_init(vm);
    interpreter_ez_call(vm,"obfuscatedDataType","interp",interpreter_None);
    interpreter_deinit(vm);
    return(0);
}

/**/
