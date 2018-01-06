//-----------------------------------------------------------------------------
// USER IMPLEMENTATION
// This file contains compile-time options for ImGui.
// Other options (memory allocation overrides, callbacks, etc.) can be set at runtime via the ImGuiIO structure - ImGui::GetIO().
//-----------------------------------------------------------------------------

#pragma once

#include <assert.h>
//---- Define assertion handler. Defaults to calling assert().
//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)
#define IM_ASSERT(_EXPR) assert(_EXPR)
//#define IM_ASSERT(_EXPR)

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows.
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Don't define obsolete functions names. Consider enabling from time to time or when updating to reduce like hood of using already obsolete function/names
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Include imgui_user.h at the end of imgui.h
//#define IMGUI_INCLUDE_IMGUI_USER_H

//---- Don't implement default handlers for Windows (so as not to link with OpenClipboard() and others Win32 functions)
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS

//---- Don't implement demo windows functionality (ShowDemoWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
//---- It is very strongly recommended to NOT disable the demo windows. Please read the comment at the top of imgui_demo.cpp to learn why.
//#define IMGUI_DISABLE_DEMO_WINDOWS

//---- Don't implement ImFormatString(), ImFormatStringV() so you can reimplement them yourself.
//#define IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

//---- Pack colors to BGRA instead of RGBA (remove need to post process vertex buffer in back ends)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Implement STB libraries in a namespace to avoid linkage conflicts
//#define IMGUI_STB_NAMESPACE     ImGuiStb

//---- Define constructor and implicit cast operators to convert back<>forth from your math types and ImVec2/ImVec4.
/*
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const MyVec2& f) { x = f.x; y = f.y; }                       \
        operator MyVec2() const { return MyVec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const MyVec4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
        operator MyVec4() const { return MyVec4(x,y,z,w); }
*/

//---- Use 32-bit vertex indices (instead of default: 16-bit) to allow meshes with more than 64K vertices
#define ImDrawIdx unsigned int

//---- Tip: You can add extra functions within the ImGui:: namespace, here or in your own headers files.
//---- e.g. create variants of the ImGui::Value() helper for your low-level math types, or your own widgets/helpers.
/*
namespace ImGui
{
    void    Value(const char* prefix, const MyMatrix44& v, const char* float_format = NULL);
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
	int _colorshift;
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
	float colorshift;
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