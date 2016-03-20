#include <stdlib.h>
#include <stdio.h>
#include <OpenGL/GL.h>
#include <GLFW/glfw3.h>
#include "ulib.h"

void uglVertex3f(Stk *x) { 
  glVertex3f(popf(x),popf(x),popf(x)); }
void init_gl(Stk *x) { if(!glfwInit()) { exit(EXIT_FAILURE); } }

