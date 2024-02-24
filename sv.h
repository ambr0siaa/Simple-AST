#ifndef SV_H_
#define SV_H_

#include <ctype.h>
#include <string.h>

typedef struct {
    char *data;
    size_t count;
} String_View;

#define SV_Fmt "%.*s"
#define SV_Args(sv) (int) (sv).count, (sv).data

String_View sv_div_by_next_symbol(String_View *sv);
String_View sv_from_cstr(char *cstr);
String_View sv_trim_left(String_View sv);
String_View sv_trim_right(String_View sv);
String_View sv_trim(String_View sv);
String_View sv_div_by_delim(String_View *sv, char delim);

int sv_cmp(String_View sv1, String_View sv2);
int sv_to_int(String_View sv);
int sv_is_float(String_View sv);

#endif // SV_H_