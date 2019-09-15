#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "common.h"
#include "config.h"
#include "log.h"
#include "SDL_helper.h"

static SDL_Window *WINDOW;
static SDL_Renderer *RENDERER;
static FC_Font *Roboto, *Roboto_large, *Roboto_small;

SDL_Window *SDL_GetWindow(void) {
	return WINDOW;
}

static FC_Font *SDL_GetFont(int size) {
	if (size == 20)
		return Roboto_small;
	else if (size == 25)
		return Roboto;
	else if (size == 30)
		return Roboto_large;
	
	return Roboto;
}

int SDL_HelperInit(void) {
	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
		return -1;

	WINDOW = SDL_CreateWindow("NX-Shell", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN);
	if (WINDOW == NULL)
		return -1;

	RENDERER = SDL_CreateRenderer(WINDOW, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	int flags = IMG_INIT_JPG | IMG_INIT_PNG;
	if ((IMG_Init(flags) & flags) != flags) {
		if (config.dev_options)
			DEBUG_LOG("IMG_Init failed: %s\n", IMG_GetError());
		
		return -1;
	}

	Roboto_small = FC_CreateFont();
	FC_LoadFont(Roboto_small, RENDERER, "romfs:/res/Roboto-Regular.ttf", 20, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

	Roboto = FC_CreateFont();
	FC_LoadFont(Roboto, RENDERER, "romfs:/res/Roboto-Regular.ttf", 25, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

	Roboto_large = FC_CreateFont();
	FC_LoadFont(Roboto_large, RENDERER, "romfs:/res/Roboto-Regular.ttf", 30, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

	return 0;
}

void SDL_HelperTerm(void) {
	FC_FreeFont(Roboto_large);
	FC_FreeFont(Roboto);
	//FC_FreeFont(Roboto_small);
	TTF_Quit();

	IMG_Quit();

	SDL_DestroyRenderer(RENDERER);
	SDL_DestroyWindow(WINDOW);
	SDL_Quit();
}

void SDL_ClearScreen(SDL_Color colour) {
	SDL_SetRenderDrawColor(RENDERER, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderClear(RENDERER);
}

void SDL_DrawRect(int x, int y, int w, int h, SDL_Color colour) {
	SDL_Rect rect;
	rect.x = x; rect.y = y; rect.w = w; rect.h = h;
	SDL_SetRenderDrawBlendMode(RENDERER, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(RENDERER, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderFillRect(RENDERER, &rect);
}

void SDL_DrawCircle(int x, int y, int r, SDL_Color colour) {
	filledCircleRGBA(RENDERER, x, y, r, colour.r, colour.g, colour.b, colour.a);
	return;
}

void SDL_DrawText(int x, int y, int size, SDL_Color colour, const char *text) {
	FC_DrawColor(SDL_GetFont(size), RENDERER, x, y, colour, text);
}

void SDL_DrawTextf(int x, int y, int size, SDL_Color colour, const char* text, ...) {
	char buffer[256];
	va_list args;
	va_start(args, text);
	vsnprintf(buffer, 256, text, args);
	SDL_DrawText(x, y, size, colour, buffer);
	va_end(args);
}

void SDL_GetTextDimensions(int size, const char *text, u32 *width, u32 *height) {
	FC_Font *font = SDL_GetFont(size);

	if (width != NULL) 
		*width = FC_GetWidth(font, text);
	if (height != NULL) 
		*height = FC_GetHeight(font, text);
}

void SDL_LoadImage(SDL_Texture **texture, char *path) {
	SDL_Surface *image = NULL;

	image = IMG_Load(path);
	if (!image) {
		if (config.dev_options)
			DEBUG_LOG("IMG_Load failed: %s\n", IMG_GetError());
		
		return;
	}
	
	SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_RGBA8888, 0);
	*texture = SDL_CreateTextureFromSurface(RENDERER, image);
	SDL_FreeSurface(image);
	image = NULL;
}

void SDL_LoadImageMem(SDL_Texture **texture, void *data, int size) {
	SDL_Surface *image = NULL;

	image = IMG_Load_RW(SDL_RWFromMem(data, size), 1);
	if (!image) {
		if (config.dev_options)
			DEBUG_LOG("IMG_Load_RW failed: %s\n", IMG_GetError());
		
		return;
	}
	
	SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_RGBA8888, 0);
	*texture = SDL_CreateTextureFromSurface(RENDERER, image);
	SDL_FreeSurface(image);
	image = NULL;
}

void SDL_DrawImage(SDL_Texture *texture, int x, int y) {
	SDL_Rect position;
	position.x = x; position.y = y;
	SDL_QueryTexture(texture, NULL, NULL, &position.w, &position.h);
	SDL_RenderCopy(RENDERER, texture, NULL, &position);
}

void SDL_DrawImageScale(SDL_Texture *texture, int x, int y, int w, int h) {
	SDL_Rect position;
	position.x = x; position.y = y; position.w = w; position.h = h;
	SDL_RenderCopy(RENDERER, texture, NULL, &position);
}
