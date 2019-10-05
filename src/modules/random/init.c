#include "random.c"

/*
 * random_mod_init()
 *
 * random module initialization function
 */
void random_init(TP)
{
    /*
     * module dict for random
     */
    interpreter_obj random_mod = interpreter_dict(tp);

    /*
     * bind functions to random module
     */
    interpreter_set(tp, random_mod, interpreter_string("seed"),       interpreter_fnc(tp, random_seed));
    interpreter_set(tp, random_mod, interpreter_string("getstate"),   interpreter_fnc(tp, random_getstate));
    interpreter_set(tp, random_mod, interpreter_string("setstate"),   interpreter_fnc(tp, random_setstate));
    interpreter_set(tp, random_mod, interpreter_string("jumpahead"),  interpreter_fnc(tp, random_jumpahead));
    interpreter_set(tp, random_mod, interpreter_string("random"),     interpreter_fnc(tp, random_random));

    /*
     * bind usual distribution random variable generator
     */
    interpreter_set(tp, random_mod, interpreter_string("uniform"),        interpreter_fnc(tp, random_uniform));
    interpreter_set(tp, random_mod, interpreter_string("normalvariate"),  interpreter_fnc(tp, random_normalvariate));
    interpreter_set(tp, random_mod, interpreter_string("lognormvariate"), interpreter_fnc(tp, random_lognormvariate));
    interpreter_set(tp, random_mod, interpreter_string("expovariate"),    interpreter_fnc(tp, random_expovariate));
    interpreter_set(tp, random_mod, interpreter_string("vonmisesvariate"), interpreter_fnc(tp, random_vonmisesvariate));
    interpreter_set(tp, random_mod, interpreter_string("gammavariate"),   interpreter_fnc(tp, random_gammavariate));
    interpreter_set(tp, random_mod, interpreter_string("betavariate"),    interpreter_fnc(tp, random_betavariate));
    interpreter_set(tp, random_mod, interpreter_string("paretovariate"),  interpreter_fnc(tp, random_paretovariate));
    interpreter_set(tp, random_mod, interpreter_string("weibullvariate"), interpreter_fnc(tp, random_weibullvariate));
    interpreter_set(tp, random_mod, interpreter_string("randrange"),      interpreter_fnc(tp, random_randrange));
    interpreter_set(tp, random_mod, interpreter_string("randint"),        interpreter_fnc(tp, random_randint));
    interpreter_set(tp, random_mod, interpreter_string("choice"),         interpreter_fnc(tp, random_choice));
    interpreter_set(tp, random_mod, interpreter_string("shuffle"),        interpreter_fnc(tp, random_shuffle));

    /*
     * bind special attributes to random module
     */
    interpreter_set(tp, random_mod, interpreter_string("__doc__"),  interpreter_string("Random variable generators."));
    interpreter_set(tp, random_mod, interpreter_string("__name__"), interpreter_string("random"));
    interpreter_set(tp, random_mod, interpreter_string("__file__"), interpreter_string(__FILE__));

    /*
     * bind random module to interpreter modules[]
     */
    interpreter_set(tp, tp->modules, interpreter_string("random"), random_mod);
}
