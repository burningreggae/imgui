#include "imgui.h"
#define IMGUI_DEFINE_PLACEMENT_NEW
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_internal.h"
#include "imgui_dock.h"
#include <stdlib.h>

#define strcpy(a,b) strncpy((a),(b),sizeof((a))),(a)[sizeof((a))-1] = 0;

// https://bitbucket.org/duangle/liminal/src/tip/src/liminal/imgui_dock.cpp?fileviewer=file-view-default
const char* COM_Parse( const char* *data_p, bool allowLineBreaks = true );
char* loadFile(const char* filename);
int saveFile(const char* filename, const void* data, size_t size);
bool canDock();

#define DOCK_GUARD 1000

namespace ImGui
{

enum EndAction_
{
	EndAction_None,
	EndAction_Panel,
	EndAction_End,
	EndAction_EndChild
};




struct Dock
{
	Dock()
		: id(0)
		, next_tab(0)
		, prev_tab(0)
		, parent(0)
		, pos(0, 0)
		, size(-1, -1)
		, active(true)
		, status(Status_Float)
		, opened(false)
		, last_frame(0)
		, invalid_frames(0)
		, first(true)
	{
		label[0] = 0;
		location[0] = 0;
		children[0] = children[1] = 0;
	}


	virtual ~Dock()
	{
		//label.clear();
		//label[0] = 0;
		invalid_frames += 1;
		//printf("Free for %p %s\n",this,label);
	}


	ImVec2 getMinSize() const
	{
		if (!children[0]) return ImVec2(16, 16 + GetTextLineHeightWithSpacing());

		ImVec2 s0 = children[0]->getMinSize();
		ImVec2 s1 = children[1]->getMinSize();
		return isHorizontal() ? ImVec2(s0.x + s1.x, ImMax(s0.y, s1.y))
			: ImVec2(ImMax(s0.x, s1.x), s0.y + s1.y);
	}


	bool isHorizontal() const { return children[0]->pos.x < children[1]->pos.x; }


	void setParent(Dock* dock)
	{
		parent = dock;
		int run = 0;
		for (Dock* tmp = prev_tab; tmp && ++run < DOCK_GUARD; tmp = tmp->prev_tab) tmp->parent = dock;
		for (Dock* tmp = next_tab; tmp && ++run < DOCK_GUARD; tmp = tmp->next_tab) tmp->parent = dock;
	}

	Dock& getRoot()
	{
		Dock *dock = this;
		while (dock->parent)
			dock = dock->parent;
		return *dock;
	}

	Dock* getSibling()
	{
		IM_ASSERT(parent);
		if (parent->children[0] == getFirstTab()) return parent->children[1];
		return parent->children[0];
	}


	Dock* getFirstTab()
	{
		Dock* tmp = this;
		int run = 0;
		while (tmp->prev_tab && ++run < DOCK_GUARD) tmp = tmp->prev_tab;
		return tmp;
	}


	void setActive()
	{
		//msg ("setActive: %s\n",label);
		active = true;
		system_redraw(__FUNCTION__,label);
		int run = 0;
		Dock* tmp;
		for (tmp = prev_tab; tmp && ++run < DOCK_GUARD; tmp = tmp->prev_tab) tmp->active = false;
		for (tmp = next_tab; tmp && ++run < DOCK_GUARD; tmp = tmp->next_tab) tmp->active = false;
	}


	bool isContainer() const { return children[0] != 0; }


	void setChildrenPosSize(const ImVec2& _pos, const ImVec2& _size)
	{
		int divisor;
		ImVec2 s = children[0]->size;
		if (isHorizontal())
		{
			s.y = _size.y;
			divisor = (children[0]->size.x + children[1]->size.x);
			s.x = (float) (divisor ? int( _size.x * children[0]->size.x / divisor ) : 0);

			if (s.x < children[0]->getMinSize().x)
			{
				s.x = children[0]->getMinSize().x;
			}
			else if (_size.x - s.x < children[1]->getMinSize().x)
			{
				s.x = _size.x - children[1]->getMinSize().x;
			}
			children[0]->setPosSize(_pos, s);

			s.x = _size.x - children[0]->size.x;
			ImVec2 p = _pos;
			p.x += children[0]->size.x;
			children[1]->setPosSize(p, s);
		}
		else
		{
			s.x = _size.x;
			divisor = (children[0]->size.y + children[1]->size.y);
			s.y = (float) (divisor ? int( _size.y * children[0]->size.y / divisor ) : 0);
			if (s.y < children[0]->getMinSize().y)
			{
				s.y = children[0]->getMinSize().y;
			}
			else if (_size.y - s.y < children[1]->getMinSize().y)
			{
				s.y = _size.y - children[1]->getMinSize().y;
			}
			children[0]->setPosSize(_pos, s);

			s.y = _size.y - children[0]->size.y;
			ImVec2 p = _pos;
			p.y += children[0]->size.y;
			children[1]->setPosSize(p, s);
		}
	}


	void setPosSize(const ImVec2& _pos, const ImVec2& _size)
	{
		//don't destroy dock layout completly..
		ImVec2 _size2(_size);
		if (_size2.x < 32 ) _size2.x = 32;
		if (_size2.y < 32 ) _size2.y = 32;

		size = _size2;
		pos = _pos;

		int run = 0;
		for (Dock* tmp = prev_tab; tmp && ++run < DOCK_GUARD; tmp = tmp->prev_tab)
		{
			tmp->size = _size2;
			tmp->pos = _pos;
		}
		for (Dock* tmp = next_tab; tmp && ++run < DOCK_GUARD; tmp = tmp->next_tab)
		{
			tmp->size = _size2;
			tmp->pos = _pos;
		}

		if (!isContainer()) return;
		setChildrenPosSize(_pos, _size2);
	}

	ImU32 id;
	//ImVector<char> label;
	char label[1024];

	Dock* next_tab;
	Dock* prev_tab;
	Dock* children[2];
	Dock* parent;
	bool active;
	ImVec2 pos;
	ImVec2 size;
	DockState status; // Status_
	int last_frame;
	int invalid_frames;
	char location[16];
	bool opened;
	bool first;
};


struct DockContext
{
	DockContext();
	virtual ~DockContext();

	ImVector<Dock*> m_docks;
	ImVec2 m_drag_offset;
	Dock* m_current;
	Dock *m_next_parent;
	int m_last_frame;
	EndAction_ m_end_action;
	ImVec2 m_workspace_pos;
	ImVec2 m_workspace_size;
	Slot_ m_next_dock_slot;
	bool dock_open;
	bool asChild;
	//ImVector<char> label;
	char label[1024];
	int index;
	int draw_tabbar;
	int draw_tabbar_list;

	void reset();
	void verify();
	Dock& getDock(const char* label, bool opened, const ImVec2& default_size);
	void putInBackground();
	//void beginPanel();
	//void endPanel();
	void splits();
	void checkNonexistent();
	Dock* getDockAt(const ImVec2& pos);
	void doUndock(Dock& dock);

	Dock* getRootDock();
	void doDock(Dock& dock, Dock* dest, Slot_ dock_slot);
	bool dockSlots(Dock& dock, Dock* dest_dock, const ImRect& rect, bool on_border);
	void handleDrag(Dock& dock);
	bool tabbar(Dock* dock, bool close_button, bool enabled, bool need_HorizontalScrollbar);
	void rootDock(const ImVec2& pos, const ImVec2& size);
	void setDockActive(const char* name, int disable_all_other);
	void setDockWindowPos(const char* name, const ImVec2& pos, const ImVec2& size,DockState newState);
	const char* getDockActive();
	void tryDockToStoredLocation(Dock& dock);
	bool begin(const char* label, bool* opened, bool border, ImGuiWindowFlags extra_flags, const ImVec2& default_size);
	void end();
	void designer(bool *v_open);
	int getDockIndex(Dock* dock);
	Dock* getDockByIndex(const char *name);
	void save(ImGuiTextBuffer &out);
	void load(int slot, const char *filename, int reload=0);
	void dump();
};

DockContext::DockContext()
{
	reset();
}

DockContext::~DockContext()
{
	reset();
}


void DockContext::reset()
{
	for (int i = 0; i < m_docks.size(); ++i)
	{
		if ( m_docks[i] )
		{
			m_docks[i]->~Dock();
			MemFree(m_docks[i]);
			m_docks[i] = 0;
		}
	}
	m_docks.clear();

	m_current = 0;
	m_last_frame = 0;
	m_next_parent = 0;
	m_next_dock_slot = Slot_Right;
	dock_open = true;
	asChild = true;
	//label.clear();
	label[0] = 0;
	index = 0;
	draw_tabbar = 1;
	draw_tabbar_list = 1;
	m_end_action = EndAction_None;
}

Dock& DockContext::getDock(const char* label, bool opened, const ImVec2& default_size)
{
	ImU32 id = ImHash(label, 0);
	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (m_docks[i]->id == id) return *m_docks[i];
	}

	//dump();

	Dock* new_dock = (Dock*)MemAlloc(sizeof(Dock));
	IM_PLACEMENT_NEW(new_dock) Dock();
	m_docks.push_back(new_dock);
	//new_dock->label.ImStrdup(label);
	strcpy(new_dock->label,label ? label : "");

	//IM_ASSERT(new_dock->label);
	new_dock->id = id;
	new_dock->setActive();
	//new_dock->status = Status_Float;
	new_dock->status = (m_docks.size() == 1)?Status_Docked:Status_Float;
	new_dock->pos = ImVec2(0, 0);
	//new_dock->size.x = 256; // = GetIO().DisplaySize;
	//new_dock->size.y = 256;
	new_dock->size.x = default_size.x < 0 ? GetIO().DisplaySize.x : default_size.x;
	new_dock->size.y = default_size.y < 0 ? GetIO().DisplaySize.y : default_size.y;
	new_dock->opened = opened;
	new_dock->first = true;
	new_dock->last_frame = 0; //ImGui::GetFrameCount();
	new_dock->invalid_frames = 0;
	new_dock->location[0] = 0;
	return *new_dock;
}


void DockContext::putInBackground()
{
	ImGuiWindow* win = GetCurrentWindow();
	ImGuiContext& g = *GImGui;
	if (g.Windows[0] == win) return;

	for (int i = 0; i < g.Windows.Size; i++)
	{
		if (g.Windows[i] == win)
		{
			for (int j = i - 1; j >= 0; --j)
			{
				g.Windows[j + 1] = g.Windows[j];
			}
			g.Windows[0] = win;
			break;
		}
	}
}


void DockContext::splits()
{
	if (GetFrameCount() == m_last_frame) return;
	m_last_frame = GetFrameCount();

	putInBackground();

	int i;
	for (i = 0; i < m_docks.size(); ++i)
	{
		Dock& dock = *m_docks[i];
		if (!dock.parent && (dock.status == Status_Docked))
		{
			dock.setPosSize(m_workspace_pos, m_workspace_size);
		}
	}

	ImU32 color = GetColorU32(ImGuiCol_Separator);
	ImU32 color_hovered = GetColorU32(ImGuiCol_SeparatorHovered);
	ImDrawList* draw_list = GetWindowDrawList();
	ImGuiIO& io = GetIO();
	for (i = 0; i < m_docks.size(); ++i)
	{
		Dock& dock = *m_docks[i];
		if (!dock.isContainer()) continue;

		PushID(i);
		if (!IsMouseDown(0)) dock.status = Status_Docked;

		ImVec2 pos0 = dock.children[0]->pos;
		ImVec2 pos1 = dock.children[1]->pos;
		ImVec2 size0 = dock.children[0]->size;
		ImVec2 size1 = dock.children[1]->size;
            

		ImVec2 size = dock.children[0]->size;
		ImVec2 dsize(0, 0);
		SetCursorScreenPos(dock.children[1]->pos);
		ImVec2 min_size0 = dock.children[0]->getMinSize();
		ImVec2 min_size1 = dock.children[1]->getMinSize();

		bool hovered = false;
		bool held = false;

		ImGuiMouseCursor cursor = ImGuiMouseCursor_Arrow;
		if (dock.isHorizontal())
		{
			cursor = ImGuiMouseCursor_ResizeEW;
			SetCursorScreenPos(ImVec2(dock.pos.x + size0.x, dock.pos.y));

			InvisibleButton("split", ImVec2(2.f, dock.size.y),&hovered,&held);
			if (dock.status == Status_Dragged) dsize.x = io.MouseDelta.x;
			dsize.x = -ImMin(-dsize.x, dock.children[0]->size.x - min_size0.x);
			dsize.x = ImMin(dsize.x, dock.children[1]->size.x - min_size1.x);

			size0 += dsize;
			size1 -= dsize;
			pos0 = dock.pos;
			pos1.x = pos0.x + size0.x + 4.f;
			pos1.y = dock.pos.y;
			size0.y = size1.y = dock.size.y;
			size1.x = ImMax(min_size1.x, dock.size.x - size0.x);
			size0.x = ImMax(min_size0.x, dock.size.x - size1.x);
		}
		else
		{
			cursor = ImGuiMouseCursor_ResizeNS;
			SetCursorScreenPos(ImVec2(dock.pos.x, dock.pos.y + size0.y));

			InvisibleButton("split", ImVec2(dock.size.x, 2.f),&hovered,&held);
			if (dock.status == Status_Dragged) dsize.y = io.MouseDelta.y;
			dsize.y = -ImMin(-dsize.y, dock.children[0]->size.y - min_size0.y);
			dsize.y = ImMin(dsize.y, dock.children[1]->size.y - min_size1.y);

			size0 += dsize;
			size1 -= dsize;
			pos0 = dock.pos;
			pos1.x = dock.pos.x;
			pos1.y = pos0.y + size0.y + 4.f;
			size0.x = size1.x = dock.size.x;
			size1.y = ImMax(min_size1.y, dock.size.y - size0.y);
			size0.y = ImMax(min_size0.y, dock.size.y - size1.y);
		}
/*
		ImVec2 new_size0 = dock.children[0]->size + dsize;
		ImVec2 new_size1 = dock.children[1]->size - dsize;
		ImVec2 new_pos1 = dock.children[1]->pos + dsize;
		dock.children[0]->setPosSize(dock.children[0]->pos, new_size0);
		dock.children[1]->setPosSize(new_pos1, new_size1);
*/

		dock.children[0]->setPosSize(pos0, size0);
		dock.children[1]->setPosSize(pos1, size1);

		const bool isItemHovered = hovered || held; //IsItemHovered();
		if (isItemHovered)
		{
	        SetMouseCursor(cursor);
			system_redraw(__FUNCTION__,dock.label);
		}
            
		if (isItemHovered && IsMouseClicked(0))
		{
			dock.status = Status_Dragged;
		}
		// split line
		//draw_list->AddRectFilled(GetItemRectMin(), GetItemRectMax(), isItemHovered ? color_hovered : color);
		{
			ImVec2 a;
			ImVec2 b;
			if (dock.isHorizontal())
			{
				a.x = dock.pos.x + size0.x + 1.f;
				a.y = dock.pos.y;
				b.x = a.x;
				b.y = a.y + dock.size.y;
			}
			else
			{
				a.x = dock.pos.x;
				a.y = dock.pos.y + size0.y + 1.f;
				b.x = a.x + dock.size.x;
				b.y = a.y;
			}
			

			//draw_list->AddLine(a, b, isItemHovered ? color_hovered : color, 1.f, false);
			draw_list->AddLine(a, b, GetColorU32(ImGuiCol_Separator,ImGuiCol_SeparatorHovered,
				envelope_get(GetID("split"))), 1.f);
		}
		PopID();
	}
}


void DockContext::checkNonexistent()
{
	int frame_limit = ImMax(0, ImGui::GetFrameCount() - 2);
	//ImVector<char> label;

	for (int i = 0; i < m_docks.size(); ++i)
	{
		Dock& dock = *m_docks[i];

		if (dock.isContainer()) continue;
		if (dock.status == Status_Float) continue;
		if (dock.last_frame < frame_limit)
		{
			dock.invalid_frames += 1;
			if (dock.invalid_frames > 5)
			{
				//Call as closed dock
				bool open = false;
				//label.ImStrdup(dock.label.c_str());
				//strcpy(label,dock.label);

				begin(dock.label,&open,false,0,ImVec2(100,10));
				end();

				dock.invalid_frames = 0;
				dock.active = 0;
				i = 0;
				//doUndock(dock);
				//dock.status = Status_Float;
			}
			//return;
		}
		//dock.invalid_frames = 0;
	}
}

#if 0
void DockContext::beginPanel()
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		//ImGuiWindowFlags_ShowBorders | 
		ImGuiWindowFlags_NoBringToFrontOnFocus;
	Dock* root = getRootDock();
	if (root)
	{
		SetNextWindowPos(root->pos);
		SetNextWindowSize(root->size);
	}
	else
	{
		SetNextWindowPos(ImVec2(0, 0));
		SetNextWindowSize(GetIO().DisplaySize);
	}
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	Begin("###DockPanel", 0, flags);
	splits();

	checkNonexistent();
}

void DockContext::endPanel()
{
	End();
	//ImGui::PopStyleVar();
}
#endif

Dock* DockContext::getDockAt(const ImVec2& pos)
{
	for (int i = 0; i < m_docks.size(); ++i)
	{
		Dock& dock = *m_docks[i];
		if (dock.isContainer()) continue;
		if (dock.status != Status_Docked) continue;
		if (IsMouseHoveringRect(dock.pos, dock.pos + dock.size, false))
		{
			return &dock;
		}
	}

	return 0;
}


static ImRect getDockedRect(const ImRect& rect, Slot_ dock_slot)
{
	ImVec2 half_size = rect.GetSize() * 0.5f;
	switch (dock_slot)
	{
	default: return rect;
		//case Slot_Top: return ImRect(rect.Min, rect.Min + ImVec2(rect.Max.x, half_size.y));
	case Slot_Top: return ImRect(rect.Min, rect.Min + ImVec2(rect.Max.x - rect.Min.x, half_size.y));
	case Slot_Right: return ImRect(rect.Min + ImVec2(half_size.x, 0), rect.Max);
	case Slot_Bottom: return ImRect(rect.Min + ImVec2(0, half_size.y), rect.Max);
	case Slot_Left: return ImRect(rect.Min, rect.Min + ImVec2(half_size.x, rect.Max.y));
	}
}


static ImRect getSlotRect(ImRect parent_rect, Slot_ dock_slot)
{
	ImVec2 size = parent_rect.Max - parent_rect.Min;
	ImVec2 center = parent_rect.Min + size * 0.5f;
	switch (dock_slot)
	{
	default: return ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));
	case Slot_Top: return ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));
	case Slot_Right: return ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));
	case Slot_Bottom: return ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));
	case Slot_Left: return ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
	}
}


static ImRect getSlotRectOnBorder(ImRect parent_rect, Slot_ dock_slot)
{
	ImVec2 size = parent_rect.Max - parent_rect.Min;
	ImVec2 center = parent_rect.Min + size * 0.5f;
	switch (dock_slot)
	{
	case Slot_Top:
		return ImRect(ImVec2(center.x - 20, parent_rect.Min.y + 10),
			ImVec2(center.x + 20, parent_rect.Min.y + 30));
	case Slot_Left:
		return ImRect(ImVec2(parent_rect.Min.x + 10, center.y - 20),
			ImVec2(parent_rect.Min.x + 30, center.y + 20));
	case Slot_Bottom:
		return ImRect(ImVec2(center.x - 20, parent_rect.Max.y - 30),
			ImVec2(center.x + 20, parent_rect.Max.y - 10));
	case Slot_Right:
		return ImRect(ImVec2(parent_rect.Max.x - 30, center.y - 20),
			ImVec2(parent_rect.Max.x - 10, center.y + 20));
	}
	IM_ASSERT(false);
	return ImRect();
}


Dock* DockContext::getRootDock()
{
	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (!m_docks[i]->parent &&
			(m_docks[i]->status == Status_Docked || m_docks[i]->children[0]))
		{
			return m_docks[i];
		}
	}
	return 0;
}


DockContext& GetDockContext(int index);
bool DockContext::dockSlots(Dock& dock, Dock* dest_dock, const ImRect& rect, bool on_border)
{
	ImDrawList* canvas = GetWindowDrawList();
	ImU32 color = GetColorU32(ImGuiCol_Button);
	ImU32 color_hovered = GetColorU32(ImGuiCol_ButtonHovered);
	ImVec2 mouse_pos = GetIO().MousePos;
	float window_rounding = GetStyle().WindowRounding;
	for (int i = 0; i < (on_border ? 4 : 5); ++i)
	{
		ImRect r =
			on_border ? getSlotRectOnBorder(rect, (Slot_)i) : getSlotRect(rect, (Slot_)i);
		bool hovered = r.Contains(mouse_pos);
		canvas->AddRectFilled(r.Min, r.Max, hovered ? color_hovered : color,window_rounding);
		if (!hovered) continue;

		if (!IsMouseDown(0))
		{
			//if (dest_dock) doDock(dock, dest_dock, (Slot_)i);
			//else GetDockContext(0).doDock(dock, GetDockContext(0).getRootDock(), (Slot_)i);
			doDock(dock, dest_dock ? dest_dock : getRootDock(), (Slot_)i);
			return true;
		}
		ImRect docked_rect = getDockedRect(rect, (Slot_)i);
		canvas->AddRectFilled(docked_rect.Min, docked_rect.Max,GetColorU32(ImGuiCol_Button),GetStyle().WindowRounding);
	}
	return false;
}


void DockContext::handleDrag(Dock& dock)
{
	Dock* dest_dock = getDockAt(GetIO().MousePos);

/*
	Begin("##Overlay",
		NULL,
		ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
		//| ImGuiWindowFlags_AlwaysAutoResize
		);
*/
	ImDrawList* canvas = GetWindowDrawList();

	canvas->PushClipRectFullScreen();

	//ImU32 docked_color = GetColorU32(ImGuiCol_FrameBg);
	//docked_color = (docked_color & 0x00ffFFFF) | 0x22000000;
	ImColor docked_color(GetStyle().Colors[ImGuiCol_WindowBg]);
	docked_color.Value.w *= 0.9f;
	//PushStyleColor(ImGuiCol_WindowBg, docked_color );

	dock.pos = GetIO().MousePos - m_drag_offset;
	if (dest_dock)
	{
		if (dockSlots(dock,
			dest_dock,
			ImRect(dest_dock->pos, dest_dock->pos + dest_dock->size),
			false))
		{
			canvas->PopClipRect();
//			PopStyleColor();
//			End();
			return;
		}
	}
/*
	if (dockSlots(dock, 0, ImRect(ImVec2(0, 0), GetIO().DisplaySize), true))
	{
		canvas->PopClipRect();
		//	PopStyleColor();
//		End();
		return;
	}
*/
	//canvas->AddRectFilled(dock.pos, dock.pos + dock.size, docked_color);
	canvas->PopClipRect();
	//PopStyleColor();
	if (!IsMouseDown(0))
	{
		dock.status = Status_Float;
		dock.location[0] = 0;
		dock.setActive();
	}

//	End();
}


static char getLocationCode(Dock* dock)
{
	if (!dock) return '0';

	if (dock->parent->isHorizontal())
	{
		if (dock->pos.x < dock->parent->children[0]->pos.x) return '1';
		if (dock->pos.x < dock->parent->children[1]->pos.x) return '1';
		return '0';
	}
	else
	{
		if (dock->pos.y < dock->parent->children[0]->pos.y) return '2';
		if (dock->pos.y < dock->parent->children[1]->pos.y) return '2';
		return '3';
	}
}

void fillLocation(Dock& dock)
{
	if (dock.status == Status_Float) return;
	int count = 0;
	Dock* tmp = &dock;
	while (tmp->parent && count < sizeof(dock.location)-1)
	{
		dock.location[count] = getLocationCode(tmp);
		tmp = tmp->parent;
		count += 1;
	}
	dock.location[count] = 0;
}

void DockContext::verify()
{
	int lastActive = -1;
	for (int i = 0; i < m_docks.size(); ++i)
	{
		Dock* d = m_docks[i];
		if ( !d->location[0])
			fillLocation(*d);

		d->id = ImHash(d->label,0);
		//disable self referencing
		if ( d->children[0] == d ) d->children[0] = 0;
		if ( d->children[1] == d ) d->children[1] = 0;
		if ( d->next_tab == d )
			d->next_tab = 0;
		if ( d->prev_tab == d )
			d->prev_tab = 0;
/*
		if ( d->active )
		{
			if (lastActive<0) lastActive = d->active;
			else d->active = 0;
		}
*/
	}
	//order
}

void DockContext::doUndock(Dock& dock)
{
	system_redraw(__FUNCTION__,dock.label);

	//setDockActive(dock.next_tab?dock.next_tab->label:dock.prev_tab?dock.prev_tab->label:0);
/*
	dock.active = false;
	if (dock.next_tab) dock.next_tab->setActive();
	else if (dock.prev_tab) dock.prev_tab->setActive();
*/
	//char isactive[1024];
	//sprintf(isactive,"%s",getDockActive() );

	if ( dock.active )
	{
		if (dock.prev_tab) dock.prev_tab->setActive();
		else if (dock.next_tab) dock.next_tab->setActive();
		else dock.active = false;
	}

	Dock* container = dock.parent;
	if (container)
	{
		Dock* sibling = dock.getSibling();
		if (container->children[0] == &dock)
		{
			container->children[0] = dock.next_tab;
		}
		else if (container->children[1] == &dock)
		{
			container->children[1] = dock.next_tab;
		}

		bool remove_container = !container->children[0] || !container->children[1];
		if (remove_container)
		{
			if (container->parent)
			{
				Dock*& child = container->parent->children[0] == container ? container->parent->children[0]	: container->parent->children[1];
				child = sibling;
				if ( child )
				{
					child->setPosSize(container->pos, container->size);
					child->setParent(container->parent);
				}
			}
			else
			{
				if (container->children[0])
				{
					container->children[0]->setParent(0);
					container->children[0]->setPosSize(container->pos, container->size);
				}
				if (container->children[1])
				{
					container->children[1]->setParent(0);
					container->children[1]->setPosSize(container->pos, container->size);
				}
			}
			for (int i = 0; i < m_docks.size(); ++i)
			{
				Dock* d = m_docks[i];

				if ( d->parent == container )
					d->parent = 0;
				if ( d->next_tab == container )
					d->next_tab = 0;
				if ( d->prev_tab == container )
					d->prev_tab = 0;
				if ( d->children[0] == container )
					d->children[0] = 0;
				if ( d->children[1] == container )
					d->children[1] = 0;

				if (d == container)
				{
					m_docks.erase(m_docks.begin() + i);
					//break;
					i = 0;
				}
			}

			if (container == m_next_parent)
				m_next_parent = 0;

			container->~Dock();
			MemFree(container);
			container = 0;
		}
	}
	if (dock.prev_tab)
		dock.prev_tab->next_tab = dock.next_tab;
	if (dock.next_tab)
		dock.next_tab->prev_tab = dock.prev_tab;
	dock.parent = 0;
	dock.prev_tab = dock.next_tab = 0;
	verify();
}


void drawTabbarListButton(Dock& dock)
{
	if (!dock.next_tab) return;

	ImDrawList* draw_list = GetWindowDrawList();
	if (InvisibleButton("list", ImVec2(16, 16)))
	{
		OpenPopup("tab_list_popup");
	}
	if (BeginPopup("tab_list_popup"))
	{
		Dock* tmp = &dock;
		int run = 0;
		while (tmp && ++run < DOCK_GUARD)
		{
			bool dummy = false;
			if (Selectable(tmp->label, &dummy))
			{
				tmp->setActive();
			}
			tmp = tmp->next_tab;
		}
		EndPopup();
	}

	//bool hovered = IsItemHovered();
	ImVec2 min = GetItemRectMin();
	ImVec2 max = GetItemRectMax();
	ImVec2 center = (min + max) * 0.5f;
	//ImU32 text_color = GetColorU32(ImGuiCol_Text);
	//ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
	ImU32 col = GetColorU32(ImGuiCol_Text,ImGuiCol_FrameBgActive,envelope_get(GetID("list")));
	draw_list->AddRectFilled(ImVec2(center.x - 4, min.y + 3),
		ImVec2(center.x + 4, min.y + 5),
		col );//hovered ? color_active : text_color);
	draw_list->AddTriangleFilled(ImVec2(center.x - 4, min.y + 7),
		ImVec2(center.x + 4, min.y + 7),
		ImVec2(center.x, min.y + 12),
		col );//hovered ? color_active : text_color);

}


bool DockContext::tabbar(Dock* dock_in, bool close_button, bool enabled,bool need_HorizontalScrollbar)
{
	if ( 0 == dock_in ) return false;
	Dock &dock = *dock_in;

	float line_height = GetTextLineHeightWithSpacing();

	ImVec2 size(dock.size.x, line_height * (need_HorizontalScrollbar ? 1.7f : 1.f) );
	bool tab_closed = false;

	SetCursorScreenPos(dock.pos - ImVec2(4,0));
	char tmp[256];
	ImFormatString(tmp, IM_ARRAYSIZE(tmp), "tabs%d", (int)dock.id);
	if (!BeginChild(tmp, size, false,
		(need_HorizontalScrollbar ? ImGuiWindowFlags_HorizontalScrollbar : 0)
		//| ImGuiWindowFlags_NoZoom
		| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings
		))
	{
		EndChild();
		return tab_closed;
	}

	Dock* dock_tab = &dock;

	ImDrawList* draw_list = GetWindowDrawList();
	//ImU32 color = GetColorU32(ImGuiCol_FrameBg);
	//ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
	//ImU32 color_hovered = GetColorU32(ImGuiCol_FrameBgHovered);
	//ImU32 text_color = GetColorU32(ImGuiCol_Text);
	//float tab_base;

	if (draw_tabbar_list)
	{
		drawTabbarListButton(dock);
	}
	SameLine();

	const ImGuiStyle& style = GetStyle();
	while (dock_tab)
	{
		//SameLine(0, 15);
		const char* label = dock_tab->label;
		const char* text_end = FindRenderedTextEnd(label);
					
		ImVec2 size(CalcTextSize(label, text_end).x + style.FramePadding.x * 2.0f, line_height);
		bool hovered;
		if (InvisibleButton(label, size,&hovered))
		{
			if (enabled)
			{
				dock_tab->setActive();
				//SetScrollX(GetItemRectMin().x);
				m_next_parent = dock_tab;
			}
		}

		if (IsItemActive() && IsMouseDragging())
		{
			if (enabled)
			{
				m_drag_offset = GetMousePos() - dock_tab->pos;
				doUndock(*dock_tab);
				dock_tab->status = Status_Dragged;
			}
		}

		//const bool hovered = IsItemHovered();
		ImVec2 pos = GetItemRectMin();
		if (dock_tab->active && close_button)
		{
			size.x += 16 + 2 + style.FramePadding.x;
			SameLine();
			bool hovered,held;
			tab_closed = InvisibleButton("#CLOSE", ImVec2(16, 16),&hovered ,&held);
			ImVec2 center = (GetItemRectMin() + GetItemRectMax()) * 0.5f;
			//center.y += style.FramePadding.y;
			center.y = pos.y + size.y*0.5f;

			float alpha = envelope_get(GetID("#CLOSE"));
			const ImU32 col = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_Button,ImGuiCol_ButtonHovered,alpha);
			float radius = 3.5f;
			draw_list->AddCircleFilled(center, radius * 2.f, col, 12);

			draw_list->AddLine(
				center + ImVec2(-radius, -radius), center + ImVec2(radius, radius), GetColorU32(ImGuiCol_Text,alpha));
			draw_list->AddLine(
				center + ImVec2(radius, -radius), center + ImVec2(-radius, radius), GetColorU32(ImGuiCol_Text,alpha));

			//if (CloseButton(window->GetID("#CLOSE"), window->Pos + ImVec2( window->Size.x - pad,title_bar_rect.GetHeight()*0.5f), style.CloseButtonSize))

		}
		//tab_base = pos.y;
/*
		draw_list->PathClear();
		draw_list->PathLineTo(pos + ImVec2(-15, size.y));
		draw_list->PathBezierCurveTo(
			pos + ImVec2(-10, size.y), pos + ImVec2(-5, 0), pos + ImVec2(0, 0), 10);
		draw_list->PathLineTo(pos + ImVec2(size.x, 0));
		draw_list->PathBezierCurveTo(pos + ImVec2(size.x + 5, 0),
			pos + ImVec2(size.x + 10, size.y),
			pos + ImVec2(size.x + 15, size.y),
			10);
		draw_list->PathFill(
			hovered ? color_hovered : (dock_tab->active ? color_active : color));
*/
	//ImU32 color = GetColorU32(ImGuiCol_FrameBg);
	//ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
	//ImU32 color_hovered = GetColorU32(ImGuiCol_FrameBgHovered);

		draw_list->AddRectFilled(pos,pos+size,
			//hovered ? color_hovered : (dock_tab->active ? color_active : color),
			GetColorU32(dock_tab->active ? ImGuiCol_FrameBgActive:ImGuiCol_FrameBg,ImGuiCol_FrameBgHovered,	envelope_get(GetID(label))),
			style.FrameRounding);
		pos.x += style.FramePadding.x;
		draw_list->AddText(pos, GetColorU32(ImGuiCol_Text), label, text_end);
		SameLine(0);
		dock_tab = dock_tab->next_tab;
	}
	//ImVec2 cp(dock.pos.x, tab_base + line_height);
	//draw_list->AddLine(cp, cp + ImVec2(dock.size.x, 0), color);
	EndChild();
	return tab_closed;
}


static void setDockPosSize(Dock& dest, Dock& dock, Slot_ dock_slot, Dock& container)
{
	//IM_ASSERT(!dock.prev_tab && !dock.next_tab && !dock.children[0] && !dock.children[1]);
	msg("setDockPosSize assert\n");

	dest.pos = container.pos;
	dest.size = container.size;
	dock.pos = container.pos;
	dock.size = container.size;

	switch (dock_slot)
	{
	case Slot_Bottom:
		dest.size.y *= 0.5f;
		dock.size.y *= 0.5f;
		dock.pos.y += dest.size.y;
		break;
	case Slot_Right:
		dest.size.x *= 0.5f;
		dock.size.x *= 0.5f;
		dock.pos.x += dest.size.x;
		break;
	case Slot_Left:
		dest.size.x *= 0.5f;
		dock.size.x *= 0.5f;
		dest.pos.x += dock.size.x;
		break;
	case Slot_Top:
		dest.size.y *= 0.5f;
		dock.size.y *= 0.5f;
		dest.pos.y += dock.size.y;
		break;
	default:
		IM_ASSERT(false);
		break;
	}
	dest.setPosSize(dest.pos, dest.size);

	if (container.children[1]->pos.x < container.children[0]->pos.x ||
		container.children[1]->pos.y < container.children[0]->pos.y)
	{
		Dock* tmp = container.children[0];
		container.children[0] = container.children[1];
		container.children[1] = tmp;
	}
}


void DockContext::doDock(Dock& dock, Dock* dest, Slot_ dock_slot)
{
	//IM_ASSERT(!dock.parent);
	if ( !dock.parent )
	{
		msg ("doDock assert dock.parent. %s\n" ,dock.label);
	}
	if (!dest)
	{
		dock.status = Status_Docked;
		dock.setPosSize(ImVec2(0, 0), GetIO().DisplaySize);
	}
	else if (dock_slot == Slot_Tab)
	{
		//insert at current
		Dock* tmp = dest;
		int run = 0;
		while (++run<DOCK_GUARD && !tmp->active && tmp->next_tab && tmp->next_tab != tmp)
		{
			tmp = tmp->next_tab;
		}

		dock.next_tab = tmp->next_tab;
		if (dock.next_tab) dock.next_tab->prev_tab = &dock;
		tmp->next_tab = &dock;
		dock.prev_tab = tmp;

		dock.size = tmp->size;
		dock.pos = tmp->pos;
		dock.parent = dest->parent;
		dock.status = Status_Docked;

/*
		//insert at end
		Dock* tmp = dest;
		while (tmp->next_tab && tmp->next_tab != tmp)
		{
			tmp = tmp->next_tab;
		}

		tmp->next_tab = &dock;
		dock.prev_tab = tmp;
		dock.size = tmp->size;
		dock.pos = tmp->pos;
		dock.parent = dest->parent;
		dock.status = Status_Docked;
*/
	}
	else if (dock_slot == Slot_None)
	{
		dock.status = Status_Float;
	}
	else
	{
		Dock* container = (Dock*)MemAlloc(sizeof(Dock));
		IM_PLACEMENT_NEW(container) Dock();
		m_docks.push_back(container);
		container->children[0] = dest->getFirstTab();
		container->children[1] = &dock;
		container->next_tab = 0;
		container->prev_tab = 0;
		container->parent = dest->parent;
		container->size = dest->size;
		container->pos = dest->pos;
		container->status = Status_Docked;
		container->label[0] = 0; //.ImStrdup("");

		if (!dest->parent)
		{
		}
		else if (dest->getFirstTab() == dest->parent->children[0])
		{
			dest->parent->children[0] = container;
		}
		else
		{
			dest->parent->children[1] = container;
		}

		dest->setParent(container);
		dock.parent = container;
		dock.status = Status_Docked;

		setDockPosSize(*dest, dock, dock_slot, *container);
	}
	dock.setActive();
	system_redraw(__FUNCTION__,dock.label);
}


void DockContext::rootDock(const ImVec2& pos, const ImVec2& size)
{
	Dock* root = getRootDock();
	if (!root) return;

	ImVec2 min_size = root->getMinSize();
	ImVec2 requested_size = size;
	root->setPosSize(pos, ImMax(min_size, requested_size));
}

const char* DockContext::getDockActive()
{
	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (m_docks[i]->active)
		{
			return m_docks[i]->label;
		}
	}
	return "";
}

void DockContext::setDockActive(const char* name, int disable_all_other)
{
/*
	if (0==name && m_current)
	{
		m_current->setActive();
	}
	else
*/
	if ( 0 == name && m_docks.size())
	{
		name = m_docks[0]->label;
	}
	if (name)
	{
		const char* active = getDockActive();
		msg ( "setDockActive slot:%d from %s to %s\n",index,active,name);

		ImU32 id = ImHash(name, 0);
		for (int i = 0; i < m_docks.size(); ++i)
		{
			if (m_docks[i]->id == id)
			{
				m_docks[i]->setActive();
			}
			else
			if ( disable_all_other )
			{
				m_docks[i]->active = false;
			}
		}
	}
}

void DockContext::setDockWindowPos(const char* name, const ImVec2& pos,const ImVec2& size,DockState newdockstate)
{
	if (0==name && m_current)
	{
		m_current->pos = pos;
		m_current->size = size;
		m_current->status = newdockstate;
	}
	else if (name)
	{
		ImU32 id = ImHash(name, 0);
		for (int i = 0; i < m_docks.size(); ++i)
		{
			if ( !m_docks[i] ) continue;
			Dock &dock = *m_docks[i];

			if (dock.id == id && dock.opened == false)
			{
				dock.pos = pos;
				dock.size = size;
				dock.status = newdockstate;
/*
				if ( newdockstate == Status_Float )
				{
					fillLocation(dock);
					doUndock(dock);
					dock.status = Status_Float;
				}
*/
				//break;
			}
		}
	}

}


static Slot_ getSlotFromLocationCode(char code)
{
	switch (code)
	{
		case '1': return Slot_Left;
		case '2': return Slot_Top;
		case '3': return Slot_Bottom;
	}
	return Slot_Right;
}




void DockContext::tryDockToStoredLocation(Dock& dock)
{
	if (dock.status == Status_Docked) return;
	if (dock.location[0] == 0) return;

	Dock* tmp = getRootDock();
	if (!tmp) return;

	Dock* prev = 0;
	char* c = dock.location + strlen(dock.location) - 1;
	while (c >= dock.location && tmp)
	{
		prev = tmp;
		tmp = *c == getLocationCode(tmp->children[0]) ? tmp->children[0] : tmp->children[1];
		if(tmp) --c;
	}
	if (tmp && tmp->children[0]) tmp = tmp->parent;
	doDock(dock, tmp ? tmp : prev, tmp && !tmp->children[0] ? Slot_Tab : getSlotFromLocationCode(*c));
}

bool DockContext::begin(const char* label, bool* opened, bool border, ImGuiWindowFlags extra_flags, const ImVec2& default_size)
{
	Slot_ next_slot = m_next_dock_slot;
	m_next_dock_slot = Slot_Tab;

	Dock& dock = getDock(label, !opened || *opened, default_size);
	if (!dock.opened && (!opened || *opened))
		tryDockToStoredLocation(dock);
	dock.last_frame = ImGui::GetFrameCount();
	if (strcmp(dock.label, label) != 0)
	{
		//MemFree(dock.label);
		//dock.label.ImStrdup(label);
		strcpy(dock.label,label);
	}

	m_end_action = EndAction_None;

	bool prev_opened = dock.opened;
	bool first = dock.first;

	if (dock.first && opened) *opened = dock.opened;
	dock.first = false;
	if (opened && !*opened)
	{
		if (dock.status != Status_Float)
		{
			fillLocation(dock);
			doUndock(dock);
			dock.status = Status_Float;
		}
		dock.opened = false;
		return false;
	}
	dock.opened = true;

	ImGuiContext& g = *GImGui;
	ImVec2 nextWindowContentSizeVal = g.NextWindowData.ContentSizeVal;
	//m_end_action = EndAction_Panel;
	//beginPanel();

	//checkNonexistent();
	if (first || (prev_opened != dock.opened))
	{
		Dock* root = m_next_parent ? m_next_parent : getRootDock();
		if (root && (&dock != root) && !dock.parent)
		{
			doDock(dock, root, next_slot);
		}
		m_next_parent = &dock;
	}

	m_current = &dock;
	//if (dock.status == Status_Dragged) handleDrag(dock);
	if (dock.status == Status_Float|| dock.status == Status_Dragged)
	{
		SetNextWindowPos(dock.pos);
		SetNextWindowSize(dock.size);
		bool ret = Begin(label,
			opened,
			//ImGuiWindowFlags_NoCollapse | 
			//ImGuiWindowFlags_ShowBorders |
			extra_flags);
		m_end_action = EndAction_End;
		dock.pos = GetWindowPos();
		if (ret) 
			dock.size = GetWindowSize();

		ImGuiContext& g = *GImGui;

		if (dock.status == Status_Float && g.ActiveId == GetCurrentWindow()->MoveId && g.IO.MouseDown[0])
		{
			m_drag_offset = GetMousePos() - dock.pos;
			doUndock(dock);
			dock.status = Status_Dragged;
		}
		else
		if (dock.status == Status_Dragged)
		{
			handleDrag(dock);
		}
		return ret;
	}

	if (!dock.active && dock.status != Status_Dragged)
		return false;

	m_end_action = EndAction_EndChild;
	splits();

	//PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	//PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));

	//Need Horizontal Scrollbar ?
	Dock* dock_tab = dock.getFirstTab();
	ImVec2 full_size(16.f,0);
	float pad = GetStyle().FramePadding.x * 2.f;
	while (dock_tab)
	{
		const char* label = dock_tab->label;
		const char* text_end = FindRenderedTextEnd(label);
				
		full_size.x += CalcTextSize(label, text_end).x + pad;
		dock_tab = dock_tab->next_tab;
	}
	bool need_HorizontalScrollbar = full_size.x > dock.size.x;
	if ( draw_tabbar )
	{
		if (tabbar(dock.getFirstTab(),
			opened != 0,
			extra_flags & ImGuiWindowFlags_NoInputs ? false : true,
			need_HorizontalScrollbar)
			)
		{
			fillLocation(dock);
			*opened = false;
		}
	}
	ImVec2 pos = dock.pos;
	ImVec2 size = dock.size;
	//window under dock distance to scrollbar
	float tabbar_height = draw_tabbar ? GetTextLineHeightWithSpacing() * (need_HorizontalScrollbar?1.8f:1.1f):0.f; // + GetStyle().WindowPadding.y;
	pos.y += tabbar_height;
	size.y -= tabbar_height;

	SetCursorScreenPos(pos);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
		extra_flags;

	// to avoid https://github.com/ocornut/imgui/issues/713
	char tmp[256];
	ImFormatString(tmp,IM_ARRAYSIZE(tmp),"%s_docked",label);
	if ( extra_flags & ImGuiWindowFlags_HorizontalScrollbar )
	{
		size.y -= GetStyle().ScrollbarSize + GetStyle().WindowPadding.y;
		SetNextWindowContentSize(nextWindowContentSizeVal);
	}

	bool ret = BeginChild(tmp, size, border, flags);
	//PopStyleColor();
	//PopStyleColor();
	return ret;
}


void DockContext::end()
{
/*
	if (m_current && m_current->status == Status_Dragged )
	{
		PopClipRect();
	}
*/
	m_current = 0;
	if (m_end_action != EndAction_None)
	{
		if (m_end_action == EndAction_End)
		{
			End();
		}
		else if (m_end_action == EndAction_EndChild)
		{
			//PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
			//PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
			EndChild();
			//PopStyleColor();
			//PopStyleColor();
		}
		m_current = 0;
		//if (m_end_action > EndAction_None) endPanel();
	}

}


void DockContext::designer(bool *v_open)
{
	if (v_open && !*v_open) return;

	//SetNextWindowSize(ImVec2(300, 300));
	SetNextWindowSize(ImVec2(600,GetIO().DisplaySize.y*0.9f),ImGuiCond_FirstUseEver);
	if (!Begin("Dock Designer",v_open))
	{
		End();
		return;
	}
	PushID((void*)this);
	Separator();
	Text("%s",label);
	static char filename[8][256] = { {0},{0},{0},{0},{0},{0},{0},{0} };
	if ( filename[index][0] == 0 )
	{
		sprintf(filename[index],"dock%d.json",index);
	}
	if ( Button("SaveDock"))
	{
		ImGuiTextBuffer out;
		save(out);
		saveFile(filename[index],out.c_str(),out.size());
	}
	SameLine();
	if (Button("LoadDock"))
	{
		load(index,loadFile(filename[index]));
	}
	SameLine();
	InputText("File", filename[index], sizeof(filename[index]) );

	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (!TreeNode((void*)(i), "Dock %d %s", i, m_docks[i]->label))
			continue;
		Dock &dock = *m_docks[i];
		if ( Button("Erase" ) )
		{
			Dock* container = &dock;
			int thisIndex = getDockIndex(container);
			for (int i2 = 0; i2 < m_docks.size(); ++i2)
			{
				Dock* d = m_docks[i2];

				if ( d->parent == container )
					d->parent = 0;
				if ( d->next_tab == container )
					d->next_tab = 0;
				if ( d->prev_tab == container )
					d->prev_tab = 0;
				if ( d->children[0] == container )
					d->children[0] = 0;
				if ( d->children[1] == container )
					d->children[1] = 0;
			}

			m_docks.erase(m_docks.begin() + thisIndex);
			if (container == m_next_parent) m_next_parent = 0;

			container->~Dock();
			MemFree(container);
			container = 0;
			TreePop();
			break;
		}

		Text("index = %d,\n",i);
		Text("id = 0x%x,\n",dock.id);
		Text("label = '%s',\n",dock.label);
		Text("pos_x = %.0f,\n",dock.pos.x);
		Text("pos_y = %.0f,\n",dock.pos.y);
		Text("size_x = %.0f,\n",dock.size.x);
		Text("size_y = %.0f,\n",dock.size.y);
		Text("location = '%s',\n",dock.location);

		int state = dock.status;
		RadioButton("Docked", &state, Status_Docked);
		SameLine();
		RadioButton("Dragged", &state, Status_Dragged);
		SameLine();
		RadioButton("Float", &state, Status_Float);
		dock.status = (DockState)state;
/*
		Text("status = %d (%s),\n",dock.status, dock.status == Status_Docked?"Docked":
				dock.status == Status_Dragged?"Dragged": 
				dock.status == Status_Float?"Float": "?");
*/
		Text("active = %d,\n",dock.active);
		SameLine();
		if (Button("Set Active"))
		{
			dock.setActive();
		}
		Text("opened = %d,\n",dock.opened);
		Text("prev = %d,\n",getDockIndex(dock.prev_tab));
		Text("next = %d,\n",getDockIndex(dock.next_tab));
		Text("child0 = %d,\n",getDockIndex(dock.children[0]));
		Text("child1 = %d,\n",getDockIndex(dock.children[1]));
		Text("parent = %d\n",getDockIndex(dock.parent));
/*
		Text("label = ",dock.label.c_str() );
		Text("pos=(%.1f %.1f) size=(%.1f %.1f)",
			dock.pos.x, dock.pos.y,
			dock.size.x, dock.size.y);
		Text("parent = %p\n",dock.parent);
		Text("isContainer() == %s\n",dock.isContainer()?"true":"false");
		Text("status = %s\n",
			(dock.status == Status_Docked)?"Docked":
				((dock.status == Status_Dragged)?"Dragged": 
					((dock.status == Status_Float)?"Float": "?")));
*/
		TreePop();
	}

	PopID();
	End();
}

int DockContext::getDockIndex(Dock* dock)
{
	if (!dock) return -1;

	for (int i = 0; i < m_docks.size(); ++i)
	{
		if (dock == m_docks[i]) return i;
	}
	return -1;
}

Dock* DockContext::getDockByIndex(const char* name)
{
	int index = atoi(name);
	if ( index < 0 ) return 0;
	while ( index >= m_docks.size() )
	{
		Dock* current = (Dock*)MemAlloc(sizeof(Dock));
		IM_PLACEMENT_NEW(current) Dock();
		m_docks.push_back(current);
	}
	return m_docks[index];
}

void DockContext::dump()
{
	msg("docks={\n");
	msg("label=\"%s\",\n",label);
	for (int i = 0; i < m_docks.size(); ++i)
	{
		Dock& dock = *m_docks[i];

		msg( "dock%d={\n",i);

		msg("index=%d,\n",i);
		msg("label=\"%s\",\n",dock.label );
		msg("pos_x=%.0f,\n",dock.pos.x);
		msg("pos_y=%.0f,\n",dock.pos.y);
		msg("size_x=%.0f,\n",dock.size.x);
		msg("size_y=%.0f,\n",dock.size.y);
		msg("location=\"%s\",\n",dock.location);
		msg("status=%d,\n",dock.status);
		msg("active=%d,\n",dock.active);
		msg("opened=%d,\n",dock.opened);
		msg("first=%d,\n",dock.first);
		msg("prev=%d,\n",getDockIndex(dock.prev_tab));
		msg("next=%d,\n",getDockIndex(dock.next_tab));
		msg("child0=%d,\n",getDockIndex(dock.children[0]));
		msg("child1=%d,\n",getDockIndex(dock.children[1]));
		msg("parent=%d\n",getDockIndex(dock.parent));
		if (i < m_docks.size() - 1)
			msg ("},\n");
		else
			msg ("}\n");
	}
	msg("}\n");
}

void DockContext::save(ImGuiTextBuffer &out)
{
	out.reserve(4096);
	out.json_object("docks",1);
	out.json("label",label);
	for (int i = 0; i < m_docks.size(); ++i)
	{
		Dock& dock = *m_docks[i];
		if ( !dock.location[0] ) fillLocation(dock);

		char name[64];
		sprintf(name,"dock%d",i);
		out.json_object(name,1);
		out.json("index",i);
		out.json("label",dock.label );
		out.json("pos_x",dock.pos.x,0);
		out.json("pos_y",dock.pos.y,0);
		out.json("size_x",dock.size.x,0);
		out.json("size_y",dock.size.y,0);
		out.json("location",dock.location);
		out.json("status",dock.status);
		out.json("active",dock.active);
		out.json("opened",dock.opened);
		out.json("first",dock.first);
		out.json("prev",getDockIndex(dock.prev_tab));
		out.json("next",getDockIndex(dock.next_tab));
		out.json("child0",getDockIndex(dock.children[0]));
		out.json("child1",getDockIndex(dock.children[1]));
		out.json("parent",getDockIndex(dock.parent));

		out.json_object(name,0);
	}
	out.json_object("docks",0);
}


void DockContext::load(int slot,const char *filename, int reload)
{
	const char* ip;
	const char* token;
	int state = 0;
	int depth;
	char var[128];
	Dock* current = 0;

	reset();
	index = slot;

	//must allocate all before..
	ip = filename;
	depth = 0;
	int used = 0;
	do
	{
		token = COM_Parse(&ip);
		switch ( token[0] )
		{
			case '{': depth += 1; used += ( depth == 2 ); break;
			case '}': depth -= 1; state = 0; break;
		}
	} while (ip);

	while ( used > m_docks.size() )
	{
		current = (Dock*)MemAlloc(sizeof(Dock));
		IM_PLACEMENT_NEW(current) Dock();
		current->last_frame = 0;//ImGui::GetFrameCount();
		//current->invalid_frames = 0;
		//current->location[0] = 0;
		m_docks.push_back(current);
	}

	ip = filename;
	depth = 0;
	//json
	do
	{
		token = COM_Parse(&ip);
		switch ( token[0] )
		{
			case '{': depth += 1; state = 0; break;
			case '}': depth -= 1; state = 0; break;
			case ',': state = 0; break;
			case ':':
			case '=': state = 2; break;
			default:
			{
				//var = { value,value }
				if ( 0 == state )
				{
					strcpy(var,token);
					state = 1;
					break;
				}
				if ( state != 2 ) break;
				if ( depth == 1 )
				{
					if ( !strcmp(var,"label")) { strcpy(label,token); } // label.ImStrdup(token);
				}
				else if ( depth == 2 )
				{
					if ( !strcmp(var,"index"))
					{
						current = getDockByIndex(token);
						//current->active = true;
						//current->first = false;
						//current->first = reload ? false : true;
					}
					else if ( !strcmp(var,"label")) { strcpy(current->label,token); }// current->label.ImStrdup(token);
					else if ( !strcmp(var,"pos_x")) current->pos.x=(float)atof(token);
					else if ( !strcmp(var,"pos_y")) current->pos.y=(float)atof(token);
					else if ( !strcmp(var,"size_x")) current->size.x=(float)atof(token);
					else if ( !strcmp(var,"size_y")) current->size.y=(float)atof(token);
					else if ( !strcmp(var,"location")) { strcpy(current->location,token); }
					else if ( !strcmp(var,"status")) current->status=(DockState)atoi(token);
					else if ( !strcmp(var,"opened"))
					{
						//current->opened=true;
						current->opened=atoi(token) != 0;
					}
					else if ( !strcmp(var,"active"))
					{
						//current->active=true;
						current->active=atoi(token) != 0;
					}
					else if ( !strcmp(var,"first"))
					{
						current->first = atoi(token) != 0;
					}
					else if ( !strcmp(var,"prev")) current->prev_tab = getDockByIndex(token);
					else if ( !strcmp(var,"next")) current->next_tab = getDockByIndex(token);
					else if ( !strcmp(var,"child0")) current->children[0] = getDockByIndex(token);
					else if ( !strcmp(var,"child1")) current->children[1] = getDockByIndex(token);
					else if ( !strcmp(var,"parent")) current->parent = getDockByIndex(token);
				}
				state = 0;
			}
		}

	} while (ip);
	verify();

}


static DockContext g_dock[MAX_WORKSPACE];
int dock_current = 0;

void Print()
{
	for ( int g = 0; g < MAX_WORKSPACE; ++g)
	for (int i = 0; i < g_dock[g].m_docks.size(); ++i)
	{
		const Dock *d = g_dock[g].m_docks[i];
		ImGui::Text("i=%d this=0x%.8p state=(%d %d) pos=(%.0f %.0f) size=(%.0f %.0f) children=(%s %s) tabs=(%s %s) parent=%s status=%d  location='%s' label='%s'\n", i, 
			(void*)d,
			d->active,
			d->opened,
			d->pos.x,
			d->pos.y,
			d->size.x,
			d->size.y,
			d->children[0] ? d->children[0]->label : "None",
			d->children[1] ? d->children[1]->label : "None",
			d->prev_tab    ? d->prev_tab->label    : "None",
			d->next_tab    ? d->next_tab->label    : "None",
			d->parent      ? d->parent->label     : "None",
			d->status,
			d->location,
			d->label);
	}
}

void ShutdownDock(int slot)
{
	for ( int g = 0; g < MAX_WORKSPACE; ++g)
	{
		if ( slot < 0 || slot == g )
			g_dock[g].reset();
	}
	dock_current = 0;
}


void SetNextDockSlot(Slot_ val)
{
	g_dock[dock_current].m_next_dock_slot = val;
}

bool DockWorkspaceClosed()
{
	return g_dock[dock_current].dock_open == false;
}

bool DockBeginWorkspace(const char* name, int slot,int draw_tabbar,int draw_tabbar_list)
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing; // | ImGuiWindowFlags_NoBringToFrontOnFocus;

	dock_current = slot;
	g_dock[dock_current].index = slot;
	g_dock[dock_current].draw_tabbar = draw_tabbar;
	g_dock[dock_current].draw_tabbar_list = draw_tabbar_list;
	SetNextDockSlot(Slot_Tab);

	if (!canDock())
	{
		g_dock[dock_current].asChild = false;
		flags = 0;
	}
	else
	if ( !strcmp("HUD", name) )
	{
		flags = ImGuiWindowFlags_NoTitleBar |
			//ImGuiWindowFlags_NoResize |
			//ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;
			//ImGuiWindowFlags_ShowBorders | 
			//ImGuiWindowFlags_NoBringToFrontOnFocus;
		//SetNextWindowPos(ImVec2(0, 0));
		//SetNextWindowSize(GetIO().DisplaySize);

		g_dock[dock_current].asChild = false;
	}
	else
	{
		g_dock[dock_current].asChild = true;
	}
	//g_dock[dock_current].label.ImStrdup(name);
	strcpy(g_dock[dock_current].label,name);

	bool doBegin = g_dock[dock_current].asChild ?
		BeginChild(name,ImVec2(0,0), false,flags):
		Begin(name,&g_dock[dock_current].dock_open, flags);
	if (!doBegin) return false;
	//BeginChild("###dock_workspace", ImVec2(0,0), false, flags);

	const ImGuiStyle &style = GetStyle();
	if ( dock_current == 0 )
	{
		//g_dock[dock_current].m_workspace_pos = ImVec2(0,0);
		//g_dock[dock_current].m_workspace_size = GetIO().DisplaySize;
	}
	else
	{
		//above dockbar
		g_dock[dock_current].m_workspace_pos = GetWindowPos() + GetCursorPos();
		g_dock[dock_current].m_workspace_size = GetContentRegionMax() - GetStyle().WindowPadding;
	}
	return true;
}

void DockEndWorkspace(int toslot)
{
	g_dock[dock_current].asChild ? EndChild() : End();
	g_dock[dock_current].checkNonexistent();
	dock_current = toslot;
}

bool BeginDock(const char* name, bool* opened,ImGuiWindowFlags extra_flags, const ImVec2& default_size, bool border)
{
	//return Begin(name,opened,extra_flags);
	return g_dock[dock_current].begin(name, opened, border ,extra_flags, default_size);
}


void EndDock()
{
	//End();
	g_dock[dock_current].end();
}

int DockGetActiveWorkspace()
{
	return dock_current;
}

void DockDesigner(bool *v_open)
{
	for ( int g = 0; g < MAX_WORKSPACE; ++g ) g_dock[g].designer(v_open);
}

DockContext& GetDockContext(int index)
{
	return g_dock[index];
}
void RootDock(const ImVec2& pos, const ImVec2& size)
{
	g_dock[dock_current].rootDock(pos, size);
}


void SetDockActive(int slot, const char* name,int disable_all_other)
{
	g_dock[slot].setDockActive(name,disable_all_other);
}

const char* GetDockActive(int slot)
{
	return g_dock[slot].getDockActive();
}

void SetDockWindowPos(int slot, const char* name, const ImVec2& pos,const ImVec2& size,DockState dockState)
{
	g_dock[slot].setDockWindowPos(name,pos,size,dockState);
}


void SaveDock(int slot,ImGuiTextBuffer *out)
{
	ImGuiTextBuffer temp;
	g_dock[slot].save(out ? *out: temp);
	if ( 0 == out )
	{
		char filename[256];
		sprintf(filename,"dock%d.json",slot);
		saveFile(filename,temp.c_str(),temp.size());
	}
}


void LoadDock(int slot, const char *filename, int reload)
{
	g_dock[slot].load(slot,filename,reload);
}


} // namespace ImGui
