#ifndef __IMGUI_IMPL_GLUT_H_INCLUDED__
#define __IMGUI_IMPL_GLUT_H_INCLUDED__

// ImGui GLFW binding with freeglut

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

IMGUI_API bool ImGui_ImplGLUT_Init();
IMGUI_API bool ImGui_ImplGLUT_CreateDeviceObjects();
IMGUI_API void ImGui_ImplGLUT_Shutdown();
IMGUI_API void ImGui_ImplGLUT_MouseCursor();
IMGUI_API int SystemMouseCursor( int cursorID );
IMGUI_API void SetStyle(int style,float fontSize);
void savePNG ( const char *filename,unsigned char* pixels,int width, int height,int samples );

#endif// __IMGUI_IMPL_GLUT_H_INCLUDED__


