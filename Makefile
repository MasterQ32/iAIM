

all: aim

aim: main.c
	gcc -o $@ -lm -lSDL2 -lSDL2_image $^