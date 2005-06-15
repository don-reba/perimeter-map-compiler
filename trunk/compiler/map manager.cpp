#include "StdAfx.h"














/*
#include "btdb.h"
#include "map manager.h"
#include "resource.h"
#include <algorithm>
#include <commctrl.h>
#include <fstream>
#include <iterator>
#include <set>
#include <shellapi.h>
#include <sstream>
using namespace std;

//----------------------------
// CInfoManager implementation
//----------------------------

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
}

void CMapManager::Create(HWND hWndParent)
{
	// create the window
	hWnd = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_MAP_MANAGER),
		hWndParent,
		DialogProc,
		ri_cast<LPARAM>(this));
}

INT_PTR CALLBACK CMapManager::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// prevent processing before the dialog is fully initialized
	static bool initialized(false);
	if (WM_INITDIALOG == uMsg)
		initialized = true;
	if (false == initialized)
		return FALSE;
	// get the object pointer
	CMapManager *obj;
	if (WM_INITDIALOG == uMsg)
		obj = ri_cast<CMapManager*>(lParam);
	else
		obj = ri_cast<CMapManager*>(GetWindowLong(hWnd, DWL_USER));
	// process messages
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_COMMAND, obj->OnCommand);
		HANDLE_MSG(hWnd, WM_DESTROY, obj->OnDestroy);
		HANDLE_MSG(hWnd, WM_INITDIALOG, obj->OnInitDialog);
	}
	return FALSE;
}


BOOL CMapManager::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
	case IDCANCEL:
		DestroyWindow(hWnd);
		break;
	case IDC_DELETE_MAP:
		{
			TCHAR path[MAX_PATH]; // placeholder for path manipulations
			// get the index of the map to delete
			TCHAR *name(NULL);
			const HWND ctrl(GetDlgItem(hWnd, IDC_MAP_LIST));
			const int sel_index(ListBox_GetCurSel(ctrl));
			if (LB_ERR == sel_index)
				break;
			name = new TCHAR[ListBox_GetTextLen(ctrl, sel_index) + 1];
			ListBox_GetText(ctrl, sel_index, name);
			// get installation path
			string install_path;
			if (!GetInstallPath(install_path))
			{
				delete [] name;
				break;
			}
			// create a list of files to delete
			TCHAR *files_list;
			{
				vector<string> files;
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
			fos.hwnd   = hWnd;
			fos.wFunc  = FO_DELETE;
			fos.pFrom  = files_list;
			fos.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI;
			if (0 != SHFileOperation(&fos) || TRUE == fos.fAnyOperationsAborted)
			{
				delete [] files_list;
				delete [] name;
				break;
			}
			delete [] files_list;
			// remove the map from the dialog list
			ListBox_DeleteString(ctrl, sel_index);
			// remove the Texts.btdb entry
			{
				CBTDB btdb(PathCombine(path, install_path.c_str(), _T("RESOURCE\\LocData\\Russian\\Text\\Texts.btdb")));
				btdb.RemoveMapEntry(name);
			}
			// clean up
			delete [] name;
		}
		break;
	}
	return FALSE;
}

BOOL CMapManager::OnDestroy(HWND hWnd)
{
	DestroyWindow(hWnd);
	this->hWnd = NULL;
	return TRUE;
}

BOOL CMapManager::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	// save pointer to this class in window user data
	SetWindowLong(hWnd, DWL_USER, lParam);
	// get a list of maps installed by set difference of folder names in "RESOURCE\Worlds"
	//  and the list of reserved map names
	set<string> map_list;
	{
		// get the list of folders in "RESOURCE\Worlds"
		set<string> prm_list;
		{
			// get the installation path
			string install_path;
			if (!GetInstallPath(install_path))
				return FALSE;
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
					if (FILE_ATTRIBUTE_DIRECTORY == find_data.dwFileAttributes)
						prm_list.insert(find_data.cFileName);
					if (FALSE == FindNextFile(find_handle, &find_data))
						break;
				}
				FindClose(find_handle);
			}
		}
		// get the list of reserved map names
		set<string> reserved_list;
		{
			istringstream reserved_stream;
			// load the list of reserved names
			{
				HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_WORLDS_LIST), "Text"));
				HGLOBAL resource(LoadResource(NULL, resource_info));
				char *text(ri_cast<char*>(LockResource(resource)));
				reserved_stream.str(text);
			}
			// read the names
			string world;
			while (reserved_stream)
			{
				reserved_stream >> world;
				if (!world.empty())
					reserved_list.insert(world);
			}
			// add some extra names
			reserved_list.insert(_T("."));
			reserved_list.insert(_T(".."));
		}
		// get the set difference
		set_difference(
			prm_list.begin(),
			prm_list.end(),
			reserved_list.begin(),
			reserved_list.end(),
			insert_iterator<set<string> >(map_list, map_list.begin()));
	}
	// feed the list into the IDC_MAP_LIST list box
	{
		HWND map_list_ctrl(GetDlgItem(hWnd, IDC_MAP_LIST));
		      set<string>::const_iterator i  (map_list.begin());
		const set<string>::const_iterator end(map_list.end());
		for (; i != end; ++i)
			ListBox_AddString(map_list_ctrl, i->c_str());
	}
	return TRUE;
}

// generates an assertion in debug mode and a warning in release mode
void CMapManager::Error(string message) const
{
	string new_message = _T("CMapManager: \n");
	new_message.append(message);
	_RPT0(_CRT_ERROR, new_message.c_str());
	MessageBox(hWnd, new_message.c_str(), NULL, MB_OK | MB_ICONERROR);
}

// get Perimeter installation path from the registry
bool CMapManager::GetInstallPath(string &install_path) const
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
		MessageBox(
			hWnd,
			_T("Please make sure you have Perimeter installed."),
			_T("Installation Error"),
			MB_OK);
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
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		if (version_length != 4 || version_type != REG_DWORD)
		{
			RegCloseKey(perimeter_key);
			Error(_T("Wrong Version type."));
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
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		if (version != 101)
		{
			RegCloseKey(perimeter_key);
			MessageBox(
				hWnd,
				_T("Incompatible Perimeter version. Please make sure you have version 1.01."),
				_T("Installation Error"),
				MB_OK);
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
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		if (REG_SZ != install_path_type)
		{
			RegCloseKey(perimeter_key);
			Error(_T("Wrong Install_Path type."));
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
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		install_path = install_path_temp;
	}
	RegCloseKey(perimeter_key);
	return true;
}
*/