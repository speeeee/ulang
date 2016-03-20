#include <stdlib.h>
#include <stdio.h>

typedef struct { union { char c; int i; long l; double f;
                         char *ca; int *ia; long *la; double *fa; void *v; }; } Lit;

typedef struct Stk { Lit x; struct Stk *prev; } Stk;

void push_int(int); void push_flt(double); void push_chr(char);
void push_lng(long); void pop(void);
