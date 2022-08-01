#ifndef NX_SHELL_GUI_H
#define NX_SHELL_GUI_H

namespace GUI {
    bool Init(void);
    bool SwapBuffers(void);
    bool Loop(u64 &key);
    void Render(void);
    void Exit(void);
}

#endif
