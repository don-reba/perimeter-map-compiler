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

#include "btdb.h"
#include "map manager.h"
#include "resource.h"
#include "resource management.h"

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

void MapManager::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
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

void MapManager::OnDeleteMap(Msg<WM_COMMAND> &msg)
{
	TCHAR path[MAX_PATH]; // buffer for path manipulations
	// get the index of the map to delete
	TCHAR *name(NULL);
	const HWND ctrl(GetDlgItem(hwnd_, IDC_MAP_LIST));
	const int sel_index(ListBox_GetCurSel(ctrl));
	if (LB_ERR == sel_index)
		return;
	name = new TCHAR[ListBox_GetTextLen(ctrl, sel_index) + 1];
	ListBox_GetText(ctrl, sel_index, name);
	// get installation path
	tstring install_path;
	if (!GetInstallPath(install_path))
	{
		delete [] name;
		return;
	}
	// create a list of files to delete
	TCHAR *files_list;
	{
		vector<tstring> files;
		// define paths
		// map folder
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Worlds"));
		PathCombine(path, path, name);
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		// battle
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".dat"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".gmp"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".spg"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".sph"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		// survival
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".dat"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".gmp"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".spg"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".sph"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		// multiplayer
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".dat"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".gmp"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name);
		PathAddExtension(path, _T(".spg"));
		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(path))
			files.push_back(path);
		PathCombine(path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(path, path, name);
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
			CopyMemory(files_i, files[i].c_str(), (files[i].size() + 1) * sizeof(TCHAR));
			files_i += files[i].size() + 1;
		}
		*files_i = _T('\0');
	}
	// delete the files
	SHFILEOPSTRUCT fos;
	ZeroMemory(&fos, sizeof(fos));
	fos.hwnd   = hwnd_;
	fos.wFunc  = FO_DELETE;
	fos.pFrom  = files_list;
	fos.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI;
	if (0 != SHFileOperation(&fos) || TRUE == fos.fAnyOperationsAborted)
	{
		delete [] files_list;
		delete [] name;
		return;
	}
	delete [] files_list;
	// remove the map from the dialog list
	ListBox_DeleteString(ctrl, sel_index);
	// remove the Texts.btdb entry
	{
		Btdb btdb(PathCombine(path, install_path.c_str(), _T("RESOURCE\\LocData\\Russian\\Text\\Texts.btdb")));
		btdb.RemoveMapEntry(name);
	}
	// clean up
	delete [] name;
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void MapManager::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&MapManager::OnCommand,
		&MapManager::OnInitDialog
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

bool MapManager::GetInstallPath(tstring &install_path)
{
	// check the registry for necessary information
	HKEY perimeter_key;
	// open perimeter's registry key
	if (ERROR_SUCCESS != RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Codemasters\\Perimeter"),
		0,
		KEY_READ,
		&perimeter_key))
	{
		MacroDisplayError(_T("Please make sure you have Perimeter installed."));
		return false;
	}
	// verify Perimeter version
	{
		DWORD version;
		DWORD version_length;
		DWORD version_type;
		if (ERROR_SUCCESS != RegQueryValueEx(
			perimeter_key,
			_T("Version"),
			NULL,
			&version_type,
			NULL,
			&version_length))
		{
			RegCloseKey(perimeter_key);
			MacroDisplayError(_T("RegQueryValueEx failed"));
			return false;
		}
		if (version_length != 4 || version_type != REG_DWORD)
		{
			RegCloseKey(perimeter_key);
			MacroDisplayError(_T("Wrong Version type."));
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
			MacroDisplayError(_T("RegQueryValueEx failed"));
			return false;
		}
		if (version != 101)
		{
			RegCloseKey(perimeter_key);
			MacroDisplayError(_T("Incompatible Perimeter version. Please make sure you have version 1.01."));
			return false;
		}
	}
	// get path to Perimeter's installation folder
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
			MacroDisplayError(_T("RegQueryValueEx failed"));
			return false;
		}
		if (REG_SZ != install_path_type)
		{
			RegCloseKey(perimeter_key);
			MacroDisplayError(_T("Wrong Install_Path type."));
			return false;
		}
		TCHAR install_path_temp[MAX_PATH];
		if (ERROR_SUCCESS != RegQueryValueEx(
			perimeter_key,
			_T("Install_Path"),
			NULL,
			NULL,
			ri_cast<BYTE*>(install_path_temp),
			&install_path_length))
		{
			RegCloseKey(perimeter_key);
			MacroDisplayError(_T("RegQueryValueEx failed"));
			return false;
		}
		install_path = install_path_temp;
	}
	RegCloseKey(perimeter_key);
	return true;
}
