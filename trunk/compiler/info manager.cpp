#include "StdAfx.h"
#include "resource.h"
#include "info manager.h"
#include <commctrl.h>


//----------------------------
// additional message crackers
//----------------------------

#define UD_SetRange(hwndCtl, nLower, nUpper) ((void)SNDMSG((hwndCtl), UDM_SETRANGE, 0L, MAKELPARAM((nLower), (nUpper))))
#define UD_SetPos(hwndCtl, nPos) ((void)SNDMSG((hwndCtl), UDM_SETPOS, 0L, MAKELPARAM((nPos), 0)))
#define UD_GetPos(hwndCtl, nPos) ((int)SNDMSG((hwndCtl), UDM_GETPOS, 0L, 0L))
#define Button_SetIcon(hwndCtl, icon_id) ((void)SNDMSG((hwndCtl), BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_id), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)))

//----------------------------
// CInfoManager implementation
//----------------------------

CInfoManager::CInfoManager(CData<CMapInfo> *map_info, int id) :
	map_info (map_info),
	id       (id),
	hWnd     (NULL),
	hButton  (NULL),
	read_only(false)
{
	fog_colour = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	for (int i(0); i != 16; ++i)
		custom_colors[i] = RGB(255, 255, 255);
}

CInfoManager::~CInfoManager()
{
}

void CInfoManager::Create(HWND hWndParent, const RECT &window_rect, HWND hButton)
{
	// create the window
	this->hButton = hButton;
	hWnd = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_INFO_DLG),
		hWndParent,
		DialogProc,
		ri_cast<LPARAM>(this));
	DWORD swp_uflags(SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(hWnd, NULL, window_rect.left, window_rect.top, 0, 0, swp_uflags);
	this->window_rect = window_rect;
	is_visible = false;
}

void CInfoManager::Destroy()
{
	DestroyWindow(hWnd);
}

void CInfoManager::Update(int caller)
{
	if (id == caller)
		return;
	const CMapInfo map_data(map_info->SignOutConst());
	map_info->SignIn();
	SetDlgItemInt(hWnd, IDC_FOG_START,   map_data.fog_start,   FALSE);
	SetDlgItemInt(hWnd, IDC_FOG_END,     map_data.fog_end,     FALSE);
	if (!read_only)
	{
		EnableWindow(GetDlgItem(hWnd, IDC_ZERO_PLAST),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_ZERO_PLAST_SPIN), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_FOG_START),       TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_FOG_END),         TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_FOG_COLOUR_BTN),  TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_FOG_COLOUR),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP1_X),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP1_X_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP1_Y),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP1_Y_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP2_X),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP2_X_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP2_Y),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP2_Y_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP3_X),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP3_X_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP3_Y),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP3_Y_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP4_X),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP4_X_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP4_Y),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP4_Y_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP0_X),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP0_X_SPIN),      TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP0_Y),           TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_SP0_Y_SPIN),      TRUE);
	}
	UD_SetRange(GetDlgItem(hWnd, IDC_SP1_X_SPIN), exp2(map_data.map_power_x) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP1_Y_SPIN), exp2(map_data.map_power_y) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP2_X_SPIN), exp2(map_data.map_power_x) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP2_Y_SPIN), exp2(map_data.map_power_y) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP3_X_SPIN), exp2(map_data.map_power_x) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP3_Y_SPIN), exp2(map_data.map_power_y) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP4_X_SPIN), exp2(map_data.map_power_x) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP4_Y_SPIN), exp2(map_data.map_power_y) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP0_X_SPIN), exp2(map_data.map_power_x) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_SP0_Y_SPIN), exp2(map_data.map_power_y) - 1, 0);
	UD_SetRange(GetDlgItem(hWnd, IDC_ZERO_PLAST_SPIN), 255, 0);
	UD_SetPos(GetDlgItem(hWnd, IDC_ZERO_PLAST_SPIN), map_data.zero_plast);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP1_X_SPIN),      map_data.start_pos[1].x);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP1_Y_SPIN),      map_data.start_pos[1].y);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP2_X_SPIN),      map_data.start_pos[2].x);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP2_Y_SPIN),      map_data.start_pos[2].y);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP3_X_SPIN),      map_data.start_pos[3].x);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP3_Y_SPIN),      map_data.start_pos[3].y);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP4_X_SPIN),      map_data.start_pos[4].x);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP4_Y_SPIN),      map_data.start_pos[4].y);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP0_X_SPIN),      map_data.start_pos[0].x);
	UD_SetPos(GetDlgItem(hWnd, IDC_SP0_Y_SPIN),      map_data.start_pos[0].y);
	DeleteObject(fog_colour);
	fog_colour = CreateSolidBrush(map_data.fog_colour);
	InvalidateRect(GetDlgItem(hWnd, IDC_FOG_COLOUR), NULL, true);
}

void CInfoManager::ToggleVisibility(bool show)
{
	ShowWindow(hWnd, show ? SW_SHOW : SW_HIDE);
}

CViewer::WndSaveInfo CInfoManager::GetSaveInfo()
{
	if (hWnd)
		GetWindowRect(hWnd, &window_rect);
	CViewer::WndSaveInfo wsi;
	wsi.is_visible = is_visible;
	wsi.name = _T("info_wnd");
	wsi.rect = window_rect;
	return wsi;
}

void CInfoManager::SetReadOnly(bool on)
{
	read_only = true;
}

INT_PTR CALLBACK CInfoManager::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// prevent processing before the dialog is fully initialized
	static bool initialized(false);
	if (WM_INITDIALOG == uMsg)
		initialized = true;
	if (false == initialized)
		return FALSE;
	// process messages
	CInfoManager *obj = ri_cast<CInfoManager*>(GetWindowLong(hWnd, DWL_USER));
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_CTLCOLORSTATIC, obj->OnColorStatic);
		HANDLE_MSG(hWnd, WM_COMMAND,        obj->OnCommand);
		HANDLE_MSG(hWnd, WM_DESTROY,        obj->OnDestroy);
		HANDLE_MSG(hWnd, WM_INITDIALOG,     obj->OnInitDialog);
		HANDLE_MSG(hWnd, WM_SHOWWINDOW,     obj->OnShowWindow);
	}
	return FALSE;
}
// HANDLE_WM_COMMAND handler
BOOL CInfoManager::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDCANCEL:
		ToggleVisibility(false);
		return TRUE;
	case IDC_ZERO_PLAST:
		if (EN_CHANGE == codeNotify)
		{
			// get the value and make sure the it is within range
			const unsigned int max_val(255);
			unsigned int val(GetDlgItemInt(hWnd, IDC_ZERO_PLAST, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			CMapInfo &map_data(map_info->SignOut());
			map_data.zero_plast = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_FOG_START:
		if (EN_CHANGE == codeNotify)
		{
			unsigned int val(GetDlgItemInt(hWnd, IDC_FOG_START, NULL, FALSE));
			CMapInfo &map_data(map_info->SignOut());
			map_data.fog_start = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_FOG_END:
		if (EN_CHANGE == codeNotify)
		{
			unsigned int val(GetDlgItemInt(hWnd, IDC_FOG_END, NULL, FALSE));
			CMapInfo &map_data(map_info->SignOut());
			map_data.fog_end = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_FOG_COLOUR_BTN:
		{
			CHOOSECOLOR cc;
			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = hWnd;
			cc.lpCustColors = custom_colors;
			const CMapInfo &map_data(map_info->SignOutConst());
			cc.rgbResult = map_data.fog_colour;
			map_info->SignIn();
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;
			if (ChooseColor(&cc))
			{
				CMapInfo &map_data(map_info->SignOut());
				map_data.fog_colour = cc.rgbResult;
				DeleteObject(fog_colour);
				fog_colour = CreateSolidBrush(map_data.fog_colour);
				InvalidateRect(GetDlgItem(hWnd, IDC_FOG_COLOUR), NULL, true);
				map_info->SignIn();
				map_info->Update(id);
			}
		} break;
	case IDC_SP1_X:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_x) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP1_X, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[1].x = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP1_Y:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_y) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP1_Y, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[1].y = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP2_X:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_x) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP2_X, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[2].x = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP2_Y:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_y) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP2_Y, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[2].y = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP3_X:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_x) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP3_X, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[3].x = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP3_Y:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_y) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP3_Y, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[3].y = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP4_X:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_x) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP4_X, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[4].x = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP4_Y:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_y) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP4_Y, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[4].y = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP0_X:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_x) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP0_X, NULL, TRUE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[0].x = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	case IDC_SP0_Y:
		if (EN_CHANGE == codeNotify)
		{
			CMapInfo &map_data(map_info->SignOut());
			// get the value and make sure the it is within range
			const unsigned int max_val(exp2(map_data.map_power_y) - 1);
			unsigned int val(GetDlgItemInt(hWnd, IDC_SP0_Y, NULL, FALSE));
			val = min(val, max_val);
			// set the value
			map_data.start_pos[0].y = val;
			map_info->SignIn();
			map_info->Update(this->id);
		} break;
	}
	return FALSE;
}

// WM_CTLCOLORSTATIC handler
HBRUSH CInfoManager::OnColorStatic(HWND hWnd, HDC hdc, HWND hwndChild, int type)
{
	if (hwndChild == GetDlgItem(hWnd, IDC_FOG_COLOUR))
	{
		HBRUSH brush(fog_colour);
		return brush;
	}
	return FALSE;
}

// WM_DESTROY handler
BOOL CInfoManager::OnDestroy(HWND hWnd)
{
	GetWindowRect(hWnd, &window_rect);
	DestroyWindow(hWnd);
	this->hWnd = NULL;
	return TRUE;
}

// WM_INITDIALOG handler
BOOL CInfoManager::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	// save pointer to this class in window user data
	SetWindowLong(hWnd, DWL_USER, lParam);
	// initialise controls
	UD_SetRange(GetDlgItem(hWnd, IDC_ZERO_PLAST_SPIN), 255, 0);
	Button_SetIcon(GetDlgItem(hWnd, IDC_FOG_COLOUR_BTN), IDI_TRIANGLE);
	return TRUE;
}

// WM_SHOWWINDOW handler
BOOL CInfoManager::OnShowWindow(HWND hWnd, BOOL fShow, UINT status)
{
	is_visible = (fShow == TRUE);
	Button_SetCheck(hButton, (TRUE == fShow) ? BST_CHECKED : BST_UNCHECKED);
	return TRUE;
}

inline int CInfoManager::exp2(unsigned int n) const
{
	int e(1);
	while (0 != n)
	{
		e <<= 1;
		--n;
	}
	return e;
}