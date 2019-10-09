#include "vm.c"
#include "modules/math/init.c"

int main(int argc, char *argv[]) {
    type_vm *vm = vm_init(argc,argv);
    math_init(vm);
    vm_call(vm,"obfuscatedDataType","interp",vm_none);
    vm_deinit(vm);
    return(0);
}

/**/
