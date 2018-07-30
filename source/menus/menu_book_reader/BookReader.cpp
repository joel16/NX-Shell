#include "BookReader.hpp"
#include "PageLayout.hpp"
#include "LandscapePageLayout.hpp"
#include "common.h"
#include <algorithm>
#include <libconfig.h>

extern "C" 
{
    #include "SDL_helper.h"
    #include "status_bar.h"
    #include "config.h"
}

fz_context *ctx = NULL;
config_t *config = NULL;

static int load_last_page(const char *book_name) 
{
    if (!config)
    {
        config = (config_t *)malloc(sizeof(config_t));
        config_init(config);
        config_read_file(config, "/switch/NX-Shell/last_book_pages.cfg");
    }
    
    config_setting_t *setting = config_setting_get_member(config_root_setting(config), book_name);
    
    if (setting)
        return config_setting_get_int(setting);

    return 0;
}

static void save_last_page(const char *book_name, int current_page)
{
    config_setting_t *setting = config_setting_get_member(config_root_setting(config), book_name);
    
    if (!setting)
        setting = config_setting_add(config_root_setting(config), book_name, CONFIG_TYPE_INT);
    
    if (setting)
    {
        config_setting_set_int(setting, current_page);
        config_write_file(config, "/switch/NX-Shell/last_book_pages.cfg");
    }
}

BookReader::BookReader(const char *path)
{
    if (ctx == NULL)
    {
        ctx = fz_new_context(NULL, NULL, 128 << 10);
        fz_register_document_handlers(ctx);
    }
    
    book_name = std::string(path).substr(std::string(path).find_last_of("/\\") + 1);
    
    std::string invalid_chars = " :/?#[]@!$&'()*+,;=.";
    for (char& c: invalid_chars)
        book_name.erase(std::remove(book_name.begin(), book_name.end(), c), book_name.end());
    
    doc = fz_open_document(ctx, path);
    
    int current_page = load_last_page(book_name.c_str());
    switch_current_page_layout(_currentPageLayout, current_page);
    
    if (current_page > 0)
        show_status_bar();
}

BookReader::~BookReader()
{
    fz_drop_document(ctx, doc);
    
    delete layout;
}

void BookReader::previous_page()
{
    layout->previous_page();
    show_status_bar();
    save_last_page(book_name.c_str(), layout->current_page());
}

void BookReader::next_page()
{
    layout->next_page();
    show_status_bar();
    save_last_page(book_name.c_str(), layout->current_page());
}

void BookReader::zoom_in()
{
    layout->zoom_in();
    show_status_bar();
}

void BookReader::zoom_out()
{
    layout->zoom_out();
    show_status_bar();
}

void BookReader::move_page_up()
{
    layout->move_up();
}

void BookReader::move_page_down()
{
    layout->move_down();
}

void BookReader::move_page_left()
{
    layout->move_left();
}

void BookReader::move_page_right()
{
    layout->move_right();
}

void BookReader::reset_page()
{
    layout->reset();
    show_status_bar();
}

void BookReader::switch_page_layout()
{
    switch (_currentPageLayout)
    {
        case BookPageLayoutPortrait:
            switch_current_page_layout(BookPageLayoutLandscape, 0);
            break;
        case BookPageLayoutLandscape:
            switch_current_page_layout(BookPageLayoutPortrait, 0);
            break;
    }
}

void BookReader::draw()
{
    SDL_ClearScreen(RENDERER, SDL_MakeColour(33, 39, 43, 255));
    SDL_RenderClear(RENDERER);
    
    layout->draw_page();
    
#ifdef __SWITCH__
    if (--status_bar_visible_counter > 0) 
    {
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

void BookReader::show_status_bar()
{
    status_bar_visible_counter = 50;
}

void BookReader::switch_current_page_layout(BookPageLayout bookPageLayout, int current_page)
{
    if (layout)
    {
        current_page = layout->current_page();
        delete layout;
        layout = NULL;
    }
    
    _currentPageLayout = bookPageLayout;
    
    switch (bookPageLayout)
    {
        case BookPageLayoutPortrait:
            layout = new PageLayout(doc, current_page);
            break;
        case BookPageLayoutLandscape:
            layout = new LandscapePageLayout(doc, current_page);
            break;
    }
}
