#pragma once

#include "error handler.h"
#include "window.h"

//----------------------------------------
// functionality common to all PMC windows
//----------------------------------------

struct PMCWindow : public ErrorHandler, public Window
{
// construction/destruction
public:
	PMCWindow();
// interface
public:
	RECT GetRect() const;
	virtual void ToggleWaitCursor(bool on);
// message handlers
protected:
	void OnMove     (Msg<WM_MOVE>      &msg);
	void OnSetCursor(Msg<WM_SETCURSOR> &msg);
	void OnSize     (Msg<WM_SIZE>      &msg);
// message pump
protected:
	void ProcessMessage(WndMsg &msg);
// descendant services
protected:
	void SetRect(const RECT &rect);
// data
private:
	HCURSOR cursor_;
	HCURSOR old_cursor_;
	RECT    window_rect_;
};