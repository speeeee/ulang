all:
	gcc -g -o uous uous.c -framework OpenGL -lglfw3 -framework Cocoa -framework CoreVideo -framework IOKit