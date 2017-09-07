#ifndef __IMGUI_CURVE_H_INCLUDED__
#define __IMGUI_CURVE_H_INCLUDED__

#include "imgui.h"

namespace ImGui
{
	IMGUI_API int CurveEditor(const char *label, const ImVec2& size, ImVec4 *points, const size_t maxpoints );
	IMGUI_API float CurveValue(float p, size_t maxpoints, const ImVec4 *points);
	IMGUI_API float CurveValueSmooth(float p, size_t maxpoints, const ImVec4 *points);
};

#endif

