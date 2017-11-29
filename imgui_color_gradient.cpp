//
//  imgui_color_gradient.cpp
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.


#include "imgui_color_gradient.h"
#include "imgui_internal.h"
#define snprintf _snprintf

ImGradient::ImGradient()
{
	//default black white
	addMark(0.0f, ImColor(0.0f,0.0f,0.0f,1.f));
	addMark(1.0f, ImColor(1.0f,1.0f,1.0f,1.f));

	setFilename("colormap", ".txt");
	gui_expanded = false;
	gui_child = true;
	gui_floating  = false;
	draggingMark = 0;
	selectedMark = 0;
}

ImGradient::~ImGradient()
{
	clear();
}

void ImGradient::clear()
{
	draggingMark = 0;
	selectedMark = 0;
	for(ImGradientMarkList::const_iterator m = marks.begin(); m != marks.end(); ++m) delete *m;

	marks.clear();
}

void ImGradient::setFilename( const char* filename, const char *n2 )
{
	snprintf(fileName,sizeof(fileName),"%s%s",filename ? filename : "bscan", n2 ? n2 : "");
	fileName[sizeof(fileName)-1] = 0;
}

void ImGradient::save()
{
	FILE *f = fopen(fileName,"wb");
	if (!f) return;

	refreshCache();
	for(ImGradientMarkList::const_iterator markIt = marks.begin(); markIt != marks.end(); ++markIt)
	{
		const ImGradientMark* m = *markIt;
		fprintf(f,"%f %f %f %f %f\n",m->position,m->color[0],m->color[1],m->color[2],m->color[3] );
	}
	fclose(f);
}

bool ImGradient::load()
{
	FILE *f = fopen(fileName,"rb");
	if (!f) return false;

	clear();
	ImGradientMark m;
	while (fscanf(f,"%f %f %f %f %f\n",&m.position,m.color+0,m.color+1,m.color+2,m.color+3 ) == 5) addMark(m);

	fclose(f);
	return marks.size() > 0;
}

void ImGradient::addMark(const ImGradientMark& mark)
{
	ImGradientMark* newMark = new ImGradientMark();
	newMark->position = ImClamp(mark.position, 0.f, 1.f);
	newMark->color[0] = ImClamp(mark.color[0], 0.f, 1.f);
	newMark->color[1] = ImClamp(mark.color[1], 0.f, 1.f);
	newMark->color[2] = ImClamp(mark.color[2], 0.f, 1.f);
	newMark->color[3] = ImClamp(mark.color[3], 0.f, 1.f);

	marks.push_back(newMark);
}

void ImGradient::addMark(float position, const ImColor& color)
{
	ImGradientMark* newMark = new ImGradientMark();
	newMark->position = ImClamp(position,0.f,1.f);
	newMark->color[0] = color.Value.x;
	newMark->color[1] = color.Value.y;
	newMark->color[2] = color.Value.z;
	newMark->color[3] = color.Value.w;

	marks.push_back(newMark);
}

void ImGradient::removeMark(ImGradientMark* mark)
{
	if (mark)
	{
		marks.remove(mark);
		delete mark;
	}
	draggingMark = 0;
	selectedMark = 0;
}

/*
void ImGradient::getColorAt(float position, float color[4]) const
{
	position = ImClamp(position, 0.0f, 1.0f);

	int cachePos = (int)floorf(position * 255.f);
	cachePos *= 4;
	color[0] = m_cachedValues[cachePos + 0];
	color[1] = m_cachedValues[cachePos + 1];
	color[2] = m_cachedValues[cachePos + 2];
	color[3] = m_cachedValues[cachePos + 3];
}
*/

void ImGradient::computeColorAt(float position, float color[4]) const
{
	position = ImClamp(position, 0.0f, 1.0f);

	ImGradientMark* lower = 0;
	ImGradientMark* upper = 0;

	for(ImGradientMark* mark : marks)
	{
		if(mark->position < position)
		{
			if(!lower || lower->position < mark->position)
			{
				lower = mark;
			}
		}

		if(mark->position >= position)
		{
			if(!upper || upper->position > mark->position)
			{
				upper = mark;
			}
		}
	}

	if(upper && !lower)
	{
		lower = upper;
	}
	else if(!upper && lower)
	{
		upper = lower;
	}
	else if(!lower && !upper)
	{
		color[0] = color[1] = color[2] = color[3] = 0.f;
		return;
	}

	if(upper == lower)
	{
		color[0] = upper->color[0];
		color[1] = upper->color[1];
		color[2] = upper->color[2];
		color[3] = upper->color[3];
	}
	else
	{
		float distance = upper->position - lower->position;
		float delta = fabs(distance) > 0.0001f ? (position - lower->position) / distance : 0.f;

		//lerp
		color[0] = ((1.f - delta) * lower->color[0]) + ((delta)* upper->color[0]);
		color[1] = ((1.f - delta) * lower->color[1]) + ((delta)* upper->color[1]);
		color[2] = ((1.f - delta) * lower->color[2]) + ((delta)* upper->color[2]);
		color[3] = ((1.f - delta) * lower->color[3]) + ((delta)* upper->color[3]);
	}
}

void ImGradient::refreshCache()
{
	for(ImGradientMarkList::const_iterator markIt = marks.begin(); markIt != marks.end(); ++markIt)
	{
		ImGradientMark* mark = *markIt;
		if ( mark->position< 0.f ) mark->position = 0.f;
		else if ( mark->position > 1.f ) mark->position = 1.f;
	}

	marks.sort([](const ImGradientMark * a, const ImGradientMark * b) { return a->position < b->position; });
}

void ImGradient::sample1D (unsigned int*dest, int size)
{
	ImVec4 c;
	ImVec4 d;

	refreshCache();

	const ImGradientMark *prevMark = 0;
	for(ImGradientMarkList::const_iterator markIt = marks.begin(); markIt != marks.end(); ++markIt)
	{
		const ImGradientMark* mark = *markIt;
		int dsx = prevMark ? (int) floorf(prevMark->position * size) : 0;
		if (!prevMark) prevMark = mark;
		int dex = (int) floorf(mark->position * size);
		int dx = dex - dsx;
		if ( dx <= 0 )
		{
			prevMark = mark;
			continue;
		}

		c.x = prevMark->color[0];
		c.y = prevMark->color[1];
		c.z = prevMark->color[2];
		c.w = prevMark->color[3];

		float idx = 1.f / dx;
		d.x = (mark->color[0] - c.x)*idx;
		d.y = (mark->color[1] - c.y)*idx;
		d.z = (mark->color[2] - c.z)*idx;
		d.w = (mark->color[3] - c.w)*idx;

		register ImU32 out;
		for ( int i = dsx; i < dex; ++i )
		{
			out  = (int)floorf(c.x * 255.f) << 16;
			out |= (int)floorf(c.y * 255.f) << 8;
			out |= (int)floorf(c.z * 255.f);
			out |= (int)floorf(c.w * 255.f) << 24;

			dest[i] = out;

			c.x += d.x;
			c.y += d.y;
			c.z += d.z;
			c.w += d.w;
		}
		prevMark = mark;
	}

	if(prevMark && prevMark->position < 1.f)
	{
		int dsx = (int) floorf(prevMark->position * size);

		register ImU32 out;
		out  = (int)floorf(prevMark->color[0] * 255.f) << 16;
		out |= (int)floorf(prevMark->color[1] * 255.f) << 8;
		out |= (int)floorf(prevMark->color[2] * 255.f);
		out |= (int)floorf(prevMark->color[3] * 255.f) << 24;

		for ( int i = dsx; i < size; ++i )
		{
			dest[i] = out;
		}
	}

}


namespace ImGui
{
	static void DrawGradientBar(ImGradient* gradient,
	struct ImVec2 const & bar_pos,
		float maxWidth,
		float height)
	{
		ImVec4 colorA( 1, 1, 1, 1);
		ImVec4 colorB(1, 1, 1, 1);
		float prevX = bar_pos.x;
		float barBottom = bar_pos.y + height;
		ImGradientMark* prevMark = 0;
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const ImGuiStyle& style = ImGui::GetStyle();

		/*
				draw_list->AddRectFilled(ImVec2(bar_pos.x - 2, bar_pos.y - 2),
				ImVec2(bar_pos.x + maxWidth + 2, barBottom + 2),
				IM_COL32(100, 100, 100, 255));

				if(gradient->getMarks().size() == 0)
				{
				draw_list->AddRectFilled(ImVec2(bar_pos.x, bar_pos.y),
				ImVec2(bar_pos.x + maxWidth, barBottom),
				IM_COL32(255, 255, 255, 255));

				}
				*/
		ImU32 colorAU32 = 0;
		ImU32 colorBU32 = 0;

		for(ImGradientMarkList::const_iterator markIt = gradient->getMarks().begin(); markIt != gradient->getMarks().end(); ++markIt)
		{
			ImGradientMark* mark = *markIt;

			float from = prevX;
			float to = prevX = bar_pos.x + mark->position * maxWidth;

			if(prevMark == 0) prevMark = mark;
			colorA.x = prevMark->color[0] * style.Brightness;
			colorA.y = prevMark->color[1] * style.Brightness;
			colorA.z = prevMark->color[2] * style.Brightness;
			colorA.w = prevMark->color[3] * style.Alpha;

			colorB.x = mark->color[0] * style.Brightness;
			colorB.y = mark->color[1] * style.Brightness;
			colorB.z = mark->color[2] * style.Brightness;
			colorB.w = mark->color[3] * style.Alpha;

			colorAU32 = ImGui::ColorConvertFloat4ToU32(colorA);
			colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);

			if(mark->position > 0.f)
			{
				draw_list->AddRectFilledMultiColor(ImVec2(from, bar_pos.y),
					ImVec2(to, barBottom),
					colorAU32, colorBU32, colorBU32, colorAU32);
			}

			prevMark = mark;
		}

		if(prevMark && prevMark->position < 1.0)
		{
			draw_list->AddRectFilledMultiColor(ImVec2(prevX, bar_pos.y),
				ImVec2(bar_pos.x + maxWidth, barBottom),
				colorBU32, colorBU32, colorBU32, colorBU32);
		}

		//ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + height + 9.0f));
	}

	static void DrawGradientMarks(ImGradient* gradient,	struct ImVec2 const & bar_pos, float maxWidth, float height)
	{
		ImVec4 colorA(1, 1, 1, 1);
		ImVec4 colorB(1, 1, 1, 1);
		float barBottom = bar_pos.y + height;
		ImGradientMark* prevMark = 0;
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImU32 colorAU32 = 0;
		ImU32 colorBU32 = 0;
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImU32 black = IM_COL32(0, 0, 0, (int)floorf(style.Alpha * 255.f));

		for(ImGradientMarkList::const_iterator markIt = gradient->getMarks().begin(); markIt != gradient->getMarks().end(); ++markIt)
		{
			ImGradientMark* mark = *markIt;

			if(!gradient->selectedMark) gradient->selectedMark = mark;

			float to = bar_pos.x + mark->position * maxWidth;

			if(prevMark == 0) prevMark = mark;
			colorA.x = prevMark->color[0] * style.Brightness;
			colorA.y = prevMark->color[1] * style.Brightness;
			colorA.z = prevMark->color[2] * style.Brightness;
			colorA.w = prevMark->color[3] * style.Alpha;

			colorB.x = mark->color[0] * style.Brightness;
			colorB.y = mark->color[1] * style.Brightness;
			colorB.z = mark->color[2] * style.Brightness;
			colorB.w = mark->color[3] * style.Alpha;

			colorAU32 = ImGui::ColorConvertFloat4ToU32(colorA);
			colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);

/*
			draw_list->AddTriangleFilled(ImVec2(to, bar_pos.y + (height - 6)),
				ImVec2(to - 6, barBottom),
				ImVec2(to + 6, barBottom), IM_COL32(100, 100, 100, a));

			draw_list->AddRectFilled(ImVec2(to - 6, barBottom),
				ImVec2(to + 6, bar_pos.y + (height + 12)),
				IM_COL32(100, 100, 100, a), 1.0f, 1);
*/
/*
			draw_list->AddRectFilled(ImVec2(to - 3, bar_pos.y + (height + 1)),
				ImVec2(to + 3, bar_pos.y + (height + 11)),
				IM_COL32(0, 0, 0, a), 1.0f, 1);
*/
			ImVec2 a(to - 3, bar_pos.y + (height + 3));
			ImVec2 b(to + 0, bar_pos.y + (height + 0));
			ImVec2 c(to + 3, bar_pos.y + (height + 3));

			if(gradient->selectedMark == mark)
			{
				draw_list->AddTriangleFilled(a,b,c,black );
			}
			else
			{
				draw_list->AddTriangle(a,b,c,black,1.f,false );
			}

			draw_list->AddRectFilledMultiColor(ImVec2(to - 3.5f, bar_pos.y + (height + 3)),
				ImVec2(to + 3.5f, bar_pos.y + (height + 11)),
				colorBU32, colorBU32, colorBU32, colorBU32);

			ImGui::SetCursorScreenPos(ImVec2(to - 3, barBottom));
			ImGui::InvisibleButton("mark", ImVec2(8, 8));

			if(ImGui::IsItemHovered())
			{
				if(ImGui::IsMouseClicked(0))
				{
					gradient->selectedMark = mark;
					gradient->draggingMark = mark;
				}
			}


			prevMark = mark;
		}

		//ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + height + 20.0f));
	}

	bool GradientButton(ImGradient* gradient)
	{
		if(!gradient) return false;

		ImVec2 widget_pos = ImGui::GetCursorScreenPos();
		// ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float maxWidth = ImMax(250.0f, ImGui::GetContentRegionAvailWidth() - 100.0f);
		bool clicked = ImGui::InvisibleButton("gradient_bar", ImVec2(maxWidth, GRADIENT_BAR_WIDGET_HEIGHT));

		DrawGradientBar(gradient, widget_pos, maxWidth, GRADIENT_BAR_WIDGET_HEIGHT);

		return clicked;
	}

	bool GradientEditor(ImGradient* gradient, float gradient_bar_editor_height)
	{
		if(!gradient) return false;

		bool modified = false;

		ImVec2 bar_pos = ImGui::GetCursorScreenPos();
		bar_pos.x += GRADIENT_BAR_EDITOR_INDENT_X;
		float maxWidth = ImGui::GetContentRegionAvailWidth() - (GRADIENT_BAR_EDITOR_INDENT_X*2.f);
		if(maxWidth<1.f) maxWidth = 1.f;
		float barBottom = bar_pos.y + gradient_bar_editor_height;

		ImGui::InvisibleButton("gradient_editor_bar", ImVec2(maxWidth, gradient_bar_editor_height));

		if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
		{
			float pos = (ImGui::GetIO().MousePos.x - bar_pos.x) / maxWidth;

			float newMarkCol[4];
			gradient->computeColorAt(pos, newMarkCol);
			gradient->addMark(pos, ImColor(newMarkCol[0], newMarkCol[1], newMarkCol[2],newMarkCol[3]));
			modified = true;
		}

		DrawGradientBar(gradient, bar_pos, maxWidth, gradient_bar_editor_height);
		DrawGradientMarks(gradient, bar_pos, maxWidth, gradient_bar_editor_height);

		if(!ImGui::IsMouseDown(0) && gradient->draggingMark)
		{
			gradient->draggingMark = 0;
		}

		if(ImGui::IsMouseDragging(0) && gradient->draggingMark)
		{
			float increment = ImGui::GetIO().MouseDelta.x / maxWidth;
			bool insideZone = (ImGui::GetIO().MousePos.x > bar_pos.x) &&
				(ImGui::GetIO().MousePos.x < bar_pos.x + maxWidth);

			if(increment != 0.0f && insideZone)
			{
				gradient->draggingMark->position += increment;
				gradient->draggingMark->position = ImClamp(gradient->draggingMark->position, 0.0f, 1.0f);
				modified = true;
			}

			float diffY = ImGui::GetIO().MousePos.y - barBottom;

			if(diffY >= GRADIENT_MARK_DELETE_DIFFY && gradient->getMarks().size() > 1)
			{
				gradient->removeMark(gradient->draggingMark);
				modified = true;
			}
		}

		if(!gradient->selectedMark && gradient->getMarks().size() > 0)
		{
			gradient->selectedMark = gradient->getMarks().front();
		}

#if 0
		if(gradient->selectedMark)
		{
			PushItemWidth(30);
			modified |= ImGui::DragFloat("##pos", &gradient->selectedMark->position,0.002f,0.f,1.f,"%.2f");
			PopItemWidth();
			SameLine();
			modified |= ImGui::ColorEdit4("##col", gradient->selectedMark->color);
		}
#endif
#if 1
		char id[64];
		int i = 0;
		for(ImGradientMarkList::const_iterator markIt = gradient->getMarks().begin(); markIt != gradient->getMarks().end(); ++markIt)
		{
			ImGradientMark* mark = *markIt;
			sprintf(id,"##p%d",i);
			PushItemWidth(30);
			int show = (int) floorf(mark->position * 255.f);
			if ( DragInt(id, &show,1.f,0,255 ) )
			{
				mark->position = show * (1.f/255.f);
				modified |= true;
			}
			//modified |= DragFloat(id, &mark->position,0.002f,0.f,1.f,"%.2f");
			PopItemWidth();
			SameLine();

			sprintf(id,"##c%d",i);
			PushItemWidth(GetContentRegionAvailWidth()-GRADIENT_BAR_EDITOR_INDENT_X);
			modified |= ColorEdit4(id, mark->color);
			PopItemWidth();
			i += 1;
		}
#endif
		if(modified)
		{
			gradient->refreshCache();
		}

		return modified;
	}
};
