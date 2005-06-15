#include "StdAfx.h"

#include "resource.h"
#include "project create wnd.h"

#include <sstream>

//--------------------------------
// statistics panel implementation
//--------------------------------

INT_PTR CreateProjectDlg::DoModal(HWND parent_wnd)
{
	return DialogBoxParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_PROJECT_DLG),
		parent_wnd,
		DlgProc<CreateProjectDlg>,
		ri_cast<LPARAM>(this));
}

void CreateProjectDlg::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDOK:     OnOk    (msg); break;
	case IDCANCEL: OnCancel(msg); break;
	}
}

void CreateProjectDlg::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	// initialize IDC_MAP_SIZE
	HWND ctrl(GetDlgItem(hwnd_, IDC_MAP_SIZE));
	ComboBox_AddString(ctrl, _T("2048x2048"));
	ComboBox_AddString(ctrl, _T("4096x4096"));
	ComboBox_SetCurSel(ctrl, 0);
}

void CreateProjectDlg::OnCancel(Msg<WM_COMMAND> &msg)
{
	EndDialog(hwnd_, IDCANCEL);
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void CreateProjectDlg::OnOk(Msg<WM_COMMAND> &msg)
{
	if (ExchangeData())
	{
		EndDialog(hwnd_, IDOK);
		msg.result_  = FALSE;
		msg.handled_ = true;
	}
}

void CreateProjectDlg::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&CreateProjectDlg::OnCommand,
		&CreateProjectDlg::OnInitDialog
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

bool CreateProjectDlg::ExchangeData()
{
	// retrieve map size
	{
		HWND map_size_wnd = GetDlgItem(hwnd_, IDC_MAP_SIZE);
		int current_selection = ComboBox_GetCurSel(map_size_wnd);
		switch (current_selection)
		{
		case 0:
			map_size_.cx = 2048;
			map_size_.cy = 2048;
			break;
		case 1:
			map_size_.cx = 4096;
			map_size_.cy = 4096;
			break;
		default:
			map_size_.cx = 0;
			map_size_.cy = 0;
		}
	}
	// retrieve map name
	{
		// retrieve unvalidated
		const size_t max_name_length(21); // WARN: uncertain whether this is the game's limit
		TCHAR name[max_name_length];
		GetDlgItemText(hwnd_, IDC_MAP_NAME, name, max_name_length);
		size_t name_length(_tcslen(name));
		// character replacements
		{
			TCHAR *source(" ");
			TCHAR *target("_");
			for (size_t i(0); i != name_length; ++i)
			{
				TCHAR *chr_i(_tcschr(source, name[i]));
				if (NULL != chr_i)
					name[i] = *(target + (chr_i - source));
			}
		}
		// validate
		{
			// make sure the name is not empty
			if (0 == name_length)
			{
				MessageBox(hwnd_, _T("Please enter a map name."), NULL, MB_OK);
				return false;
			}
			// make sure the name contains only valid characters
			// the actual validation is performed on the server during registration
			// validation at the stage of creation only saves the user from later surprises
			bool invalid_char(false);
			TCHAR *valid_chars(_T("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'_-"));
			for (size_t i(0); i != name_length; ++i)
			{
				if (NULL == _tcschr(valid_chars, name[i]))
				{
					invalid_char = true;
					break;
				}
			}
			if (invalid_char)
			{
				MessageBox(hwnd_, _T("The map name contains an invalid character.\nOnly numbers, letters, spaces, dashes, and apostrophies are allowed."), NULL, MB_OK);
				return false;
			}
			// make sure the name does not match one of the reserved namesstring worlds_list;
			{
				tstring worlds_list;
				// load the list of reserved names
				{
					HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_WORLDS_LIST), "Text"));
					HGLOBAL resource(LoadResource(NULL, resource_info));
					char *text(ri_cast<char*>(LockResource(resource)));
					worlds_list = text;
				}
				// check if the name occurs in the list
				std::istringstream worlds_list_stream(worlds_list);
				string world;
				bool is_reserved(false);
				while (worlds_list_stream)
				{
					worlds_list_stream >> world;
					if (0 == _tcsicmp(world.c_str(), name))
					{
						MessageBox(hwnd_, _T("This map name is reserved."), NULL, MB_OK);
						is_reserved = true;
						break;
					}
				}
				if (is_reserved)
					return false;
			}
		}
		// save the validated map name
		map_name_ = name;
	}
	return true;
}