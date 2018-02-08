//-----------------------------------------------------------------------------
// COMPILE-TIME OPTIONS FOR DEAR IMGUI
// Most options (memory allocation, clipboard callbacks, etc.) can be set at runtime via the ImGuiIO structure - ImGui::GetIO().
//-----------------------------------------------------------------------------
// A) You may edit imconfig.h (and not overwrite it when updating imgui, or maintain a patch/branch with your modifications to imconfig.h)
// B) or add configuration directives in your own file and compile with #define IMGUI_USER_CONFIG "myfilename.h" 
// Note that options such as IMGUI_API, IM_VEC2_CLASS_EXTRA or ImDrawIdx needs to be defined consistently everywhere you include imgui.h, not only for the imgui*.cpp compilation units.
//-----------------------------------------------------------------------------

#pragma once

#ifdef _DEBUG
#include <assert.h>
//---- Define assertion handler. Defaults to calling assert().
//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)
#define IM_ASSERT(_EXPR) assert(_EXPR)
#else
#define IM_ASSERT(_EXPR)
#endif

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows.
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Don't define obsolete functions names. Consider enabling from time to time or when updating to reduce likelihood of using already obsolete function/names
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Don't implement default handlers for Windows (so as not to link with certain functions)
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS   // Don't use and link with OpenClipboard/GetClipboardData/CloseClipboard etc.
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS         // Don't use and link with ImmGetContext/ImmSetCompositionWindow.

//---- Don't implement demo windows functionality (ShowDemoWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
//---- It is very strongly recommended to NOT disable the demo windows. Please read the comment at the top of imgui_demo.cpp.
#ifdef _DEBUG
#else
#define IMGUI_DISABLE_DEMO_WINDOWS
#endif

//---- Don't implement ImFormatString(), ImFormatStringV() so you can reimplement them yourself.
//#define IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

//---- Include imgui_user.h at the end of imgui.h as a convenience
//#define IMGUI_INCLUDE_IMGUI_USER_H

//---- Pack colors to BGRA8 instead of RGBA8 (if you needed to convert from one to another anyway)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Implement STB libraries in a namespace to avoid linkage conflicts (defaults to global namespace)
//#define IMGUI_STB_NAMESPACE     ImGuiStb

//---- Define constructor and implicit cast operators to convert back<>forth from your math types and ImVec2/ImVec4.
// This will be inlined as part of ImVec2 and ImVec4 class declarations.
/*
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const MyVec2& f) { x = f.x; y = f.y; }                       \
        operator MyVec2() const { return MyVec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const MyVec4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
        operator MyVec4() const { return MyVec4(x,y,z,w); }
*/

//---- Use 32-bit vertex indices (instead of default 16-bit) to allow meshes with more than 64K vertices. Render function needs to support it.
#define ImDrawIdx unsigned int

//---- Tip: You can add extra functions within the ImGui:: namespace, here or in your own headers files.
/*
namespace ImGui
{
    void MyFunction(const char* name, const MyMatrix44& v);
}
*/

#define IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT struct ImDrawVert\
{\
    ImVec2  pos;\
    ImVec4  uv;\
    ImU32   col;\
};

#define IM_VEC4_CLASS_EXTRA                                           \
	ImVec4(const ImVec2& f):x(f.x),y(f.y),z(f.x),w(f.y) {}

//shader
#define shader_raw_density 1
#define maximumIntensityProjection 4
#define blendAlpha 8
#define blendAlphaPremul 16
#define alpha_from_color 32
#define tex_filter		64
#define tex_clamp		128
#define tex_gen			256
#define tex_mipmapgen	512
#define tex_filter2		1024
#define shader_autobrightnesscontrast 2048

struct shaderparamlayer
{
	shaderparamlayer():transferTexPrivate(0) {}

	//uniform location
	int _oct;
	int _transferFunc;
	int _alpha_test;
	int _alpha_scale;
	int _alpha_from_color;
	int _raw_density;
	int _level_brightness;
	int _level_contrast;
	int _tex_gen;
	int _alpha_premul;
	int _colorMode;
	int _envelope;

	unsigned int octTex;
	unsigned int octTexType; 
	float alpha_test;
	float alpha_scale;
	unsigned flags;
	unsigned int transferTex;			//volume colormap
	unsigned int transferTexPrivate;	//private colormap
	unsigned int transferTexLink;		//linked colormap
	float level_brightness;
	float level_contrast;
	int colorMode;
	float envelope;
};

struct shaderparam
{
	shaderparam():id(0) {}

	unsigned int id;	//shader nr
	shaderparamlayer layer[2];
};

void setShader(shaderparam &shader);
void msg(const char* fmt, ...);
void system_redraw( const char *function, const char* caller = 0, int value=1);

#define GUI_ENVELOPE_WINDOW
enum eEnvelopeGroup
{
	envelope_hover = 0,	 // widget hover
	envelope_window,	// window active
	envelope_scroll,
};

void envelope_gate(unsigned int id, bool isActive, int group );
float envelope_get(unsigned int id, int group=envelope_hover);
void envelope_step(float dt);
void envelope_load(int type);
void envelope_reset();
