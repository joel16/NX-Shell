#include <cstring>
#include <switch.h>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "log.h"
#include "textures.h"
#include "lang.hpp"
SDL_Window *window = nullptr;
char __application_path[FS_MAX_PATH];

using namespace lang::literals;


//init the pl lib
extern "C" void userAppInit() {
    setsysInitialize();
    plInitialize(PlServiceType_User);
    //romfsInit();
#ifdef DEBUG
    socketInitializeDefault();
    nxlinkStdio();
#endif
}

extern "C" void userAppExit() {
    setsysExit();
    plExit();
    //romfsExit();
#ifdef DEBUG
    socketExit();
#endif
}

namespace Services {
	SDL_GLContext gl_context;

	void SetDefaultTheme(void) {
		ImGui::GetStyle().FrameRounding = 4.0f;
		ImGui::GetStyle().GrabRounding = 4.0f;
		
		ImVec4 *colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.50f, 0.50f, 1.0f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}
	
	int InitImGui(void) {
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
			Log::Error("Error: %s\n", SDL_GetError());
			return -1;
		}
		
		for (int i = 0; i < 2; i++) {
			if (!SDL_JoystickOpen(i)) {
				Log::Error("SDL_JoystickOpen: %s\n", SDL_GetError());
				SDL_Quit();
				return -1;
			}
		}
		
		// GL 3.0 + GLSL 130
		const char *glsl_version = "#version 130";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		
		// Create window with graphics context
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		window = SDL_CreateWindow("NX-Shell", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
		gl_context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, gl_context);
		SDL_GL_SetSwapInterval(1); // Enable vsync
		
		// Initialize OpenGL loader
		bool err = gladLoadGL() == 0;
		if (err) {
			Log::Error("Failed to initialize OpenGL loader!\n");
			return -1;
		}
		
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		io.IniFilename = nullptr;
		io.MouseDrawCursor = false;
		
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		
		// Setup Platform/Renderer bindings
		ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
		ImGui_ImplOpenGL3_Init(glsl_version);
		

		// Load nintendo font  from machine

		if (auto rc = lang::initialize_to_system_language(); R_FAILED(rc))
				Log::Error("Failed to init language: %#x, will fall back to key names\n");


		ImFont *font  ; 
		PlFontData standard, extended, chinese;
		static ImWchar extended_range[] = {0xe000, 0xe152};
		if (R_SUCCEEDED(plGetSharedFontByType(&standard,     PlSharedFontType_Standard)) &&
				R_SUCCEEDED(plGetSharedFontByType(&extended, PlSharedFontType_NintendoExt)) &&
				R_SUCCEEDED(plGetSharedFontByType(&chinese,  PlSharedFontType_ChineseSimplified))  
				//&&R_SUCCEEDED(plGetSharedFontByType(&korean,   PlSharedFontType_KO))
				)
				//add the language you need when add new language 
				 {
			std::uint8_t *px;
			int w, h, bpp;
			ImFontConfig font_cfg;

			font_cfg.FontDataOwnedByAtlas = false;
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 25.0f, &font_cfg, io.Fonts->GetGlyphRangesDefault());
			font_cfg.MergeMode            = true;
			io.Fonts->AddFontFromMemoryTTF(extended.address, extended.size, 25.0f, &font_cfg, extended_range);
			font = io.Fonts->AddFontFromMemoryTTF(chinese.address,  chinese.size,  25.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
			//io.Fonts->AddFontFromMemoryTTF(korean.address,   korean.size,   35.0f, &font_cfg, io.Fonts->GetGlyphRangesKorean());

			// build font atlas
			io.Fonts->GetTexDataAsAlpha8(&px, &w, &h, &bpp);
			io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
			io.Fonts->Build();
		}
		//

		//ImFont *font = io.Fonts->AddFontFromFileTTF("romfs:/NotoSans-Regular.ttf", 35.0f);
		//use AddFontFromMemoryTTF   insteal of  AddFontFromFileTTF  :)   



		IM_ASSERT(font != nullptr);
		Services::SetDefaultTheme();
		Textures::Init();

		return 0;
	}
	
	void ExitImGui(void) {
		Textures::Exit();

		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
		
		SDL_GL_DeleteContext(gl_context);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
	
	int Init(void) {
		// FS
		devices[0] = *fsdevGetDeviceFileSystem("sdmc");
		fs = &devices[0];

		Config::Load();

		socketInitializeDefault();
		
		if (config.dev_options)
			nxlinkStdio();

		Log::Init();

		Result ret = 0;
		if (R_FAILED(ret = romfsInit())) {
			Log::Error("romfsInit() failed: 0x%x\n", ret);
			return ret;
		}

		if (R_FAILED(ret = nifmInitialize(NifmServiceType_User))) {
			Log::Error("nifmInitialize(NifmServiceType_User) failed: 0x%x\n", ret);
			return ret;
		}

		return 0;
	}
	
	void Exit(void) {
		romfsExit();  
		nifmExit();
		Log::Exit();
		socketExit();
	}
}

int main(int, char *argv[]) {
	Result ret = 0;
	
	// Strip "sdmc:" from application path 
	std::string __application_path_string = argv[0];
	__application_path_string.erase(0, 5);
	std::strcpy(__application_path, __application_path_string.c_str());
	
	if (R_FAILED(ret = Services::Init()))
		return ret;
	
	if (R_FAILED(ret = Services::InitImGui()))
		return ret;
	
	if (R_FAILED(ret = GUI::RenderLoop()))
		return ret;

	Services::ExitImGui();
	Services::Exit();
	return 0;
}
