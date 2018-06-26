#include "BookReader.hpp"
#include "common.h"
#include <string>

extern "C" {
#include "SDL_helper.h"
#include "status_bar.h"
#include "config.h"
}

fz_context *ctx = NULL;

static inline void FreeTextureIfNeeded(SDL_Texture **texture) {
    if (texture && *texture) {
        SDL_DestroyTexture(*texture);
        *texture = NULL;
    }
}

BookReader::BookReader(const char *path) {
    if (ctx == NULL) {
        ctx = fz_new_context(NULL, NULL, 128 << 10);
        fz_register_document_handlers(ctx);
    }
    
    doc = fz_open_document(ctx, path);
    pdf = pdf_specifics(ctx, doc);
    
    pages_count = fz_count_pages(ctx, doc);
    
    SDL_Rect viewport;
    SDL_RenderGetViewport(RENDERER, &viewport);
    screen_bounds = fz_make_rect(0, 0, viewport.w, viewport.h);
    
    load_page(0);
}

BookReader::~BookReader() {
    fz_drop_document(ctx, doc);
    
    FreeTextureIfNeeded(&page_texture);
}

void BookReader::previous_page() {
    load_page(current_page - 1);
}

void BookReader::next_page() {
    load_page(current_page + 1);
}

void BookReader::zoom_in() {
    set_zoom(zoom + 0.1);
}

void BookReader::zoom_out() {
    set_zoom(zoom - 0.1);
}

void BookReader::move_page_up() {
    move_page(0, -50);
}

void BookReader::move_page_down() {
    move_page(0, 50);
}

void BookReader::move_page_left() {
    move_page(-50, 0);
}

void BookReader::move_page_right() {
    move_page(50, 0);
}

void BookReader::reset_page() {
    page_center = fz_make_point(screen_bounds.x1 / 2, screen_bounds.y1 / 2);
    set_zoom(min_zoom);
}

void BookReader::draw() {
    float w = page_bounds.x1 * zoom, h = page_bounds.y1 * zoom;
    
    SDL_Rect rect;
    rect.x = page_center.x - w / 2;
    rect.y = page_center.y - h / 2;
    rect.w = w;
    rect.h = h;
    
    SDL_ClearScreen(RENDERER, SDL_MakeColour(33, 39, 43, 255));
    SDL_RenderClear(RENDERER);
    
    SDL_RenderCopy(RENDERER, page_texture, NULL, &rect);
    
    if (--status_bar_visible_counter > 0) {
        char title[128];
        sprintf(title, "%i/%i, %.2f%%", current_page + 1, pages_count, zoom * 100);
        
        int title_width = 0, title_height = 0;
        TTF_SizeText(Roboto, title, &title_width, &title_height);
        
        SDL_Color color = config_dark_theme ? STATUS_BAR_DARK : STATUS_BAR_LIGHT;
        
        SDL_DrawRect(RENDERER, 0, 0, 1280, 40, SDL_MakeColour(color.r, color.g, color.b , 128));
        SDL_DrawText(Roboto, (screen_bounds.x1 - title_width) / 2, (44 - title_height) / 2, WHITE, title);
        
        StatusBar_DisplayTime();
    }
    
    SDL_RenderPresent(RENDERER);
}

void BookReader::load_page(int num) {
    current_page = std::min(std::max(0, num), pages_count - 1);
    
    fz_drop_page(ctx, page);
    page = fz_load_page(ctx, doc, current_page);
    
    fz_rect bounds;
    fz_bound_page(ctx, page, &bounds);
    
    if (page_bounds.x1 != bounds.x1 || page_bounds.y1 != bounds.y1) {
        page_bounds = bounds;
        page_center = fz_make_point(screen_bounds.x1 / 2, screen_bounds.y1 / 2);
        
        min_zoom = fmin(screen_bounds.x1 / bounds.x1, screen_bounds.y1 / bounds.y1);
        max_zoom = fmax(screen_bounds.x1 / bounds.x1, screen_bounds.y1 / bounds.y1);
        zoom = min_zoom;
    }
    
    render_page_to_texture();
    
    status_bar_visible_counter = 50;
}

void BookReader::render_page_to_texture() {
    FreeTextureIfNeeded(&page_texture);
    
    fz_matrix m = fz_identity;
    fz_scale(&m, zoom, zoom);
    
    fz_pixmap *pix = fz_new_pixmap_from_page_contents(ctx, page, &m, fz_device_rgb(ctx), 0);
    SDL_Surface *image = SDL_CreateRGBSurfaceFrom(pix->samples, pix->w, pix->h, pix->n * 8, pix->w * pix->n, 0x000000FF, 0x0000FF00, 0x00FF0000, 0);
    page_texture = SDL_CreateTextureFromSurface(RENDERER, image);
    
    SDL_FreeSurface(image);
    fz_drop_pixmap(ctx, pix);
}

void BookReader::set_zoom(float value) {
    value = fmin(fmax(min_zoom, value), max_zoom);
    
    if (value == zoom) return;
    
    zoom = value;
    
    load_page(current_page);
    move_page(0, 0);
    status_bar_visible_counter = 50;
}

void BookReader::move_page(float x, float y) {
    float w = page_bounds.x1 * zoom, h = page_bounds.y1 * zoom;
    
    page_center.x = fmin(fmax(page_center.x + x, w / 2), screen_bounds.x1 - w / 2);
    page_center.y = fmin(fmax(page_center.y + y, screen_bounds.y1 - h / 2), h / 2);
}
