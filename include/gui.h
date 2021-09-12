#ifndef NX_SHELL_GUI_H
#define NX_SHELL_GUI_H

#include <deko3d.hpp>

extern dk::UniqueDevice s_device;
extern dk::UniqueQueue s_queue;
extern dk::UniqueMemBlock s_imageMemBlock;
extern dk::UniqueCmdBuf s_cmdBuf[2u];
extern dk::SamplerDescriptor *s_samplerDescriptors;
extern dk::ImageDescriptor *s_imageDescriptors;

namespace GUI {
    bool Init(void);
    bool Loop(void);
    void Render(void);
    void Exit(void);
}

#endif
