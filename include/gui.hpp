#pragma once

namespace GUI {
    bool Init(void);
    bool SwapBuffers(void);
    bool Loop(u64 &key);
    void Render(void);
    void Exit(void);
}
