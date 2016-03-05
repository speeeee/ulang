/* bytecode
   op-codes: 00 - push 
             -- - pushw (4 bytes, ints, pointers)
             -- - pushd (8 bytes, doubles, long ints)
             01 - malloc (pushes address to stack) (4 bytes)
             02 - realloc (4 bytes) [4-byte address from stack]
             03 - free   (frees address at top of stack) (4 bytes)
             04 - mov (4 bytes) [address from stack]
             05 - mov_s [stack-only]
             06 - call (4 bytes)
             07 - out (4 bytes)
             08 - in (returns char)
             09 - label
             0A - ref (references stack by displacement) (4 bytes)
             0B - jns (jump if null stack) (LABEL or 4 bytes)
             0C - jmp (LABEL or 4 bytes)
             0D - terminate
             0E - pop
             0F - out_s
             10 - in_s

   constants: TSTK - always points to the top of the stack. Note that referencing
                     this does not pop.

   sample:
   label end
   terminate
   label main        ; labels are unnecessary, but good for gotos. 
   pushw 32
   pushw 33
   pushw 34
   label loop
   jns end
   out_s
   pop
   jmp loop 
  
   or: 0B 00000000 0F 00000000 0B 00000001 01 00000020 01 00000021 01 00000022
       0B 00000002 0D 00000000 09 00000000 10 00000000 0E 00000023
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INT 0
#define FLT 1
#define CHR 2
#define SYM 3
#define END 4
#define LNG 5
#define ADR 6

#define B sizeof(char)
#define I sizeof(int)
#define L sizeof(long)
#define F sizeof(double)
#define II -2

typedef struct { char *name; int argsz; } OpC;
OpC opcodes[] = { { "push", -1 /* varies */ }, { "pushw", -1 }, { "pushf", -1 },
                  { "pushc", -1 }, { "pushl", -1 }, { "malloci", I }, 
                  { "mallocf", I }, { "mallocc", I }, { "mallocl", I }, 
                  { "realloc", 0 }, { "free", 0 }, { "mov", I },
                  { "mov_s", 0 }, { "call", I }, { "call_s", 0 },
                  { "out", I }, { "in", 0 }, { "label", -1 /* varies */ },
                  { "ref", I }, { "ref_s", 0 }, { "jns", I }, { "jns_s", 0 },
                  { "jmp", I }, { "jmp_s", 0 },
                  { "terminate", 0 }, { "pop", 0 }, { "out_s", 0 },
                  { "in_s", 0 }, { "main", 0 }, { "refi", 0 }, { "reff", 0 },
                  { "refc", 0 }, { "refl", 0 }, { "return", 0 }, { "movi", 0 },
                  { "movf", 0 }, { "movc", 0 }, { "movl", 0 },
                  { "swap", 0 }, { "sref", 0 }, { "link", -1 /* varies */ },
                  { "addi", 0 }, { "addf", 0 }, { "addc", 0 }, { "addl", 0 },
                  { "import", -1 /* varies */ }, { "lfun", -1 /* varies */ },
                  { "done", 0 }, { "ocall", 0 }, { "ojmp", 0 },
                  { "ojns", 0 }, { "ojez", 0 } };
typedef struct { char **s; int sz; } Ls;
int osz = 52; int md = 0;
// defined labels
Ls *ls; int lsz = 0;

void addLabel(char *x) { ls[0].s = realloc(ls[0].s,(ls[0].sz+1)*sizeof(char *)); 
  ls[0].s[ls[0].sz] = malloc(sizeof(char)*strlen(x));
  ls[0].s[ls[0].sz++] = x; }
void addLabelGen(int m,char *x) { if(ls[m].sz) { 
    ls[m].s = realloc(ls[m].s,(ls[m].sz+1)*sizeof(char *)); }
  else { ls[m].s = malloc((ls[m].sz+1)*sizeof(char *)); } 
  ls[m].s[ls[m].sz] = malloc(sizeof(char)*strlen(x));
  ls[m].s[ls[m].sz++] = x; }

typedef int    Word;
typedef long   DWord;
typedef double Flt;
typedef char   Byte;

typedef struct Lit Lit;
typedef struct Addr { int m; int x; } Addr;
struct Lit { union { Word i; DWord l; Flt f; Byte c; Byte *s; Addr a; } x;
             unsigned int type; };

Lit lexd(FILE *, int);

Lit liti(long i) { Lit l; l.x.i = i; l.type = INT; return l; }
Lit litsy(char *x) { Lit l; l.x.s = x; l.type = SYM; return l; }

void write_c(char c, FILE *f) { fwrite(&c,1,1,f); }
int opcode(char *x) { int i; for(i=0;i<osz;i++) {
  if(!strcmp(x,opcodes[i].name)) { return i; } } return -1; }
void n_out(Lit x, int n, FILE *o) { void *q; switch(x.type) {
  case INT: q = &x.x.i; break; case FLT: q = &x.x.f; case CHR: q = &x.x.c;
  case LNG: q = &x.x.l; default: exit(0); }
  fwrite(q,n,1,o); }  
void nmodule(char *lb) { char *q; q = malloc((strlen(lb)+4)*sizeof(char));
  strcpy(q,lb); strcat(q,".usm"); FILE *e; e = fopen(q,"r");
  ls = realloc(ls,(lsz+1)*sizeof(Ls)); Lit l;
  while((l = lexd(e,EOF)).type != END) {
    if(l.type == SYM) { if(!strcmp(l.x.s,"label")) { l = lexd(e,EOF);
      if(l.type == SYM) { addLabelGen(lsz,l.x.s); }
      else { printf("error 1\n"); } } } }
  fclose(e); lsz++; }

char *tok(FILE *s,int c) {
  int sz = 0; int lsz = 10; char *str = malloc(lsz*sizeof(char));
  while(!isspace(c)&&c!='('&&c!=')'&&c!=EOF) { 
    if(sz==lsz) { str = realloc(str,(lsz+=10)*sizeof(char)); }
    str[sz++] = c; c = fgetc(s); }
    if(c!=EOF) { fseek(s,-1,SEEK_CUR); } str[sz] = '\0'; return str; }
Lit tokl(FILE *s,int c) { 
  int sz = 0; int lsz = 10; char *str = malloc(lsz*sizeof(char));
  while(isdigit(c)&&c!='w'&&c!='d'&&c!='f'&&c!='b') {
    if(sz==lsz) { str = realloc(str,(lsz+=10)*sizeof(char)); }
    str[sz++] = c; c = fgetc(s); } str[sz] = '\0'; Lit e;
    switch(c) { case 'w': e.x.i = atoi(str); e.type = INT; break;
                case 'd': e.x.l = atol(str); e.type = LNG; break;
                case 'f': e.x.f = atof(str); e.type = FLT; break;
                case 'b': e.x.c = (char) atoi(str); e.type = CHR; break;
                default: fseek(s,-1,SEEK_CUR); e.x.i = atoi(str); e.type = INT; }
    return e; }
Lit lexd(FILE *s, int eofchar) { int c;
  while(/*isspace(c = fgetc(s)));*/(c = fgetc(s))==' '||c=='\t'||c=='\n');
  if(isdigit(c)) { //Lit q; fscanf(s,"%li",&q.x.i); q.type = INT; return q; }
                   return tokl(s,c); }
  if(c==eofchar||c==EOF) { Lit e; e.x.i = EOF; e.type = END; return e; }
  else { char *x = tok(s,c); for(int q=0;q<lsz;q++) {
    for(int i=0;i<ls[q].sz;i++) { if(!strcmp(x,ls[q].s[i])) { Lit l; l.x.a.x = i;
      l.x.a.m = q; l.type = ADR; return l; } } }
    return litsy(x); } }
// ** make better search here ** //
Lit lex(FILE *s) { return lexd(s,EOF); }

// bad code
void parse(FILE *o, FILE *i, int eo, int m) { Lit l;
  while((l = lexd(i,eo)).type != END) {
  if(l.type != SYM) { printf("ERROR: must start with op-code.\n"); }
  else { if(!strcmp(l.x.s,"push")) { Lit l = lexd(i,eo);
           switch(l.type) { case INT: write_c(0,o); 
                              fwrite(&l.x.i,sizeof(int),1,o); break;
                            case FLT: write_c(1,o); 
                              fwrite(&l.x.f,sizeof(double),1,o); break;
                            case CHR: write_c(2,o);
                              fwrite(&l.x.c,sizeof(char),1,o); break;
                            case LNG: write_c(3,o);
                              fwrite(&l.x.l,sizeof(long),1,o); break; 
                            case ADR: write_c(0,o);
                              fwrite(&l.x.a.x,sizeof(int),1,o); write_c(0,o);
                              fwrite(&l.x.a.m,sizeof(int),1,o); break;
                            default: printf("error push:%i\n",l.type); exit(0); } }
         else if(!strcmp(l.x.s,"label")) { write_c(17,o); l = lexd(i,eo);
           if(l.type == SYM) {
             fwrite(&m,sizeof(int),1,o); addLabelGen(m,l.x.s); }
           else { printf("error\n"); exit(0); } }
         else if(!strcmp(l.x.s,"link")) { write_c(40,o); l = lexd(i,eo);
           if(l.type == SYM) { int x = strlen(l.x.s); 
             fwrite(&x,sizeof(int),1,o); fwrite(&l.x.s,sizeof(char),x,o); }
           else { printf("error\n"); exit(0); } }
         else if(!strcmp(l.x.s,"lfun")) { write_c(46,o); l = lexd(i,eo);
           if(l.type == SYM) { int x = strlen(l.x.s);
             fwrite(&x,sizeof(int),1,o); fwrite(&l.x.s,sizeof(char),x,o); }
           else { printf("error\n"); exit(0); } }
         else if(!strcmp(l.x.s,"import")) { write_c(45,o); l = lexd(i,eo);
           if(l.type == SYM) { int x = strlen(l.x.s); nmodule(l.x.s);
             fwrite(&x,sizeof(int),1,o); fwrite(l.x.s,sizeof(char),x,o); }
           else { printf("error\n"); exit(0); } }
         else if(!strcmp(l.x.s,":q")) { exit(0); }
         else if(opcode(l.x.s)!=-1) { int ii = opcode(l.x.s); write_c(ii,o); 
           if(opcodes[ii].argsz) {
             Lit l = lexd(i,eo); n_out(l,opcodes[ii].argsz,o); } } } } }

int main(int argc, char **argv) { ls = malloc(sizeof(Ls));
  ls[lsz++].s = malloc(sizeof(char *)); FILE *g;
  FILE *f; //f = fopen("sample.uo","wb"); g = fopen("sample.usm","r");
  char *in; in = malloc((strlen(argv[2])+4)*sizeof(char)); strcpy(in,argv[2]);
  strcat(in,".usm"); char *out; out = malloc((strlen(argv[1])+5)*sizeof(char));
  strcpy(out,argv[1]); strcat(out,".uo"); f = fopen(out,"wb"); g = fopen(in,"r");
  parse(f,g,EOF,0); fclose(f); fclose(g); return 0; }
