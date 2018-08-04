extern "C"
{
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
        
        if (kDown & KEY_DLEFT)
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->previous_page();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->zoom_out();
        }
        else if (kDown & KEY_DRIGHT)
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->next_page();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->zoom_in();
        }

        if ((kDown & KEY_DUP) || (kHeld & KEY_RSTICK_UP))
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->zoom_in();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->previous_page();
        }
        else if ((kDown & KEY_DDOWN) || (kHeld & KEY_RSTICK_DOWN))
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->zoom_out();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->next_page();
        }

        if (kHeld & KEY_LSTICK_UP)
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->move_page_up();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->move_page_left();
        }
        else if (kHeld & KEY_LSTICK_DOWN)
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->move_page_down();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->move_page_right();
        }
        else if (kHeld & KEY_LSTICK_LEFT)
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->move_page_left();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->move_page_up();
        }
        else if (kHeld & KEY_LSTICK_RIGHT)
        {
            if (reader->currentPageLayout() == BookPageLayoutPortrait || (!hidGetHandheldMode()))
                reader->move_page_right();
            else if ((reader->currentPageLayout() == BookPageLayoutLandscape) && (hidGetHandheldMode()))
                reader->move_page_down();
        }

        if (kDown & KEY_B)
            break;
            
        if (kDown & KEY_LSTICK || kDown & KEY_RSTICK)
            reader->reset_page();
        
        if (kDown & KEY_Y)
            reader->switch_page_layout();
            
        if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone)
        {
            float tapRegion = 120;
            
            switch (reader->currentPageLayout())
            {
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
