#include "StdAfx.h"
#include "resource.h"
#include "preferences.h"
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

CPreferences::CPreferences(
	CPreview::CSerializable *preview_settings,
	CProjectManager::CSerializable *project_settings)
	:preview_settings(preview_settings)
	,project_settings(project_settings)
{
	zero_plast_colour = GetSysColor(COLOR_BTNFACE);
	zero_plast_colour_brush = CreateSolidBrush(zero_plast_colour);
	for (int i(0); i != 16; ++i)
		custom_colors[i] = RGB(255, 255, 255);
}

CPreferences::~CPreferences()
{
}

void CPreferences::Create(HWND hWndParent)
{
	// create the window
	hWnd = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_PREFERENCES),
		hWndParent,
		DialogProc,
		ri_cast<LPARAM>(this));
}

INT_PTR CALLBACK CPreferences::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// prevent processing before the dialog is fully initialized
	static bool initialized(false);
	if (WM_INITDIALOG == uMsg)
		initialized = true;
	if (false == initialized)
		return FALSE;
	// get the object pointer
	CPreferences *obj;
	if (WM_INITDIALOG == uMsg)
		obj = ri_cast<CPreferences*>(lParam);
	else
		obj = ri_cast<CPreferences*>(GetWindowLong(hWnd, DWL_USER));
	// process messages
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_COMMAND, obj->OnCommand);
		HANDLE_MSG(hWnd, WM_CTLCOLORSTATIC, obj->OnColorStatic);
		HANDLE_MSG(hWnd, WM_DESTROY, obj->OnDestroy);
		HANDLE_MSG(hWnd, WM_INITDIALOG, obj->OnInitDialog);
	}
	return FALSE;
}


BOOL CPreferences::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		Apply();
		DestroyWindow(hWnd);
		break;
	case IDCANCEL:
		DestroyWindow(hWnd);
		break;
	case ID_APPLY_NOW:
		Apply();
		break;
	case IDC_ZERO_PLAST_COLOUR_BTN:
		{
			CHOOSECOLOR cc;
			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = hWnd;
			cc.lpCustColors = custom_colors;
			cc.rgbResult = zero_plast_colour;
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;
			if (ChooseColor(&cc))
			{
				zero_plast_colour= cc.rgbResult;
				DeleteObject(zero_plast_colour_brush);
				zero_plast_colour_brush = CreateSolidBrush(zero_plast_colour);
				InvalidateRect(GetDlgItem(hWnd, IDC_ZERO_PLAST_COLOUR), NULL, true);
			}
		} break;
	}
	return FALSE;
}

HBRUSH CPreferences::OnColorStatic(HWND hWnd, HDC hdc, HWND hwndChild, int type)
{
	if (hwndChild == GetDlgItem(hWnd, IDC_ZERO_PLAST_COLOUR))
		return zero_plast_colour_brush;
	return FALSE;
}

BOOL CPreferences::OnDestroy(HWND hWnd)
{
	DestroyWindow(hWnd);
	this->hWnd = NULL;
	return TRUE;
}

BOOL CPreferences::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	// save pointer to this class in window user data
	SetWindowLong(hWnd, DWL_USER, lParam);
	// initialise controls
	const size_t buffer_size(256);
	TCHAR buffer[buffer_size];
	preview_settings->SignOut();
	project_settings->SignOut();
	// threshold
	{
		_stprintf(buffer, "%.2f", preview_settings->threshold);
		SetDlgItemText(hWnd, IDC_LOD, buffer);
	}
	// opacity
	{
		HWND zero_plast_opacity_spin(GetDlgItem(hWnd, IDC_ZERO_PLAST_OPACITY_SPIN));
		UD_SetRange(GetDlgItem(hWnd, IDC_ZERO_PLAST_OPACITY_SPIN), 255, 0);
		UD_SetPos(zero_plast_opacity_spin, preview_settings->zero_layer_colour >> 24);
		Button_SetIcon(GetDlgItem(hWnd, IDC_ZERO_PLAST_COLOUR_BTN), IDI_TRIANGLE);
	}
	// colour
	{
		D3DCOLOR &d3d_colour(preview_settings->zero_layer_colour);
		zero_plast_colour = RGB(
			(d3d_colour & 0x00FF0000) >> 16,
			(d3d_colour & 0x0000FF00) >> 8,
			(d3d_colour & 0x000000FF) >> 0);
		DeleteObject(zero_plast_colour_brush);
		zero_plast_colour_brush = CreateSolidBrush(zero_plast_colour);
		InvalidateRect(GetDlgItem(hWnd, IDC_ZERO_PLAST_COLOUR), NULL, TRUE);
	}
	// texture_quality
	{
		CheckRadioButton(
			hWnd,
			IDC_TEXTURE_COLOUR_SPEED,
			IDC_TEXTURE_COLOUR_QUALITY,
			project_settings->texture_colour_quality ? IDC_TEXTURE_COLOUR_QUALITY : IDC_TEXTURE_COLOUR_SPEED);
	}
	// lighting
	{
		CheckDlgButton(hWnd, IDC_LIGHTING, project_settings->enable_lighting ? TRUE : FALSE);
	}
	// wrap up
	project_settings->SignIn();
	preview_settings->SignIn();
	return TRUE;
}

void CPreferences::Apply()
{
	// general
	{
		// texture_colour_quality
		bool texture_colour_quality(BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_TEXTURE_COLOUR_QUALITY));
		// enable_lighting
		bool enable_lighting(BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_LIGHTING));
		// commit settings
		project_settings->SignOut();
		project_settings->texture_colour_quality = texture_colour_quality;
		project_settings->enable_lighting        = enable_lighting;
		project_settings->SignIn();
		// update
		project_settings->Update();
	}
	// map preview
	{
		// get control info
		float        threshold(3.0f);
		unsigned int opacity(0x28);
		bool         enable_lighting(true);
		// threshold
		{
			HWND ctrl(GetDlgItem(hWnd, IDC_LOD));
			const size_t buffer_size(GetWindowTextLength(ctrl) + 1);
			TCHAR *buffer(new TCHAR[buffer_size]);
			GetWindowText(ctrl, buffer, buffer_size);
			_stscanf(buffer, "%f", &threshold);
			delete [] buffer;
			threshold = __max(0, __min(8.0f, threshold));
		}
		// opacity
		{
			opacity = GetDlgItemInt(hWnd, IDC_LIGHTING, NULL, FALSE);
			opacity = __min(255, opacity);
		}
		// enable_lighting
		{
			enable_lighting = BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_LIGHTING);
		}
		// commit settings
		preview_settings->SignOut();
		preview_settings->threshold = threshold;
		preview_settings->zero_layer_colour = D3DCOLOR_ARGB(
			opacity,
			GetRValue(zero_plast_colour),
			GetGValue(zero_plast_colour),
			GetBValue(zero_plast_colour));
		preview_settings->enable_lighting = enable_lighting;
		preview_settings->SignIn();
		// update
		preview_settings->Update();
	}
}