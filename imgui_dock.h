#ifndef __IMGUI_H_INCLUDED__
#define __IMGUI_H_INCLUDED__

#include "imgui.h"

namespace ImGui
{

	IMGUI_API void ShutdownDock();
	IMGUI_API void SetNextDock();
	IMGUI_API void RootDock(const ImVec2& pos, const ImVec2& size);
	IMGUI_API bool BeginDock(const char* name, bool* opened = 0, ImGuiWindowFlags extra_flags = 0, const ImVec2& default_size = ImVec2(-1, -1),bool border = false);
	IMGUI_API void EndDock();
	IMGUI_API void SetDockActive();
	IMGUI_API void SaveDock(ImGuiTextBuffer &out);
	IMGUI_API void LoadDock(const char *filename);
	IMGUI_API void Print();

	IMGUI_API bool DockBeginWorkspace(const char *name);
	IMGUI_API void DockEndWorkspace();

	IMGUI_API bool DockWorkspaceClosed();
	IMGUI_API void DockDebugWindow();
}

#endif
