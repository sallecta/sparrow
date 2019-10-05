#include "interpreter.c"
#include "modules/random/init.c"
#include "modules/math/init.c"

int main(int argc, char *argv[]) {
    interpreter_vm *tp = interpreter_init(argc,argv);
    random_init(tp);
    math_init(tp);
    interpreter_ez_call(tp,"obfuscatedDataType","interp",interpreter_None);
    interpreter_deinit(tp);
    return(0);
}

/**/
