#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/param.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define RAD_TO_DEG(x) ((x) * 180.0 / M_PI)
#define DEG_TO_RAD(x) ((x) * M_PI / 180.0)

#define AFFECTOR_TYPE_COUNT 5

typedef struct {
	float x, y;
} float2;

typedef struct {
	SDL_Color color;
	int protectors[13];
	int resources[AFFECTOR_TYPE_COUNT];
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

typedef struct affector {
	int type; /* 0=positive, 1=negative, 2=boost, 3=splitter3, 4=splitter2 */
	float2 center;
	float rotation;
	struct affector *next;
} affector_t;

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Texture *texPlayArea;
SDL_Texture *texBase;
SDL_Texture *texNumbers;
SDL_Texture *texLeftPanel;
SDL_Texture *texRightPanel;
SDL_Texture *texProjectile;
SDL_Texture *texParticle;
SDL_Texture *texBarricade[3];
SDL_Texture *texAffector[AFFECTOR_TYPE_COUNT];

base_t leftBase = {
	{ 92, 75, 255, 255 },
	{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 5, 4, 3, 2, 1},
};

base_t rightBase = {
	{ 85, 182, 74, 255 },
	{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 5, 5, 5, 5, 5},
};

particle_t *particles = NULL;
projectile_t *projectiles = NULL;
affector_t *affectors = NULL;

SDL_Rect battleground = {
	128, 0,
	1280 - 256, 720
};

float distance(float2 a, float2 b);
float length(float2 a);

// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines 
// intersect the intersection point may be stored in the floats i_x and i_y.
bool get_line_intersection(
	float2 p0, float2 p1,
	float2 p2, float2 p3);

bool check_collision(
	float2 start,
	float2 end,
	float2 center,
	float2 size,
	float rot);

void load_resources();

void menu();

void start_round();

void spawn_particle(base_t const * base, int x, int y, float rot);

void fire_projectile(base_t const * base, float2 pos, float2 vel);

affector_t * create_affector(int type, float2 pos);


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
	
	// Draw affectors
	{		
		for(affector_t *p = affectors; p != NULL; p = p->next)
		{
			SDL_Rect target = {
				battleground.x + p->center.x - 32, p->center.y - 32,
				64, 64
			};
		
			SDL_RenderCopyEx(
				renderer,
				texAffector[p->type],
				NULL,
				&target,
				p->rotation,
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
		
		
		{ // Draw closed tool panels
			SDL_Rect leftPanel = {
				0, 0, 128, 720
			};
			SDL_RenderCopy(
				renderer,
				texLeftPanel,
				NULL,
				&leftPanel);
				
			SDL_Rect rightPanel = {
				battleground.x + battleground.w, 0, 128, 720
			};
			SDL_RenderCopy(
				renderer,
				texRightPanel,
				NULL,
				&rightPanel);
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
			
			float2 leftBasePos = { 0, battleground.h / 2 };
			float2 rightBasePos = { battleground.w, battleground.h / 2 };
			
			if(distance(p->pos, leftBasePos) <= 126) {
				// hit left base
				printf("left base hit!\n");
				p->active = false;
			}
			if(distance(p->pos, rightBasePos) <= 126) {
				// hit right base
				printf("right base hit!\n");
				p->active = false;
			}
			
			if(p->active == false) {
				continue;
			}
			
			float2 accel = { 0 };
			
			for(affector_t *a = affectors; a != NULL; a = a->next)
			{
				float2 dst = {
					p->pos.x - a->center.x,
					p->pos.y - a->center.y,
				};
				float len = length(dst);
				dst.x /= len;
				dst.y /= len;
				
				if(len <= 16) { // 32 diameter
					// we crashen in an affector
					
					if(a->type >= 2 && a->type <= 4) {
						// and this affector is a booster
						printf("BOOST!\n");
						
						int offset[] = { 0, -45, 45 };
						int len = 0;
						float speed = length(p->vel);
						switch(a->type) {
							case 2: 
								len = 1; 
								speed *= 2.0;
								break;
							case 3: len = 3; break;
							case 4: len = 2; 
								offset[0] = -30;
								offset[1] =  30;
								break;
						}
						
						for(int i = 0; i < len; i++) {
							float2 dir = {
								24 * cos(DEG_TO_RAD(a->rotation + offset[i])),
								24 * sin(DEG_TO_RAD(a->rotation + offset[i])),
							};
							
							float2 xvel = dir;
							xvel.x *= speed / 24;
							xvel.y *= speed / 24;
							
							anyProjectileAlive = true;
							fire_projectile(
								p->base, 
								(float2){ a->center.x + dir.x, a->center.y + dir.y }, 
								xvel);
						}
					}
					
					p->active = false;
					break;
				}
				
				if(len <= 0) {
					continue;
				}
				
				float strength = 2000.0 / len;
				strength *= strength;
				
				dst.x *= strength;
				dst.y *= strength;
				
				switch(a->type) {
					case 0:
						accel.x -= dst.x;
						accel.y -= dst.y;
						break;
					case 1:
						accel.x += dst.x;
						accel.y += dst.y;
						break;
				}
			}
			
			float2 vel = p->vel;
			vel.x += accel.x * dt;
			vel.y += accel.y * dt;
			
			float2 delta = {
				vel.x * dt,
				vel.y * dt,
			};
			float2 newPos = {
				p->pos.x + delta.x,
				p->pos.y + delta.y,
			};
			
			for(int i = 0; i < 13; i++) {
				int baseRadius = 136;
			
				// left base
				if(leftBase.protectors[i] > 0) {
					SDL_Rect target = {
						baseRadius * sinf(DEG_TO_RAD(15 * i)) - 6,
						battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(15 * i)) - 15,
						12,
						30,
					};
					float a = -15 * i - 90;
					
					bool hit = check_collision(
						p->pos,
						newPos,
						(float2){ target.x, target.y },
						(float2){ target.w, target.h },
						a);
					if(hit != false) {
						leftBase.protectors[i] -= 1;
						// todo: play sound
						p->active = false;
						break;
					}
				}
				
				if(rightBase.protectors[i] > 0) {
					SDL_Rect target = {
						battleground.w - baseRadius * sinf(DEG_TO_RAD(15 * i)) - 6,
						battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(15 * i)) - 15,
						12,
						30,
					};
					float a = 15 * i - 90;
					bool hit = check_collision(
						p->pos,
						newPos,
						(float2){ target.x, target.y },
						(float2){ target.w, target.h },
						a);
					if(hit != false) {
						rightBase.protectors[i] -= 1;
						// todo: play sound
						p->active = false;
						break;
					}
				}
			}
			
			// Spawn particles on the way of moving, even if we aren't active any more
			float rot = 90 - RAD_TO_DEG(atan2(p->vel.x, p->vel.y));
			int cnt = 3 + sqrt(delta.x*delta.x + delta.y*delta.y);
			for(int i = 0; i < cnt; i++) {
				float2 ppos = {
					p->pos.x + i * delta.x / (cnt - 1),
					p->pos.y + i * delta.y / (cnt - 1),
				};
				spawn_particle(p->base, ppos.x, ppos.y, rot);
			}
			
			if(p->active == false) {
				continue;
			}
			
			anyProjectileAlive = true;
			
			p->vel = vel;
			p->pos.x += delta.x;
			p->pos.y += delta.y;
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
		
		{ // Draw closed tool panels
			SDL_Rect leftPanel = {
				0, 0, 128, 720
			};
			SDL_RenderCopy(
				renderer,
				texLeftPanel,
				NULL,
				&leftPanel);
				
			SDL_Rect rightPanel = {
				battleground.x + battleground.w, 0, 128, 720
			};
			SDL_RenderCopy(
				renderer,
				texRightPanel,
				NULL,
				&rightPanel);
		}
		
		
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
	
	
	for(affector_t *p = affectors; p != NULL; )
	{
		affector_t *k = p;
		p = p->next;
		free(k);
	}
	affectors = NULL;
}

void player_build(base_t *player)
{
	/*
	affector_t *booster, *splitter;
	
	create_affector(0, (float2){ 512, 360 - 100 }); // positive affector
	create_affector(1, (float2){ 512, 360 + 100 }); // positive affector
	booster = create_affector(2, (float2){ 360, 260 }); // positive affector
	booster->rotation = 45;
	
	splitter = create_affector(3, (float2){ 360, 460 }); // positive affector
	splitter->rotation = -75;
	
	splitter = create_affector(4, (float2){ 260, 260 }); // positive affector
	splitter->rotation = -45;;
	// */
	
	int draggingAffector = -1;
	
	SDL_Event e;
	uint32_t nextFrameTime = 0;
	
	base_t * currentPlayer = &leftBase;
	
	while(true)
	{
		float dt = 1.0 / 60.0;
		
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) return;
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return;
			
			if(e.type == SDL_MOUSEBUTTONDOWN)
			{
				SDL_Rect target = {
					32, 32, 64, 64
				};
				for(int i = 0; i < AFFECTOR_TYPE_COUNT; i++) {
					int count = player->resources[i];
					if(count <= 0) {
						continue;
					}
					
					if(e.button.x >= target.x && e.button.y >= target.y &&
					   e.button.x < (target.x + target.w) &&
						 e.button.y < (target.y + target.h))
					{
						draggingAffector = i;
						break;
					}
				
					target.y += 96;
				}
			}
			
			if(e.type == SDL_MOUSEBUTTONUP)
			{
				if(e.button.x >= 128) {
					create_affector(draggingAffector, (float2){e.button.x - 128, e.button.y});
					player->resources[draggingAffector] -= 1;
				}
				draggingAffector = -1;
			}
		}
		
		SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		SDL_RenderClear(renderer);
		
		render_battleground();
		
		{ // Draw closed tool panels
			SDL_Rect leftPanel = {
				0, 0, 128, 720
			};
			if(player == &leftBase) {
				
				SDL_Rect target = {
					32, 32, 64, 64
				};
				for(int i = 0; i < AFFECTOR_TYPE_COUNT; i++) {
					int count = player->resources[i];
					if(count <= 0) {
						continue;
					}
				
					SDL_RenderCopy(
						renderer,
						texAffector[i],
						NULL,
						&target);
					
					SDL_Rect number = {
						target.x + 48, target.y + 48,
						16, 16
					};
					SDL_Rect numberSrc = {
						16 * count, 0,
						16, 16
					};
					SDL_RenderCopy(
						renderer,
						texNumbers,
						&numberSrc,
						&number);
				
					target.y += 96;
				}
			
			} else {
				SDL_RenderCopy(
					renderer,
					texLeftPanel,
					NULL,
					&leftPanel);
			}
				
			SDL_Rect rightPanel = {
				battleground.x + battleground.w, 0, 128, 720
			};
			if(player != &rightBase) {
				SDL_RenderCopy(
					renderer,
					texRightPanel,
					NULL,
					&rightPanel);
			}
		}
		
		if(draggingAffector >= 0)
		{
			SDL_Rect target = {
				0, 0,
				64, 64
			};
			
			SDL_GetMouseState(&target.x, &target.y);
			target.x -= 32;
			target.y -= 32;
			
			SDL_RenderCopy(
				renderer,
				texAffector[draggingAffector],
				NULL,
				&target);
		}
		
		SDL_RenderPresent(renderer);
		
		while(SDL_GetTicks() < nextFrameTime) {
			; // BURN!
		}
		nextFrameTime = SDL_GetTicks() + 15;
	}
}

void start_round()
{
	while(true)
	{
		// todo: player_build()
		fprintf(stdout, "Battle setup...\n");
		player_build(&leftBase);
	
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

affector_t * create_affector(int type, float2 pos)
{
	affector_t *a = malloc(sizeof(affector_t));
	a->type = type; /* 0=positive, 1=negative */
	a->center = pos;
	a->rotation = 0;
	a->next = affectors;
	
	affectors = a;
	
	return a;
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
	LOAD(texNumbers, "tex/numbers.png");
	LOAD(texLeftPanel, "tex/left-panel.png");
	LOAD(texRightPanel, "tex/right-panel.png");
	LOAD(texParticle, "tex/particles.png");
	LOAD(texProjectile, "tex/projectile.png");
	LOAD(texBarricade[0], "tex/barricade-0.png");
	LOAD(texBarricade[1], "tex/barricade-1.png");
	LOAD(texBarricade[2], "tex/barricade-2.png");
	LOAD(texAffector[0], "tex/positive-affector.png");
	LOAD(texAffector[1], "tex/negative-affector.png");
	LOAD(texAffector[2], "tex/boost-affector.png");
	LOAD(texAffector[3], "tex/split3-affector.png");
	LOAD(texAffector[4], "tex/split2-affector.png");
#undef LOAD
}


float distance(float2 a, float2 b)
{
	return sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

float length(float2 a)
{
	return sqrt(a.x*a.x + a.y*a.y);
}


// Given three colinear points p, q, r, the function checks if
// point q lies on line segment 'pr'
static bool onSegment(float2 p, float2 q, float2 r)
{
    if (q.x <= MAX(p.x, r.x) && q.x >= MIN(p.x, r.x) &&
        q.y <= MAX(p.y, r.y) && q.y >= MIN(p.y, r.y))
       return true;
    return false;
}
 
// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
static int orientation(float2 p, float2 q, float2 r)
{
    // See http://www.geeksforgeeks.org/orientation-3-ordered-points/
    // for details of below formula.
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);
    if (val == 0) return 0;  // colinear
    return (val > 0)? 1: 2; // clock or counterclock wise
}

// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool get_line_intersection(
	float2 p1, float2 q1, 
	float2 p2, float2 q2)
{
	// Find the four orientations needed for general and
	// special cases
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases
	// p1, q1 and p2 are colinear and p2 lies on segment p1q1
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and p2 are colinear and q2 lies on segment p1q1
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are colinear and p1 lies on segment p2q2
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are colinear and q1 lies on segment p2q2
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases
}

bool check_collision(
	float2 start,
	float2 end,
	float2 center,
	float2 size,
	float rot)
{
	size.x /= 2;
	size.y /= 2;
	
	rot = DEG_TO_RAD(rot);
	
	float2 points[] = {
		{ // top-left
			-size.x,
			-size.y
		},
		{ // top-right
			size.x,
			-size.y
		},
		{ // bottom-right
			size.x,
			size.y
		},
		{ // bottom-left
			-size.x,
			size.y
		},
	};
	for(int i = 0; i < 4; i++) {
		float2 np = {
			points[i].x * cos(rot) - points[i].y * sin(rot),
			points[i].x * sin(rot) + points[i].y * cos(rot),
		};
		points[i].x = center.x + np.x;
		points[i].y = center.y + np.y;
	}
	
	// SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	
	for(int i = 0; i < 4; i++) {
		/*
		SDL_RenderDrawLine(
			renderer,
			points[i].x,
			points[i].y,
			points[(i+1)%4].x,
			points[(i+1)%4].y);
		*/
		bool hit = get_line_intersection(
			points[i], points[(i+1)%4],
			start, end);
		if(hit) return true;
	}
	// SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	/*
	SDL_RenderDrawLine(
		renderer,
    start.x,
		start.y,
		end.x,
		end.y);
	*/
	return false;
}