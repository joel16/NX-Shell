extern "C" {
#include "menu_book_reader.h"
}

#include "BookReader.hpp"

void Menu_OpenBook(char *path)
{
    BookReader *reader = new BookReader(path);
    
    while(appletMainLoop())
    {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
        
        if (kDown & KEY_B)
            break;
        
        if (kDown & KEY_DLEFT)
            reader->previous_page();
        
        if (kDown & KEY_DRIGHT)
            reader->next_page();
        
        if (kDown & KEY_LSTICK)
            reader->reset_page();
        
        if (kHeld & KEY_DUP)
            reader->zoom_in();
        
        if (kHeld & KEY_DDOWN)
            reader->zoom_out();
        
        if (kHeld & KEY_LSTICK_UP)
            reader->move_page_up();
        
        if (kHeld & KEY_LSTICK_DOWN)
            reader->move_page_down();
        
        if (kHeld & KEY_LSTICK_LEFT)
            reader->move_page_left();
        
        if (kHeld & KEY_LSTICK_RIGHT)
            reader->move_page_right();
        
        reader->draw();
    }
    
    delete reader;
}
