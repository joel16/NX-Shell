#include "common.h"
#include "SDL_helper.h"

static SDL_Window *WINDOW;
static SDL_Renderer *RENDERER;
static FC_Font *Roboto, *Roboto_large, *Roboto_small, *Roboto_OSK;
static PlFontData fontData, fontExtData;

SDL_Renderer *SDL_GetMainRenderer(void) {
	return RENDERER;
}

SDL_Window *SDL_GetMainWindow(void) {
	return WINDOW;
}

static FC_Font *GetFont(int size) {
	if (size == 20)
		return Roboto_small;
	else if (size == 25)
		return Roboto;
	else if (size == 30)
		return Roboto_large;
	else
		return Roboto_OSK;

	return Roboto;
}

Result SDL_HelperInit(void) {
	Result ret = 0;

	SDL_Init(SDL_INIT_EVERYTHING);
	WINDOW = SDL_CreateWindow("NX-Shell", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN);
	RENDERER = SDL_CreateRenderer(WINDOW, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawBlendMode(RENDERER, SDL_BLENDMODE_BLEND);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

	Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_MID);

	if (R_FAILED(ret = plGetSharedFontByType(&fontData, PlSharedFontType_Standard)))
		return ret;

	if (R_FAILED(ret = plGetSharedFontByType(&fontExtData, PlSharedFontType_NintendoExt)))
		return ret;

	Roboto = FC_CreateFont();
	FC_LoadFont_RW(Roboto, RENDERER, SDL_RWFromMem((void*)fontData.address, fontData.size), SDL_RWFromMem((void*)fontExtData.address, fontExtData.size), 1, 25, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

	Roboto_large = FC_CreateFont();
	FC_LoadFont_RW(Roboto_large, RENDERER, SDL_RWFromMem((void*)fontData.address, fontData.size), SDL_RWFromMem((void*)fontExtData.address, fontExtData.size), 1, 30, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

	Roboto_small = FC_CreateFont();
	FC_LoadFont_RW(Roboto_small, RENDERER, SDL_RWFromMem((void*)fontData.address, fontData.size), SDL_RWFromMem((void*)fontExtData.address, fontExtData.size), 1, 20, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

	Roboto_OSK = FC_CreateFont();
	FC_LoadFont_RW(Roboto_OSK, RENDERER, SDL_RWFromMem((void*)fontData.address, fontData.size), SDL_RWFromMem((void*)fontExtData.address, fontExtData.size), 1, 50, FC_MakeColor(0, 0, 0, 255), TTF_STYLE_NORMAL);

	return 0;
}

void SDL_HelperTerm(void) {
	FC_FreeFont(Roboto_OSK);
	FC_FreeFont(Roboto_small);
	FC_FreeFont(Roboto_large);
	FC_FreeFont(Roboto);
	TTF_Quit();

	Mix_Quit();

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
	SDL_SetRenderDrawColor(RENDERER, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderFillRect(RENDERER, &rect);
}

void SDL_DrawCircle(int x, int y, int r, SDL_Color colour) {
	filledCircleRGBA(RENDERER, x, y, r, colour.r, colour.g, colour.b, colour.a);
	return;
}

void SDL_DrawText(int x, int y, int size, SDL_Color colour, const char *text) {
	FC_DrawColor(GetFont(size), RENDERER, x, y, colour, text);
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
	FC_Font *font = GetFont(size);

	if (width != NULL) 
		*width = FC_GetWidth(font, text);
	if (height != NULL) 
		*height = FC_GetHeight(font, text);
}

void SDL_LoadImage(SDL_Texture **texture, char *path) {
	SDL_Surface *loaded_surface = NULL;
	loaded_surface = IMG_Load(path);

	if (loaded_surface) {
		SDL_ConvertSurfaceFormat(loaded_surface, SDL_PIXELFORMAT_RGBA8888, 0);
		*texture = SDL_CreateTextureFromSurface(RENDERER, loaded_surface);
	}

	SDL_FreeSurface(loaded_surface);
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

void SDL_Renderdisplay(void) {
	SDL_RenderPresent(RENDERER);
}
