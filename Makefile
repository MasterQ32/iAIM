

all: aim

aim: main.c
	gcc -o $@ -g -lm -lSDL2 -lSDL2_image -lSDL2_mixer $^