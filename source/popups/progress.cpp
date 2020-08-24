#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "popups.h"
#include "windows.h"

namespace Popups {
    static bool focus = false, first_item = false;
    
    void ProgressPopup(float offset, float size, const std::string &title, const std::string &text) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		Windows::FileBrowserWindow(&item, &focus, &first_item);
        Popups::SetupPopup(title.c_str());
		
		if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text(text.c_str());
			ImGui::ProgressBar(offset/size, ImVec2(0.0f, 0.0f));
			ImGui::EndPopup();
		}
		
		ImGui::PopStyleVar();
		ImGui::Render();
		glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
		glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
}
