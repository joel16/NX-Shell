#ifndef NX_SHELL_SDL_HELPER_H
#define NX_SHELL_SDL_HELPER_H

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h> 
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "SDL_FontCache.h"

#define WHITE                 FC_MakeColor(255, 255, 255, 255)
#define BLACK_BG              FC_MakeColor(48, 48, 48, 255)
#define STATUS_BAR_LIGHT      FC_MakeColor(37, 79, 174, 255)
#define STATUS_BAR_DARK       FC_MakeColor(38, 50, 56, 255)
#define MENU_BAR_LIGHT        FC_MakeColor(51, 103, 214, 255)
#define MENU_BAR_DARK         FC_MakeColor(55, 71, 79, 255)
#define BLACK                 FC_MakeColor(0, 0, 0, 255)
#define SELECTOR_COLOUR_LIGHT FC_MakeColor(241, 241, 241, 255)
#define SELECTOR_COLOUR_DARK  FC_MakeColor(76, 76, 76, 255)
#define TITLE_COLOUR          FC_MakeColor(30, 136, 229, 255)
#define TITLE_COLOUR_DARK     FC_MakeColor(0, 150, 136, 255)
#define TEXT_MIN_COLOUR_LIGHT FC_MakeColor(32, 32, 32, 255)
#define TEXT_MIN_COLOUR_DARK  FC_MakeColor(185, 185, 185, 255)
#define BAR_COLOUR            FC_MakeColor(200, 200, 200, 255)

SDL_Renderer *SDL_GetMainRenderer(void);
SDL_Window *SDL_GetMainWindow(void);
Result SDL_HelperInit(void);
void SDL_HelperTerm(void);
void SDL_ClearScreen(SDL_Color colour);
void SDL_DrawRect(int x, int y, int w, int h, SDL_Color colour);
void SDL_DrawCircle(int x, int y, int r, SDL_Color colour);
void SDL_DrawText(int x, int y, int size, SDL_Color colour, const char *text);
void SDL_DrawTextf(int x, int y, int size, SDL_Color colour, const char* text, ...);
void SDL_GetTextDimensions(int size, const char *text, u32 *width, u32 *height);
void SDL_LoadImage(SDL_Texture **texture, char *path);
void SDL_LoadImageBuf(SDL_Texture **texture, void *mem, int size);
void SDL_DrawImage(SDL_Texture *texture, int x, int y);
void SDL_DrawImageScale(SDL_Texture *texture, int x, int y, int w, int h);
void SDL_Renderdisplay(void);

#endif