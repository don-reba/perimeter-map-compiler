#include "StdAfx.h"
#include ".\project dialog.h"
#include "resource.h"
#include <sstream>
using std::istringstream;

INT_PTR CProjectDlg::DoModal(HINSTANCE hInstance, HWND hWnd)
{
	return DialogBoxParam(
		hInstance,
		MAKEINTRESOURCE(IDD_PROJECT_DLG),
		hWnd,
		DialogProc,
		ri_cast<LPARAM>(this));
}

INT_PTR CALLBACK CProjectDlg::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CProjectDlg *obj = ri_cast<CProjectDlg*>(GetWindowLong(hWnd, DWL_USER));
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_INITDIALOG, obj->OnInitDialog);
		HANDLE_MSG(hWnd, WM_COMMAND,    obj->OnCommand);
	}
	return FALSE;
}

BOOL CProjectDlg::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	// save pointer to this class in window user data
	SetWindowLong(hWnd, DWL_USER, lParam);
	// initialize IDC_MAP_SIZE
	HWND hWndCtrl = GetDlgItem(hWnd, IDC_MAP_SIZE);
	ComboBox_AddString(hWndCtrl, "2048x2048");
	ComboBox_AddString(hWndCtrl, "4096x4096");
	ComboBox_SetCurSel(hWndCtrl, 0);
	return TRUE;
}

BOOL CProjectDlg::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		{
			// save map size
			HWND hWndMapSize = GetDlgItem(hWnd, IDC_MAP_SIZE);
			int current_selection = ComboBox_GetCurSel(hWndMapSize);
			switch (current_selection)
			{
			case 0:
				map_size.cx = 2048;
				map_size.cy = 2048;
				break;
			case 1:
				map_size.cx = 4096;
				map_size.cy = 4096;
				break;
			default:
				map_size.cx = 0;
				map_size.cy = 0;
			}
			// get map name
			const int max_name_length(21); // WARN: uncertain whether this is the game's limit
			TCHAR name[max_name_length];
			GetDlgItemText(hWnd, IDC_MAP_NAME, name, max_name_length);
			const size_t name_size(_tcslen(name));
			// replace some characters in the name of the map by others
			{
				TCHAR *source(" ");
				TCHAR *target("_");
				for (size_t i(0); i != name_size; ++i)
				{
					TCHAR *chr_i(_tcschr(source, name[i]));
					if (NULL != chr_i)
						name[i] = *(target + (chr_i - source));
				}
			}
			// validate map name
			{
				// make sure the name is not empty
				if (0 == name_size)
				{
					MessageBox(hWnd, _T("Please enter a map name."), NULL, MB_OK);
					break;
				}
				// make sure the name contains only valid characters
				bool invalid_char(false);
				TCHAR *valid_chars(_T("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-"));
				for (size_t i(0); i != name_size; ++i)
					if (NULL == _tcschr(valid_chars, name[i]))
					{
						invalid_char = true;
						break;
					}
				if (invalid_char)
				{
					MessageBox(hWnd, _T("This map name contains an invalid character."), NULL, MB_OK);
					break;
				}
				// make sure the name does not match one of the reserved namesstring worlds_list;
				{
					string worlds_list;
					// load the list of reserved names
					{
						HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_WORLDS_LIST), "Text"));
						HGLOBAL resource(LoadResource(NULL, resource_info));
						char *text(ri_cast<char*>(LockResource(resource)));
						worlds_list = text;
					}
					// check if the name occurs in the list
					istringstream worlds_list_stream(worlds_list);
					string world;
					bool is_reserved(false);
					while (worlds_list_stream)
					{
						worlds_list_stream >> world;
						if (0 == _tcsicmp(world.c_str(), name))
						{
							MessageBox(hWnd, _T("This map name is reserved."), NULL, MB_OK);
							is_reserved = true;
							break;
						}
					}
					if (is_reserved)
						break;
				}
			}
			// dave map name
			map_name = name;
		} // fall through
	case IDCANCEL:
		// quit
		EndDialog(hWnd, id);
		return TRUE;
	}
	return FALSE;
}