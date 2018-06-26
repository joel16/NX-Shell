#ifndef NX_SHELL_BOOK_READER_HPP
#define NX_SHELL_BOOK_READER_HPP

#include <mupdf/pdf.h>

struct SDL_Texture;

class BookReader {
public:
    BookReader(const char *path);
    ~BookReader();
    
    void previous_page();
    void next_page();
    void zoom_in();
    void zoom_out();
    void move_page_up();
    void move_page_down();
    void move_page_left();
    void move_page_right();
    void reset_page();
    void draw();
    
private:
    void load_page(int num);
    void render_page_to_texture();
    void set_zoom(float value);
    void move_page(float x, float y);
    
    fz_document *doc = NULL;
    pdf_document *pdf = NULL;
    fz_page *page = NULL;
    fz_rect page_bounds = fz_empty_rect, screen_bounds = fz_empty_rect;
    fz_point page_center = fz_make_point(0, 0);
    
    SDL_Texture *page_texture = NULL;
    
    int current_page = -1, pages_count = 0;
    float min_zoom = 1, max_zoom = 1, zoom = 1;
    
    int status_bar_visible_counter = 0;
};

#endif
