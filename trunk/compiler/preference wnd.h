#pragma once

#include "pmc wnd.h"

class PreviewWnd;
class ProjectManager;

//--------------------------
// PreferenceWnd declaration
//--------------------------

class PreferenceWnd : public PMCWindow
{
// construction/destruction
public:
	PreferenceWnd(PreviewWnd &preview_wnd, ProjectManager &project_manager);
	~PreferenceWnd();
// interface
public:
	void Create(HWND hWndParent);
// message handlers
private:
	void OnCommand    (Msg<WM_COMMAND>        &msg);
	void OnColorStatic(Msg<WM_CTLCOLORSTATIC> &msg);
	void OnInitDialog (Msg<WM_INITDIALOG>     &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
private:
	void Apply();
// data
protected:
	PreviewWnd     &preview_wnd_;
	ProjectManager &project_manager_;
	// window
	COLORREF custom_colors_[16];
	COLORREF zero_plast_colour_;
	HBRUSH   zero_plast_colour_brush_;
};
