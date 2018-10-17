#include "PageLayout.hpp"
#include "common.h"
#include <algorithm>

extern "C" {
    #include "SDL_helper.h"
}

PageLayout::PageLayout(fz_document *doc, int current_page):doc(doc),pdf(pdf_specifics(ctx, doc)),pages_count(fz_count_pages(ctx, doc)) {
    _current_page = std::min(std::max(0, current_page), pages_count - 1);
    
    SDL_RenderGetViewport(SDL_GetMainRenderer(), &viewport);
    render_page_to_texture(_current_page, false);
}

void PageLayout::previous_page(int n) {
    render_page_to_texture(_current_page - n, false);
}

void PageLayout::next_page(int n) {
    render_page_to_texture(_current_page + n, false);
}

void PageLayout::zoom_in() {
    set_zoom(zoom + 0.1);
}

void PageLayout::zoom_out() {
    set_zoom(zoom - 0.1);
}

void PageLayout::move_up() {
    move_page(0, 50);
}

void PageLayout::move_down() {
    move_page(0, -50);
}

void PageLayout::move_left() {
    move_page(-50, 0);
}

void PageLayout::move_right() {
    move_page(50, 0);
}

void PageLayout::reset() {
    page_center = fz_make_point(viewport.w / 2, viewport.h / 2);
    set_zoom(min_zoom);
}

void PageLayout::draw_page() {
    float w = page_bounds.x1 * zoom, h = page_bounds.y1 * zoom;
    
    SDL_Rect rect;
    rect.x = page_center.x - w / 2;
    rect.y = page_center.y - h / 2;
    rect.w = w;
    rect.h = h;
    
    SDL_RenderCopy(SDL_GetMainRenderer(), page_texture, NULL, &rect);
}

char *PageLayout::info() {
    static char title[128];
    sprintf(title, "%i/%i, %.2f%%", _current_page + 1, pages_count, zoom * 100);
    return title;
}

void PageLayout::render_page_to_texture(int num, bool reset_zoom) {
    FreeTextureIfNeeded(&page_texture);
    
    _current_page = std::min(std::max(0, num), pages_count - 1);
    
    fz_page *page = fz_load_page(ctx, doc, _current_page);
    fz_rect bounds = fz_bound_page(ctx, page);
    
    if (page_bounds.x1 != bounds.x1 || page_bounds.y1 != bounds.y1 || reset_zoom) {
        page_bounds = bounds;
        page_center = fz_make_point(viewport.w / 2, viewport.h / 2);
        
        min_zoom = fmin(viewport.w / bounds.x1, viewport.h / bounds.y1);
        max_zoom = fmax(viewport.w / bounds.x1, viewport.h / bounds.y1);
        zoom = min_zoom;
    }
    
    fz_pixmap *pix = fz_new_pixmap_from_page_contents(ctx, page, fz_scale(zoom, zoom), fz_device_rgb(ctx), 0);
    SDL_Surface *image = SDL_CreateRGBSurfaceFrom(pix->samples, pix->w, pix->h, pix->n * 8, pix->w * pix->n, 0x000000FF, 0x0000FF00, 0x00FF0000, 0);
    page_texture = SDL_CreateTextureFromSurface(SDL_GetMainRenderer(), image);
    
    SDL_FreeSurface(image);
    fz_drop_pixmap(ctx, pix);
    
    fz_drop_page(ctx, page);
}

void PageLayout::set_zoom(float value) {
    value = fmin(fmax(min_zoom, value), max_zoom);
    
    if (value == zoom)
        return;
    
    zoom = value;
    
    render_page_to_texture(_current_page, false);
    move_page(0, 0);
}

void PageLayout::move_page(float x, float y) {
    float w = page_bounds.x1 * zoom, h = page_bounds.y1 * zoom;
    
    page_center.x = fmin(fmax(page_center.x + x, w / 2), viewport.w - w / 2);
    page_center.y = fmin(fmax(page_center.y + y, viewport.h - h / 2), h / 2);
}
