#ifndef __IMGUI_IMPL_GLUT_H_INCLUDED__
#define __IMGUI_IMPL_GLUT_H_INCLUDED__

// ImGui GLFW binding with freeglut

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

IMGUI_API bool ImGui_ImplGLUT_Init();
IMGUI_API void ImGui_ImplGLUT_CreateDeviceObjects();
IMGUI_API void ImGui_ImplGLUT_Shutdown();
IMGUI_API void ImGui_ImplGLUT_MouseCursor();
IMGUI_API void ImGui_ImplGLUT_RenderDrawLists(ImDrawData* draw_data);
IMGUI_API int SystemMouseCursor( int cursorID );
IMGUI_API void SetStyle(int style, bool styleShadow, bool styleInvert,bool styleAntiAliased, float alpha,float saturate,int fontNr, float fontSize, float fontRasterMultiply,int fontoversample);

void savePNG ( const char *filename,unsigned char* pixels,int width, int height,int samples );

struct Camera
{
	void glMatrixMode(unsigned mode );
	void glLoadIdentity();
	void glPushMatrix();
	void glPopMatrix();

	void glFrustum(double left, double right, double bottom, double top, double nearval, double farval);
	void glOrtho (float left, float right, float bottom, float top, float zNear, float zFar);
	void gluPerspective(float fovy, float aspect, float zNear, float zFar);

	void gluPickMatrix(float x, float y,float width, float height, const int viewport[4]);
	void glViewport(const int viewport[4]);

	void transform_point(float out[4], const float m[16], const float in[4]);

	int viewport[4];
	double projection[16];
	double modelview[16];
	double modelview_inverse[16];
	float viewer[3];

	unsigned int selectBuf[256]; // Space for selection buffer
	unsigned char pickedFrameBuffer[32];
	//int pickedId;
};

#define GLUT_SPECIAL_OFFSET 256	//Map Glut special key

#endif// __IMGUI_IMPL_GLUT_H_INCLUDED__


