#include <cstdio>
#include <cstring>

// GL includes
#include <glad/glad.h>

#include "imgui_impl_switch.hpp"

// Vertex arrays are not supported on ES2/WebGL1 unless Emscripten which uses an extension
#ifndef IMGUI_IMPL_OPENGL_ES2
#define IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
#endif

// Desktop GL 2.0+ has glPolygonMode() which GL ES and WebGL don't have.
#ifdef GL_POLYGON_MODE
#define IMGUI_IMPL_HAS_POLYGON_MODE
#endif

// Desktop GL 3.2+ has glDrawElementsBaseVertex() which GL ES and WebGL don't have.
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_2)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
#endif

// Desktop GL 3.3+ has glBindSampler()
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_3)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
#endif

// Desktop GL 3.1+ has GL_PRIMITIVE_RESTART state
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_1)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
#endif

// Desktop GL use extension detection
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_EXTENSIONS
#endif

// OpenGL Data
struct ImGui_ImplSwitch_Data {
    GLuint GlVersion;            // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
    char GlslVersionString[32];  // Specified by user or detected based on compile time GL settings.
    GLuint FontTexture;
    GLuint ShaderHandle;
    GLint AttribLocationTex;     // Uniforms location
    GLint AttribLocationProjMtx;
    GLuint AttribLocationVtxPos; // Vertex attributes location
    GLuint AttribLocationVtxUV;
    GLuint AttribLocationVtxColor;
    unsigned int VboHandle, ElementsHandle;
    GLsizeiptr VertexBufferSize;
    GLsizeiptr IndexBufferSize;
    bool HasClipOrigin;
    bool UseBufferSubData;
    PadState pad;
    u64 start_time = 0;
    float prev_time = 0.f;

    ImGui_ImplSwitch_Data() { std::memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplSwitch_Data *ImGui_ImplSwitch_GetBackendData(void) {
    return ImGui::GetCurrentContext() ? (ImGui_ImplSwitch_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// OpenGL vertex attribute state (for ES 1.0 and ES 2.0 only)
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
struct ImGui_ImplSwitch_VtxAttribState {
    GLint Enabled, Size, Type, Normalized, Stride;
    GLvoid *Ptr;

    void GetState(GLint index) {
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &Enabled);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &Size);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &Type);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &Normalized);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &Stride);
        glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &Ptr);
    }

    void SetState(GLint index) {
        glVertexAttribPointer(index, Size, Type, static_cast<GLboolean>(Normalized), Stride, Ptr);
        
        if (Enabled)
            glEnableVertexAttribArray(index);
        else
            glDisableVertexAttribArray(index);
    }
};
#endif

// Functions
bool ImGui_ImplSwitch_Init(const char *glsl_version) {
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplSwitch_Data *bd = IM_NEW(ImGui_ImplSwitch_Data)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "imgui_impl_switch";
    io.IniFilename = nullptr;

    // Query for GL version (e.g. 320 for GL 3.2)
#if !defined(IMGUI_IMPL_OPENGL_ES2)
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major == 0 && minor == 0) {
        // Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
        const char *gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
        std::sscanf(gl_version, "%d.%d", &major, &minor);
    }
    bd->GlVersion = static_cast<GLuint>(major * 100 + minor * 10);

    //printf("GL_MAJOR_VERSION = %d\nGL_MINOR_VERSION = %d\nGL_VENDOR = '%s'\nGL_RENDERER = '%s'\n", major, minor, (const char*)glGetString(GL_VENDOR), (const char*)glGetString(GL_RENDERER)); // [DEBUG]
#else
    bd->GlVersion = 200; // GLES 2
#endif

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
    if (bd->GlVersion >= 320)
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
#endif

    // Store GLSL version string so we can refer to it later in case we recreate shaders.
    // Note: GLSL version is NOT the same as GL version. Leave this to nullptr if unsure.
    if (glsl_version == nullptr) {
#if defined(IMGUI_IMPL_OPENGL_ES2)
        glsl_version = "#version 100";
#elif defined(IMGUI_IMPL_OPENGL_ES3)
        glsl_version = "#version 300 es";
#else
        glsl_version = "#version 130";
#endif
    }
    IM_ASSERT(static_cast<int>(strlen(glsl_version)) + 2 < IM_ARRAYSIZE(bd->GlslVersionString));
    strcpy(bd->GlslVersionString, glsl_version);
    strcat(bd->GlslVersionString, "\n");

    // Make an arbitrary GL call (we don't actually need the result)
    // IF YOU GET A CRASH HERE: it probably means the OpenGL function loader didn't do its job. Let us know!
    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    // Detect extensions we support
    bd->HasClipOrigin = (bd->GlVersion >= 450);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_EXTENSIONS
    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (GLint i = 0; i < num_extensions; i++) {
        const char* extension = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
        if (extension != nullptr && strcmp(extension, "GL_ARB_clip_control") == 0)
            bd->HasClipOrigin = true;
    }
#endif

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&bd->pad);

    // Initialize start_time
    bd->start_time = armGetSystemTick();

    return true;
}

void ImGui_ImplSwitch_Shutdown(void) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplSwitch_DestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    IM_DELETE(bd);
}

static u64 ImGui_ImplSwitch_UpdateGamepads(void) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();
    ImGuiIO &io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return -1;
        
    // Get gamepad
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    
    padUpdate(&bd->pad);
    HidAnalogStickState r_stick = padGetStickPos(&bd->pad, 1);

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    
    // Update gamepad inputs
    #define IM_SATURATE(V)                      (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
    #define MAP_BUTTON(KEY_NO, BUTTON_NO)       { io.AddKeyEvent(KEY_NO, (padGetButtons(&bd->pad) & BUTTON_NO)); }
    #define MAP_ANALOG(KEY_NO, AXIS_NO, V0, V1) { float vn = (float)(AXIS_NO - V0) / (float)(V1 - V0); vn = IM_SATURATE(vn); io.AddKeyAnalogEvent(KEY_NO, vn > 0.1f, vn); }
    const int thumb_dead_zone = 8000;           // SDL_gamecontroller.h suggests using this value.
    MAP_BUTTON(ImGuiKey_GamepadStart,           HidNpadButton_A);
    MAP_BUTTON(ImGuiKey_GamepadBack,            HidNpadButton_B);
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,        HidNpadButton_A);
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,       HidNpadButton_B);
    // MAP_BUTTON(ImGuiKey_GamepadFaceLeft,        HidNpadButton_Y);
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,          HidNpadButton_X);
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,        HidNpadButton_Left);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,       HidNpadButton_Right)
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,          HidNpadButton_Up);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,        HidNpadButton_Down);
    MAP_BUTTON(ImGuiKey_GamepadL1,              HidNpadButton_L);
    MAP_BUTTON(ImGuiKey_GamepadR1,              HidNpadButton_R);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,      r_stick.x, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight,     r_stick.x, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,        r_stick.y, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,      r_stick.y, -thumb_dead_zone, -32767);
    #undef MAP_BUTTON
    #undef MAP_ANALOG

    return padGetButtonsDown(&bd->pad);
}

u64 ImGui_ImplSwitch_NewFrame(void) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplSwitch_Init()?");

    if (!bd->ShaderHandle)
        ImGui_ImplSwitch_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();
	
	// Setup display size (every frame to accommodate for window resizing)
	int w = 0, h = 0;
	int display_w = 0, display_h = 0;
	
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	w = display_w = viewport[2];
	h = display_h = viewport[3];
	
	io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
	
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2(static_cast<float>(display_w / w),static_cast<float>(display_h) / h);

    u64 elapsed_time = armGetSystemTick() - bd->start_time;
    float curr_time = (elapsed_time * 625 / 12) / 1000000000.0;
    io.DeltaTime = curr_time - bd->prev_time;
    bd->prev_time = curr_time;

    return ImGui_ImplSwitch_UpdateGamepads();
}

static void ImGui_ImplSwitch_SetupRenderState(ImDrawData * draw_data, int fb_width, int fb_height, GLuint vertex_array_object) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    if (bd->GlVersion >= 310)
        glDisable(GL_PRIMITIVE_RESTART);
#endif
#ifdef IMGUI_IMPL_HAS_POLYGON_MODE
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
#if defined(GL_CLIP_ORIGIN)
    bool clip_origin_lower_left = true;
    
    if (bd->HasClipOrigin) {
        GLenum current_clip_origin = 0;
        glGetIntegerv(GL_CLIP_ORIGIN, static_cast<GLint *>(&current_clip_origin));

        if (current_clip_origin == GL_UPPER_LEFT)
            clip_origin_lower_left = false;
    }
#endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    glViewport(0, 0, static_cast<GLsizei>(fb_width), static_cast<GLsizei>(fb_height));
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
#if defined(GL_CLIP_ORIGIN)
    if (!clip_origin_lower_left) { float tmp = T; T = B; B = tmp; } // Swap top and bottom if origin is upper left
#endif
    const float ortho_projection[4][4] = {
        { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
        { 0.0f,         0.0f,        -1.0f,   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
    };
    glUseProgram(bd->ShaderHandle);
    glUniform1i(bd->AttribLocationTex, 0);
    glUniformMatrix4fv(bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    if (bd->GlVersion >= 330)
        glBindSampler(0, 0); // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
#endif

    (void)vertex_array_object;
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindVertexArray(vertex_array_object);
#endif

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    glBindBuffer(GL_ARRAY_BUFFER, bd->VboHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle);
    glEnableVertexAttribArray(bd->AttribLocationVtxPos);
    glEnableVertexAttribArray(bd->AttribLocationVtxUV);
    glEnableVertexAttribArray(bd->AttribLocationVtxColor);
    glVertexAttribPointer(bd->AttribLocationVtxPos,   2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), static_cast<GLvoid *>(IM_OFFSETOF(ImDrawVert, pos)));
    glVertexAttribPointer(bd->AttribLocationVtxUV,    2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<GLvoid *>(IM_OFFSETOF(ImDrawVert, uv)));
    glVertexAttribPointer(bd->AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(ImDrawVert), reinterpret_cast<GLvoid *>(IM_OFFSETOF(ImDrawVert, col)));
}

// OpenGL3 Render function.
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly.
// This is in order to be able to run within an OpenGL engine that doesn't do so.
void ImGui_ImplSwitch_RenderDrawData(ImDrawData *draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();

    // Backup GL state
    GLenum last_active_texture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint *>(&last_active_texture));
    glActiveTexture(GL_TEXTURE0);
    
    GLuint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint *>(&last_program));
    
    GLuint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint *>(&last_texture));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    GLuint last_sampler;
    if (bd->GlVersion >= 330)
        glGetIntegerv(GL_SAMPLER_BINDING, reinterpret_cast<GLint *>(&last_sampler));
    else
        last_sampler = 0;
#endif
    GLuint last_array_buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint *>(&last_array_buffer));
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    // This is part of VAO on OpenGL 3.0+ and OpenGL ES 3.0+.
    GLint last_element_array_buffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);

    ImGui_ImplSwitch_VtxAttribState last_vtx_attrib_state_pos;
    last_vtx_attrib_state_pos.GetState(bd->AttribLocationVtxPos);

    ImGui_ImplSwitch_VtxAttribState last_vtx_attrib_state_uv;
    last_vtx_attrib_state_uv.GetState(bd->AttribLocationVtxUV);

    ImGui_ImplSwitch_VtxAttribState last_vtx_attrib_state_color;
    last_vtx_attrib_state_color.GetState(bd->AttribLocationVtxColor);
#endif
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GLuint last_vertex_array_object;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<GLint *>(&last_vertex_array_object));
#endif
#ifdef IMGUI_IMPL_HAS_POLYGON_MODE
    GLint last_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
#endif
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);

    GLint last_scissor_box[4];
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);

    GLenum last_blend_src_rgb;
    glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint *>(&last_blend_src_rgb));

    GLenum last_blend_dst_rgb;
    glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint *>(&last_blend_dst_rgb));

    GLenum last_blend_src_alpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint *>(&last_blend_src_alpha));

    GLenum last_blend_dst_alpha;
    glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint *>(&last_blend_dst_alpha));

    GLenum last_blend_equation_rgb;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, reinterpret_cast<GLint *>(&last_blend_equation_rgb));

    GLenum last_blend_equation_alpha;
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, reinterpret_cast<GLint *>(&last_blend_equation_alpha));

    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    GLboolean last_enable_primitive_restart = (bd->GlVersion >= 310) ? glIsEnabled(GL_PRIMITIVE_RESTART) : GL_FALSE;
#endif

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared among GL contexts)
    // The renderer would actually work without any VAO bound, but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glGenVertexArrays(1, &vertex_array_object);
#endif
    ImGui_ImplSwitch_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        // - On Intel windows drivers we got reports that regular glBufferData() led to accumulating leaks when using multi-viewports, so we started using orphaning + glBufferSubData(). (See https://github.com/ocornut/imgui/issues/4468)
        // - On NVIDIA drivers we got reports that using orphaning + glBufferSubData() led to glitches when using multi-viewports.
        // - OpenGL drivers are in a very sorry state in 2022, for now we are switching code path based on vendors.
        const GLsizeiptr vtx_buffer_size = static_cast<GLsizeiptr>(cmd_list->VtxBuffer.Size) * static_cast<int>(sizeof(ImDrawVert));
        const GLsizeiptr idx_buffer_size = static_cast<GLsizeiptr>(cmd_list->IdxBuffer.Size ) * static_cast<int>(sizeof(ImDrawIdx));
        if (bd->UseBufferSubData) {
            if (bd->VertexBufferSize < vtx_buffer_size) {
                bd->VertexBufferSize = vtx_buffer_size;
                glBufferData(GL_ARRAY_BUFFER, bd->VertexBufferSize, nullptr, GL_STREAM_DRAW);
            }
            if (bd->IndexBufferSize < idx_buffer_size) {
                bd->IndexBufferSize = idx_buffer_size;
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, bd->IndexBufferSize, nullptr, GL_STREAM_DRAW);
            }
            glBufferSubData(GL_ARRAY_BUFFER, 0, vtx_buffer_size, static_cast<const GLvoid *>(cmd_list->VtxBuffer.Data));
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, idx_buffer_size, static_cast<const GLvoid *>(cmd_list->IdxBuffer.Data));
        }
        else {
            glBufferData(GL_ARRAY_BUFFER, vtx_buffer_size, static_cast<const GLvoid *>(cmd_list->VtxBuffer.Data), GL_STREAM_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_buffer_size, static_cast<const GLvoid *>(cmd_list->IdxBuffer.Data), GL_STREAM_DRAW);
        }

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplSwitch_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                glScissor(static_cast<int>(clip_min.x), static_cast<int>(static_cast<float>(fb_height) - clip_max.y), static_cast<int>(clip_max.x - clip_min.x), static_cast<int>(clip_max.y - clip_min.y));

                // Bind texture, Draw
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(pcmd->GetTexID())));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
                if (bd->GlVersion >= 320)
                    glDrawElementsBaseVertex(GL_TRIANGLES, static_cast<GLsizei>(pcmd->ElemCount), sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)static_cast<intptr_t>(pcmd->IdxOffset * sizeof(ImDrawIdx)), static_cast<GLint>(pcmd->VtxOffset));
                else
#endif
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(pcmd->ElemCount), sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)static_cast<intptr_t>(pcmd->IdxOffset * sizeof(ImDrawIdx)));
            }
        }
    }

    // Destroy the temporary VAO
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glDeleteVertexArrays(1, &vertex_array_object);
#endif

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    if (bd->GlVersion >= 330)
        glBindSampler(0, last_sampler);
#endif
    glActiveTexture(last_active_texture);
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindVertexArray(last_vertex_array_object);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    last_vtx_attrib_state_pos.SetState(bd->AttribLocationVtxPos);
    last_vtx_attrib_state_uv.SetState(bd->AttribLocationVtxUV);
    last_vtx_attrib_state_color.SetState(bd->AttribLocationVtxColor);
#endif
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    
    if (last_enable_blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    
    if (last_enable_cull_face)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    
    if (last_enable_depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    
    if (last_enable_stencil_test)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
    
    if (last_enable_scissor_test)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    if (bd->GlVersion >= 310) {
        if (last_enable_primitive_restart)
            glEnable(GL_PRIMITIVE_RESTART);
        else
            glDisable(GL_PRIMITIVE_RESTART);
    }
#endif

#ifdef IMGUI_IMPL_HAS_POLYGON_MODE
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
#endif
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    (void)bd; // Not all compilation paths use this
}

bool ImGui_ImplSwitch_CreateFontsTexture(void) {
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();

    // Build texture atlas
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &bd->FontTexture);
    glBindTexture(GL_TEXTURE_2D, bd->FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH // Not on WebGL/ES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(bd->FontTexture));

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    return true;
}

void ImGui_ImplSwitch_DestroyFontsTexture(void) {
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();

    if (bd->FontTexture) {
        glDeleteTextures(1, &bd->FontTexture);
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
static bool CheckShader(GLuint handle, const char* desc) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    
    if (static_cast<GLboolean>(status) == GL_FALSE)
        std::fprintf(stderr, "ERROR: ImGui_ImplSwitch_CreateDeviceObjects: failed to compile %s! With GLSL: %s\n", desc, bd->GlslVersionString);
    
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize(static_cast<int>(log_length + 1));
        glGetShaderInfoLog(handle, log_length, nullptr, static_cast<GLchar *>(buf.begin()));
        std::fprintf(stderr, "%s\n", buf.begin());
    }

    return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
static bool CheckProgram(GLuint handle, const char* desc) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    
    if (static_cast<GLboolean>(status) == GL_FALSE)
        std::fprintf(stderr, "ERROR: ImGui_ImplSwitch_CreateDeviceObjects: failed to link %s! With GLSL %s\n", desc, bd->GlslVersionString);
    
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize(static_cast<int>(log_length + 1));
        glGetShaderInfoLog(handle, log_length, nullptr, static_cast<GLchar *>(buf.begin()));
        std::fprintf(stderr, "%s\n", buf.begin());
    }

    return (GLboolean)status == GL_TRUE;
}

bool ImGui_ImplSwitch_CreateDeviceObjects(void) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();

    // Backup GL state
    GLint last_texture, last_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
#endif

    // Parse GLSL version string
    int glsl_version = 130;
    std::sscanf(bd->GlslVersionString, "#version %d", &glsl_version);

    const GLchar *vertex_shader_glsl_120 =
        "uniform mat4 ProjMtx;\n"
        "attribute vec2 Position;\n"
        "attribute vec2 UV;\n"
        "attribute vec4 Color;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_130 =
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_300_es =
        "precision highp float;\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *vertex_shader_glsl_410_core =
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_120 =
        "#ifdef GL_ES\n"
        "    precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_130 =
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_300_es =
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar *fragment_shader_glsl_410_core =
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    // Select shaders matching our GLSL versions
    const GLchar* vertex_shader = nullptr;
    const GLchar* fragment_shader = nullptr;
    if (glsl_version < 130) {
        vertex_shader = vertex_shader_glsl_120;
        fragment_shader = fragment_shader_glsl_120;
    }
    else if (glsl_version >= 410) {
        vertex_shader = vertex_shader_glsl_410_core;
        fragment_shader = fragment_shader_glsl_410_core;
    }
    else if (glsl_version == 300) {
        vertex_shader = vertex_shader_glsl_300_es;
        fragment_shader = fragment_shader_glsl_300_es;
    }
    else {
        vertex_shader = vertex_shader_glsl_130;
        fragment_shader = fragment_shader_glsl_130;
    }

    // Create shaders
    const GLchar *vertex_shader_with_version[2] = { bd->GlslVersionString, vertex_shader };
    GLuint vert_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_handle, 2, vertex_shader_with_version, nullptr);
    glCompileShader(vert_handle);
    CheckShader(vert_handle, "vertex shader");

    const GLchar *fragment_shader_with_version[2] = { bd->GlslVersionString, fragment_shader };
    GLuint frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_handle, 2, fragment_shader_with_version, nullptr);
    glCompileShader(frag_handle);
    CheckShader(frag_handle, "fragment shader");

    // Link
    bd->ShaderHandle = glCreateProgram();
    glAttachShader(bd->ShaderHandle, vert_handle);
    glAttachShader(bd->ShaderHandle, frag_handle);
    glLinkProgram(bd->ShaderHandle);
    CheckProgram(bd->ShaderHandle, "shader program");

    glDetachShader(bd->ShaderHandle, vert_handle);
    glDetachShader(bd->ShaderHandle, frag_handle);
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);

    bd->AttribLocationTex = glGetUniformLocation(bd->ShaderHandle, "Texture");
    bd->AttribLocationProjMtx = glGetUniformLocation(bd->ShaderHandle, "ProjMtx");
    bd->AttribLocationVtxPos = static_cast<GLuint>(glGetAttribLocation(bd->ShaderHandle, "Position"));
    bd->AttribLocationVtxUV = static_cast<GLuint>(glGetAttribLocation(bd->ShaderHandle, "UV"));
    bd->AttribLocationVtxColor = static_cast<GLuint>(glGetAttribLocation(bd->ShaderHandle, "Color"));

    // Create buffers
    glGenBuffers(1, &bd->VboHandle);
    glGenBuffers(1, &bd->ElementsHandle);

    ImGui_ImplSwitch_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindVertexArray(last_vertex_array);
#endif

    return true;
}

void ImGui_ImplSwitch_DestroyDeviceObjects(void) {
    ImGui_ImplSwitch_Data *bd = ImGui_ImplSwitch_GetBackendData();

    if (bd->VboHandle) {
        glDeleteBuffers(1, &bd->VboHandle);
        bd->VboHandle = 0;
    }
    
    if (bd->ElementsHandle) {
        glDeleteBuffers(1, &bd->ElementsHandle);
        bd->ElementsHandle = 0;
    }

    if (bd->ShaderHandle) {
        glDeleteProgram(bd->ShaderHandle);
        bd->ShaderHandle = 0;
    }
    
    ImGui_ImplSwitch_DestroyFontsTexture();
}
