:browse tabnew
#include <stdlib.h>
#include <stdio.h>

typedef struct { union { char c; int i; long l; double f;
                         char *ca; int *ia; long *la; double *fa; void *v; }; } Lit;

typedef struct Stk { Lit x; struct Stk *prev; } Stk;

void push_int(int i) { nstkptr(); stk->x.i = i; }
void push_flt(double f) { nstkptr(); stk->x.f = f; }
void push_chr(char c) { nstkptr(); stk->x.c = c; }
void push_lng(long l) { nstkptr(); stk->x.l = l; }
void pop(Stk *stk) { Stk *e; e = stk; stk = stk->prev; free(e); }

long popl(Stk *stk) { long q = stk->x.l; pop(); return q; }
double popf(Stk *stk) { double q = stk->x.f; pop(); return q; }
