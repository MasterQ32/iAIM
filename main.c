#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define RAD_TO_DEG(x) ((x) * 180.0 / M_PI)
#define DEG_TO_RAD(x) ((x) * M_PI / 180.0)

typedef struct {
	SDL_Color color;
	int protectors[13];
} base_t;

typedef struct particle {
	base_t const * base;
	int x, y;
	float rotation;
	int progress;
	struct particle * next;
} particle_t;

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Texture *texPlayArea;
SDL_Texture *texBase;
SDL_Texture *texParticle;
SDL_Texture *texBarricade[3];

base_t leftBase = {
	{ 92, 75, 255, 255 },
	{ 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0 }
};

base_t rightBase = {
	{ 85, 182, 74, 255 },
	{ 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0 }
};

particle_t *particles = NULL;

void load_resources();

void menu();

void start_round();

void spawn_particle(base_t const * base, int x, int y, float rot);

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

void setTextureColor(base_t const * b, SDL_Texture *tex)
{
	SDL_SetTextureColorMod(tex, b->color.r, b->color.g, b->color.b);
}

void render_battleground()
{
	SDL_Rect battleground = {
		128, 0,
		1280 - 256, 720
	};
	SDL_RenderSetClipRect(renderer, &battleground);

	SDL_RenderCopy(
		renderer,
		texPlayArea,
		NULL,
		&battleground);
	
	
	{ // Draw base background
		SDL_Rect leftBaseRect = {
			battleground.x,
			(battleground.h - 256) / 2,
			128,
			256,
		};
		
		SDL_Rect rightBaseRect = {
			battleground.x + battleground.w - 128,
			(battleground.h - 256) / 2,
			128,
			256,
		};
		
		SDL_Rect sourceRect = {
			0, 0,
			128, 256
		};
		
		setTextureColor(&leftBase, texBase);
		sourceRect.x = 128;
		SDL_RenderCopy(
			renderer,
			texBase,
			&sourceRect,
			&leftBaseRect);
			
		setTextureColor(&rightBase, texBase);
		sourceRect.x = 0;
		SDL_RenderCopy(
			renderer,
			texBase,
			&sourceRect,
			&rightBaseRect);
	}
	
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
			SDL_Texture *tex = texBarricade[3 - leftBase.protectors[i]];
			setTextureColor(&leftBase, tex);
			SDL_RenderCopyEx(
				renderer,
				texBarricade[3 - leftBase.protectors[i]],
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
			SDL_Texture *tex = texBarricade[3 - rightBase.protectors[i]];
			setTextureColor(&rightBase, tex);
			SDL_RenderCopyEx(
				renderer,
				tex,
				NULL,
				&target,
				15 * i - 90,
				NULL,
				SDL_FLIP_NONE);
		}
	}
	
	{ // Draw particles
		for(particle_t * p = particles; p != NULL; p = p->next)
		{
			if(p->progress >= 200) {
				continue;
			}
			setTextureColor(p->base, texParticle);
			
			SDL_Rect target = {
				battleground.x + p->x, p->y - 5,
				1, 11
			};
			SDL_Rect source = {
				p->progress, 0,
				1, 11
			};
			
			SDL_RenderCopyEx(
				renderer,
				texParticle,
				&source,
				&target,
				p->rotation,
				NULL,
				SDL_FLIP_NONE);
		
		}
	}
	
	SDL_RenderSetClipRect(renderer, NULL);
}

void start_round()
{
	SDL_Event e;
	uint32_t nextFrameTime = 0;
	
	int x = 100;
	while(true)
	{
		// first, tick all particles
		for(particle_t * p = particles, *prev = NULL; p != NULL; )
		{
			p->progress += 3;
			if(p->progress >= 200) {
				// remove the particle here:
				if(prev != NULL) {
					prev->next = p->next;
				}
				if(p == particles) {
					particles = p->next;
				}
				{
					particle_t *k = p;
					p = p->next;
					free(k);
				}
			} else {
				prev = p;
				p = p->next;
			}
		}
	
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) return;
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return;
		}
		
		float dt = 1.0 / 6.0;

		uint8_t *kbd = SDL_GetKeyboardState(NULL);
		if(kbd[SDL_SCANCODE_SPACE]) {
			spawn_particle(&leftBase, x + 0, 100, 0.0);
			spawn_particle(&leftBase, x + 1, 100, 0.0);
			spawn_particle(&leftBase, x + 2, 100, 0.0);
			spawn_particle(&leftBase, x + 3, 100, 0.0);
			x += 4;
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderClear(renderer);
		
		render_battleground();
		
		SDL_RenderPresent(renderer);
		
		while(SDL_GetTicks() < nextFrameTime) {
			; // BURN!
		}
		nextFrameTime = SDL_GetTicks() + 15;
	}
}


















/**
 * Spawns a particle in the particle queue.
 */
void spawn_particle(base_t const * base, int x, int y, float rot)
{
	particle_t *p = malloc(sizeof(particle_t));
	p->base = base;
	p->x = x;
	p->y = y;
	p->rotation = rot;
	p->progress = rand() % 3; // 3 different particle frames
	p->next = particles;
	
	// Prepend
	particles = p;
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
	LOAD(texBase, "tex/base.png");
	LOAD(texParticle, "tex/particles.png");
	LOAD(texBarricade[0], "tex/barricade-0.png");
	LOAD(texBarricade[1], "tex/barricade-1.png");
	LOAD(texBarricade[2], "tex/barricade-2.png");
#undef LOAD
}