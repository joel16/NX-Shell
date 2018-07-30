#ifndef NX_SHELL_LANDSCAPE_PAGE_LAYOUT_HPP
#define NX_SHELL_LANDSCAPE_PAGE_LAYOUT_HPP

#include "PageLayout.hpp"

class LandscapePageLayout: public PageLayout
{
    public:
        LandscapePageLayout(fz_document *doc, int current_page);
    
        void reset();
        void draw_page();
    
    protected:
        void move_page(float x, float y);
};

#endif
