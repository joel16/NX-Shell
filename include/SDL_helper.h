#ifndef NX_SHELL_SDL_HELPER_H
#define NX_SHELL_SDL_HELPER_H

static inline SDL_Color SDL_MakeColour(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_Color colour = {r, g, b, a};
	return colour;
}

#define WHITE                 SDL_MakeColour(255, 255, 255, 255)
#define BLACK_BG              SDL_MakeColour(48, 48, 48, 255)
#define STATUS_BAR_LIGHT      SDL_MakeColour(37, 79, 174, 255)
#define STATUS_BAR_DARK       SDL_MakeColour(38, 50, 56, 255)
#define MENU_BAR_LIGHT        SDL_MakeColour(51, 103, 214, 255)
#define MENU_BAR_DARK         SDL_MakeColour(55, 71, 79, 255)
#define BLACK                 SDL_MakeColour(0, 0, 0, 255)
#define SELECTOR_COLOUR_LIGHT SDL_MakeColour(241, 241, 241, 255)
#define SELECTOR_COLOUR_DARK  SDL_MakeColour(76, 76, 76, 255)
#define TITLE_COLOUR          SDL_MakeColour(30, 136, 229, 255)
#define TITLE_COLOUR_DARK     SDL_MakeColour(0, 150, 136, 255)
#define TEXT_MIN_COLOUR_LIGHT SDL_MakeColour(32, 32, 32, 255)
#define TEXT_MIN_COLOUR_DARK  SDL_MakeColour(185, 185, 185, 255)
#define BAR_COLOUR            SDL_MakeColour(200, 200, 200, 255)

void SDL_ClearScreen(SDL_Renderer *renderer, SDL_Color colour);
void SDL_DrawRect(SDL_Renderer *renderer, int x, int y, int w, int h, SDL_Color colour);
void SDL_DrawCircle(SDL_Renderer *renderer, int x, int y, int r, SDL_Color colour);
void SDL_DrawText(SDL_Renderer *renderer, TTF_Font *font, int x, int y, SDL_Color colour, const char *text);
void SDL_DrawTextf(SDL_Renderer *renderer, TTF_Font *font, int x, int y, SDL_Color colour, const char* text, ...);
void SDL_LoadImage(SDL_Renderer *renderer, SDL_Texture **texture, char *path);
void SDL_LoadImageBuf(SDL_Renderer *renderer, SDL_Texture **texture, void *mem, int size);
void SDL_DrawImage(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y);
void SDL_DrawImageScale(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y, int w, int h);

#endif