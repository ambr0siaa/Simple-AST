#include "../include/var.h"

void var_clean(Var_List *vl) { da_clean(vl); }
void var_push(Var_List *vl, Variable var) { da_append(vl, var); }

Variable var_create(char *name, Value val)
{
    return (Variable) {
        .name = sv_from_cstr(name),
        .val = val
    };
}

Variable var_search(Var_List *vl, String_View name)
{
    for (size_t i = 0; i < vl->count; ++i) {
        if (sv_cmp(vl->items[i].name, name)) {
            return vl->items[i];
        }
    }
    return VAR_NONE;
}