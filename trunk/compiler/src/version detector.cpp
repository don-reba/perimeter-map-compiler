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

#include "app data.h"
#include "resource.h"
#include "version detector.h"

#include <commctrl.h>
#include <shellapi.h>

using namespace std;

//-------------------------------
// VersionDetector implementation
//-------------------------------

bool VersionDetector::ShowSelectDialog(HWND parent_wnd)
{
	// check if the required data have already been stored
	if (
		MacroAppData(ID_PERIMETER_VERSION) == 1 ||
		MacroAppData(ID_PERIMETER_VERSION) == 2)
	{
		DWORD attributes(GetFileAttributes(MacroAppData(ID_PERIMETER_PATH).c_str()));
		if (INVALID_FILE_ATTRIBUTES != attributes)
		{
			version_info_.description_ = "Unknown";
			if (MacroAppData(ID_PERIMETER_PATH).size() >= MAX_PATH)
				MacroAppData(ID_PERIMETER_PATH).resize(MAX_PATH);
			if (0 == (FILE_ATTRIBUTE_DIRECTORY | attributes))
			{
				vector<TCHAR> path_v(MAX_PATH);
				TCHAR *path(&path_v[0]);
				_tcscpy(path, MacroAppData(ID_PERIMETER_PATH).c_str());
				PathRemoveFileSpec(path);
				MacroAppData(ID_PERIMETER_PATH) = path;
			}
			version_info_.path_    = MacroAppData(ID_PERIMETER_PATH);
			version_info_.version_ = MacroAppData(ID_PERIMETER_VERSION);
			return true;
		}
	}
	// if not, show dialog
	vector<VersionInfo> entries;
	FindVersions(entries);
	if (entries.size() == 0)
		return false;
	if (entries.size() == 1)
	{
		version_info_ = entries[0];
		return true;
	}
	Chooser chooser(entries);
	if (IDOK != chooser.DoModal(parent_wnd))
		return false;
	version_info_ = entries[chooser.GetChoice()];
	if (chooser.MustRemember())
	{
		MacroAppData(ID_PERIMETER_PATH)    = version_info_.path_;
		MacroAppData(ID_PERIMETER_VERSION) = version_info_.version_;
	}
	return true;
}

tstring VersionDetector::GetPath() const
{
	return version_info_.path_;
}

uint VersionDetector::GetVersion() const
{
	return version_info_.version_;
}

void VersionDetector::FindVersions(std::vector<VersionInfo> &entries)
{
	VersionInfo entry;
	if (TestPrimeter101(entry))
		entries.push_back(entry);
	if (TestPrimeter102(entry))
		entries.push_back(entry);
	if (TestPrimeterET(entry))
		entries.push_back(entry);
}

bool VersionDetector::TestPrimeter101(VersionDetector::VersionInfo &entry)
{
		// check the registry for necessary information
		HKEY perimeter_key;
		// open Perimeter's registry key
		if (ERROR_SUCCESS != RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			_T("SOFTWARE\\Codemasters\\Perimeter"),
			0,
			KEY_READ,
			&perimeter_key))
		{
			return false;
		}
		// verify Perimeter version
		{
			DWORD version;
			DWORD version_length;
			DWORD version_type;
			bool russian(true);
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Version"),
				NULL,
				&version_type,
				NULL,
				&version_length))
			{
				russian = false;
			}
			if (russian)
			{
				if (version_length != 4 || version_type != REG_DWORD)
				{
					RegCloseKey(perimeter_key);
					return false;
				}
				if (ERROR_SUCCESS != RegQueryValueEx(
					perimeter_key,
					_T("Version"),
					NULL,
					NULL,
					ri_cast<BYTE*>(&version),
					&version_length))
				{
					RegCloseKey(perimeter_key);
					return false;
				}
				if (version != 101)
				{
					RegCloseKey(perimeter_key);
					return false;
				}
			}
			else
			{
				HKEY patch_key;
				if (ERROR_SUCCESS != RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					_T("SOFTWARE\\Codemasters\\Perimeter Patch 1.01"),
					0,
					KEY_READ,
					&patch_key))
				{
					RegCloseKey(perimeter_key);
					return false;
				}
				RegCloseKey(patch_key);
			}
		}
		// get path to the installation folder
		tstring install_path;
		{
			DWORD install_path_type;
			DWORD install_path_length;
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("INSTALL_PATH"),
				NULL,
				&install_path_type,
				NULL,
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			if (REG_SZ != install_path_type)
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			vector<TCHAR> install_path_temp_v(MAX_PATH);
			TCHAR *install_path_temp(&install_path_temp_v[0]);
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Install_Path"),
				NULL,
				NULL,
				ri_cast<BYTE*>(install_path_temp),
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			install_path = install_path_temp;
		}
		RegCloseKey(perimeter_key);
		// fill in the entry
		entry.description_ = "Geometry of War 1.01";
		entry.path_        = install_path;
		entry.version_     = 1;
		return true;
}

bool VersionDetector::TestPrimeter102(VersionDetector::VersionInfo &entry)
{
		// check the registry for necessary information
		HKEY perimeter_key;
		// open Perimeter's registry key
		if (ERROR_SUCCESS != RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			_T("SOFTWARE\\Codemasters\\Perimeter"),
			0,
			KEY_READ,
			&perimeter_key))
		{
			return false;
		}
		// verify Perimeter version
		{
			DWORD version;
			DWORD version_length;
			DWORD version_type;
			bool russian(true);
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Version"),
				NULL,
				&version_type,
				NULL,
				&version_length))
			{
				russian = false;
			}
			if (russian)
			{
				if (version_length != 4 || version_type != REG_DWORD)
				{
					RegCloseKey(perimeter_key);
					return false;
				}
				if (ERROR_SUCCESS != RegQueryValueEx(
					perimeter_key,
					_T("Version"),
					NULL,
					NULL,
					ri_cast<BYTE*>(&version),
					&version_length))
				{
					RegCloseKey(perimeter_key);
					return false;
				}
				if (version != 102)
				{
					RegCloseKey(perimeter_key);
					return false;
				}
			}
			else
			{
				HKEY patch_key;
				if (ERROR_SUCCESS != RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					_T("SOFTWARE\\Codemasters\\Perimeter Patch 1.02"),
					0,
					KEY_READ,
					&patch_key))
				{
					RegCloseKey(perimeter_key);
					return false;
				}
				RegCloseKey(patch_key);
			}
		}
		// get path to the installation folder
		tstring install_path;
		{
			DWORD install_path_type;
			DWORD install_path_length;
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("INSTALL_PATH"),
				NULL,
				&install_path_type,
				NULL,
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			if (REG_SZ != install_path_type)
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			vector<TCHAR> install_path_temp_v(MAX_PATH);
			TCHAR *install_path_temp(&install_path_temp_v[0]);
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Install_Path"),
				NULL,
				NULL,
				ri_cast<BYTE*>(install_path_temp),
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			install_path = install_path_temp;
		}
		RegCloseKey(perimeter_key);
		// fill in the entry
		entry.description_ = "Geometry of War 1.02";
		entry.path_        = install_path;
		entry.version_     = 2;
		return true;
}

bool VersionDetector::TestPrimeterET(VersionDetector::VersionInfo &entry)
{
		// check the registry for necessary information
		HKEY perimeter_key;
		// open Perimeter's registry key
		if (ERROR_SUCCESS != RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			_T("Software\\1C\\PerimeterET"),
			0,
			KEY_READ,
			&perimeter_key))
			return false;
		// detect version by absence of the key "Version" :/
		if (ERROR_SUCCESS == RegQueryValueEx(perimeter_key, _T("Version"), NULL, NULL, NULL, NULL))
				return false;
		// get path to the installation folder
		tstring install_path;
		{
			DWORD install_path_type;
			DWORD install_path_length;
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Install_Path"),
				NULL,
				&install_path_type,
				NULL,
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			if (REG_SZ != install_path_type)
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			vector<TCHAR> install_path_temp_v(MAX_PATH);
			TCHAR *install_path_temp(&install_path_temp_v[0]);
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Install_Path"),
				NULL,
				NULL,
				ri_cast<BYTE*>(install_path_temp),
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				return false;
			}
			install_path = install_path_temp;
		}
		RegCloseKey(perimeter_key);
		// fill in the entry
		entry.description_ = "Emperor's Testament 2.00";
		entry.path_        = install_path;
		entry.version_     = 2;
	return true;
}

//----------------------------------------
// VersionDetector::Chooser implementation
//----------------------------------------

VersionDetector::Chooser::Chooser(const VersionDetector::Chooser::entries_t &entries)
	:entries_(entries)
{}

VersionDetector::Chooser::DoModal(HWND parent_wnd)
{
	// run dialog
	return DialogBoxParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_VERSION_CHOOSER),
		parent_wnd,
		DlgProc<Chooser>,
		ri_cast<LPARAM>(this));
}

uint VersionDetector::Chooser::GetChoice() const
{
	return choice_;
}

bool VersionDetector::Chooser::MustRemember() const
{
	return must_remember_;
}

void VersionDetector::Chooser::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDOK:
		ExchangeData();
	case IDCANCEL:
		EndDialog(hwnd_, msg.CtrlId());
		break;
	}
}

void VersionDetector::Chooser::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	// set up the version list
	{
		HWND choice_list_ctrl(GetDlgItem(hwnd_, IDC_CHOICE_LIST));
		ListView_SetExtendedListViewStyleEx(choice_list_ctrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
		enum Column
		{
			COL_DESCRIPTION,
			COL_PATH
		};
		// add columns
		{
			LVCOLUMN column;
			column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
			// map name
			column.pszText = _T("Version");
			column.cx      = 140;
			column.fmt     = LVCFMT_LEFT;
			ListView_InsertColumn(choice_list_ctrl, COL_DESCRIPTION, &column);
			// map size
			column.pszText = _T("Path");
			column.cx      = 210;
			column.fmt     = LVCFMT_LEFT;
			ListView_InsertColumn(choice_list_ctrl, COL_PATH, &column);
		}
		// find out the size to use for the pszText buffer
		size_t buffer_size(0);
		foreach (const VersionInfo &entry, entries_)
			buffer_size = __max(buffer_size, __max(entry.description_.size(), entry.path_.size()));
		++buffer_size;
		// feed in the list
		ListView_SetItemCount(choice_list_ctrl, entries_.size());
		vector<TCHAR> text_buffer(buffer_size);
		LVITEM item = { 0 };
		item.mask = LVIF_TEXT;
		item.iItem = 0;
		item.pszText = &text_buffer[0];
		foreach (const VersionInfo &entry, entries_)
		{
			// map name
			item.iSubItem = COL_DESCRIPTION;
			_tcscpy(item.pszText, entry.description_.c_str());
			ListView_InsertItem(choice_list_ctrl, &item);
			// map size
			item.iSubItem = COL_PATH;
			_tcscpy(item.pszText, entry.path_.c_str());
			ListView_SetItem(choice_list_ctrl, &item);
			++item.iItem;
		}
		ListView_SetItemState(
			choice_list_ctrl,
			0,
			LVIS_FOCUSED | LVIS_SELECTED,
			LVIS_FOCUSED | LVIS_SELECTED);
	}
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void VersionDetector::Chooser::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&Chooser::OnCommand,
		&Chooser::OnInitDialog
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

void VersionDetector::Chooser::ExchangeData()
{
	// get the index of the map to delete
	const HWND ctrl(GetDlgItem(hwnd_, IDC_CHOICE_LIST));
	int sel_index(ListView_GetSelectionMark(ctrl));
	if (sel_index < 0)
		sel_index = 0;
	choice_ = static_cast<uint>(sel_index);
	must_remember_ = BST_CHECKED == IsDlgButtonChecked(hwnd_, IDC_REMEMBER_CHOICE);
}
