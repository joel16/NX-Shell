#ifndef NX_SHELL_SDL_HELPER_H
#define NX_SHELL_SDL_HELPER_H

static inline SDL_Color SDL_MakeColour(Uint8 r, Uint8 g, Uint8 b)
{
	SDL_Color colour = {r, g, b};
	return colour;
}

#define WHITE                 SDL_MakeColour(255, 255, 255)
#define BLACK_BG              SDL_MakeColour(48, 48, 48)
#define STATUS_BAR_LIGHT      SDL_MakeColour(37, 79, 174)
#define STATUS_BAR_DARK       SDL_MakeColour(38, 50, 56)
#define MENU_BAR_LIGHT        SDL_MakeColour(51, 103, 214)
#define MENU_BAR_DARK         SDL_MakeColour(55, 71, 79)
#define BLACK                 SDL_MakeColour(0, 0, 0)
#define SELECTOR_COLOUR_LIGHT SDL_MakeColour(241, 241, 241)
#define SELECTOR_COLOUR_DARK  SDL_MakeColour(76, 76, 76)
#define TITLE_COLOUR          SDL_MakeColour(30, 136, 229)
#define TITLE_COLOUR_DARK     SDL_MakeColour(0, 150, 136)
#define TEXT_MIN_COLOUR_LIGHT SDL_MakeColour(32, 32, 32)
#define TEXT_MIN_COLOUR_DARK  SDL_MakeColour(185, 185, 185)
#define BAR_COLOUR            SDL_MakeColour(200, 200, 200)
#define PROGRESS_COLOUR       SDL_MakeColour(48, 174, 222)

void SDL_ClearScreen(SDL_Renderer *renderer, SDL_Color colour);
void SDL_DrawRect(SDL_Renderer *renderer, int x, int y, int w, int h, SDL_Color colour);
void SDL_DrawCircle(SDL_Renderer *renderer, int x, int y, int r, SDL_Color colour);
void SDL_DrawText(TTF_Font *font, int x, int y, SDL_Color colour, const char *text);
void SDL_LoadImage(SDL_Renderer *renderer, SDL_Texture **texture, char *path);
void SDL_LoadImageBuf(SDL_Renderer *renderer, SDL_Texture **texture, void *mem, int size);
void SDL_DrawImage(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y, int w, int h);

#endif