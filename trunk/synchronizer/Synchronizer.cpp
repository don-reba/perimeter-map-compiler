#include "resource.h"
#include <algorithm>
#include <crtdbg.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <shlwapi.h>
#include <tchar.h>
#include <windows.h>
#include <Wininet.h>
using namespace std;

typedef basic_string<TCHAR> tstring;

#define ri_cast reinterpret_cast

const int default_map_count(38);

//---------------------------
// application implementation
//---------------------------
UINT Error(TCHAR *message, TCHAR *title)
{
	_RPT0(_CRT_ERROR, message);
	return MessageBox(NULL, message, title, MB_ABORTRETRYIGNORE | MB_ICONERROR);
}

UINT GetPRM(tstring &worlds_prm)
{
	HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_WORLD_LIST), "TEXT"));
	HGLOBAL resource(LoadResource(NULL, resource_info));
	char *text(ri_cast<char*>(LockResource(resource)));
	worlds_prm = text;
	return IDOK;
}

UINT GetMapList(tstring &map_list)
{
	TCHAR *error_title(_T("Map Synchronization Error"));
	// check connection status
	if (FALSE == InternetCheckConnection(
		_T("http://www.rul-clan.ru/map_registration/list_maps.php"),
		FLAG_ICC_FORCE_CONNECTION,
		0))
		return IDIGNORE;
	// register map
	{
		// establish connection
		HINTERNET wi_handle(InternetOpen(
			_T("Perimeter Map Compiler"),
			INTERNET_OPEN_TYPE_PRECONFIG,
			NULL,
			NULL,
			0));
		if (NULL == wi_handle)
			return Error(_T("InternetOpen failed"), error_title);
		// open the list_maps.php
		HINTERNET url_handle;
		url_handle = InternetOpenUrl(
			wi_handle,
#ifdef PRE_RELEASE
			_T("http://www.rul-clan.ru/map_registration/pre_release/list_maps.php"),
#else
			_T("http://www.rul-clan.ru/map_registration/list_maps.php"),
#endif
			NULL,
			0L,
			0L,
			0L);
		if (NULL == url_handle)
		{
			InternetCloseHandle(wi_handle);
			return Error(_T("InternetOpenUrl failed"), error_title);
		}
		// get data
		{
			const size_t buffer_size(1024);
			TCHAR buffer[buffer_size];
			DWORD bytes_read;
			for (;;)
			{
				if (FALSE == InternetReadFile(
					url_handle,
					buffer,
					buffer_size,
					&bytes_read))
				{
					InternetCloseHandle(url_handle);
					InternetCloseHandle(wi_handle);
					return Error(_T("InternetReadFile failed"), error_title);
				}
				buffer[bytes_read] = _T('\0');
				if (0 != bytes_read)
					map_list += buffer;
				else
					break;
			}
		}
		// wrap up
		InternetCloseHandle(url_handle);
		InternetCloseHandle(wi_handle);
	}
	// process result
	if (map_list.c_str() == _T("falure"))
		return Error(_T("Map could not be registered."), error_title);
	return IDOK;
}

UINT SavePRM(const tstring &worlds_prm, const tstring &map_list)
{
	TCHAR *error_title(_T("Map Synchronization Error"));
	// open WORLDS.PRM
	ofstream worlds_prm_file(_T("RESOURCE\\Worlds\\WORLDS.PRM"));
	if (!worlds_prm_file.is_open())
		return Error(_T("Could not open WORLDS.PRM."), error_title);
	// count the number of maps in map_list
	// ASSUME: each map name is followed by a newline simbol
	int new_map_count(static_cast<int>(count(map_list.begin(), map_list.end(), _T('\n'))));
	// output new data
	worlds_prm_file << (128 + new_map_count) << endl << endl;
	worlds_prm_file << worlds_prm;
	for (int i(0); i != 128 - default_map_count; ++i)
		worlds_prm_file << setw(0) << "*" << setw(24) << "*" << endl;
	istringstream map_list_stream(map_list);
	tstring line;
	for (int i(0); i != new_map_count; ++i)
	{
		getline(map_list_stream, line);
		worlds_prm_file << setw(0) << line << setw(24) << line << endl;
	}
	return IDOK;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UINT result;
	// get WORLDS.PRM
	tstring worlds_prm;
	do
	{
		worlds_prm.clear();
		result = GetPRM(worlds_prm);
	} while (IDRETRY == result);
	if (IDABORT == result)
		return 0;
	// get the list of maps
	tstring map_list;
	if (IDIGNORE != result)
	{
		do
		{
			map_list.clear();
			result = GetMapList(map_list);
		} while (IDRETRY == result);
		if (IDABORT == result)
			return 0;
	}
	// save the new WORLDS.PRM
	if (IDIGNORE != result)
	{
		do
		{
			result = SavePRM(worlds_prm, map_list);
		} while (IDRETRY == result);
		if (IDABORT == result)
			return 0;
	}
	// run RealPerimeter.exe with the parameters passed to us
	PROCESS_INFORMATION process_info;
	ZeroMemory(&process_info, sizeof(process_info));
	do
	{
		STARTUPINFO startup_info;
		ZeroMemory(&startup_info, sizeof(startup_info));
		startup_info.cb = sizeof(startup_info);
		if (FALSE == CreateProcess(
			_T("RealPerimeter.exe"),
			GetCommandLine(),
			NULL,
			NULL,
			FALSE,
			0L,
			NULL,
			NULL,
			&startup_info,
			&process_info))
			result = Error(
				_T("Could not start Perimeter. Make sure it is named RealPerimeter.exe and located in its default directory."),
				_T("Startup Error"));
	} while (IDRETRY == result);
	if (IDABORT == result)
		return 0;
	// wait for Perimeter to exit
	WaitForSingleObject(process_info.hProcess, INFINITE);
	CloseHandle(process_info.hProcess);
	CloseHandle(process_info.hThread);
	return 0;
}

