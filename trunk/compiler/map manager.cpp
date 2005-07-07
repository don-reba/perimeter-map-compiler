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
#include "btdb.h"
#include "map manager.h"
#include "resource.h"
#include "resource management.h"
#include "task common.h"

#include <algorithm>
#include <commctrl.h>
#include <fstream>
#include <iterator>
#include <set>
#include <shellapi.h>
#include <sstream>

using namespace RsrcMgmt;

INT_PTR MapManager::DoModal(HWND parent_wnd)
{
	return DialogBoxParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_MAP_MANAGER),
		parent_wnd,
		DlgProc<MapManager>,
		ri_cast<LPARAM>(this));
}

void MapManager::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDOK:
	case IDCANCEL:
		EndDialog(hwnd_, 0);
		break;
	case IDC_DELETE_MAP:
		OnDeleteMap(msg);
	}
}

void MapManager::OnGetMinMaxInfo(Msg<WM_GETMINMAXINFO> &msg)
{
	msg.MinMaxInfo()->ptMinTrackSize.x = min_size_.cx;
	msg.MinMaxInfo()->ptMinTrackSize.y = min_size_.cy;
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void MapManager::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	// gather layout data
	{
		RECT  rect;
		SIZE  client;
		// minimum (current) size
		GetWindowRect(hwnd_, &rect);
		min_size_.cx = rect.right  - rect.left;
		min_size_.cy = rect.bottom - rect.top;
		// client area size (minimum size)
		GetClientRect(hwnd_, &rect);
		client.cx = rect.right  - rect.left;
		client.cy = rect.bottom - rect.top;
		// map list
		GetWindowRect(GetDlgItem(hwnd_, IDC_MAP_LIST), &rect);
		ScreenToClient(hwnd_, &rect);
		map_list_border_.left   = rect.left;
		map_list_border_.top    = rect.top;
		map_list_border_.right  = rect.right  - rect.left - client.cx;
		map_list_border_.bottom = rect.bottom - rect.top  - client.cy;
		// "delete" button
		GetWindowRect(GetDlgItem(hwnd_, IDC_DELETE_MAP), &rect);
		ScreenToClient(hwnd_, &rect);
		delete_btn_offset_ = rect.top - client.cy;
	}
	// get a list of maps installed by set difference of folder names in "RESOURCE\Worlds"
	//  and the list of reserved map names
	std::set<tstring> map_list;
	{
		// get the list of folders in "RESOURCE\Worlds"
		std::set<tstring> prm_list;
		{
			// get the installation path
			tstring install_path;
			if (!GetInstallPath(install_path))
				return;
			// get the "RESOURCE\Worlds" path
			TCHAR path[MAX_PATH];
			PathCombine(path, install_path.c_str(), _T("RESOURCE\\Worlds\\*"));
			// get the folder names
			{
				HANDLE find_handle;
				WIN32_FIND_DATA find_data;
				find_handle = FindFirstFile(path, &find_data);
				while (INVALID_HANDLE_VALUE != find_handle)
				{
					if (0 != (FILE_ATTRIBUTE_DIRECTORY & find_data.dwFileAttributes))
						prm_list.insert(find_data.cFileName);
					if (FALSE == FindNextFile(find_handle, &find_data))
						break;
				}
				FindClose(find_handle);
			}
		}
		// get the list of reserved map names
		std::set<tstring> reserved_list;
		{
			// add strings from resources
			{
				vector<TCHAR> text;
				const size_t text_alloc(1024); // 1 KB
				text.resize(text_alloc);
				// add "Geometry of War" reserved names
				{
					if (!UncompressResource(IDR_WORLDS_LIST, ri_cast<BYTE*>(&text[0]), text_alloc))
					{
						MacroDisplayError(_T("Resource could not be loaded."));
						return;
					}
					tistringstream text_stream;
					text_stream.str(&text[0]);
					std::copy(
						std::istream_iterator<tstring>(text_stream),
						std::istream_iterator<tstring>(),
						std::inserter(reserved_list, reserved_list.begin()));
				}
				// add "Emperor's Statement" reserved names
				{
					std::fill(text.begin(), text.end(), _T('\0'));
					if (!UncompressResource(IDR_WORLDS_LIST_2, ri_cast<BYTE*>(&text[0]), text_alloc))
					{
						MacroDisplayError(_T("Resource could not be loaded."));
						return;
					}
					tistringstream text_stream;
					text_stream.str(&text[0]);
					std::copy(
						std::istream_iterator<tstring>(text_stream),
						std::istream_iterator<tstring>(),
						std::inserter(reserved_list, reserved_list.begin()));
				}
			}
			// add some extra names
			reserved_list.insert(_T("."));
			reserved_list.insert(_T(".."));
		}
		// get the set difference
		std::set_difference(
			prm_list.begin(),
			prm_list.end(),
			reserved_list.begin(),
			reserved_list.end(),
			std::inserter(map_list, map_list.begin()));
	}
	// feed the list into the IDC_MAP_LIST list box
	{
		HWND map_list_ctrl(GetDlgItem(hwnd_, IDC_MAP_LIST));
		foreach (tstring &map, map_list)
			ListBox_AddString(map_list_ctrl, map.c_str());
	}
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void MapManager::OnSize(Msg<WM_SIZE> &msg)
{
	// stretch the map list
	MoveWindow(
		GetDlgItem(hwnd_, IDC_MAP_LIST),
		map_list_border_.left,
		map_list_border_.top,
		map_list_border_.right  + msg.Size().cx,
		map_list_border_.bottom + msg.Size().cy,
		TRUE);
	// horizontally center the delete button
	{
		RECT rect;
		HWND btn(GetDlgItem(hwnd_, IDC_DELETE_MAP));
		GetWindowRect(btn, &rect);
		ScreenToClient(hwnd_, &rect);
		MoveWindow(
			btn,
			(msg.Size().cx - (rect.right - rect.left)) / 2,
			delete_btn_offset_ + msg.Size().cy,
			rect.right  - rect.left,
			rect.bottom - rect.top,
			TRUE);
	}
	msg.handled_ = true;
	msg.result_  = FALSE;
}

void MapManager::OnDeleteMap(Msg<WM_COMMAND> &msg)
{
	TCHAR path[MAX_PATH]; // buffer for path manipulations
	// get the index of the map to delete
	const HWND ctrl(GetDlgItem(hwnd_, IDC_MAP_LIST));
	const int sel_index(ListBox_GetCurSel(ctrl));
	if (LB_ERR == sel_index)
		return;
	// get the name
	tstring name;
	{
		vector<TCHAR> name_temp;
		name_temp.resize(ListBox_GetTextLen(ctrl, sel_index) + 1);
		ListBox_GetText(ctrl, sel_index, &name[0]);
	}
	// get installation path
	tstring install_path;
	if (!GetInstallPath(install_path))
		return;
	// create a list of files to delete
	TCHAR *files_list;
	{
		vector<tstring> files;
		// define paths
		// map folder
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Worlds"));
		PathCombine(path, path, name.c_str());
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		// battle
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".dat"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".gmp"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".spg"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".sph"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		// survival
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".dat"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".gmp"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".spg"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".sph"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		// multiplayer
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".dat"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".gmp"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".spg"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name.c_str());
		PathAddExtension(path, _T(".sph"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		// calculate total size
		size_t files_length(1); // one for the extra terminating zero
		for (size_t i(0); i != files.size(); ++i)
			files_length += files[i].size() + 1; // one for the terminating zero
		// combine paths
		files_list = new TCHAR[files_length];
		TCHAR *files_i(files_list);
		for (size_t i(0); i != files.size(); ++i)
		{
			const size_t string_length(files[i].size() + 1);
			CopyMemory(files_i, files[i].c_str(), string_length * sizeof(TCHAR));
			files_i += string_length;
		}
		*files_i = _T('\0');
	}
	// delete the files
	SHFILEOPSTRUCT fos;
	ZeroMemory(&fos, sizeof(fos));
	fos.hwnd   = hwnd_;
	fos.wFunc  = FO_DELETE;
	fos.pFrom  = files_list;
	fos.fFlags = FOF_NOERRORUI;
	if (0 != SHFileOperation(&fos) || TRUE == fos.fAnyOperationsAborted)
	{
		delete [] files_list;
		return;
	}
	delete [] files_list;
	// remove the map from the dialog list
	ListBox_DeleteString(ctrl, sel_index);
	// remove the Texts.btdb entry
	{
		Btdb btdb(PathCombine(path, install_path.c_str(), _T("RESOURCE\\LocData\\Russian\\Text\\Texts.btdb")));
		btdb.RemoveMapEntry(name.c_str()); // WARN: not UNICODE safe
	}
	// wrap up
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void MapManager::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&MapManager::OnCommand,
		&MapManager::OnGetMinMaxInfo,
		&MapManager::OnInitDialog,
		&MapManager::OnSize
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

bool MapManager::GetInstallPath(string &install_path)
{
	TCHAR path[MAX_PATH];
	DWORD attributes(GetFileAttributes(MacroAppData(ID_PERIMETER_PATH).c_str()));
	if (INVALID_FILE_ATTRIBUTES == attributes)
	{
		return TaskCommon::GetInstallPath(install_path, *this);
	}
	else
	{
		if (MacroAppData(ID_PERIMETER_PATH).size() >= MAX_PATH)
		{
			MacroDisplayError(_T("The installation path is too long."));
			return false;
		}
		if (0 == (FILE_ATTRIBUTE_DIRECTORY | attributes))
		{
			_tcscpy(path, MacroAppData(ID_PERIMETER_PATH).c_str());
			PathRemoveFileSpec(path);
			install_path = path;
		}
		else
			install_path = MacroAppData(ID_PERIMETER_PATH);
	}
	return true;
}

void MapManager::ScreenToClient(HWND hwnd, RECT *rect)
{
	POINT corner;
	RECT &rect_ref(*rect);
	corner.x = rect_ref.left;
	corner.y = rect_ref.top;
	::ScreenToClient(hwnd, &corner);
	rect_ref.left = corner.x;
	rect_ref.top  = corner.y;
	corner.x = rect_ref.right;
	corner.y = rect_ref.bottom;
	::ScreenToClient(hwnd, &corner);
	rect_ref.right  = corner.x;
	rect_ref.bottom = corner.y;
}
