
main.c-- vm=mc.vm_init
	vm.c--type_vm *vm = sub_vm_init();
	vm.c--vm_regbuiltins(vm);
	vm.c--vm_args(vm,argc,argv);
	vm.c--vm_compiler(vm);
	vm.c--return vm;