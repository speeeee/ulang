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
#define END 1000
#define NIL 1001

typedef struct { union { int64_t i; double f; } x; unsigned int type; } Lit;
typedef struct Elem { Lit x; struct Elem *next; } Elem;
typedef struct Stk { Lit x; struct Stk *prev; } Stk;

Stk *stk; Elem *sth; Elem *sto;
int *lbls; int lsz;

void astk(Lit x) { if(stk->x.type==NIL) { stk->x = x; }
  else { Stk *e = malloc(sizeof(Stk)); e->prev = stk; stk = e; } }
void asto(Lit x) { if(sto->x.type==NIL) { sto->x = x; }
  else { Elem *e = malloc(sizeof(Elem)); e->next = NULL; e->x = x; sto->next = e;
         sto = e; } }
void albl(int64_t x) { if(lsz!=0) { 
  lbls = realloc(lbls,(lsz+1)*sizeof(int64_t)); } lbls[lsz++] = x; }

Lit tok(FILE *in) { Lit l; int c = fgetc(in); printf("%i\n",c); switch(c) {
  case INT: fread(&l.x.i,sizeof(int64_t),1,in); l.type = INT; break;
  case FLT: fread(&l.x.f,sizeof(double),1,in); l.type = FLT; break;
  case LBL: fread(&l.x.i,sizeof(int64_t),1,in); l.type = LBL; break;
  case EOF: l.x.i = 0; l.type = END; } return l; }
void lexer(FILE *in, int lim) { Lit l; for(int i=0;(l=tok(in)).type!=END&&i<lim;i++) {
  if(l.type==LBL) { albl(l.x.i); } else { asto(x); } } }

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

void paint(GLFWwindow *win, int mode) { 
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glLoadIdentity();
  glClearColor(0.4,0.4,0.55,1.0);
  //glTranslatef(-c.x/10,-c.y/10,0); //printf("%f, %f, %f\n",c.z,c.x,c.y); 
  /*glBegin(GL_TRIANGLES); glVertex3f(0,0,0); glVertex3f(1,0,0);
  glVertex3f(1,1,0); glVertex3f(0,1,0); glEnd();*/ }

int pressed(GLFWwindow *win,int x) { return glfwGetKey(win,x)!=GLFW_RELEASE; }
int mpressed(GLFWwindow *win, int x) { return glfwGetMouseButton(win,x); }
// making drawing tool
int bhover(GLFWwindow *win, Rect b) { double x, y;
  glfwGetCursorPos(win,&x,&y); return x>b.x&&x<b.x+b.w&&y>b.y&&y<b.y+b.h; }
int bclick(GLFWwindow *win, Rect b) { 
  return mpressed(win,GLFW_MOUSE_BUTTON_LEFT)&&bhover(win,b); }

/*int parse_input(GLFWwindow *win, int mode) { Rect ng = { 30, 30, 100, 30 };
  switch(mode) { case MTITLE:
    if(bclick(win,ng)) { return MSETUP; } break;
  case MSIM: break;
  default: return mode; } }*/
  
int main(void) {
  FILE *init; init = fopen("root.uo","rb");
  //FILE *mem; mem = fopen("mem.uo,"rb");
  lbls = malloc(sizeof(int64_t)); lsz = 0;
  stk = malloc(sizeof(Stk)); stk->x.type = NIL; stk->prev = NULL;
  sth = sto = malloc(sizeof(Elem)); sto->x.type = NIL; sto->next = NULL; lexer(init); 

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
  while (!glfwWindowShouldClose(window)) { paint(window,mode);
    glfwSwapBuffers(window); glfwPollEvents(); }
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS); }
