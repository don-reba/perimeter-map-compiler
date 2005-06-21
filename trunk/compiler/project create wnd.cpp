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
// • Neither the name of Don Reba nor the names of its contributors may be used
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

#include "resource.h"
#include "resource management.h"
#include "project create wnd.h"
#include "task common.h"

#include <algorithm>
#include <set>
#include <sstream>

using namespace RsrcMgmt;

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
			// make sure the name does not match one of the reserved names
			{
				std::set<tstring> reserved_names;
				// add strings
				{
					vector<TCHAR> text;
					const size_t text_alloc(1024); // 1 KB
					text.resize(text_alloc);
					// add "Geometry of War" reserved names
					{
						if (!UncompressResource(IDR_WORLDS_LIST, ri_cast<BYTE*>(&text[0]), text_alloc))
						{
							MacroDisplayError(_T("Resource could not be loaded."));
							return false;
						}
						tistringstream text_stream;
						text_stream.str(&text[0]);
						std::copy(
							std::istream_iterator<tstring>(text_stream),
							std::istream_iterator<tstring>(),
							std::inserter(reserved_names, reserved_names.begin()));
					}
					// add "Emperor's Statement" reserved names
					{
						std::fill(text.begin(), text.end(), _T('\0'));
						if (!UncompressResource(IDR_WORLDS_LIST_2, ri_cast<BYTE*>(&text[0]), text_alloc))
						{
							MacroDisplayError(_T("Resource could not be loaded."));
							return false;
						}
						tistringstream text_stream;
						text_stream.str(&text[0]);
						std::copy(
							std::istream_iterator<tstring>(text_stream),
							std::istream_iterator<tstring>(),
							std::inserter(reserved_names, reserved_names.begin()));
					}
				}
				// make sure the current name does not match a reserved string
				if (reserved_names.find(name) != reserved_names.end())
				{
					MessageBox(hwnd_, _T("This map name is reserved."), NULL, MB_OK);
					return false;
				}
			}
		}
		// save the validated map name
		map_name_ = name;
	}
	return true;
}