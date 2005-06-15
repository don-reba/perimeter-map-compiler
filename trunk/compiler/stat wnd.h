#pragma once

#include "panel wnd.h"

//----------------------------
// statistics panel definition
//----------------------------

class StatWnd : public PanelWindow
{
// interface
public:
	bool Create(HWND parent_wnd, const RECT &window_rect);
	void SetAverageHeight(uint height);
	void SetAverageColour(COLORREF colour);
// message handlers
private:
	void OnColorStatic(Msg<WM_CTLCOLORSTATIC> &msg);
	void OnInitDialog(Msg<WM_INITDIALOG> &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
// data
private:
	HBRUSH average_colour_;
};
