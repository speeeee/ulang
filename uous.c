// super OS world
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <OpenGL/GL.h>
#include <GLFW/glfw3.h>

#define INT 0
#define FLT 1
#define LBL 2
#define ADR 3
#define SYS 4
#define END 1000
#define NIL 1001

#define CAL 0
#define RET 1
#define JMP 2
#define ADD 3
#define SUB 4
#define MUL 5
#define DIV 6
#define MEM 7
#define AND 8
#define OR  9
#define XOR 10
#define SHL 11
#define SHR 12
#define QUIT 13
#define DUMP 14 // UNTESTED from here on.
#define LIFT 15
#define DROP 16
#define JNZ  17
#define JEZ  18
#define COL  19

// sth: head
// stc: current position
// sto: current position (lexer; always at end of list when in 'parse')

typedef struct { union { int64_t i; double f; char x; } x; unsigned int type; } Lit;
typedef struct Elem { Lit x; struct Elem *next; } Elem;
typedef struct Stk { Lit x; struct Stk *prev; } Stk;
typedef struct Ref { Elem *x; struct Ref *prev; } Ref;

Stk *stk; Elem *sth; Elem *sto; Elem *stc;
Elem **lbls; int lsz;

Ref *refs;

void pop(void) { Stk *e = stk; stk = stk->prev; free(e); 
  if(!stk) { stk = malloc(sizeof(Stk)); stk->prev = NULL; stk->x.type = NIL; } }
void astk(Lit x) { if(stk->x.type==NIL) { stk->x = x; }
  else { Stk *e = malloc(sizeof(Stk)); e->prev = stk; stk = e; stk->x = x; } }
void asto(Lit x) { if(sto->x.type==NIL) { sto->x = x; }
  else { Elem *e = malloc(sizeof(Elem)); e->next = NULL; e->x = x; sto->next = e;
         sto = e; } }
void albl(Elem *x) { if(lsz!=0) { 
  lbls = realloc(lbls,(lsz+1)*sizeof(Elem *)); } lbls[lsz++] = x; }
void aref(Elem *x) { if(refs->x) { Ref *r = malloc(sizeof(Ref)); r->prev = refs;
  refs = r; } refs->x = x; }
void popr(void) { Ref *r = refs; refs = refs->prev; free(r);
  if(!refs) { refs = malloc(sizeof(Ref)); refs->prev = NULL; refs->x = NULL; } }

void out(Stk *e, FILE *d) { int8_t *c = malloc(sizeof(int8_t)); switch(e->x.type) {
  case INT: { *(c) = 0; fwrite(c,1,1,d); fwrite(&(e->x.x.i),8,1,d); break; }
  case FLT: { *(c) = 1; fwrite(c,1,1,d); fwrite(&(e->x.x.f),8,1,d); break; } } }

void lexer(FILE *, int);

void prim(char x, Elem **st, FILE *m, FILE *d) { switch(x) {
  case 3: case 4: case 5: case 6: { 
    if(stk->x.type==INT) { switch(x) {
      case 3: { stk->prev->x.x.i = stk->x.x.i+stk->prev->x.x.i; break; }
      case 4: { stk->prev->x.x.i = stk->x.x.i-stk->prev->x.x.i; break; }
      case 5: { stk->prev->x.x.i = stk->x.x.i*stk->prev->x.x.i; break; }
      case 6: { stk->prev->x.x.i = stk->x.x.i/stk->prev->x.x.i; } } }
    else { switch(x) { 
             case 3: { stk->prev->x.x.f = stk->x.x.f+stk->prev->x.x.f; break; }
             case 4: { stk->prev->x.x.f = stk->x.x.f-stk->prev->x.x.f; break; }
             case 5: { stk->prev->x.x.f = stk->x.x.f*stk->prev->x.x.f; break; }
  	     case 6: { stk->prev->x.x.f = stk->x.x.f/stk->prev->x.x.f; } } } pop(); 
    break; }
  case 8: { stk->prev->x.x.i = stk->prev->x.x.i&stk->x.x.i; pop(); break; }
  case 9: { stk->prev->x.x.i = stk->prev->x.x.i|stk->x.x.i; pop(); break; }
  case 10: { stk->prev->x.x.i = stk->prev->x.x.i^stk->x.x.i; pop(); break; }
  case 11: { stk->prev->x.x.i = stk->prev->x.x.i<<stk->x.x.i; pop(); break; }
  case 12: { stk->prev->x.x.i = stk->prev->x.x.i>>stk->x.x.i; pop(); break; }
  case 2: *(st) = lbls[stk->x.x.i]; pop(); break;
  case 0: aref(*st); *(st) = lbls[stk->x.x.i]; pop(); break; 
  case 1: *(st) = refs->x; popr(); break;
  case 7: fseek(m,stk->prev->x.x.i,SEEK_SET); lexer(m,stk->x.x.i); 
          pop(); pop(); break;
  case 14: { Stk *e = stk; while(e) { out(e,d); Stk *f = e->prev; free(e); e = f; }
    stk = e; stk = malloc(sizeof(Stk)); stk->x.type = NIL; break; }
  case 15: { int64_t x = stk->x.x.i; pop(); Stk *e = stk;
    for(int i=0;i<x-1;i++) { e = e->prev; } Stk *f = e; e->prev = e->prev->prev;
    f->prev = e; break; } case 16: pop(); break; 
  case 17: { int64_t q = stk->x.x.i; pop();
    if(q) { *(st) = lbls[stk->x.x.i]; } pop(); break; }
  case 18: { int64_t q = stk->x.x.i; pop(); 
    if(!q) { *(st) = lbls[stk->x.x.i]; } pop(); break; }
  case 19: { double r,g,b; b = stk->x.x.f; pop(); g = stk->x.x.f; pop();
    r = stk->x.x.f; pop(); glColor3f(r,g,b); break; }
  case 13: printf("%lli\n",stk->x.x.i); exit(EXIT_SUCCESS); } }

Lit tok(FILE *in) { Lit l; int c = fgetc(in); /*printf("%i\n",c);*/ switch(c) {
  case INT: fread(&l.x.i,sizeof(int64_t),1,in); l.type = INT; break;
  case FLT: fread(&l.x.f,sizeof(double),1,in); l.type = FLT; break;
  case LBL: /*fread(&l.x.i,sizeof(int64_t),1,in);*/ l.type = LBL; break;
  case ADR: fread(&l.x.i,sizeof(int64_t),1,in); l.type = ADR; break;
  case SYS: fread(&l.x.x,sizeof(char),1,in); l.type = SYS; break;
  case EOF: l.x.i = 0; l.type = END; } return l; }
void lexer(FILE *in, int lim) { Lit l; 
  for(int i=0;(l=tok(in)).type!=END&&(i<lim||lim==-1);i++) {
    if(l.type==LBL) { albl(sto); } else { asto(l); } } }
void parse(Elem *st, FILE *m, FILE *d, GLFWwindow *w) { 
  while(st&&st->x.type!=NIL&&!glfwWindowShouldClose(w)) {
    if(st->x.type==SYS) { prim(st->x.x.x,&st,m,d); } else { astk(st->x); }
    st = st->next; glfwSwapBuffers(w); glfwPollEvents(); } }

void error_callback(int error, const char* description) {
    fputs(description, stderr); }
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE); }
void rsz(GLFWwindow *win, int w, int h) {
  glViewport(0,0,w,h); float ratio = w/ (float) h;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 2*ratio, 0, 2.f, 2.f, 0);
  glMatrixMode(GL_MODELVIEW); }

void setup(GLFWwindow *win) {
  float ratio;
  int width, height;
  glfwGetFramebufferSize(win, &width, &height);
  ratio = width / (float) height;
  glViewport(0, 0, width, height);
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 2*ratio, 0, 2.f, 2.f, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity(); }

void paint(GLFWwindow *win) { 
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glLoadIdentity();
  glClearColor(0.4,0.4,0.55,1.0);
  //glTranslatef(-c.x/10,-c.y/10,0); //printf("%f, %f, %f\n",c.z,c.x,c.y); 
  /*glBegin(GL_TRIANGLES); glVertex3f(0,0,0); glVertex3f(1,0,0);
  glVertex3f(1,1,0); glVertex3f(0,1,0); glEnd();*/ }

int pressed(GLFWwindow *win,int x) { return glfwGetKey(win,x)!=GLFW_RELEASE; }
int mpressed(GLFWwindow *win, int x) { return glfwGetMouseButton(win,x); }

/*int parse_input(GLFWwindow *win, int mode) { Rect ng = { 30, 30, 100, 30 };
  switch(mode) { case MTITLE:
    if(bclick(win,ng)) { return MSETUP; } break;
  case MSIM: break;
  default: return mode; } }*/
  
int main(void) {
  GLFWwindow* window; 
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) exit(EXIT_FAILURE);
  window = glfwCreateWindow(800, 800, "uous", NULL, NULL);
  if (!window) {
      glfwTerminate();
      exit(EXIT_FAILURE); }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glfwSetKeyCallback(window, key_callback); setup(window);
  glfwSetFramebufferSizeCallback(window, rsz);

  FILE *init; init = fopen("root.uo","rb");
  FILE *mem; mem = fopen("mem.uo","rb");
  FILE *dump; dump = fopen("dump.uo","wb");
  lbls = malloc(sizeof(int64_t)); lsz = 0;
  refs = malloc(sizeof(Ref)); refs->x = NULL; refs->prev = NULL;
  stk = malloc(sizeof(Stk)); stk->x.type = NIL; stk->prev = NULL;
  sth = sto = malloc(sizeof(Elem)); sto->x.type = NIL; sto->next = NULL;
  lexer(init,-1); fclose(init); stc = sth; parse(stc,mem,dump,window);
  printf("%lli\n",stk->x.x.i);
  fclose(mem); fclose(dump); dump = fopen("dump.uo","rb");
  mem = fopen("mem.uo","ab"); int c;
  while((c=fgetc(dump))!=EOF) { fwrite(&c,1,1,mem); } fclose(mem); fclose(dump);
  //while (!glfwWindowShouldClose(window)) { paint(window);
  //  glfwSwapBuffers(window); glfwPollEvents(); }
  glfwDestroyWindow(window); // these lines will be up to the interptreter later.
  glfwTerminate();
  exit(EXIT_SUCCESS); return 0; }
