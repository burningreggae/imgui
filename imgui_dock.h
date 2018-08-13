#ifndef __IMGUI_DOCK_H_INCLUDED__
#define __IMGUI_DOCK_H_INCLUDED__

#include "imgui.h"

namespace ImGui
{
	enum Slot_
	{
		Slot_Left,
		Slot_Right,
		Slot_Top,
		Slot_Bottom,
		Slot_Tab,

		Slot_Float,
		Slot_None
	};

	enum DockState
	{
		Status_Docked = 0,
		Status_Float,
		Status_Dragged
	};
	#define MAX_WORKSPACE 8

	IMGUI_API void ShutdownDock(int slot=-1);
	IMGUI_API void SetNextDockSlot(Slot_ val);
	IMGUI_API void RootDock(const ImVec2& pos, const ImVec2& size);
	IMGUI_API bool BeginDock(const char* label, bool* opened = 0, ImGuiWindowFlags extra_flags = 0, const ImVec2& default_size = ImVec2(-1, -1),bool border = false);
	IMGUI_API void EndDock();
	IMGUI_API void SetDockActive(int slot, const char* label, int disable_all_other=0);
	IMGUI_API const char* GetDockActive(int slot);
	IMGUI_API void SetDockWindowPos(int slot, const char* name, const ImVec2& pos,const ImVec2& size, DockState newDockState );

	IMGUI_API void SaveDock(int slot, ImGuiTextBuffer *out=0);
	IMGUI_API void LoadDock(int slot, const char *filename, int reload = 0);
	IMGUI_API void Print();

	IMGUI_API bool DockBeginWorkspace(const char *name, int slot,int draw_tabbar=1,int draw_tabbar_list=0);
	IMGUI_API void DockEndWorkspace(int toslot);
	IMGUI_API int  DockGetActiveWorkspace();
	IMGUI_API bool DockWorkspaceClosed();
	IMGUI_API void DockDesigner(bool *v_open = 0);
}

#endif
