

all: iAim_x64

iAim_x64: main.c
	gcc -o $@ -g -lm -lSDL2 -lSDL2_image -lSDL2_mixer -liniparser $^