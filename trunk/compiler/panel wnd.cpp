#include "stdafx.h"

#include "panel wnd.h"

//---------------------------
// PanelWindow implementation
//---------------------------

PanelWindow::PanelWindow()
	:is_visible_             (false)
	,visibility_event_       (NULL)
	,visibility_notification_(NULL)
{}

bool PanelWindow::IsVisible() const
{
	return is_visible_;
}

// PRE: event must be allocated on the heap
void PanelWindow::SetVisibilityEvent(PanelWindow::ToggleVisibility *visibility_event)
{
	_ASSERTE(NULL == visibility_event_);
	visibility_event_ = visibility_event;
}

// PRE: notification must be allocated on the heap
void PanelWindow::SetVisibilityNotification(PanelWindow::ToggleVisibility *visibility_notification)
{
	if (NULL != visibility_notification_)
		delete visibility_notification_;
	visibility_notification_ = visibility_notification;
}

void PanelWindow::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDCANCEL:
		ShowWindow(hwnd_, SW_HIDE);
	}
	msg.handled_ = true;
}

void PanelWindow::OnShowWindow(Msg<WM_SHOWWINDOW> &msg)
{
	is_visible_ = msg.IsShown();
	if (NULL != visibility_event_)
		(*visibility_event_)(is_visible_);
	if (NULL != visibility_notification_ && is_visible_)
	{
		(*visibility_notification_)(is_visible_);
		delete visibility_notification_;
		visibility_notification_ = NULL;
	}
	msg.handled_ = true;
}

void PanelWindow::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&PanelWindow::OnCommand,
		&PanelWindow::OnShowWindow
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}