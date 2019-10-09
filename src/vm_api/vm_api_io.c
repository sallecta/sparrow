type_vmObj vm_api_io_print(type_vm *tp) {
    int n = 0;
    type_vmObj e;
    int len = tp->params.list.val->len;
    int i; 
	for (i=0; i<len; i++) { 
    (e) = vm_list_get(tp,tp->params.list.val,i,"interpreter_LOOP");
        if (n) { printf(" "); }
        vm_echo(tp,e);
        n += 1;
    }	
    printf("\n");
    return vm_none;
}