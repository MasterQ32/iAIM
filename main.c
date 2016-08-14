#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#if defined(_MSC_VER)
#include <windows.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#define MAX max
#define MIN min
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <sys/param.h>
#endif

#define RAD_TO_DEG(x) ((x) * 180.0 / M_PI)
#define DEG_TO_RAD(x) ((x) * M_PI / 180.0)

#define AFFECTOR_TYPE_COUNT 5
#define AFFECTOR_LIFE (gameOptions.affectorLivespan)
#define AFFECTOR_COOLDOWNS { 1, 1, 1, 4, 3 }

#define BASE_LIFEPOINTS 4

typedef struct {
	float x, y;
} float2;

typedef struct {
	SDL_Color color;
	int protectors[13];
	int resources[AFFECTOR_TYPE_COUNT];
	int respawn[AFFECTOR_TYPE_COUNT];
	int lifepoints;
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
	int type; /* -1=removed, 0=positive, 1=negative, 2=boost, 3=splitter3, 4=splitter2 */
	base_t const * owner; 
	float2 center;
	float rotation;
	int lifepoints;
	struct affector *next;
} affector_t;

typedef struct block {
	SDL_Rect rect;
	struct block *next;
} block_t;

SDL_Window *window;
SDL_Renderer *renderer;

/**
 * Ingame textures
 **/
SDL_Texture *texPlayArea;
SDL_Texture *texBase;
SDL_Texture *texBaseShips;
SDL_Texture *texMetal;
SDL_Texture *texNumbers;
SDL_Texture *texBackPanel;
SDL_Texture *texLeftPanel;
SDL_Texture *texRightPanel;
SDL_Texture *texProjectile;
SDL_Texture *texParticle;
SDL_Texture *texBarricade[3];
SDL_Texture *texAffector[AFFECTOR_TYPE_COUNT];
SDL_Texture *texButtonLaunch[3];

SDL_Texture *texLevelBackground;
SDL_Texture *texLevelSelector;
SDL_Texture *texLevels[4];
SDL_Texture *texButtonBack[3];

#define BUTTON_NORMAL 0
#define BUTTON_HOVER  1
#define BUTTON_PRESSED 2

/**
 * Mainmenu textures
 **/
SDL_Texture *texMenuBackground;
SDL_Texture *texMenuItems;
SDL_Texture *texMenuSelector;
SDL_Texture *texMenuHelp;

SDL_Texture *texCredits;

SDL_Texture *texFinalBlue;
SDL_Texture *texFinalGreen;

Mix_Chunk *sndStartup;
Mix_Chunk *sndLaunch;
Mix_Chunk *sndSplit2;
Mix_Chunk *sndSplit3;
Mix_Chunk *sndBoost;
Mix_Chunk *sndImpactBarricade;
Mix_Chunk *sndImpactBase;
Mix_Chunk *sndImpactWall;


int cooldowns[AFFECTOR_TYPE_COUNT] = AFFECTOR_COOLDOWNS;

bool isGameRunning = false;

struct {
	bool useSlowAiming;
	bool affectorsStay;
	int affectorLifespan;
} gameOptions = {
	/* useSlowAiming    = */ false,
	/* affectorsStay    = */ false, 
	/* affectorLifespan = */ 3,
};

int framecounter = 0;

base_t leftBase = {
	{ 92, 75, 255, 255 },
	{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 0, 0, 0, 0, 0 },
	AFFECTOR_COOLDOWNS,
	BASE_LIFEPOINTS
};

base_t rightBase = {
	{ 85, 182, 74, 255 },
	{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 0, 0, 0, 0, 0 },
	AFFECTOR_COOLDOWNS,
	BASE_LIFEPOINTS
};

particle_t *particles = NULL;
projectile_t *projectiles = NULL;
affector_t *affectors = NULL;
block_t *blockchain = NULL;

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

void help();

void credits();

void start_round(const char *level);

void spawn_particle(base_t const * base, int x, int y, float rot);

void fire_projectile(base_t const * base, float2 pos, float2 vel);

affector_t * create_affector(base_t const * owner, int type, float2 pos);

void load_level(const char *file);

int main(int argc, char **argv)
{
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);
	
	if(IMG_Init(IMG_INIT_PNG) < 0) {
		fprintf(stderr, "Failed to initialize IMG: %s\n", IMG_GetError());
		exit(1);
	}
	atexit(IMG_Quit);
	
	if(Mix_Init(MIX_INIT_OGG) < 0) {
		fprintf(stderr, "Failed to initialize MIX: %s\n", Mix_GetError());
		exit(1);
	}
	atexit(Mix_Quit);
	
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1) {
		printf("Mix_OpenAudio: %s\n", Mix_GetError());
		exit(1);
	}
	
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
	
	Mix_PlayChannel(-1, sndStartup, 0);
	
	menu();
	
	Mix_CloseAudio();
	
	return 0;
}

void select_level_id(int i)
{
	char name[256];
	sprintf(name, "levels/%02d.txt", i);
	fprintf(stdout, "Start round: %s\n", name);
	start_round(name);
}

void select_level()
{
	int currentSelection = 0;
	while(true)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) exit(1);
			
			if(e.type == SDL_KEYDOWN)
			{
				switch(e.key.keysym.sym)
				{
					case SDLK_ESCAPE: return;
					case SDLK_UP:
						if(currentSelection == 4) break;
						if(currentSelection > 1) currentSelection -= 2; 
						break;
					case SDLK_DOWN:
						if(currentSelection == 4) break;
						if(currentSelection < 2) currentSelection += 2;
						break;
					case SDLK_LEFT:
						if(currentSelection == 4) {
							currentSelection = 3;
						} else {
							currentSelection = (currentSelection&(~1)) + 0;
						}
						break;
					case SDLK_RIGHT:
						if(currentSelection == 1 || currentSelection == 3) {
							currentSelection = 4;
						} else {
							currentSelection = (currentSelection&(~1)) + 1;
						}
						break;
					case SDLK_RETURN:
					case SDLK_SPACE:
						if(currentSelection < 4) {
							select_level_id(currentSelection + 1);
						} else {
							return;
						}
						break;
				}
			}
			
			if(e.type == SDL_MOUSEMOTION)
			{
				for(int i = 0; i < 4; i++) {
					SDL_Rect selector = {
						218 + 424 * (i % 2),
						63 + 299 * (i / 2),
						420,
						295
					};
					if(e.motion.x >= selector.x && e.motion.x < (selector.x + selector.w)  && 
						 e.motion.y >= selector.y && e.motion.y < (selector.y + selector.h))
					{
						currentSelection = i;
					}
				}
			}
			
			if(e.type == SDL_MOUSEBUTTONUP)
			{
				for(int i = 0; i < 4; i++) {
					SDL_Rect selector = {
						218 + 424 * (i % 2),
						63 + 299 * (i / 2),
						420,
						295
					};
					if(e.button.x >= selector.x && e.button.x < (selector.x + selector.w)  && 
						 e.button.y >= selector.y && e.button.y < (selector.y + selector.h))
					{
						select_level_id(i + 1);
						break;
					}
				}
				
				SDL_Rect button = {
					1109, 615,
					124, 44
				};
				if(e.button.x >= button.x && e.button.x < (button.x + button.w)  && 
					 e.button.y >= button.y && e.button.y < (button.y + button.h))
				{
					return;
				}
			}
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderClear(renderer);
		
		SDL_Rect fullscreen = {
			0, 0,
			1280, 720,
		};
		
		SDL_RenderCopy(
			renderer,
			texLevelBackground,
			NULL,
			&fullscreen);
		for(int i = 0; i < 4; i++)
		{
			SDL_Rect selector = {
				218 + 424 * (i % 2),
				63 + 299 * (i / 2),
				420,
				295
			};
			if(currentSelection == i) {
				SDL_SetTextureAlphaMod(texLevels[i], 255);
			} else {
				SDL_SetTextureAlphaMod(texLevels[i], 76);
			}
			if(currentSelection == i) {
				selector.x -= 2;
				selector.y -= 2;
				selector.w += 4;
				selector.h += 4;
				SDL_RenderCopy(
					renderer,
					texLevelSelector,
					NULL,
					&selector);
				selector.x += 2;
				selector.y += 2;
				selector.w -= 4;
				selector.h -= 4;
			} 
			SDL_RenderCopy(
				renderer,
				texLevels[i],
				NULL,
				&selector);
		}
		
		{ // Launch Button
			int x,y;
			int buttons = SDL_GetMouseState(&x, &y);
			
			SDL_Rect button = {
				1109, 615,
				124, 44
			};
			SDL_Texture *tex = texButtonBack[BUTTON_NORMAL];
			
			if((x >= button.x && x < (button.x + button.w) &&
			   y >= button.y && y < (button.y + button.h)) ||
				 currentSelection == 4)
			{
				if(buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
					tex = texButtonBack[BUTTON_PRESSED];
				} else {
					tex = texButtonBack[BUTTON_HOVER];
				}
			}
			
			SDL_RenderCopy(
				renderer,
				tex,
				NULL,
				&button);
		}
		
		SDL_RenderPresent(renderer);
	
		SDL_Delay(16);
	}
}

void menu_select(int i)
{
	switch(i)
	{
		case 0:
			// start_round("levels/04.txt");
			select_level();
			break;
		case 1: help(); break;
		case 2: credits(); break;
		case 3: exit(1);
	}
}

void menu()
{	
	int currentSelection = 0;
	while(true)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) exit(1);
			
			if(e.type == SDL_KEYDOWN)
			{
				switch(e.key.keysym.sym)
				{
					case SDLK_ESCAPE: exit(1);
					case SDLK_UP:
						if(currentSelection > 0) currentSelection--; 
						break;
					case SDLK_DOWN:
						if(currentSelection < 3) currentSelection++;
						break;
					case SDLK_RETURN:
					case SDLK_SPACE:
						menu_select(currentSelection);
						break;
				}
			}
			
			if(e.type == SDL_MOUSEMOTION)
			{
				for(int i = 0; i < 4; i++) {
					SDL_Rect selector = {
						527, 433 + 55 * i,
						226, 40,
					};
					if(e.motion.x >= selector.x && e.motion.x < (selector.x + selector.w)  && 
						 e.motion.y >= selector.y && e.motion.y < (selector.y + selector.h))
					{
						currentSelection = i;
					}
				}
			}
			
			if(e.type == SDL_MOUSEBUTTONUP)
			{
				for(int i = 0; i < 4; i++) {
					SDL_Rect selector = {
						527, 433 + 55 * i,
						226, 40,
					};
					if(e.button.x >= selector.x && e.button.x < (selector.x + selector.w)  && 
						 e.button.y >= selector.y && e.button.y < (selector.y + selector.h))
					{
						menu_select(i);
						break;
					}
				}
			}
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderClear(renderer);
		
		SDL_Rect fullscreen = {
			0, 0,
			1280, 720,
		};
		
		SDL_Rect menuitems = {
			549, 435,
			180, 206,
		};
		
		SDL_Rect selector = {
			527, 433 + 55 * currentSelection,
			226, 40,
		};
		
		SDL_RenderCopy(
			renderer,
			texMenuBackground,
			NULL,
			&fullscreen);
		SDL_RenderCopy(
			renderer,
			texMenuSelector,
			NULL,
			&selector);
		SDL_RenderCopy(
			renderer,
			texMenuItems,
			NULL,
			&menuitems);
		
		SDL_RenderPresent(renderer);
	
		SDL_Delay(16);
	}
	

}


void credits()
{
	int currentSelection = 0;
	while(true)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) exit(1);
			if(e.type == SDL_KEYDOWN) return;
			if(e.type == SDL_MOUSEBUTTONUP) return;
		}
		SDL_Rect fullscreen = {
			0, 0,
			1280, 720,
		};
		
		SDL_RenderCopy(
			renderer,
			texCredits,
			NULL,
			&fullscreen);
		
		SDL_RenderPresent(renderer);
	
		SDL_Delay(16);
	}
}

void help()
{
	while(true)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) exit(1);
			if(e.type == SDL_KEYDOWN) return;
			if(e.type == SDL_MOUSEBUTTONDOWN) return;
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderClear(renderer);
		
		SDL_Rect fullscreen = {
			0, 0,
			1280, 720,
		};
		
		
		SDL_RenderCopy(
			renderer,
			texMenuHelp,
			NULL,
			&fullscreen);
		
		SDL_RenderPresent(renderer);
	
		SDL_Delay(16);
	}
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
		
		// setTextureColor(&leftBase, texBase);
		sourceRect.x = 128;
		SDL_RenderCopy(
			renderer,
			texBaseShips,
			&sourceRect,
			&leftBaseRect);
			
		// setTextureColor(&rightBase, texBase);
		sourceRect.x = 0;
		SDL_RenderCopy(
			renderer,
			texBaseShips,
			&sourceRect,
			&rightBaseRect);
		
		leftBaseRect.x -= 128;
		leftBaseRect.w += 128;
		
		rightBaseRect.w += 128;
			
		SDL_SetTextureAlphaMod(texBase, 255 * leftBase.lifepoints / BASE_LIFEPOINTS);
		SDL_RenderCopyEx(
			renderer,
			texBase,
			NULL,
			&leftBaseRect,
			framecounter / 8.0,
			NULL,
			SDL_FLIP_NONE);
		
		SDL_SetTextureAlphaMod(texBase, 255 * rightBase.lifepoints / BASE_LIFEPOINTS);
		SDL_RenderCopyEx(
			renderer,
			texBase,
			NULL,
			&rightBaseRect,
			-framecounter / 6.0,
			NULL,
			SDL_FLIP_NONE);
			
		framecounter += 1;
	}
	
	for(block_t *b = blockchain; b != NULL; b = b->next)
	{
		SDL_Rect rect = b->rect;
		rect.x += battleground.x;
		
		// SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		// SDL_RenderFillRect(renderer, &rect);
		SDL_RenderCopy(renderer, texMetal, &rect, &rect);
			
		SDL_SetRenderDrawColor(renderer, 96, 96, 96, 255);
		SDL_RenderDrawRect(renderer, &rect);
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

bool player_aim(base_t *player)
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
			if(e.type == SDL_QUIT) exit(0);
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return false;
			
			if((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) ||
			   (e.type == SDL_MOUSEBUTTONDOWN)) {
				
				float2 pos, vel;
				if(player == &leftBase) {
					pos = (float2){
						baseRadius * sinf(DEG_TO_RAD(a)) - 6,
						battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(a)) - 6,
					};
					vel = (float2) {
						sinf(DEG_TO_RAD(a)),
						cosf(DEG_TO_RAD(a)),
					};
				} else {
					pos = (float2) {
						battleground.w - baseRadius * sinf(DEG_TO_RAD(a)) - 6,
						battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(a)) - 6,
					};
					vel = (float2){
						-sinf(DEG_TO_RAD(a)),
						cosf(DEG_TO_RAD(a)),
					};
				}
				vel.x *= 250;
				vel.y *= 250;
				
				Mix_PlayChannel(-1, sndLaunch, 0);
				fire_projectile(player, pos, vel);
				return true;
			}
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderClear(renderer);
		
		render_battleground();
		
		{ // render projectle preview
			setTextureColor(player, texProjectile);
			
			if(player == &leftBase)
			{
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
				} else {
					SDL_Rect target = {
						battleground.x + battleground.w - baseRadius * sinf(DEG_TO_RAD(a)) - 6,
						battleground.h / 2 + baseRadius * cosf(DEG_TO_RAD(a)) - 6,
						11,
						11
					};
					
					SDL_RenderCopyEx(
						renderer,
						texProjectile,
						NULL,
						&target,
						a + 90,
						NULL,
						SDL_FLIP_NONE);
				}
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
		
		
		float angularSpeed = 90.0;
		if(gameOptions.useSlowAiming) {
			angularSpeed = 45.0;
		}
		a += angularSpeed * d * dt;
		
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

void endscreen(SDL_Texture *tex)
{
	isGameRunning = false;
	
	SDL_Rect fullscreen = {
		0, 0,
		1280, 720,
	};
	SDL_RenderCopy(
		renderer,
		tex,
		NULL,
		&fullscreen);
	
	SDL_RenderPresent(renderer);
		
	while(true)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) exit(1);
			if(e.type == SDL_KEYDOWN) return;
			if(e.type == SDL_MOUSEBUTTONUP) return;
		}
	
		SDL_Delay(16);
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
			if(p->active == false) {
				continue;
			}
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
				Mix_PlayChannel(-1, sndImpactBase, 0);
				// hit left base
				leftBase.lifepoints--;
				if(leftBase.lifepoints < 0) {
					endscreen(texFinalGreen);
					return;
				}
				p->active = false;
			}
			if(distance(p->pos, rightBasePos) <= 126) {
				Mix_PlayChannel(-1, sndImpactBase, 0);
				// hit right base
				rightBase.lifepoints--;
				if(rightBase.lifepoints < 0) {
					endscreen(texFinalBlue);
					return;
				}
				p->active = false;
			}
			
			if(p->active == false) {
				continue;
			}
			
			float2 accel = { 0 };
			
			for(affector_t *a = affectors; a != NULL; a = a->next)
			{
				if(a->type < 0) {
					// ignore all destroyed affectors.
					continue;
				}
				float2 dst = {
					p->pos.x - a->center.x,
					p->pos.y - a->center.y,
				};
				float len = length(dst);
				dst.x /= len;
				dst.y /= len;
				
				if(len <= 16) { // 32 diameter
					// we crashen in an affector
					
					if(a->lifepoints > 0 && a->type >= 2 && a->type <= 4) {
						// and this affector is a booster
						
						int offset[] = { 0, -45, 45 };
						int len = 0;
						float speed = length(p->vel);
						switch(a->type) {
							case 2: 
								len = 1; 
								speed *= 1.5;
								Mix_PlayChannel(-1, sndBoost, 0);
								break;
							case 3: 
								Mix_PlayChannel(-1, sndSplit3, 0);
								len = 3; 
								break;
							case 4:
								Mix_PlayChannel(-1, sndSplit2, 0);
								len = 2; 
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
					
					a->lifepoints -= 1;
					
					if(a->lifepoints <= 0) {
						a->type = -1; // Destroy the affector.
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
			
			// check collision against blocks
			for(block_t *b = blockchain; b != NULL; b = b->next)
			{
				SDL_Rect rect = b->rect;
				
				bool hit = check_collision(
						p->pos,
						newPos,
						(float2){ rect.x, rect.y },
						(float2){ rect.w, rect.h },
						0.0);
				
				if(hit) {
					p->active = false;
					Mix_PlayChannel(-1, sndImpactWall, 0);
					break;
				}
			}
			
			if(p->active)
			{
				// check collision against protectors
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
							Mix_PlayChannel(-1, sndImpactBarricade, 0);
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
							Mix_PlayChannel(-1, sndImpactBarricade, 0);
							p->active = false;
							break;
						}
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
			if(e.type == SDL_QUIT) exit(1);
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
				isGameRunning = false;
				return;
			}
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
	
	
	if(gameOptions.affectorsStay) {
		affector_t *it = affectors;
		affector_t *prev = NULL;
		while(it != NULL)
		{
			if(it->type >= 0) {
				// Just skip
				prev = it;
				it = it->next;
			} else {
				// delete entry
				if(prev == NULL) {
					affectors = it->next;
				}
				
				affector_t *tmp = it;
				it = it->next;
				free(tmp);
				
				if(prev != NULL) {
					prev->next = it;
				}
			}
		}
	
	} else {
		for(affector_t *p = affectors; p != NULL; )
		{
			affector_t *k = p;
			p = p->next;
			free(k);
		}
		affectors = NULL;
	}
}

void player_build(base_t *player)
{
	int draggingAffector = -1;
	
	SDL_Event e;
	uint32_t nextFrameTime = 0;
	
	affector_t *currentAffector = NULL;
	
	bool isRotating = true;
	int isMoving = 0;
	
	while(true)
	{
		float dt = 1.0 / 60.0;
		
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) exit(0);
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
				isGameRunning = false;
				return;
			}
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) return;
			
			if(e.type == SDL_MOUSEBUTTONDOWN)
			{
				SDL_Rect target = {
					32, 32, 64, 64
				};
				if(player == &rightBase) {
					target.x += battleground.x;
					target.x += battleground.w;
				}
				draggingAffector = -1;
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
				
				if(draggingAffector < 0)
				{
					// Check for dragging the move point
					if(currentAffector != NULL)
					{							
						float2 pos = {
							48 * cosf(DEG_TO_RAD(-currentAffector->rotation)),
							-48 * sinf(DEG_TO_RAD(-currentAffector->rotation)),
						};
						pos.x += currentAffector->center.x;
						pos.y += currentAffector->center.y;
						
						SDL_Rect grabbag = {
							battleground.x + pos.x - 6, pos.y - 6,
							12, 12
						};
						
						if(e.button.x >= grabbag.x && e.button.y >= grabbag.y &&
					   e.button.x < (grabbag.x + grabbag.w) &&
						 e.button.y < (grabbag.y + grabbag.h))
						{
							isRotating = true;
						}
						
						float2 delta = {
							e.button.x - currentAffector->center.x - battleground.x,
							e.button.y - currentAffector->center.y,
						};
						if(length(delta) < 16) {
							isMoving = 1;
							SDL_SetRelativeMouseMode(SDL_TRUE);
						}
						
						if(length(delta) > 64) {
							isMoving = 0;
							isRotating = false;
							SDL_SetRelativeMouseMode(SDL_FALSE);
							currentAffector = NULL;
						}
					}
					// No else-if: Allows reselection of other affectors.
					if(currentAffector == NULL)
					{
						float minDist = 16;
						for(affector_t *p = affectors; p != NULL; p = p->next)
						{
							if(p->owner != player) {
								continue;
							}
							float2 pos = {
								battleground.x + p->center.x,
								p->center.y,
							};
							pos.x -= e.button.x;
							pos.y -= e.button.y;
							float l = length(pos);
							if(l > minDist) {
								continue;
							}
							l = minDist;
							currentAffector = p;
						}
					}
				}
			}
			
			if(e.type == SDL_MOUSEBUTTONUP)
			{
				if(draggingAffector >= 0) {
					fprintf(stderr, "%d,%d,%d\n", e.button.x, battleground.x, battleground.w);
					if(e.button.x >= battleground.x && e.button.x < (battleground.x + battleground.w)) {
						affector_t *a = create_affector(player, draggingAffector, (float2){e.button.x - 128, e.button.y});
						if(player == &rightBase) {
							a->rotation = 180;
						}
						currentAffector = a;
						player->resources[draggingAffector] -= 1;
					}
					draggingAffector = -1;
				}
				
				if(isRotating) {
					isRotating = false;
				}
				if(isMoving) {
					if(currentAffector != NULL && (e.button.x <= battleground.x || e.button.x > (battleground.x + battleground.w))) {
						player->resources[currentAffector->type] += 1; // return affector to inventory
						currentAffector->type = -1;
						currentAffector->center.x = -100000;
					}
					isMoving = 0;
					SDL_SetRelativeMouseMode(SDL_FALSE);
				}
				
				SDL_Rect button = {
					16, 668,
					96, 36
				};
				if(player == &rightBase) {
					button.x += battleground.x + battleground.w;
				}
				if(e.button.x >= button.x && e.button.x < (button.x + button.w) &&
					 e.button.y >= button.y && e.button.y < (button.y + button.h)) 
				{
					// On click: finish!
					return;
				}
				
				{ // Check for click on base:
					float x = e.button.x - battleground.x;
					if(player == &rightBase) {
						x -= battleground.w;
					}
					float y = e.button.y - battleground.h/2;
					
					if((x*x+y*y) < (125*125))
					{
						if(player == &rightBase) {
							if(x < 0) {
								// Launch when click on base.
								return;
							}
						}
						if(player == &leftBase) {
							if(x > 0) {
								// Launch
								return;
							}
						}
					}
				}
			}
			
			if(e.type == SDL_MOUSEMOTION);
			{
				if(currentAffector != NULL && isRotating)
				{
					float2 delta = {
						e.motion.x - currentAffector->center.x - battleground.x,
						e.motion.y - currentAffector->center.y,
					};
					
					currentAffector->rotation = 90 - RAD_TO_DEG(atan2(delta.x, delta.y));
				}
				if(currentAffector != NULL && isMoving)
				{
					if(isMoving == 1) {
						isMoving = 2;
					} else {
						currentAffector->center.x += e.motion.xrel;
						currentAffector->center.y += e.motion.yrel;
					}
				}
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
				
				SDL_RenderCopy(
					renderer,
					texBackPanel,
					NULL,
					&leftPanel);
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
			if(player == &rightBase) {
				
				SDL_RenderCopy(
					renderer,
					texBackPanel,
					NULL,
					&rightPanel);
				SDL_Rect target = {
					battleground.x + battleground.w + 32, 32, 
					64, 64
				};
				for(int i = 0; i < AFFECTOR_TYPE_COUNT; i++) {
					int count = player->resources[i];
					if(count <= 0) {
						continue;
					}
				
					SDL_RenderCopyEx(
						renderer,
						texAffector[i],
						NULL,
						&target,
						180,
						NULL,
						SDL_FLIP_NONE);
					
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
					texRightPanel,
					NULL,
					&rightPanel);
			}
		}
		
		{ // Launch Button
			int x,y;
			int buttons = SDL_GetMouseState(&x, &y);
			
			SDL_Rect button = {
				16, 668,
				96, 36
			};
			if(player == &rightBase) {
				button.x += battleground.x + battleground.w;
			}
			SDL_Texture *tex = texButtonLaunch[BUTTON_NORMAL];
			
			if(x >= button.x && x < (button.x + button.w) &&
			   y >= button.y && y < (button.y + button.h)) 
			{
				if(buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
					tex = texButtonLaunch[BUTTON_PRESSED];
				} else {
					tex = texButtonLaunch[BUTTON_HOVER];
				}
			}
			
			SDL_RenderCopy(
				renderer,
				tex,
				NULL,
				&button);
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
				
			SDL_RenderCopyEx(
				renderer,
				texAffector[draggingAffector],
				NULL,
				&target,
				(player == &rightBase) ? 180 : 0,
				NULL,
				SDL_FLIP_NONE);
		}
		
		if(currentAffector != NULL)
		{
			SDL_SetRenderDrawColor(
				renderer,
				192, 192, 192, 255);
				
			float2 pos = {
				48 * cosf(DEG_TO_RAD(-currentAffector->rotation)),
				-48 * sinf(DEG_TO_RAD(-currentAffector->rotation)),
			};
			pos.x += currentAffector->center.x;
			pos.y += currentAffector->center.y;
			SDL_RenderDrawLine(
				renderer,
				currentAffector->center.x + battleground.x, currentAffector->center.y,
				battleground.x + pos.x, pos.y);
			
			SDL_Rect grabbag = {
				battleground.x + pos.x - 6, pos.y - 6,
				12, 12
			};
			if(isRotating) {
				SDL_SetRenderDrawColor(
					renderer,
					255, 128, 128, 255);
			}
			SDL_RenderFillRect(renderer, &grabbag);
		}
		
		SDL_RenderPresent(renderer);
		
		while(SDL_GetTicks() < nextFrameTime) {
			; // BURN!
		}
		nextFrameTime = SDL_GetTicks() + 15;
	}
}

void start_round(const char *level)
{
	// Load level
	load_level(level);

	// Initialize game state	
	leftBase = (base_t) {
		{ 92, 75, 255, 255 },
		{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
		{ 0, 0, 0, 0, 0 },
		AFFECTOR_COOLDOWNS,
		BASE_LIFEPOINTS
	};

	rightBase = (base_t) {
		{ 85, 182, 74, 255 },
		{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
		{ 0, 0, 0, 0, 0 },
		AFFECTOR_COOLDOWNS,
		BASE_LIFEPOINTS
	};
	
	
	// Start game
	base_t *player = &leftBase;
	isGameRunning = true;
	while(true)
	{
		fprintf(stdout, "Reset battle...\n");
		battle_reset();
		if(isGameRunning == false) return;
		
		fprintf(stdout, "Resupplement...\n");
		for(int i = 0; i < AFFECTOR_TYPE_COUNT; i++) 
		{
			player->respawn[i] -= 1;
			if(player->respawn[i] <= 0) {
				player->resources[i] += 1;
				player->respawn[i] = cooldowns[i];
			}
		}
		
		do {
			fprintf(stdout, "Battle setup...\n");
			player_build(player);
			if(isGameRunning == false) return;
		
			fprintf(stdout, "Start aiming...\n");
		} while(player_aim(player) == false);
		
		// todo: 
		fprintf(stdout, "Battle simulation...\n");
		battle_simulation();
		if(isGameRunning == false) return;
		
		if(player == &leftBase) {
			player = &rightBase;
		} else {
			player = &leftBase;
		}
	}
}

void load_level(const char *file)
{
	FILE *f = fopen(file, "r");

	fscanf(f, "iAIM Level 1.0\n");
	if(ferror(f)) {
		fprintf(stderr, "Failed to load level %s\n", file);
		exit(1);
	}
	
	// clean current level first:
	for(block_t *b = blockchain; b != NULL;)
	{
		block_t *t = b;
		b = b->next;
		free(t);
	}
	blockchain = NULL;
	
	while(!feof(f))
	{
		SDL_Rect block;
		fscanf(f, "%d,%d,%d,%d", &block.x, &block.y, &block.w, &block.h);
		if(ferror(f)) {
			fprintf(stderr, "Failed to load level %s\n", file);
			exit(1);
		}
		
		block_t *b = malloc(sizeof(block_t));
		b->next = blockchain;
		b->rect = block;
		blockchain = b;
	}
	fclose(f);
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

affector_t * create_affector(base_t const * owner, int type, float2 pos)
{
	affector_t *a = malloc(sizeof(affector_t));
	a->type = type; /* 0=positive, 1=negative */
	a->owner = owner;
	a->center = pos;
	a->rotation = 0;
	a->lifepoints = AFFECTOR_LIFE;
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
#define SOUND(snd, file) if((snd = Mix_LoadWAV(file)) == NULL) \
	{ \
		fprintf(stderr, "Failed to load " file ": %s\n", Mix_GetError()); \
		exit(1); \
	}
#define BUTTON(tex, img, imgp) \
	LOAD(tex[BUTTON_NORMAL],  img "normal" imgp); \
	LOAD(tex[BUTTON_HOVER],   img "hover" imgp); \
	LOAD(tex[BUTTON_PRESSED], img "press" imgp)
	
	
	LOAD(texPlayArea, "tex/play-area.png");
	LOAD(texBase, "tex/base.png");
	LOAD(texBaseShips, "tex/base-bg.png");
	LOAD(texNumbers, "tex/numbers.png");
	LOAD(texMetal, "tex/metalbackground.png");
	LOAD(texBackPanel, "tex/sidepanel.png");
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
	
	LOAD(texMenuBackground, "tex/mainmenu-bg.png");
	LOAD(texMenuItems, "tex/mainmenu-items.png");
	LOAD(texMenuSelector, "tex/mainmenu-selector.png");
	LOAD(texMenuHelp, "tex/helpmenu.png");
	
	LOAD(texCredits, "tex/credits.png");
	
	LOAD(texFinalBlue, "tex/winscreen-blue.png");
	LOAD(texFinalGreen, "tex/winscreen-green.png");
	
	LOAD(texLevelBackground, "tex/levelselection.png");
	LOAD(texLevelSelector, "tex/level-selector.png");
	LOAD(texLevels[0], "levels/01.png");
	LOAD(texLevels[1], "levels/02.png");
	LOAD(texLevels[2], "levels/03.png");
	LOAD(texLevels[3], "levels/04.png");
	
	BUTTON(texButtonLaunch, "tex/launch-button-", ".png");
	BUTTON(texButtonBack, "tex/back-button-", ".png");
	
	SOUND(sndStartup, "sounds/startup.wav");
	SOUND(sndLaunch, "sounds/launch.wav");
	SOUND(sndSplit2, "sounds/split2.wav");
	SOUND(sndSplit3, "sounds/split3.wav");
	SOUND(sndBoost, "sounds/boost.wav");
	SOUND(sndImpactBarricade, "sounds/barricade.wav");
	SOUND(sndImpactWall, "sounds/crush.wav");
	SOUND(sndImpactBase, "sounds/base.wav");
	
#undef BUTTON
#undef SOUND
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
		points[i].x = center.x + np.x + size.x;
		points[i].y = center.y + np.y + size.y;
	}
	
	for(int i = 0; i < 4; i++) {
		bool hit = get_line_intersection(
			points[i], points[(i+1)%4],
			start, end);
		if(hit) return true;
	}
	return false;
}