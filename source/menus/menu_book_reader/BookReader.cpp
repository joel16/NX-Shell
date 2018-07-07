#include "BookReader.hpp"
#include "PageLayout.hpp"
#include "LandscapePageLayout.hpp"
#include "common.h"
#include <string>

extern "C" {
#include "SDL_helper.h"
#include "status_bar.h"
#include "config.h"
}

fz_context *ctx = NULL;

BookReader::BookReader(const char *path) {
    if (ctx == NULL) {
        ctx = fz_new_context(NULL, NULL, 128 << 10);
        fz_register_document_handlers(ctx);
    }
    
    doc = fz_open_document(ctx, path);
    switch_current_page_layout(_currentPageLayout);
}

BookReader::~BookReader() {
    fz_drop_document(ctx, doc);
    
    delete layout;
}

void BookReader::previous_page() {
    layout->previous_page();
    show_status_bar();
}

void BookReader::next_page() {
    layout->next_page();
    show_status_bar();
}

void BookReader::zoom_in() {
    layout->zoom_in();
    show_status_bar();
}

void BookReader::zoom_out() {
    layout->zoom_out();
    show_status_bar();
}

void BookReader::move_page_up() {
    layout->move_up();
}

void BookReader::move_page_down() {
    layout->move_down();
}

void BookReader::move_page_left() {
    layout->move_left();
}

void BookReader::move_page_right() {
    layout->move_right();
}

void BookReader::reset_page() {
    layout->reset();
    show_status_bar();
}

void BookReader::switch_page_layout() {
    switch (_currentPageLayout) {
        case BookPageLayoutPortrait:
            switch_current_page_layout(BookPageLayoutLandscape);
            break;
        case BookPageLayoutLandscape:
            switch_current_page_layout(BookPageLayoutPortrait);
            break;
    }
}

void BookReader::draw() {
    SDL_ClearScreen(RENDERER, SDL_MakeColour(33, 39, 43, 255));
    SDL_RenderClear(RENDERER);
    
    layout->draw_page();
    
#ifdef __SWITCH__
    if (--status_bar_visible_counter > 0) {
        char *title = layout->info();
        
        int title_width = 0, title_height = 0;
        TTF_SizeText(Roboto, title, &title_width, &title_height);
        
        SDL_Color color = config_dark_theme ? STATUS_BAR_DARK : STATUS_BAR_LIGHT;
        
        SDL_DrawRect(RENDERER, 0, 0, 1280, 40, SDL_MakeColour(color.r, color.g, color.b , 128));
        SDL_DrawText(RENDERER, Roboto, (1280 - title_width) / 2, (44 - title_height) / 2, WHITE, title);
        
        StatusBar_DisplayTime();
    }
#endif
    
    SDL_RenderPresent(RENDERER);
}

void BookReader::show_status_bar() {
    status_bar_visible_counter = 50;
}

void BookReader::switch_current_page_layout(BookPageLayout bookPageLayout) {
    int current_page = 0;
    
    if (layout) {
        current_page = layout->current_page();
        delete layout;
        layout = NULL;
    }
    
    _currentPageLayout = bookPageLayout;
    
    switch (bookPageLayout) {
        case BookPageLayoutPortrait:
            layout = new PageLayout(doc, current_page);
            break;
        case BookPageLayoutLandscape:
            layout = new LandscapePageLayout(doc, current_page);
            break;
    }
}
