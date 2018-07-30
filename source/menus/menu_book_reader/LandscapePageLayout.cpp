#include "LandscapePageLayout.hpp"
#include "common.h"
#include <algorithm>

LandscapePageLayout::LandscapePageLayout(fz_document *doc, int current_page):PageLayout(doc, current_page)
{
    int w = viewport.w;
    viewport.w = viewport.h;
    viewport.h = w;
    
    render_page_to_texture(_current_page, true);
    reset();
}

void LandscapePageLayout::reset()
{
    page_center = fz_make_point(viewport.h / 2, viewport.w / 2);
    set_zoom(min_zoom);
};

void LandscapePageLayout::draw_page()
{
    float w = page_bounds.x1 * zoom, h = page_bounds.y1 * zoom;
    
    SDL_Rect rect;
    rect.y = page_center.y - h / 2;
    rect.x = page_center.x - w / 2;
    rect.w = w;
    rect.h = h;
    
    SDL_RenderCopyEx(RENDERER, page_texture, NULL, &rect, 90, NULL, SDL_FLIP_NONE);
}

void LandscapePageLayout::move_page(float x, float y)
{
    float w = page_bounds.x1 * zoom, h = page_bounds.y1 * zoom;
    
    page_center.x = fmin(fmax(page_center.x + y, h / 2), viewport.h - h / 2);
    page_center.y = fmin(fmax(page_center.y + x, viewport.w - w / 2), w / 2);
}
