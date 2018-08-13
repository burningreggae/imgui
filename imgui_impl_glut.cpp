// ImGui Glut binding with OpenGL
// You can copy and use unmodified imgui_impl_* files in your project.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// See main.cpp for an example of using this.
// https://github.com/ocornut/imgui


#include "imgui.h"
#include "imgui_impl_glut.h"

#include <GL/glew.h>
#include <GL/glut.h>

// Data
//static bool         g_MousePressed[3] = { false, false, false };
//static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
float g_FontWishSize = 15.f;
int g_OversampleHWish = 3;
float g_FontWishRasterizerMultiply = 1.f;
int g_StyleWish = 0;
int g_FontWish = 0;
int fontUseBit = 32;


// Data
static GLhandleARB  g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = -1, g_AttribLocationEnvelope = -1,g_AttribLocationProjMtx = -1,g_AttribLocationColorMode=-1;
static int          g_AttribLocationPosition = -1, g_AttribLocationUV = -1, g_AttribLocationColor =-1;
static GLhandleARB g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

extern Camera cam;
int sceneFile_ColorMode();
GLuint currentTexture2D = 0;

void setEnvelope(const ImDrawCmd* cmd)
{
	if ( cmd->shader )
	{
		if (cmd->shader->layer[0]._envelope>=0) glUniform1f(cmd->shader->layer[0]._envelope, cmd->envelope);
		if (cmd->shader->layer[1]._envelope>=0) glUniform1f(cmd->shader->layer[1]._envelope, cmd->envelope);
	}
	else
	{
		if (g_AttribLocationEnvelope>=0) glUniform1f(g_AttribLocationEnvelope, cmd->envelope);
	}
}

void setGUIShader(GLhandleARB shader)
{
	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
	//GLint currentTexture3D = 0;
	//glGetIntegerv(GL_TEXTURE_BINDING_3D, &currentTexture3D);
	//GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	//GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	//glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
	//glPushAttrib(GL_ALL_ATTRIB_BITS);

	glUseProgram(shader);
	if (g_AttribLocationTex>=0) glUniform1i(g_AttribLocationTex, 0);
	//if (g_AttribLocationEnvelope>=0) glUniform1f(g_AttribLocationEnvelope, ImGui::GetTime()); 
	if (g_AttribLocationColorMode>=0) glUniform1i(g_AttribLocationColorMode, sceneFile_ColorMode()); 
	glShadeModel(GL_SMOOTH);

	glBlendEquationEXT(GL_FUNC_ADD_EXT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 0.f);

	glDisable(GL_MULTISAMPLE);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glEnable(GL_SCISSOR_TEST);

	glClientActiveTexture(GL_TEXTURE1);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);

	glClientActiveTexture(GL_TEXTURE0);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_3D);
	glDisable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);

	currentTexture2D = g_FontTexture;
	glBindTexture(GL_TEXTURE_2D, currentTexture2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

void unsetGUIShader()
{
	// Restore modified state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	//glPopAttrib();
	//glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
	//glMatrixMode(GL_MODELVIEW);
	//glPopMatrix();
	//glMatrixMode(GL_PROJECTION);
	//glPopMatrix();
	//glPopAttrib();
	//glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGLUT_RenderDrawLists(ImDrawData* draw_data)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if(fb_width <= 0 || fb_height <= 0) return;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	setGUIShader(g_ShaderHandle);
	
	// Setup viewport, orthographic projection matrix
	int viewport[4] = {0, 0,fb_width,fb_height};
	cam.glViewport(viewport);
	glMatrixMode(GL_PROJECTION);
	//glPushMatrix();
	glLoadIdentity();
	cam.glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	//glPushMatrix();
	glLoadIdentity();

	// Render command lists
	for(int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const unsigned char* vtx_buffer = 0; // (const unsigned char*)&cmd_list->VtxBuffer.front();
		const ImDrawIdx* idx_buffer = 0; //&cmd_list->IdxBuffer.front();

        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

		glClientActiveTexture(GL_TEXTURE0);
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));


		for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			GLuint t = (GLuint)(intptr_t)pcmd->TextureId;

			if(pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			if(t)
			{
				if ( pcmd->shader )
				{
					//pcmd->shader->layer[0].octTex = t;
					setShader(*pcmd->shader);
					glEnable(GL_SCISSOR_TEST);
					glDisable(GL_CULL_FACE);
					glDisable(GL_DEPTH_TEST);
					glDepthMask(GL_FALSE);

					glTexCoordPointer(3, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));

					if (pcmd->shader->layer[0].octTexType == GL_TEXTURE_2D )
					{
						//currentTexture2D = t;
					}

				}
				else
				if ( t != currentTexture2D )
				{
					glBindTexture(GL_TEXTURE_2D, t);
					currentTexture2D = t;
				}

				setEnvelope(pcmd);
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);

				if ( pcmd->shader)
				{
					glUseProgram(g_ShaderHandle);
					if (g_AttribLocationTex>=0) glUniform1i(g_AttribLocationTex, 0);

					glClientActiveTexture(GL_TEXTURE2);
					glActiveTexture(GL_TEXTURE2);
					glDisable(GL_TEXTURE_3D);

					glClientActiveTexture(GL_TEXTURE1);
					glActiveTexture(GL_TEXTURE1);
					glDisable(GL_TEXTURE_2D);

					glClientActiveTexture(GL_TEXTURE0);
					glActiveTexture(GL_TEXTURE0);
					glDisable(GL_TEXTURE_3D);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, currentTexture2D);

					glBlendEquationEXT(GL_FUNC_ADD_EXT);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glDisable(GL_ALPHA_TEST);
					glAlphaFunc(GL_GEQUAL, 0.f);

					glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));

					glEnable(GL_SCISSOR_TEST);
				}

			}
			idx_buffer += pcmd->ElemCount;
		}
	}

	unsetGUIShader();
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGLUT_RenderDrawLists2(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
    GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box); 
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
	glBlendEquationEXT(GL_FUNC_ADD_EXT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 0.f);

	glDisable(GL_MULTISAMPLE);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glEnable(GL_SCISSOR_TEST);

	glColor4f(1.f, 1.f, 1.f, 1.f);

	glClientActiveTexture(GL_TEXTURE1);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);

	glClientActiveTexture(GL_TEXTURE0);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_3D);
	glDisable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);

	GLuint currentTexture2D = g_FontTexture;
	glBindTexture(GL_TEXTURE_2D, currentTexture2D);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
#if 0
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(g_VaoHandle);
#else
    glUseProgram(0);
	// Setup viewport, orthographic projection matrix
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glBindVertexArray(g_VaoHandle);
#endif

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
			if ( 0 && !pcmd->shader )
			{
				//glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
			}
			else
			if ( 1 )
            {
				GLuint t = (GLuint)(intptr_t)pcmd->TextureId;
				if ( pcmd->shader )
				{
					//pcmd->shader->layer[0].octTex = t;
					setShader(*pcmd->shader);
					glEnable(GL_SCISSOR_TEST);
					glDisable(GL_CULL_FACE);
					glDisable(GL_DEPTH_TEST);
					glDepthMask(GL_FALSE);

					glVertexAttribPointer(g_AttribLocationUV, 3, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));

				}
				else
				if ( t != currentTexture2D )
				{
					glBindTexture(GL_TEXTURE_2D, t);
					currentTexture2D = t;
				}

				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);

				if ( pcmd->shader)
				{
					glUseProgram(g_ShaderHandle);

					//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glClientActiveTexture(GL_TEXTURE3);
					glActiveTexture(GL_TEXTURE3);
					glDisable(GL_TEXTURE_2D);

					glClientActiveTexture(GL_TEXTURE2);
					glActiveTexture(GL_TEXTURE2);
					glDisable(GL_TEXTURE_3D);

					glClientActiveTexture(GL_TEXTURE1);
					glActiveTexture(GL_TEXTURE1);
					glDisable(GL_TEXTURE_2D);

					glClientActiveTexture(GL_TEXTURE0);
					glActiveTexture(GL_TEXTURE0);
					glDisable(GL_TEXTURE_3D);
					glEnable(GL_TEXTURE_2D);
					//glBindTexture(GL_TEXTURE_2D, currentTexture2D);

					glBlendEquationEXT(GL_FUNC_ADD_EXT);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glDisable(GL_ALPHA_TEST);
					glAlphaFunc(GL_GEQUAL, 0.f);

					glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
					glEnable(GL_SCISSOR_TEST);
				}

                //glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                //glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                //glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
	glClientActiveTexture(last_active_texture);
    glActiveTexture(last_active_texture);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFunc(last_blend_src, last_blend_dst);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

static const char* ImGui_ImplGLUT_GetClipboardText()
{
	return "";
}

static void ImGui_ImplGLUT_SetClipboardText(const char* text)
{
}

static int hw_cursor = GLUT_CURSOR_LEFT_ARROW;

int SystemMouseCursor( int cursorID )
{
	int prev = hw_cursor;
	if ( hw_cursor != cursorID )
	{
		glutSetCursor(cursorID);
		hw_cursor = cursorID;
	}
	return prev;
}

void ImGui_ImplGLUT_MouseCursor()
{
	ImGuiContext * context = ImGui::GetCurrentContext();

	// Hide OS mouse cursor if ImGui is drawing it
	int hw = GLUT_CURSOR_LEFT_ARROW;
	if ( context )
	{
		ImGuiIO& io = ImGui::GetIO();
		if ( io.MouseDrawCursor) hw = GLUT_CURSOR_NONE;
		else
		switch(ImGui::GetMouseCursor())
		{
			default:
			case ImGuiMouseCursor_ResizeAll: hw = GLUT_CURSOR_CYCLE; break;
			case ImGuiMouseCursor_Arrow: hw = GLUT_CURSOR_LEFT_ARROW; break;
			case ImGuiMouseCursor_TextInput: hw = GLUT_CURSOR_TEXT; break;
			case ImGuiMouseCursor_ResizeEW: hw = GLUT_CURSOR_LEFT_RIGHT; break;
			case ImGuiMouseCursor_ResizeNS: hw = GLUT_CURSOR_UP_DOWN; break;
			case ImGuiMouseCursor_ResizeNESW: hw = GLUT_CURSOR_TOP_RIGHT_CORNER; break;
			case ImGuiMouseCursor_ResizeNWSE: hw = GLUT_CURSOR_TOP_LEFT_CORNER; break;
			case ImGuiMouseCursor_Hand: hw = GLUT_CURSOR_INFO; break;
		}
	}
	SystemMouseCursor(hw);
}

void releaseFonts()
{
	if(g_FontTexture)
	{
		glDeleteTextures(1, &g_FontTexture);
		ImGui::GetIO().Fonts->TexID = 0;
		g_FontTexture = 0;
	}
}


void createFonts()
{
	if (g_FontTexture) return;

	ImGuiIO& io = ImGui::GetIO();
	io.FontAllowUserScaling = false;

	ImFontAtlas& atlas = *io.Fonts;

	atlas.Clear();

	ImFontConfig config;
	config.OversampleH = 1;
	config.OversampleV = 1;
	config.PixelSnapH = true;
	config.GlyphExtraSpacing.x = 0.f;
	config.SizePixels = g_FontWishSize;
	config.RasterizerMultiply = 1.f;
	config.MergeMode = false;
	atlas.AddFontDefault(&config);

	config.OversampleH = g_OversampleHWish;
	config.OversampleV = 1;
	config.PixelSnapH = false;
	config.GlyphExtraSpacing.x = 0.f;
	//config.GlyphRanges = io.Fonts->GetGlyphRangesGreek();
	//ImFont* font = io.Fonts->AddFontFromFileTTF("HelveticaLt.ttf", 12, &config);
	//font->DisplayOffset.y -= 2;   // Render 1 pixel down
	config.SizePixels = g_FontWishSize;
	config.RasterizerMultiply = g_FontWishRasterizerMultiply;
	config.MergeMode = false;

	ImWchar r0[] = { 0x0020, 0x007F, 0, };	//	// Basic Latin
	//io.Fonts->AddFontFromFileTTF("HelveticaNeue.ttf", 15.f, &config,r0);
	ImWchar r1[] = { 
		0x0020, 0x007A,
		0x00B0, 0x00B5,
		0x00DF, 0x00DF,
		0x00E4, 0x00E4,
		0x00F6, 0x00F6,
		0x00FC, 0x00FC,
		0x03A3, 0x03A3,
		0x03C3, 0x03C3,
		0,0 };	
	atlas.AddFontFromFileTTF("OptimaNeue.ttf", g_FontWishSize, &config,r1);

	io.FontDefault = g_FontWish > 0 && atlas.Fonts.size() > g_FontWish ? atlas.Fonts[g_FontWish] : 0;
	if (0 == io.FontDefault && atlas.Fonts.size() ) io.FontDefault = atlas.Fonts[0];


	// Degree Sign     ALT-248, 0x00B0, \xc2\xb0
	// Uppercase Sigma ALT-228, 0x03A3, \xce\xa3
	// lowercase sigma ALT-229, 0x03C3, \xcf\x83
	// Micro Sign Mu   ALT-230, 0x00B5, \xc2\xb5m
	// pi              ALT-227, 0x03C0, \xcf\x80
	// sharp s				  ,	0x00DF,	\xc3\x9f
	// a with Diaeresis		  ,	0x00E4,	\xc3\xa4
	// o with Diaeresis		  ,	0x00F6,	\xc3\xb6
	// u with Diaeresis		  ,	0x00FC,	\xc3\xbc
	// superscript 2   ALT-253,	0x00B2,	\xc2\xb2
	// superscript 3          ,	0x00B3,	\xc2\xb3

/*
    ImWchar ranges[] = { 0xf000, 0xf3ff, 0 };
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontDefault();
    io.Fonts->LoadFromFileTTF("fontawesome-webfont.ttf", 16.0f, &config, ranges); // Merge icon font
    io.Fonts->LoadFromFileTTF("myfontfile.ttf", size_pixels, NULL, &config, io.Fonts->GetGlyphRangesJapanese()); // Merge japanese glyphs
*/
	// Build texture atlas
	unsigned char* pixels;
	int width, height;
	if ( fontUseBit == 8 ) io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
	else if ( fontUseBit == 32 ) io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	//savePNG("font.png",pixels,width, height,fontUseBit/8 );
	// Upload texture to graphics system
	//GLint last_texture;
	//glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

	glClientActiveTexture(GL_TEXTURE0);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if ( fontUseBit == 8 ) glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
	else if ( fontUseBit == 32 ) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

	// Restore state
	//glBindTexture(GL_TEXTURE_2D, last_texture);

	io.Fonts->ClearTexData();

}

void shaderManager_log(GLhandleARB object, const char* filename, int type);
void shaderManager_build(GLhandleARB& program,const char *shadername);
int& shaderManager_reload();

void releaseShader()
{
    if (g_ShaderHandle && g_VertHandle) glDetachShader(g_ShaderHandle, g_VertHandle);
    if (g_VertHandle) glDeleteShader(g_VertHandle),g_VertHandle = 0;

    if (g_ShaderHandle && g_FragHandle) glDetachShader(g_ShaderHandle, g_FragHandle);
    if (g_FragHandle) glDeleteShader(g_FragHandle),g_FragHandle = 0;

    if (g_ShaderHandle) glDeleteProgram(g_ShaderHandle),g_ShaderHandle = 0;

}

void createShader()
{
	if (!(shaderManager_reload() & 2)) return;
	
	releaseShader();
	shaderManager_build(g_ShaderHandle,fontUseBit == 32 ? "shader2d_imgui32":"shader2d_imgui");
	g_AttribLocationTex = glGetUniformLocationARB(g_ShaderHandle, "Texture");
	g_AttribLocationEnvelope = glGetUniformLocationARB(g_ShaderHandle, "Envelope");
	g_AttribLocationColorMode = glGetUniformLocationARB(g_ShaderHandle, "colorMode");

	shaderManager_reload() &= ~2;
	return;

    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";


    g_ShaderHandle = glCreateProgram();
    g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
    g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
    glCompileShader(g_VertHandle);
	shaderManager_log(g_VertHandle,"vertex_shader",GL_OBJECT_COMPILE_STATUS_ARB);

    glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
    glCompileShader(g_FragHandle);
	shaderManager_log(g_FragHandle,"fragment_shader",GL_OBJECT_COMPILE_STATUS_ARB);

    glAttachShader(g_ShaderHandle, g_VertHandle);
    glAttachShader(g_ShaderHandle, g_FragHandle);
    glLinkProgram(g_ShaderHandle);
	shaderManager_log(g_ShaderHandle,"g_ShaderHandle",GL_OBJECT_LINK_STATUS_ARB);
	
    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

    glGenBuffers(1, &g_VboHandle);
    glGenBuffers(1, &g_ElementsHandle);

    glGenVertexArrays(1, &g_VaoHandle);
    glBindVertexArray(g_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glEnableVertexAttribArray(g_AttribLocationPosition);
    glEnableVertexAttribArray(g_AttribLocationUV);
    glEnableVertexAttribArray(g_AttribLocationColor);

    glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));

}

void createBuffer()
{
	if(g_VboHandle) return;
    glGenBuffers(1, &g_VboHandle);
    glGenBuffers(1, &g_ElementsHandle);
}


void ImGui_ImplGLUT_CreateDeviceObjects()
{
/*
	// Backup GL state
    GLint last_texture, last_array_buffer, last_vertex_array;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
*/
	createFonts();
	createShader();
	createBuffer();

/*
    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);
*/
}


void ImGui_ImplGLUT_InvalidateDeviceObjects()
{
    if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
    if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
    if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
    g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

	releaseShader();
	releaseFonts();
}

//https://github.com/ocornut/imgui/issues/249
void style3()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_Text] = ImVec4(0.31f, 0.25f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.68f, 0.68f, 0.68f, 0.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.19f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.62f, 0.70f, 0.72f, 0.56f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.33f, 0.14f, 0.47f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.97f, 0.31f, 0.13f, 0.81f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.42f, 0.75f, 1.00f, 0.53f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.65f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.40f, 0.62f, 0.80f, 0.15f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.39f, 0.64f, 0.80f, 0.30f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.67f, 0.80f, 0.59f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.48f, 0.53f, 0.67f);
    //style.Colors[ImGuiCol_ComboBg] = ImVec4(0.89f, 0.98f, 1.00f, 0.99f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.38f, 0.37f, 0.37f, 0.91f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.31f, 0.47f, 0.99f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.00f, 0.79f, 0.18f, 0.78f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.42f, 0.82f, 1.00f, 0.81f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.86f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.73f, 0.80f, 0.86f, 0.45f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.88f, 0.94f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.46f, 0.84f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.99f, 0.54f, 0.43f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.82f, 0.92f, 1.00f, 0.90f);
    style.Alpha = 1.0f;
    style.FrameRounding = 4;

}

void style4()
{
    ImGuiStyle& style = ImGui::GetStyle();
        
    // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    //style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDimBg]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    style.Alpha = 1.0f;
    style.FrameRounding = 4;

}

// https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
void style1()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_Text] = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 0.5f);
	//style.Colors[ImGuiCol_TextHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	//style.Colors[ImGuiCol_TextActive] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.25f, 0.25f, 0.25f, 0.f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.96f, 0.96f, 0.96f, 0.75f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.49f, 0.49f, 0.49f, 0.50f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	//style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 0.8f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.5f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.8f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.9f, 0.9f, 0.9f, 0.94f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	style.Colors[ImGuiCol_CollapseTriangle] = ImVec4(0.55f, 0.55f, 0.55f, 1.f);
	style.Colors[ImGuiCol_WindowBgFocused] = ImVec4(0.96f, 0.96f, 0.96f, 1.f);

    style.Alpha = 1.0f;
	style.WindowPadding.x = 8;
	style.WindowPadding.y = 8;
	style.WindowRounding = 4.f;
	style.GrabRounding = 3;
	style.ScrollbarSize = 15;
	style.ScrollbarRounding = 16;
	style.FramePadding.x = 4;
	style.FramePadding.y = 1;
	style.FrameRounding = 4;
	style.SelectableRounding = 2;
	style.ItemSpacing.x = 6;
	style.ItemSpacing.y = 4;
	style.WindowTitleAlign.x = 0.5f;
	style.IndentSpacing = 20.f;
	style.TitleBarHeight = 6.f;
	style.CloseButtonSize = 6.5f;
	style.DisplaySafeAreaPadding.x = 4.f;
	style.DisplaySafeAreaPadding.y = 4.f;
	style.TriangleScale = 0.85f;
	style.CheckmarkScale = 1.f/8.f;
	style.CircleLineSegment = 50;
	style.ColumnsMinSpacing = 0.f;
	style.PopupRounding = 4.f;
	style.WindowBorderSize = 0.f;
}

void style2()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_Text] = ImVec4(1.f, 1.f, 1.f, 0.9f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 0.5f);
	//style.Colors[ImGuiCol_TextHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	//style.Colors[ImGuiCol_TextActive] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.25f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.25f, 0.25f, 0.25f, 0.25f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.96f, 0.96f, 0.96f, 0.75f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.49f, 0.49f, 0.49f, 0.50f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	//style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 0.8f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.5f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.8f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.94f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	style.Colors[ImGuiCol_CollapseTriangle] = ImVec4(0.55f, 0.55f, 0.55f, 1.f);
	style.Colors[ImGuiCol_WindowBgFocused] = ImVec4(0.96f, 0.96f, 0.96f, 1.f);

    style.Alpha = 1.0f;
	style.WindowPadding.x = 8;
	style.WindowPadding.y = 8;
	style.WindowRounding = 8.f; //4.f;
	style.GrabRounding = 3;
	style.ScrollbarSize = 15;
	style.ScrollbarRounding = 16;
	style.FramePadding.x = 4;
	style.FramePadding.y = 1;
	style.FrameRounding = 4;
	style.SelectableRounding = 2;
	style.ItemSpacing.x = 6;
	style.ItemSpacing.y = 4;
	style.WindowTitleAlign.x = 0.5f;
	style.IndentSpacing = 20.f;
	style.TitleBarHeight = 6.f;
	style.CloseButtonSize = 6.5f;
	style.DisplaySafeAreaPadding.x = 4.f;
	style.DisplaySafeAreaPadding.y = 4.f;
	style.TriangleScale = 0.85f;
	style.CheckmarkScale = 1.f/8.f;
	style.CircleLineSegment = 50;
	style.ColumnsMinSpacing = 0.f;

}

void StyleAdjust(bool styleShadow, bool invert,bool styleAntiAliased, float alpha,float saturate )
{
	if ( alpha < 0.1f ) alpha = 0.1f;

	ImGuiStyle& style = ImGui::GetStyle();
	style.AntiAliasedLines = styleAntiAliased;
	style.AntiAliasedFill = styleAntiAliased;
	style.FrameShadow = styleShadow;

	for (int i = 0; i <= ImGuiCol_COUNT; i++)
	{
		ImVec4& col = style.Colors[i];
		float H, S, V;
		ImGui::ColorConvertRGBtoHSV( col.x, col.y, col.z, H, S, V );

		if( invert && S < 0.1f )
		{
			V = 1.0f - V;
		}
		S *= saturate;
		if ( S < 0.f ) S = 0.f; else if (S > 1.f ) S = 1.f;
		ImGui::ColorConvertHSVtoRGB( H, S, V, col.x, col.y, col.z );
		//if( col.w < 1.00f )
		{
			col.w *= alpha;
		}
		if ( col.w < 0.f ) col.w = 0.f; else if ( col.w > 1.f ) col.w = 1.f;
	}
}

void SetStyle(int style,bool styleShadow,bool styleInvert,bool styleAntiAliased, float alpha,float saturate,int fontNr, float fontSize, float fontRasterMultiply, int fontoversample)
{
	float d = g_FontWishSize-fontSize;
	float d1 = g_FontWishRasterizerMultiply-fontRasterMultiply;
	float d2 = (float)g_OversampleHWish-fontoversample;
	if ( d*d >= 0.999f && fontSize>=1.f || g_StyleWish != style || d1*d1 >= 0.01f || d2*d2 >= 0.01f )
	{
		releaseFonts();
		g_FontWishSize = fontSize;
		if ( g_FontWishSize < 4.f ) g_FontWishSize = 4.f;
		g_StyleWish = style;
		g_FontWishRasterizerMultiply = fontRasterMultiply;
		g_OversampleHWish = fontoversample;
		if(g_OversampleHWish<1) g_OversampleHWish = 1;
	}

	g_FontWish = fontNr;
	{
		ImGuiIO& io = ImGui::GetIO();
		ImFontAtlas& atlas = *io.Fonts;
		io.FontDefault = g_FontWish > 0 && atlas.Fonts.size() > g_FontWish ? atlas.Fonts[g_FontWish] : 0;
		if (0 == io.FontDefault && atlas.Fonts.size() ) io.FontDefault = atlas.Fonts[0];
	}

	ImGui::GetStyle() = ImGuiStyle();
	if ( style == 1 ) style1();
	else if ( style == 2 ) style4();
	else if ( style == 3 ) style3();

	StyleAdjust(styleShadow,styleInvert,styleAntiAliased,alpha,saturate);
}

bool ImGui_ImplGLUT_Init()
{
	ImGui::CreateContext();
	// Application init
	ImGuiIO& io = ImGui::GetIO();
	//io.DisplaySize.x = 1920.0f;
	//io.DisplaySize.y = 1280.0f;
	io.IniFilename = "dock_window.json"; //"imgui.ini";


	//io.RenderDrawListsFn = ImGui_ImplGLUT_RenderDrawLists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
	//io.SetClipboardTextFn = ImGui_ImplGLUT_SetClipboardText;
	//io.GetClipboardTextFn = ImGui_ImplGLUT_GetClipboardText;

#ifdef _WIN32
    io.ImeWindowHandle = FindWindow ("FREEGLUT",0);
#endif
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;   // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;    // We can honor io.WantSetMousePos requests (optional, rarely used)
	io.ConfigResizeWindowsFromEdges = true;

	io.KeyMap[ImGuiKey_Tab] = 9;    // tab
	io.KeyMap[ImGuiKey_LeftArrow] = GLUT_KEY_LEFT + GLUT_SPECIAL_OFFSET;	// Left
	io.KeyMap[ImGuiKey_RightArrow] = GLUT_KEY_RIGHT + GLUT_SPECIAL_OFFSET;	// Right
	io.KeyMap[ImGuiKey_UpArrow] = GLUT_KEY_UP + GLUT_SPECIAL_OFFSET;		// Up
	io.KeyMap[ImGuiKey_DownArrow] = GLUT_KEY_DOWN + GLUT_SPECIAL_OFFSET;	// Down
	io.KeyMap[ImGuiKey_Home] = GLUT_KEY_HOME + GLUT_SPECIAL_OFFSET;			// Home
	io.KeyMap[ImGuiKey_End] = GLUT_KEY_END + GLUT_SPECIAL_OFFSET;			// End
	io.KeyMap[ImGuiKey_PageUp] = GLUT_KEY_PAGE_UP + GLUT_SPECIAL_OFFSET;
	io.KeyMap[ImGuiKey_PageDown] = GLUT_KEY_PAGE_DOWN + GLUT_SPECIAL_OFFSET;

	io.KeyMap[ImGuiKey_Delete] = 127;  // Delete
	io.KeyMap[ImGuiKey_Backspace] = 8;    // Backspace
	io.KeyMap[ImGuiKey_Enter] = 13;   // Enter
	io.KeyMap[ImGuiKey_Escape] = 27;  // Escape
	io.KeyMap[ImGuiKey_Space] = 32;  // Space
	io.KeyMap[ImGuiKey_A] = 1;   // for text edit CTRL+A: select all
	io.KeyMap[ImGuiKey_C] = 3;   // for text edit CTRL+C: copy
	io.KeyMap[ImGuiKey_V] = 22;  // for text edit CTRL+V: paste
	io.KeyMap[ImGuiKey_X] = 24;  // for text edit CTRL+X: cut
	io.KeyMap[ImGuiKey_Y] = 25;  // for text edit CTRL+Y: redo
	io.KeyMap[ImGuiKey_Z] = 26;  // for text edit CTRL+Z: undo
	io.KeyMap[ImGuiKey_D] = 4;   // for text edit CTRL+D: deselect all
	io.KeyMap[ImGuiKey_R] = 18;  // for text edit CTRL+R: revert
	return true;
}

void ImGui_ImplGLUT_Shutdown()
{
	ImGui_ImplGLUT_InvalidateDeviceObjects();
	ImGui::DestroyContext();
}


