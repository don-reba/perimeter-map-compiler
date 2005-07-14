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
	// get installation path
	if (!GetInstallPath(install_path_))
		return IDCANCEL;
	// run dialog
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
			tstring install_path_;
			if (!GetInstallPath(install_path_))
				return;
			// get the "RESOURCE\Worlds" path
			TCHAR path[MAX_PATH];
			PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Worlds\\*"));
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
	// set up the map list control
	{
		HWND map_list_ctrl(GetDlgItem(hwnd_, IDC_MAP_LIST));
		ListView_SetExtendedListViewStyleEx(map_list_ctrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
		enum Column
		{
			COL_NAME,
			COL_SIZE
		};
		// add columns
		{
			LVCOLUMN column;
			column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
			// map name
			column.pszText = _T("Name");
			column.cx      = 120;
			column.fmt     = LVCFMT_LEFT;
			ListView_InsertColumn(map_list_ctrl, COL_NAME, &column);
			// map size
			column.pszText = _T("Size");
			column.cx      = 50;
			column.fmt     = LVCFMT_RIGHT;
			ListView_InsertColumn(map_list_ctrl, COL_SIZE, &column);
		}
		// find out the maximum string length
		size_t max_string_size(0);
		foreach (tstring &map, map_list)
			max_string_size = __max(max_string_size, map.size());
		max_string_size = __max(32, max_string_size + 1);
		// feed in the list
		ListView_SetItemCount(map_list_ctrl, map_list.size());
		vector<TCHAR> item_text_vector(max_string_size);
		LVITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = 0;
		item.pszText = &item_text_vector[0];
		foreach (tstring &map, map_list)
		{
			// map name
			item.iSubItem = COL_NAME;
			_tcscpy(item.pszText, map.c_str());
			ListView_InsertItem(map_list_ctrl, &item);
			// map size
			item.iSubItem = COL_SIZE;
			_tcscpy(item.pszText, GetMapFilesSize(map.c_str()).c_str());
			ListView_SetItem(map_list_ctrl, &item);
			++item.iItem;
		}
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
	vector<TCHAR> path_vector(MAX_PATH); // buffer for path manipulations
	TCHAR *path(&path_vector[0]);
	// get the index of the map to delete
	const HWND ctrl(GetDlgItem(hwnd_, IDC_MAP_LIST));
	const int sel_index(ListView_GetSelectionMark(ctrl));
	if (0 > sel_index)
		return;
	// get the name
	tstring name;
	ListView_GetItemText(ctrl, sel_index, 0, path, MAX_PATH);
	name = path;
	// create a list of files to delete
	vector<TCHAR> files_list_vector;
	TCHAR *files_list(&files_list_vector[0]);
	{
		vector<tstring> files;
		GetFilesList(name.c_str(), files, false);
		// calculate total size
		size_t files_length(1); // one for the extra terminating zero
		foreach (tstring &file, files)
			files_length += file.size() + 1; // one for the terminating zero
		// combine paths
		files_list_vector.resize(files_length);
		files_list = &files_list_vector[0];
		TCHAR *files_i(files_list);
		foreach (tstring &file, files)
		{
			_RPT1(_CRT_WARN, "• %s\n", file.c_str());
			const size_t string_length(file.size() + 1);
			CopyMemory(files_i, file.c_str(), string_length * sizeof(TCHAR));
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
		MessageBox(hwnd_, _T("Some files could not be deleted."), _T("Error"), MB_OK | MB_ICONSTOP);
		return;
	}
	// remove the map from the dialog list
	ListView_DeleteItem(ctrl, sel_index);
	// remove the Texts.btdb entry
	{
		Btdb btdb(PathCombine(path, install_path_.c_str(), _T("RESOURCE\\LocData\\Russian\\Text\\Texts.btdb")));
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

void MapManager::FilesInDir(LPCTSTR dir, vector<tstring> &files, size_t depth)
{
	if (depth > 32)
		return;
	vector<TCHAR> path_vector(MAX_PATH);
	TCHAR * const path(&path_vector[0]);
	// create a wildcard for the seatch
	_tcscpy(path, dir);
	PathAddBackslash(path);
	PathCombine(path, path, _T("*"));
	// search
	WIN32_FIND_DATA find_data;
	HANDLE find_handle(FindFirstFile(path, &find_data));
	if (INVALID_HANDLE_VALUE != find_handle)
	{
		do
		{
			if (
				0 == _tcscmp(find_data.cFileName, _T(".")) ||
				0 == _tcscmp(find_data.cFileName, _T("..")))
				continue;
			PathCombine(path, dir, find_data.cFileName);
			if (FILE_ATTRIBUTE_DIRECTORY & find_data.dwFileAttributes)
				FilesInDir(path, files, depth + 1);
			files.push_back(path);
		} while (0 != FindNextFile(find_handle, &find_data));
		FindClose(find_handle);
	}
}

void MapManager::GetFilesList(LPCTSTR map_name, vector<tstring> &files, bool recurse)
{
	vector<TCHAR> path_vector(MAX_PATH);
	TCHAR *path(&path_vector[0]);
	// define a functor for adding the files
	struct FuntorAddExisting {
		FuntorAddExisting(vector<tstring> &files, bool recurse) : files_(files), recurse_(recurse) {}
		void operator() (LPCTSTR path) {
			DWORD attributes(GetFileAttributes(path));
			if (INVALID_FILE_ATTRIBUTES != attributes)
			{
				if (recurse_ && 0 != (FILE_ATTRIBUTE_DIRECTORY & attributes))
					FilesInDir(path, files_);
				files_.push_back(path);
			}
		}
		vector<tstring> &files_;
		bool             recurse_;
	} AddExisting(files, recurse);
	// map folder
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Worlds"));
	PathCombine(path, path, map_name);
	AddExisting(path);
	// battle
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".dat"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".gmp"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".spg"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".sph"));
	AddExisting(path);
	// survival
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".dat"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".gmp"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".spg"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".sph"));
	AddExisting(path);
	// multiplayer
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".dat"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".gmp"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".spg"));
	AddExisting(path);
	PathCombine(path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".sph"));
	AddExisting(path);
}

bool MapManager::GetInstallPath(string &install_path_)
{
	TCHAR path[MAX_PATH];
	DWORD attributes(GetFileAttributes(MacroAppData(ID_PERIMETER_PATH).c_str()));
	if (INVALID_FILE_ATTRIBUTES == attributes)
	{
		return TaskCommon::GetInstallPath(install_path_, *this);
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
			install_path_ = path;
		}
		else
			install_path_ = MacroAppData(ID_PERIMETER_PATH);
	}
	return true;
}

tstring MapManager::GetMapFilesSize(LPCTSTR map_name)
{
	DWORD total_size(0);
	// get the total size, using GetFileAttributesEx if GetCompressedFileSize is unavailable
	{
		typedef DWORD (WINAPI *FileSizeType)(LPCTSTR, LPDWORD);
#ifdef UNICODE
		FileSizeType file_size_proc(ri_cast<FileSizeType>(GetProcAddress(
			GetModuleHandle("Kernel32.dll"),
			_T("GetCompressedFileSizeW"))));
#else
		FileSizeType file_size_proc(ri_cast<FileSizeType>(GetProcAddress(
			GetModuleHandle("Kernel32.dll"),
			_T("GetCompressedFileSizeA"))));
#endif
		vector<tstring> files;
		GetFilesList(map_name, files);
		foreach (tstring file, files)
		{
			DWORD size;
			if (NULL != file_size_proc)
			{
				size = (*file_size_proc)(file.c_str(), NULL);
				if (INVALID_FILE_SIZE == size)
					continue;
			}
			else
			{
				WIN32_FILE_ATTRIBUTE_DATA data;
				if (FALSE == GetFileAttributesEx(file.c_str(), GetFileExInfoStandard, &data))
					continue;
				size = data.nFileSizeLow;
			}
			total_size += size;
		}
	}
	// create a string
	TCHAR size_str[32];
	if (total_size < 0x400)
	{
		_itot(total_size, size_str, 10);
		_tcscpy(size_str + _tcslen(size_str), _T(" B"));
	}
	else if (total_size < 0x100000)
	{
		_itot(total_size / 0x400, size_str, 10);
		_tcscpy(size_str + _tcslen(size_str), _T(" KB"));
	}
	else if (total_size < 0x400000000)
	{
		_itot(total_size / 0x100000, size_str, 10);
		_tcscpy(size_str + _tcslen(size_str), _T(" MB"));
	}
	else
	{
		_itot(static_cast<int>(total_size / 0x400000000), size_str, 10);
		_tcscpy(size_str + _tcslen(size_str), _T(" GB"));
	}
	return size_str;
}
