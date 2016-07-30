#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define RAD_TO_DEG(x) ((x) * 180.0 / M_PI)
#define DEG_TO_RAD(x) ((x) * M_PI / 180.0)

typedef struct {
	float x, y;
} float2;

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

typedef struct projectile {
	base_t const * base;
	bool active;
	float2 pos;
	float2 vel;
	struct projectile * next;
} projectile_t;

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Texture *texPlayArea;
SDL_Texture *texBase;
SDL_Texture *texProjectile;
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
projectile_t *projectiles = NULL;

SDL_Rect battleground = {
	128, 0,
	1280 - 256, 720
};
	
void load_resources();

void menu();

void start_round();

void spawn_particle(base_t const * base, int x, int y, float rot);

void fire_projectile(base_t const * base, float2 pos, float2 vel);

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
	
	{ // Draw projectiles
		for(projectile_t * p = projectiles; p != NULL; p = p->next)
		{
			if(p->active == false) {
				continue;
			}
			setTextureColor(p->base, texProjectile);
			
			SDL_Rect target = {
				battleground.x + p->pos.x - 5, p->pos.y - 5,
				11, 11
			};
			
			float rot = 90 - RAD_TO_DEG(atan2(p->vel.x, p->vel.y));
			
			SDL_RenderCopyEx(
				renderer,
				texProjectile,
				NULL,
				&target,
				rot,
				NULL,
				SDL_FLIP_NONE);
		
		}
	}
	
	SDL_RenderSetClipRect(renderer, NULL);
}

void player_aim(base_t *player)
{
	uint32_t nextFrameTime = 0;
	
	float a = 15.0;
	float d = 1.0;
			
			int baseRadius = 155;
	
	while(true)
	{
		float dt = 1.0 / 60.0;
		
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) exit(1);
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) exit(1);
			
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
				
				float2 pos = {
					baseRadius * sinf(DEG_TO_RAD(a)) - 6,
					battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(a)) - 6,
				};
				float2 vel = {					
					sinf(DEG_TO_RAD(a)),
					cosf(DEG_TO_RAD(a)),
				};
				vel.x *= 250;
				vel.y *= 250;
				
				fire_projectile(player, pos, vel);
				return;
			}
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderClear(renderer);
		
		render_battleground();
		
		{ // render projectle preview
			setTextureColor(player, texProjectile);
	
			SDL_Rect target = {
				battleground.x + baseRadius * sinf(DEG_TO_RAD(a)) - 6,
				battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(a)) - 6,
				11,
				11
			};
			
			SDL_RenderCopyEx(
				renderer,
				texProjectile,
				NULL,
				&target,
				-a + 90,
				NULL,
				SDL_FLIP_NONE);
		}
		
		SDL_RenderPresent(renderer);
		
		while(SDL_GetTicks() < nextFrameTime) {
			; // BURN!
		}
		nextFrameTime = SDL_GetTicks() + 15;
		
		
		a += 90.0 * d * dt;
		
		// bounces
		if(a > 165.0) {
			a = 165.0;
			d = -1;
		}
		else if(a < 15.0) {
			a = 15.0;
			d = 1;
		}
	}
}

void battle_simulation()
{
	SDL_Event e;
	uint32_t nextFrameTime = 0;
	
	base_t * currentPlayer = &leftBase;
	
	int stage = 0;
	
	while(true)
	{
		float dt = 1.0 / 60.0;
		
		// first, tick all particles
		for(particle_t * p = particles, *prev = NULL; p != NULL; )
		{
			// progress the particle
			p->progress += 2;
			
			// this is fancy deletion code.
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
		
		// second: tick all projectiles
		bool anyProjectileAlive = false;
		for(projectile_t *p = projectiles; p != NULL; p = p->next)
		{
			// disable all out-of-screen projectiles
			if(p->pos.x < -10 || p->pos.y < -10) {
				p->active = false;
			}
			if(p->pos.x >= (battleground.w + 10) || p->pos.y >= (battleground.h + 10)) {
				p->active = false;
			}
		
			if(p->active == false) {
				continue;
			}
			
			anyProjectileAlive = true;
			
			float2 delta = {
				p->vel.x * dt,
				p->vel.y * dt,
			};
			
			p->pos.x += delta.x;
			p->pos.y += delta.y;
			
			// Spawn particles on the way of moving
			float rot = 90 - RAD_TO_DEG(atan2(p->vel.x, p->vel.y));
			int cnt = 3 + sqrt(delta.x*delta.x + delta.y*delta.y);
			for(int i = 0; i < cnt; i++) {
				float2 ppos = {
					p->pos.x - i * delta.x / (cnt - 1),
					p->pos.y - i * delta.y / (cnt - 1),
				};
				spawn_particle(p->base, ppos.x, ppos.y, rot);
			}
		}
		
		if(anyProjectileAlive == false) {
			return;
		}
	
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) return;
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return;
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

void battle_reset()
{
	for(projectile_t *p = projectiles; p != NULL; )
	{
		projectile_t *k = p;
		p = p->next;
		free(k);
	}
	projectiles = NULL;
}

void start_round()
{
	while(true)
	{
		// todo: player_build()
	
		fprintf(stdout, "Start aiming...\n");
		player_aim(&leftBase);
		
		// todo: 
		fprintf(stdout, "Battle simulation...\n");
		battle_simulation();
		
		fprintf(stdout, "Reset battle...\n");
		battle_reset();
		
		// todo: switch_player()
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

void fire_projectile(base_t const * base, float2 pos, float2 vel)
{
	projectile_t *p = malloc(sizeof(projectile_t));
	p->base = base;
	p->active = true;
	p->pos = pos;
	p->vel = vel;
	p->next = projectiles; 
	// Prepend
	projectiles = p;
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
	LOAD(texProjectile, "tex/projectile.png");
	LOAD(texBarricade[0], "tex/barricade-0.png");
	LOAD(texBarricade[1], "tex/barricade-1.png");
	LOAD(texBarricade[2], "tex/barricade-2.png");
#undef LOAD
}