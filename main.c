#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define RAD_TO_DEG(x) ((x) * 180.0 / M_PI)
#define DEG_TO_RAD(x) ((x) * M_PI / 180.0)

typedef struct {
	int protectors[13];
} base_t;

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Texture *texPlayArea;
SDL_Texture *texLeftBase;
SDL_Texture *texRightBase;
SDL_Texture *texLeftBarricade[3];
SDL_Texture *texRightBarricade[3];

base_t leftBase = {
	{ 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0 }
};

base_t rightBase = {
	{ 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0 }
};


void load_resources();

void menu();

void start_round();

int main(int argc, char **argv)
{
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);
	
	window = SDL_CreateWindow(
		"iAIM",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1280, 720,
		SDL_WINDOW_SHOWN);
	if(window == NULL) {
		fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
		exit(1);
	}
	
	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(window == NULL) {
		fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
		exit(1);
	}
	
	load_resources();
	
	menu();
	
	return 0;
}


void menu()
{
	start_round();
}


void render_battleground()
{
	SDL_Rect battleground = {
		128, 0,
		1280 - 256, 720
	};
	
	SDL_RenderSetClipRect(renderer, &battleground);
	
	SDL_Rect leftBaseRect = {
		battleground.x,
		(battleground.h - 252) / 2,
		126,
		252,
	};
	
	SDL_Rect rightBaseRect = {
		battleground.x + battleground.w - 126,
		(battleground.h - 252) / 2,
		126,
		252,
	};

	SDL_RenderCopy(
		renderer,
		texPlayArea,
		NULL,
		&battleground);
		
	SDL_RenderCopy(
		renderer,
		texLeftBase,
		NULL,
		&leftBaseRect);
	SDL_RenderCopy(
		renderer,
		texRightBase,
		NULL,
		&rightBaseRect);
	
	// draw base protectors.
	for(int i = 0; i < 13; i++) {
	
		int baseRadius = 136;
	
		// left base
		if(leftBase.protectors[i] > 0) {
			SDL_Rect target = {
				battleground.x + baseRadius * sinf(DEG_TO_RAD(15 * i)) - 6,
				battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(15 * i)) - 15,
				12,
				30,
			};
			SDL_RenderCopyEx(
				renderer,
				texLeftBarricade[3 - leftBase.protectors[i]],
				NULL,
				&target,
				-15 * i - 90,
				NULL,
				SDL_FLIP_NONE);
		}
		
		if(rightBase.protectors[i] > 0) {
			SDL_Rect target = {
				battleground.x + battleground.w - baseRadius * sinf(DEG_TO_RAD(15 * i)) - 6,
				battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(15 * i)) - 15,
				12,
				30,
			};
			SDL_RenderCopyEx(
				renderer,
				texRightBarricade[3 - rightBase.protectors[i]],
				NULL,
				&target,
				15 * i - 90,
				NULL,
				SDL_FLIP_NONE);
		}
	}
	
	SDL_RenderSetClipRect(renderer, NULL);
}

void start_round()
{
	SDL_Event e;
	while(true)
	{
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) return;
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return;
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderClear(renderer);
		
		render_battleground();
		
		SDL_RenderPresent(renderer);
	}
}













































/**
 * Loads all resources used by the game.
 **/
void load_resources()
{
#define LOAD(tex, img) if((tex = IMG_LoadTexture(renderer, img)) == NULL) \
	{ \
		fprintf(stderr, "Failed to load " img ": %s\n", IMG_GetError()); \
		exit(1); \
	}
	LOAD(texPlayArea, "tex/play-area.png");
	LOAD(texLeftBase, "tex/left-base.png");
	LOAD(texRightBase, "tex/right-base.png");
	LOAD(texLeftBarricade[0], "tex/left-barricade-0.png");
	LOAD(texLeftBarricade[1], "tex/left-barricade-1.png");
	LOAD(texLeftBarricade[2], "tex/left-barricade-2.png");
	LOAD(texRightBarricade[0], "tex/right-barricade-0.png");
	LOAD(texRightBarricade[1], "tex/right-barricade-1.png");
	LOAD(texRightBarricade[2], "tex/right-barricade-2.png");
#undef LOAD
}