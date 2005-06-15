#pragma once

#include "panel wnd.h"

class PreviewWnd;

//---------------------------------
// map information panel definition
//---------------------------------

class InfoWnd : public PanelWindow
{
// construction/destruction
public:
	InfoWnd(PreviewWnd &preview_wnd);
// interface
public:
	bool Create(HWND parent_wnd, const RECT &window_rect, bool enabled = true);
	void Update(bool read_only = false);
// message handlers
private:
	void OnColorStatic(Msg<WM_CTLCOLORSTATIC> &msg);
	void OnCommand    (Msg<WM_COMMAND>        &msg);
	void OnInitDialog (Msg<WM_INITDIALOG>     &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
private:
	static BOOL CALLBACK EnumChildrenProc(HWND hwnd, LPARAM lprm);
	void EnableControls(bool on);
// data
private:
	HBRUSH      fog_colour_;
	COLORREF    custom_colours_[16];
	PreviewWnd &preview_wnd_;
};
