extern "C" {
    #include "menu_book_reader.h"
    #include "touch_helper.h"
}

#include "BookReader.hpp"

void Menu_OpenBook(char *path)
{
    BookReader *reader = new BookReader(path);
    
    TouchInfo touchInfo;
    Touch_Init(&touchInfo);
    
    while(appletMainLoop())
    {
        reader->draw();
        
        hidScanInput();
        Touch_Process(&touchInfo);
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if (kDown & KEY_B)
            break;
        
        if ((kDown & KEY_DLEFT) || (kDown & KEY_L))
            reader->previous_page();
            
        if ((kDown & KEY_DRIGHT) || (kDown & KEY_R))
            reader->next_page();
            
        if (kDown & KEY_LSTICK)
            reader->reset_page();
        
        if (kDown & KEY_Y)
            reader->switch_page_layout();
        
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
            
        if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone)
        {
            float tapRegion = 120;
            
            switch (reader->currentPageLayout()) {
                case BookPageLayoutPortrait:
                    if (tapped_inside(touchInfo, 0, 0, tapRegion, 720))
                        reader->previous_page();
                    else if (tapped_inside(touchInfo, 1280 - tapRegion, 0, 1280, 720))
                        reader->next_page();
                    break;
                case BookPageLayoutLandscape:
                    if (tapped_inside(touchInfo, 0, 0, 1280, tapRegion))
                        reader->previous_page();
                    else if (tapped_inside(touchInfo, 0, 720 - tapRegion, 1280, 720))
                        reader->next_page();
                    break;
            }
        }
    }

    delete reader;
}
