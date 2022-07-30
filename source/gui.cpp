#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glad/glad.h>
#include <cstdio>
#include <switch.h>

#include "gui.h"
#include "imgui.h"
#include "imgui_impl_switch.h"
#include "log.h"

namespace GUI {
    static EGLDisplay s_display = EGL_NO_DISPLAY;
    static EGLContext s_context = EGL_NO_CONTEXT;
    static EGLSurface s_surface = EGL_NO_SURFACE;
    
    static bool InitEGL(NWindow* win) {
        s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        
        if (!s_display) {
            Log::Error("Could not connect to display! error: %d", eglGetError());
            return false;
        }
        
        eglInitialize(s_display, nullptr, nullptr);
        
        if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
            Log::Error("Could not set API! error: %d", eglGetError());
            eglTerminate(s_display);
            s_display = nullptr;
        }
        
        EGLConfig config;
        EGLint numConfigs;
        static const EGLint framebufferAttributeList[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_RED_SIZE,     8,
            EGL_GREEN_SIZE,   8,
            EGL_BLUE_SIZE,    8,
            EGL_ALPHA_SIZE,   8,
            EGL_DEPTH_SIZE,   24,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE
        };
        
        eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);
        if (numConfigs == 0) {
            Log::Error("No config found! error: %d", eglGetError());
            eglTerminate(s_display);
            s_display = nullptr;
        }
        
        s_surface = eglCreateWindowSurface(s_display, config, win, nullptr);
        if (!s_surface) {
            Log::Error("Surface creation failed! error: %d", eglGetError());
            eglTerminate(s_display);
            s_display = nullptr;
        }
        
        static const EGLint contextAttributeList[] = {
            EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
            EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
            EGL_CONTEXT_MINOR_VERSION_KHR, 3,
            EGL_NONE
        };
        
        s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
        if (!s_context) {
            Log::Error("Context creation failed! error: %d", eglGetError());
            eglDestroySurface(s_display, s_surface);
            s_surface = nullptr;
        }
        
        eglMakeCurrent(s_display, s_surface, s_surface, s_context);
        return true;
    }
    
    static void ExitEGL(void) {
        if (s_display) {
            eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            
            if (s_context) {
                eglDestroyContext(s_display, s_context);
                s_context = nullptr;
            }
            
            if (s_surface) {
                eglDestroySurface(s_display, s_surface);
                s_surface = nullptr;
            }
            
            eglTerminate(s_display);
            s_display = nullptr;
        }
    }

    bool Init(void) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(1280.f, 720.f);
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        
        if (!InitEGL(nwindowGetDefault()))
            return false;
        
        gladLoadGL();
        
        ImGui_ImplSwitch_Init("#version 130");
        
        // Load nintendo font
        PlFontData standard, extended, chinese, korean;
        static ImWchar extended_range[] = {0xE000, 0xE152};
        
        if (R_SUCCEEDED(plGetSharedFontByType(&standard,     PlSharedFontType_Standard)) &&
            R_SUCCEEDED(plGetSharedFontByType(&extended, PlSharedFontType_NintendoExt)) &&
            R_SUCCEEDED(plGetSharedFontByType(&chinese,  PlSharedFontType_ChineseSimplified)) &&
            R_SUCCEEDED(plGetSharedFontByType(&korean,   PlSharedFontType_KO))) {
                
            u8 *px = nullptr;
            int w = 0, h = 0, bpp = 0;
            ImFontConfig font_cfg;
            
            font_cfg.FontDataOwnedByAtlas = false;
            io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 20.f, &font_cfg, io.Fonts->GetGlyphRangesDefault());
            font_cfg.MergeMode = true;
            io.Fonts->AddFontFromMemoryTTF(extended.address, extended.size, 20.f, &font_cfg, extended_range);
            io.Fonts->AddFontFromMemoryTTF(chinese.address,  chinese.size,  20.f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
            io.Fonts->AddFontFromMemoryTTF(korean.address,   korean.size,   20.f, &font_cfg, io.Fonts->GetGlyphRangesKorean());
            
            // build font atlas
            io.Fonts->GetTexDataAsAlpha8(&px, &w, &h, &bpp);
            io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
            io.Fonts->Build();
        }

        return true;
    }
    
    bool Loop(u64 &key) {
        if (!appletMainLoop())
            return false;
        
        key = ImGui_ImplSwitch_NewFrame();
        ImGui::NewFrame();
        return !(key & HidNpadButton_Plus);
    }
    
    void Render(void) {
        ImGui::Render();
        ImGuiIO &io = ImGui::GetIO(); (void)io;
        glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
        glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplSwitch_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(s_display, s_surface);
    }
    
    void Exit(void) {
        ImGui_ImplSwitch_Shutdown();
        ExitEGL();
    }
}
