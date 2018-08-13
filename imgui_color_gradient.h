//
//  imgui_color_gradient.h
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.

/*
 
 Usage:
 
 ::GRADIENT DATA::
 ImGradient gradient;
 
 ::BUTTON::
 if(ImGui::GradientButton(&gradient))
 {
    //set show editor flag to true/false
 }
 
 ::EDITOR::
 static ImGradientMark* draggingMark = nullptr;
 static ImGradientMark* selectedMark = nullptr;
 
 bool updated = ImGui::GradientEditor(&gradient, draggingMark, selectedMark);
 
 ::GET A COLOR::
 float color[3];
 gradient.getColorAt(0.3f, color); //position from 0 to 1
 
 ::MODIFY GRADIENT WITH CODE::
 gradient.getMarks().clear();
 gradient.addMark(0.0f, ImColor(0.2f, 0.1f, 0.0f));
 gradient.addMark(0.7f, ImColor(120, 200, 255));
 
 ::WOOD BROWNS PRESET::
 gradient.getMarks().clear();
 gradient.addMark(0.0f, ImColor(0xA0, 0x79, 0x3D));
 gradient.addMark(0.2f, ImColor(0xAA, 0x83, 0x47));
 gradient.addMark(0.3f, ImColor(0xB4, 0x8D, 0x51));
 gradient.addMark(0.4f, ImColor(0xBE, 0x97, 0x5B));
 gradient.addMark(0.6f, ImColor(0xC8, 0xA1, 0x65));
 gradient.addMark(0.7f, ImColor(0xD2, 0xAB, 0x6F));
 gradient.addMark(0.8f, ImColor(0xDC, 0xB5, 0x79));
 gradient.addMark(1.0f, ImColor(0xE6, 0xBF, 0x83));
 
 */

#pragma once

#include "imgui.h"

//#include <list>

static const float GRADIENT_BAR_WIDGET_HEIGHT = 5;
//static const float GRADIENT_BAR_EDITOR_HEIGHT = 40;
static const float GRADIENT_MARK_DELETE_DIFFY = 40;
static const float GRADIENT_BAR_EDITOR_INDENT_X = 10;

struct ImGradientMark
{
    float color[4];
    float position; //0 to 1
};
//typedef std::list<ImGradientMark*> ImGradientMarkList;
//typedef std::list<ImGradientMark> ImGradientMarkList;
typedef ImVector<ImGradientMark> ImGradientMarkList;

class ImGradient
{
public:
    ImGradient();
    ~ImGradient();
    
    //void getColorAt(float position, float color[4]) const;
	void computeColorAt(float position, float color[4]) const;
    void addMark(float position, const ImColor& color);
	void addMark(const ImGradientMark& mark);
    void removeMark(ImGradientMark* mark);
	void clear();
    void refreshCache();
    ImGradientMarkList& getMarks(){ return marks; }
	void sample1D (unsigned int*dest, int size);

	void setFilename( const char* filename, const char *n2 );
    char fileName[2048];
	bool load();
	void save();
	ImGradientMark* draggingMark;
	ImGradientMark* selectedMark;
	bool gui_expanded;
	bool gui_child;
	bool gui_floating;

private:
    ImGradientMarkList marks;

    //float m_cachedValues[256 * 4];
};

namespace ImGui
{
    bool GradientButton(ImGradient* gradient);
    bool GradientEditor(ImGradient* gradient,const ImVec2& gradient_bar_editor_height);

}
