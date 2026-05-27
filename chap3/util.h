#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stdbool.h>

typedef char *string;

/* In C23 and later, `bool` is a keyword. Only provide the typedef on older
   standards where `bool` isn't a keyword. */
#if !(defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

void *checked_malloc(int);
string String(char *);

typedef struct U_boolList_ *U_boolList;
struct U_boolList_ {
  bool head;
  U_boolList tail;
};
U_boolList U_BoolList(bool head, U_boolList tail);
#endif
