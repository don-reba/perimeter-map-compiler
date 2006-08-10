//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// • Neither the name of Don Reba nor the names of his contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------


#include "StdAfx.h"

#include "info wnd.h"
#include "preview wnd.h"
#include "project data.h"
#include "resource.h"

#include <commctrl.h>

//------------
// some macros
//------------

#define MacroSafeWndCall(wnd, call) \
	if (0 != wnd.hwnd_)              \
	{                                \
		wnd.call;                     \
	}

//----------------------------
// additional message crackers
//----------------------------

#define UD_SetRange(hwndCtl, nLower, nUpper) ((void)SNDMSG((hwndCtl), UDM_SETRANGE, 0L, MAKELPARAM((nLower), (nUpper))))
#define UD_SetPos(hwndCtl, nPos) ((void)SNDMSG((hwndCtl), UDM_SETPOS, 0L, MAKELPARAM((nPos), 0)))
#define UD_GetPos(hwndCtl, nPos) ((int)SNDMSG((hwndCtl), UDM_GETPOS, 0L, 0L))
#define Button_SetIcon(hwndCtl, icon_id) ((void)SNDMSG((hwndCtl), BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_id), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)))

//-----------------------
// InfoWnd implementation
// construction
//-----------------------

InfoWnd::InfoWnd(PreviewWnd &preview_wnd, ZeroLevelChanged *zero_layer_changed)
	:preview_wnd_               (preview_wnd)
	,layout_                    (NULL)
	,zero_level_changed_        (zero_layer_changed)
	,zero_level_changes_ignored_(0)
{
	SetReadOnly(true);
}

//-----------------------
// InfoWnd implementation
// interface
//-----------------------

bool InfoWnd::Create(HWND parent_wnd, const RECT &window_rect)
{
	// create the main window
	hwnd_ = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_INFO_DLG),
		parent_wnd,
		DlgProc<InfoWnd>,
		ri_cast<LPARAM>(this));
	if (NULL == hwnd_)
	{
		MacroDisplayError(_T("The Details window could not be created."));
		return false;
	}
	// create the layout window
	layout_ = CreateWindow(
		HTMLayoutClassNameT(),
		_T(""),
		WS_CHILD|WS_VISIBLE,
		0,
		0,
		0,
		0,
		hwnd_,
		NULL,
		GetModuleHandle(NULL),
		NULL);
	if (NULL == layout_)
	{
		MacroDisplayError(_T("The Details window layout could not be created."));
		return false;
	}
	// load HTML
	{
		std::vector<wchar_t> path_v(MAX_PATH);
		wchar_t * path(&path_v[0]);
		GetCurrentDirectoryW(path_v.size(), path);
		PathCombineW(path, path, L"skin\\info_wnd.html");
		if (FALSE == HTMLayoutLoadFile(layout_, path))
		{
			MacroDisplayError(_T("The Details window layout could not be loaded."));
			return false;
		}
	}
	// position and resize the main window
	{
		SIZE size(CalculateWindowSize());
		SetWindowPos(
			hwnd_,
			NULL,
			window_rect.left,
			window_rect.top,
			size.cx,
			size.cy,
			SWP_NOACTIVATE | SWP_NOZORDER);
	}

	EnableWindow(hwnd_, TRUE);
	return true;
}

void InfoWnd::Update()
{
	using htmlayout::dom::element;
	element root(element::root_element(layout_));
	DeleteObject(fog_colour_);
	fog_colour_ = CreateSolidBrush(MacroProjectData(ID_FOG_COLOUR));
	SetHtmlText("map_fog_start", MacroProjectData(ID_FOG_START), root);
	SetHtmlText("map_fog_end", MacroProjectData(ID_FOG_END), root);
	SetHtmlText("map_zero_layer", MacroProjectData(ID_ZERO_LEVEL), root);
	root.update(true);
}

void InfoWnd::SetReadOnly(bool read_only)
{
	read_only = true;
}

//-----------------------
// InfoWnd implementation
// message processing
//-----------------------

void InfoWnd::OnColorStatic(Msg<WM_CTLCOLORSTATIC> &msg)
{
	if (msg.ChildHwnd() == GetDlgItem(hwnd_, IDC_FOG_COLOUR))
	{
		msg.SetResult(fog_colour_);
		msg.handled_ = true;
	}
}

void InfoWnd::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDC_FOG_COLOUR_BTN:
		{
			CHOOSECOLOR cc = { 0 };
			cc.lStructSize  = sizeof(cc);
			cc.hwndOwner    = hwnd_;
			cc.lpCustColors = custom_colours_;
			cc.rgbResult    = MacroProjectData(ID_FOG_COLOUR);
			cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
			if (ChooseColor(&cc))
			{
				MacroProjectData(ID_FOG_COLOUR) = cc.rgbResult;
				DeleteObject(fog_colour_);
				fog_colour_ = CreateSolidBrush(MacroProjectData(ID_FOG_COLOUR));
				InvalidateRect(GetDlgItem(hwnd_, IDC_FOG_COLOUR), NULL, true);
				MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_FOG_COLOUR));
			}
		} break;
	case IDC_FOG_END:
		if (EN_CHANGE == msg.CodeNotify())
		{
			unsigned int val(GetDlgItemInt(hwnd_, IDC_FOG_END, NULL, FALSE));
			MacroProjectData(ID_FOG_END) = val;
			MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_FOG_END));
		} break;
	case IDC_FOG_START:
		if (EN_CHANGE == msg.CodeNotify())
		{
			UINT val(GetDlgItemInt(hwnd_, IDC_FOG_START, NULL, FALSE));
			MacroProjectData(ID_FOG_START) = val;
			MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_FOG_START));
		} break;
	case IDC_LOCATION_LIST:
		if (CBN_SELCHANGE == msg.CodeNotify())
		{
			// get the index of the newly selected element
			size_t index;
			{
				int signed_index(ComboBox_GetCurSel(msg.CtrlHwnd()));
				if (signed_index < 0)
					return;
				index = static_cast<size_t>(signed_index);
			}
			// set the position edit boxes to the corresponding values
			POINT point(locations_.at(index));
			UD_SetPos(GetDlgItem(hwnd_, IDC_LOCATION_X_SPIN), point.x);
			UD_SetPos(GetDlgItem(hwnd_, IDC_LOCATION_Y_SPIN), point.y);
			// adjust index and highlight the corresponding marker
			switch (index)
			{
			case 0: index = 1; break;
			case 1: index = 2; break;
			case 2: index = 3; break;
			case 3: index = 4; break;
			case 4: index = 0; break;
			}
			MacroSafeWndCall(preview_wnd_, HighlightMarker(index));
		} break;
	case IDC_LOCATION_X:
		if (EN_CHANGE == msg.CodeNotify())
		{
			const UINT max_val(exp2(MacroProjectData(ID_POWER_X)) - 1);
			UINT val(GetDlgItemInt(hwnd_, IDC_LOCATION_X, NULL, FALSE));
			val = __min(val, max_val);
			switch (ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_LOCATION_LIST)))
			{
			case 0:
				MacroProjectData(ID_SP_1).x = val;
				preview_wnd_.ProjectDataChanged(ProjectData::ID_SP_1);
				break;
			case 1:
				MacroProjectData(ID_SP_2).x = val;
				preview_wnd_.ProjectDataChanged(ProjectData::ID_SP_2);
				break;
			case 2:
				MacroProjectData(ID_SP_3).x = val;
				preview_wnd_.ProjectDataChanged(ProjectData::ID_SP_3);
				break;
			case 3:
				MacroProjectData(ID_SP_4).x = val;
				preview_wnd_.ProjectDataChanged(ProjectData::ID_SP_4);
				break;
			case 4:
				MacroProjectData(ID_SP_0).x = val;
				preview_wnd_.ProjectDataChanged(ProjectData::ID_SP_0);
				break;
			}
		} break;
	case IDC_LOCATION_Y:
		if (EN_CHANGE == msg.CodeNotify())
		{
			const UINT max_val(exp2(MacroProjectData(ID_POWER_Y)) - 1);
			UINT val(GetDlgItemInt(hwnd_, IDC_LOCATION_Y, NULL, FALSE));
			val = __min(val, max_val);
			switch (ComboBox_GetCurSel(GetDlgItem(hwnd_, IDC_LOCATION_LIST)))
			{
			case 0:
				MacroProjectData(ID_SP_1).y = val;
				MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_SP_1));
				break;
			case 1:
				MacroProjectData(ID_SP_2).y = val;
				MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_SP_2));
				break;
			case 2:
				MacroProjectData(ID_SP_3).y = val;
				MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_SP_3));
				break;
			case 3:
				MacroProjectData(ID_SP_4).y = val;
				MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_SP_4));
				break;
			case 4:
				MacroProjectData(ID_SP_0).y = val;
				MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_SP_0));
				break;
			}
		} break;
	case IDC_ZERO_PLAST:
		if (EN_CHANGE == msg.CodeNotify())
		{
			const UINT max_val(255);
			UINT val(GetDlgItemInt(hwnd_, IDC_ZERO_PLAST, NULL, TRUE));
			val = __min(val, max_val);
			MacroProjectData(ID_ZERO_LEVEL) = val;
			MacroSafeWndCall(preview_wnd_, ProjectDataChanged(ProjectData::ID_ZERO_LEVEL));
			if (zero_level_changes_ignored_ >= 1 && IsWindowVisible(hwnd_))
				SetTimer(hwnd_, 0, 3000, NULL);
			else
				++zero_level_changes_ignored_;
		} break;
	case IDCANCEL:
		ShowWindow(hwnd_, SW_HIDE);
		break;
	}
}

void InfoWnd::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	HWND item;
	// initialize the location list
	{
		HWND locus_list(GetDlgItem(hwnd_, IDC_LOCATION_LIST));
		AddLocation(_T("Player 1"),        MacroProjectData(ID_SP_1).x, MacroProjectData(ID_SP_1).y);
		AddLocation(_T("Player 2"),        MacroProjectData(ID_SP_2).x, MacroProjectData(ID_SP_2).y);
		AddLocation(_T("Player 3"),        MacroProjectData(ID_SP_3).x, MacroProjectData(ID_SP_3).y);
		AddLocation(_T("Player 4"),        MacroProjectData(ID_SP_4).x, MacroProjectData(ID_SP_4).y);
		AddLocation(_T("Survival Player"), MacroProjectData(ID_SP_0).x, MacroProjectData(ID_SP_0).y);
		ComboBox_SetCurSel(locus_list, 0);
		{
			RECT rect = { 0 };
			GetWindowRect(locus_list, &rect);
			ScreenToClient(GetParent(locus_list), &rect);
			MoveWindow(locus_list, rect.left, rect.top, rect.right - rect.left, 128, FALSE);
		}
	}
	// set spin controls
	item = GetDlgItem(hwnd_, IDC_ZERO_PLAST_SPIN);
	UD_SetRange(item, 255, 0);
	UD_SetPos(item, 0);
	// set colours
	fog_colour_ = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	for (int i(0); i != 16; ++i)
		custom_colours_[i] = RGB(255, 255, 255);
	// set fog distance values
	SetDlgItemInt(hwnd_, IDC_FOG_START, 0u, FALSE);
	SetDlgItemInt(hwnd_, IDC_FOG_END,   0u, FALSE);
	// set colour button icon
	Button_SetIcon(GetDlgItem(hwnd_, IDC_FOG_COLOUR_BTN), IDI_TRIANGLE);
	// wrap up
	msg.result_ = TRUE;
	msg.handled_ = true;
}

void InfoWnd::OnNotify(Msg<WM_NOTIFY> &msg)
{
	if (msg.nmhdr().code == HLN_ATTACH_BEHAVIOR)
	{
		LPNMHL_ATTACH_BEHAVIOR lpab(ri_cast<LPNMHL_ATTACH_BEHAVIOR>(msg.lprm_));
		htmlayout::event_handler *pb = htmlayout::behavior::find(lpab->behaviorName, lpab->element);
		if(pb) 
		{
			lpab->elementTag  = pb;
			lpab->elementProc = htmlayout::behavior::element_proc;
			lpab->elementEvents = pb->subscribed_to;
		}
	}
}

void InfoWnd::OnSize(Msg<WM_SIZE> &msg)
{
	PositionChildren();
}

void InfoWnd::OnTimer(Msg<WM_TIMER> &msg)
{
	KillTimer(hwnd_, 0);
	(*zero_level_changed_)();
	msg.handled_ = true;
}

void InfoWnd::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&InfoWnd::OnColorStatic,
		&InfoWnd::OnCommand,
		&InfoWnd::OnInitDialog,
		&InfoWnd::OnNotify,
		&InfoWnd::OnSize,
		&InfoWnd::OnTimer,
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

//-----------------------
// InfoWnd implementation
// internal function
//-----------------------

void InfoWnd::AddLocation(tstring name, uint x, uint y)
{
	ComboBox_AddString(GetDlgItem(hwnd_, IDC_LOCATION_LIST), name.c_str());
	POINT point = { x, y };
	locations_.push_back(point);
}

SIZE InfoWnd::CalculateMinWindowSize() const
{
	RECT client;
	GetClientRect(hwnd_, &client);
	RECT window;
	GetWindowRect(hwnd_, &window);
	LONG dx(window.right  - window.left - client.right );
	LONG dy(window.bottom - window.top  - client.bottom);
	UINT min_w(::HTMLayoutGetMinWidth( layout_));
	UINT min_h(::HTMLayoutGetMinHeight(layout_, min_w));
	SIZE size = { dx + min_w, dy + min_h };
	return size;
}

SIZE InfoWnd::CalculateWindowSize() const
{
	using htmlayout::dom::element;
	SIZE size = { 0 };
	// get the expandable list
	element root(htmlayout::dom::element::root_element(layout_));
	element bar(root.get_element_by_id(L"thebar"));
	if (bar.is_valid())
	{
		uint bar_child_count(bar.children_count());
		// get the content elements
		vector<element> elements;
		elements.reserve(bar_child_count);
		for (uint i(0); i != bar_child_count; ++i)
		{
			htmlayout::dom::element child(bar.child(i));
			htmlayout::dom::element content(child.find_first(".content"));
			if (content.is_valid())
				elements.push_back(content);
		}
		// show each content element, and measure the size
		// keep the maximum
		foreach (element &content, elements)
		{
			content.set_attribute("class", L"no_content");
			root.update(true);
			SIZE content_size(CalculateMinWindowSize());
			if (content_size.cx > size.cx)
				size.cx = content_size.cx;
			if (content_size.cy > size.cy)
				size.cy = content_size.cy;
			content.set_attribute("class", L"content");
		}
	}
	else
	{
		size = CalculateMinWindowSize();
	}
	return size;
}

void InfoWnd::PositionChildren()
{
	if (NULL != layout_)
	{
		RECT client_rect;
		GetClientRect(hwnd_, &client_rect);
		SetWindowPos(layout_, NULL, 0, 0, client_rect.right, client_rect.bottom, SWP_NOZORDER);
	}
}

//-----------------------
// InfoWnd implementation
// html manipulation
//-----------------------

bool InfoWnd::SetHtmlText(const char * id, int value, htmlayout::dom::element root)
{
	using htmlayout::dom::element;
	// buffer for integer conversion
	wchar_t w_int_str[34];
	// find the element
	element e(root.get_element_by_id(L"map_zero_layer"));
	if (!e.is_valid())
		return false;
	// convert the value to string, and set it
	_itow(MacroProjectData(ID_ZERO_LEVEL), w_int_str, 10);
	e.set_text(w_int_str);
	return true;
}
