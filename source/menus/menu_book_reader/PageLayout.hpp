#ifndef NX_SHELL_PAGE_LAYOUT_HPP
#define NX_SHELL_PAGE_LAYOUT_HPP

#include <mupdf/pdf.h>
#include <SDL2/SDL.h>

extern fz_context *ctx;

static inline void FreeTextureIfNeeded(SDL_Texture **texture)
{
    if (texture && *texture)
    {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }
}

class PageLayout
{
    public:
        PageLayout(fz_document *doc, int current_page);
    
        int current_page()
        {
            return _current_page;
        }
    
        virtual void previous_page(int n);
        virtual void next_page(int n);
        virtual void zoom_in();
        virtual void zoom_out();
        virtual void move_up();
        virtual void move_down();
        virtual void move_left();
        virtual void move_right();
        virtual void reset();
        virtual void draw_page();
        virtual char* info();
    
    protected:
        virtual void render_page_to_texture(int num, bool reset_zoom);
        virtual void set_zoom(float value);
        virtual void move_page(float x, float y);
    
        fz_document *doc = NULL;
        pdf_document *pdf = NULL;
        const int pages_count = 0;
    
        int _current_page = 0;
        fz_rect page_bounds = fz_empty_rect;
        fz_point page_center = fz_make_point(0, 0);
        float min_zoom = 1, max_zoom = 1, zoom = 1;
    
        SDL_Rect viewport;
        SDL_Texture *page_texture = NULL;
};

#endif
