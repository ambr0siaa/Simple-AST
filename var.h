#ifndef VAR_H_
#define VAR_H_

#ifndef LEXER_H_
#   include <assert.h>
#   include <stdlib.h>
#   include <stdint.h>
#   include "./sv.h"
#endif

typedef enum {
    VAL_FLOAT = 0,
    VAL_INT
} Value_Type;

typedef struct {
    Value_Type type;
    union {
        int64_t i64;
        double f64;
    };
} Value;

#define VALUE_INT(val) (Value) { .type = VAL_INT, .i64 = (val) }
#define VALUE_FLOAT(val) (Value) { .type = VAL_FLOAT, .f64 = (val) }

// It's only for numbers
typedef struct {
    String_View name;
    Value val;
} Variable;

#define VAR_NONE (Variable) { .name = sv_from_cstr("None") }

typedef struct {
    Variable *items;
    size_t capacity;
    size_t count;
} Var_List;

#define INIT_CAPACITY 256

// macro for append item to dynamic array
#define da_append(da, new_item)                                                         \
    do {                                                                                \
        if ((da)->count + 1 >= (da)->capacity) {                                        \
            (da)->capacity = (da)->capacity > 0 ? (da)->capacity * 2 : INIT_CAPACITY;   \
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));  \
            assert((da)->items != NULL);                                                \
        }                                                                               \
        (da)->items[(da)->count++] = (new_item);                                        \
    } while(0)

#define da_clean(da)        \
    do {                    \
        free((da)->items);  \
        (da)->count = 0;    \
        (da)->capacity = 0; \
    } while(0)

void var_push(Var_List *vl, Variable var);
void var_clean(Var_List *vl);
Variable var_search(Var_List *vl, String_View name);
Variable var_create(char *name, Value val);

#endif // VAR_H_