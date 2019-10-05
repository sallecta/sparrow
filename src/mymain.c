#include "interpreter.c"
#include "modules/math/init.c"

int main(int argc, char *argv[]) {
    interpreter_vm *tp = interpreter_init(argc,argv);
    math_init(tp);
    interpreter_ez_call(tp,"obfuscatedDataType","interp",interpreter_None);
    interpreter_deinit(tp);
    return(0);
}

/**/
