// byte-code interpreter (see assembler.c for an assembler and opcodes)
// interpreter uses a stack made out of a linked-list

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <math.h>

#define P 4
#define PW 0
#define PF 1
#define PC 2
#define PL 3
#define MALLOCI 5
#define MALLOCF 6
#define MALLOCC 7
#define MALLOCL 8
#define REALL 9
#define FREE 10
#define MOV 11
#define MOV_S 12
#define CALL 13
#define CALL_S 14
#define OUT 15
#define IN 16
#define LABEL 17
#define REF 18
#define REF_S 19
#define JNS 20
#define JNS_S 21
#define JMP 22
#define JMP_S 23
#define TERM 24
#define POP 25
#define OUT_S 26
#define IN_S 27
#define MAIN 28
#define REFI 29
#define REFF 30
#define REFC 31
#define REFL 32
#define RETURN 33
#define MOVI 34
#define MOVF 35
#define MOVC 36
#define MOVL 37
#define SWAP 38
#define SREF 39
#define LINK 40
#define ARITH 41
#define ADDF 42
#define ADDC 43
#define ADDL 44
#define IMPORT 45
#define LFUN 46
#define DONE 47
#define OCALL 48
#define OJMP 49
#define NS 50
#define OJEZ 51
#define LCALL 52

typedef int    Word;
typedef long   DWord;
typedef double Flt;
typedef char   Byte;

/*typedef struct Lit Lit;
struct Lit { union { Word i; DWord l; Flt f; Byte c; Byte *s; void *v; } x;
             unsigned int type; };*/
//typedef struct Arr { 

/* on modules: The interpreter reads in the label opcodes as well as an
               integer.  This integer represents the module that the
               label being defined refers to.  A new module is added during
               import (unfinished).  The assembler handles calls outside
               of its own self by quickly scanning the imported file's
               contents for the labels.  There are versions of jmp and call
               for calling out-of-main-file labels:

               ocall [int : module] [int : label]
               ocall_s [int : module]
               ojmp [int : module] [int : label]
               ojmp_s [int : module]
            
               This continues with jns and jez. */

#define INT 0
#define FLT 1
#define CHR 2
#define SYM 3
#define END 4
#define LNG 5

#define B sizeof(char)
#define I sizeof(int)
#define L sizeof(long)
#define F sizeof(double)

void read_prgm(FILE *, int);

//typedef struct Stk { void *x; struct Stk *prev; } Stk;
typedef struct { union { char c; int i; long l; double f;
                         char *ca; int *ia; long *la; double *fa; void *v; }; } Lit;
typedef struct { char op; Lit q; int m; } Expr;
typedef struct Stk { Lit x; struct Stk *prev; } Stk;
typedef Stk *FFun(Stk);
typedef struct Ref { long r; struct Ref *prev; } Ref;
typedef struct { long *l; unsigned int sz; } Modu;
typedef struct { Stk *(*f)(Stk *); char *nm; int sz; } Ffn;
// lib.fun
//typedef struct FFn { void *x; } FFn;
Modu *lbls; int lsz = 0;
// linked-list with literal pointers as labels?
Expr *exprs; long esz = 0; long mn = -1;
Ffn *ffn; int ffsz = 0;
Stk *stk; Ref *refs;
int md = 0;

void push_f(void *x, char *nm, int sz) { if(ffn) { 
    ffn = realloc(ffn,(ffsz+1)*sizeof(Ffn)); }
  else { ffn = malloc(sizeof(Ffn)); } ffn[ffsz++] = (Ffn) { x, nm, sz }; }
void push_lbl(long plc) { if(lbls[0].sz) { 
    lbls[0].l = realloc(lbls[0].l,(lbls[0].sz+1)*sizeof(long)); }
  else { lbls[0].l = malloc(sizeof(long)); }
  lbls[0].l[lbls[0].sz++] = plc; }
void push_lbl_gen(int m,long plc) { if(lbls[m].sz) {
    lbls[m].l = realloc(lbls[m].l,(lbls[m].sz+1)*sizeof(long)); }
  else { lbls[m].l = malloc(sizeof(long)); } lbls[m].l[lbls[m].sz++] = plc; }
void push_expr(char op, Lit q, int m) { exprs = realloc(exprs,(esz+1)*sizeof(Expr));
  exprs[esz++] = (Expr) { op, q, m }; }
// will be better made later.
void nstkptr(void) { if(stk) { Stk *q = malloc(sizeof(Stk));
  q->prev = malloc(sizeof(Stk)); q->prev = stk; stk = q; }
  else { stk = malloc(sizeof(Stk)); } }
void push_int(int i) { nstkptr(); stk->x.i = i; }
void push_flt(double f) { nstkptr(); stk->x.f = f; }
void push_chr(char c) { nstkptr(); stk->x.c = c; }
void push_lng(long l) { nstkptr(); stk->x.l = l; }
//void push_v(void *v) { nstkptr(); stk->x.v = v; }
void *getptr(Lit x) { if(x.ia) { return x.ia; } else if(x.fa) { return x.fa; }
  else if(x.ca) { return x.ca; } else if(x.la) { return x.la; }
  else { printf("not a pointer.\n"); exit(0); } }
void pop(void) { Stk *e; e = stk; stk = stk->prev; free(e); }
void out_s(int i, Lit q) { switch(i) { 
  case INT: printf("%i",q.i); break; case FLT: printf("%lg",q.f);
  case CHR: printf("%c",q.c); break; case LNG: printf("%li",q.l); } }
char *getstr(int i, FILE *f) { char *l = malloc((i+1)*sizeof(char));
  for(int z=0;z<i;z++) { l[z] = fgetc(f); } l[i] = '\0'; return l; }

void add(Stk *a,Stk *b,int t) { switch(t) { 
  case INT: b->x.i = a->x.i+b->x.i; break;
  case FLT: b->x.f = a->x.f+b->x.f; break; case CHR: b->x.c = a->x.c+b->x.c; break;
  case LNG: b->x.l = a->x.l+b->x.l; break; } pop(); }
void sub(Stk *a,Stk *b,int t) { switch(t) { 
  case INT: b->x.i = a->x.i-b->x.i; break;
  case FLT: b->x.f = a->x.f-b->x.f; break; case CHR: b->x.c = a->x.c-b->x.c; break;
  case LNG: b->x.l = a->x.l-b->x.l; break; } pop(); }
void mul(Stk *a,Stk *b,int t) { switch(t) { 
  case INT: b->x.i = a->x.i*b->x.i; break;
  case FLT: b->x.f = a->x.f*b->x.f; break; case CHR: b->x.c = a->x.c*b->x.c; break;
  case LNG: b->x.l = a->x.l*b->x.l; break; } pop(); }
void dv(Stk *a,Stk *b,int t) { switch(t) { 
  case INT: b->x.i = a->x.i/b->x.i; break;
  case FLT: b->x.f = a->x.f/b->x.f; break; case CHR: b->x.c = a->x.c/b->x.c; break;
  case LNG: b->x.l = a->x.l/b->x.l; break; } pop(); }
void pw(Stk *a,Stk *b,int t) { switch(t) { 
  case INT: b->x.i = pow(a->x.i,b->x.i); break;
  case FLT: b->x.f = pow(a->x.f,b->x.f); break; 
  case CHR: b->x.c = pow(a->x.c,b->x.c); break; 
  case LNG: b->x.l = pow(a->x.l,b->x.l); break; } pop(); }

void arith(int op, int typ) { pop(); pop(); switch(op) {
  case 0: add(stk,stk->prev,typ); break; case 1: sub(stk,stk->prev,typ); break;
  case 2: mul(stk,stk->prev,typ); break; case 3: dv(stk,stk->prev,typ); break;
  case 4: pw(stk,stk->prev,typ); break; } }

// void wrap_f(Ffn f) { switch(sz) { ... } }
void exec_ffun(char *nm) { for(int i=0;i<ffsz;i++) {
  if(!strcmp(nm,ffn[i].nm)) { ffn[i].f(stk); } } }
Stk *stkref(int e) { Stk *q; q = stk;
  for(int i=0;i<e;i++) { q = q->prev; } return q; }

/*void DESTROY(void) { if(!stk->prev) { free(stk); }
  else { printf("%i",*(int *)stk->x); } }*/
void DESTROY(Stk *x) {
 if(x) {
 if(x->prev) { DESTROY(x->prev); } 
  free(x); } }

int opcodes[] = { /*push*/INT,FLT,CHR,LNG,-1,/*malloc*/INT,INT,INT,INT,
                  /*realloc*/-1,/*free*/-1,
                  /*mov*/INT,-1,/*call*/INT,-1,/*out*/INT,/*in*/-1,/*label*/INT,
                  /*ref*/INT,-1,/*jns*/INT,-1,/*jmp*/INT,-1,/*terminate*/-1,
                  /*pop*/-1,/*out_s*/-1,/*in_s*/-1,/*main*/-1, /*refi*/-1,
                  /*reff*/-1,/*refc*/-1,/*refl*/-1,/*return*/-1,/*movi*/-1,
                  /*movf*/-1,/*swap*/-1,/*sref*/-1, /*link*/INT,/*addi*/-1,
                  /*addf*/-1,/*addc*/-1,/*addl*/-1,/*import*/INT,/*lfun*/INT,
                  /*done*/-1,/*ocall*/-1,/*ojmp*/-1,/*ojns*/-1,/*ojez*/-1,
                  /*lcall*/INT };

// pop for all necessary functions.
void parse(void) {
  for(long i=mn;i<esz;i++) { switch(exprs[i].op) { 
    case PW: push_int(exprs[i].q.i); break;
    case PF: push_flt(exprs[i].q.f); break;
    case PC: push_chr(exprs[i].q.c); break;
    case PL: push_lng(exprs[i].q.l); break;
    case MALLOCI: nstkptr(); stk->x.ia = malloc(exprs[i].q.i*I); break;
    case MALLOCF: nstkptr(); stk->x.fa = malloc(exprs[i].q.i*F); break;
    case MALLOCC: nstkptr(); stk->x.ca = malloc(exprs[i].q.i*B); break;
    case MALLOCL: nstkptr(); stk->x.la = malloc(exprs[i].q.i*L); break;
    case REALL: { void *x = getptr(stk->x); x = realloc(x,exprs[i].q.i); break; }
    case FREE: free(getptr(stk->x)); pop(); break;
    case OUT_S: out_s(stk->x.i,stk->prev->x); pop(); pop(); break;
    case JMP_S: { i=lbls[0].l[stk->x.i]-1; pop(); break; }
    case JMP: { i=lbls[0].l[exprs[i].q.i]-1; break; } // it's correct
    case POP: pop(); break;
    case IN: push_chr(getchar()); break;
    case REFI: { int q = (stk->x.ia)[stk->prev->x.i]; pop(); pop(); nstkptr();
                 stk->x.i = q; break; }
    case REFF: { double q = (stk->x.fa)[stk->prev->x.i]; pop(); pop(); nstkptr();
                 stk->x.f = q; break; }
    case REFC: { char q = (stk->x.ca)[stk->prev->x.i]; pop(); pop(); nstkptr();
                 stk->x.c = q; break; }
    case REFL: { long q = (stk->x.la)[stk->prev->x.i]; pop(); pop(); nstkptr();
                 stk->x.l = q; break; }
    case CALL_S: { Ref *r = malloc(sizeof(Ref)); r->r = i;
      if(refs) { r->prev = refs; } refs = r; i=lbls[0].l[stk->x.i]-1; pop(); break; }
    case CALL: { Ref *r = malloc(sizeof(Ref)); r->r = i;
      if(refs) { r->prev = refs; } refs = r; i=lbls[0].l[exprs[i].q.i]-1; break; }
    case RETURN: { Ref *r; r = refs; i = r->r; refs = refs->prev; free(r); break; }
    case MOVI: { (stk->x.ia)[stk->prev->x.i] = stk->prev->prev->x.i; 
                 pop(); pop(); pop(); break; }
    case MOVF: { (stk->x.fa)[stk->prev->x.i] = stk->prev->prev->x.f;
                 pop(); pop(); pop(); break; }
    case SWAP: { stk->prev->prev = stk; stk = stk->prev; break; }
    case SREF: { Stk *e = stkref(stk->x.i); pop(); nstkptr();
                 *(stk) = *(e); break; }
    case ARITH: { arith(stk->x.i,stk->prev->x.i); break; }
    case LINK: { nstkptr(); stk->x.v = dlopen(exprs[i].q.ca,RTLD_LAZY); break; }
    case LFUN: { void *z; z = dlsym(stk->prev->x.v,exprs[i].q.ca); 
      push_f(z,exprs[i].q.ca,stk->x.i); pop(); break; }
    case LCALL: { exec_ffun(exprs[i].q.ca); break; }
    case IMPORT: { char *in; in = malloc((strlen(exprs[i].q.ca)+4)*sizeof(char));
      strcpy(in,exprs[i].q.ca); strcat(in,".uo"); FILE *u; u = fopen(in,"rb");
      lbls = realloc(lbls,(++lsz)*sizeof(Modu)); lbls[lsz-1].sz = 0;
      read_prgm(u,md++); fclose(u); break; /* simply allocate the next lblarr */ }
    case OJMP: { // pop module, then word
      i = lbls[stk->x.i+exprs[i].m].l[stk->prev->x.i]-1; pop(); pop(); break; }
    case OCALL: { Ref *r; r = malloc(sizeof(Ref)); r->r = i;
      if(refs) { r->prev = refs; } refs = r; 
      i=lbls[stk->x.i+exprs[i].m].l[stk->prev->x.i]-1; pop(); pop(); break; }
    case NS: { if(!stk) { nstkptr(); stk->x.i = 1; } else { nstkptr();
      stk->x.i = 0; } break; }
    case OJEZ: { if(!stk->prev->prev->x.i) { 
      i = lbls[stk->x.i+exprs[i].m].l[stk->prev->x.i]-1; }
      pop(); pop(); pop(); break; }
    case TERM: exit(0); break;
    default: printf("what"); exit(0); } } }

void read_prgm(FILE *f, int m) { char op;
  while(((op = fgetc(f)) != TERM||mn<0)&&op!=DONE) { switch(op) {
    case LABEL: { int x; fread(&x,sizeof(int),1,f); push_lbl_gen(x+m,esz); break; }
    case LINK: { Lit l; int i; fread(&i,sizeof(int),1,f); l.ca = getstr(i,f);
                 push_expr(op,l,m); break; }
    case LFUN: { Lit l; int i; fread(&i,sizeof(int),1,f); l.ca = getstr(i,f);
                 push_expr(op,l,m); break; }
    case LCALL: { Lit l; int i; fread(&i,sizeof(int),1,f); l.ca = getstr(i,f);
                  push_expr(op,l,m); break; }
    case IMPORT: { Lit l; int i; fread(&i,sizeof(int),1,f); l.ca = getstr(i,f);
                   push_expr(op,l,m); break; }
    case MAIN: { mn = esz; break; }
    default: { Lit l; switch(opcodes[(int)op]) {
      case CHR: { char i; fread(&i,sizeof(char),1,f); l.c = i; break; }
      case INT: { int i; fread(&i,sizeof(int),1,f); l.i = i; break; }
      case FLT: { double i; fread(&i,sizeof(double),1,f); l.f = i; break; }
      case LNG: { long i; fread(&i,sizeof(long),1,f); l.l = i; break; } }
    push_expr(op,l,m); } } }
  Lit q; push_expr(TERM,q,m); }

int main(int argc, char **argv) { //stk = malloc(sizeof(Stk));
  exprs = malloc(sizeof(Expr)); char *in; 
  lbls = malloc(sizeof(Modu)); lbls[0].sz = lsz++;
  in = malloc((strlen(argv[1])+4)*sizeof(char)); strcpy(in,argv[1]);
  strcat(in,".uo"); FILE *f; f = fopen(in,"rb"); read_prgm(f,md++); fclose(f); parse();
  free(exprs); /*DESTROY(stk);*/ return 0; }

/*#define P 4
#define PW 0
#define PF 1
#define PC 2
#define PL 3
#define MALLOC 5
#define REALL 6
#define FREE 7
#define MOV 8
#define MOV_S 9
#define CALL 10
#define CALL_S 11
#define OUT 12
#define IN 13
#define LABEL 14
#define REF 15
#define REF_S 16
#define JNS 17
#define JNS_S 18
#define JMP 19
#define JMP_S 20
#define TERM 21
#define POP 22
#define OUT_S 23
#define IN_S 24
#define MAIN 25*/

