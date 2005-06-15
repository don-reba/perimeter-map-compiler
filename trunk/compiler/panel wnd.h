#pragma once

#include "pmc wnd.h"

class PanelHolderWindow;

//------------------------
// panel window definition
//------------------------

class PanelWindow : public PMCWindow
{
// nested types
public:
	// functor called by OnShowWindow
	struct ToggleVisibility {
		virtual void operator() (bool on) = 0;
		virtual ~ToggleVisibility() {};
	};
// creation/destruction
public:
	PanelWindow();
// interface
public:
	bool IsVisible() const;
	void SetVisibilityEvent(ToggleVisibility *visibility_event);
	void SetVisibilityNotification(ToggleVisibility *visibility_notification);
// message handlers
protected:
	void OnCommand   (Msg<WM_COMMAND>    &msg);
	void OnShowWindow(Msg<WM_SHOWWINDOW> &msg);
// message pump
protected:
	void ProcessMessage(WndMsg &msg);
// data
private:
	ToggleVisibility *visibility_event_;
	ToggleVisibility *visibility_notification_;
	bool is_visible_;
};