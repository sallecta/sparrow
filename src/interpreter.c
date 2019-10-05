#ifndef interpreter_COMPILER
#define interpreter_COMPILER 1
#endif

#include "interpreter.h"
#include "list.c"
#include "dict.c"
#include "misc.c"
#include "string.c"
#include "builtins.c"
#include "gc.c"
#include "ops.c"
#ifdef interpreter_SANDBOX
#include "sandbox.c"
#endif

void interpreter_compiler(TP);
#include "vm.c"

interpreter_obj interpreter_None = {interpreter_NONE};

#if interpreter_COMPILER
#include "bc.c"
void interpreter_compiler(TP) {
    interpreter_import(tp,0,"tokenize",interpreter_tokenize,sizeof(interpreter_tokenize));
    interpreter_import(tp,0,"parse",interpreter_parse,sizeof(interpreter_parse));
    interpreter_import(tp,0,"encode",interpreter_encode,sizeof(interpreter_encode));
    interpreter_import(tp,0,"obfuscatedDataType",interpreter_obfuscatedArray,sizeof(interpreter_obfuscatedArray));
    interpreter_ez_call(tp,"obfuscatedDataType","_init",interpreter_None);
}
#else
void interpreter_compiler(TP) { }
#endif

/**/
