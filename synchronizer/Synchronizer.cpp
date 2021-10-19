#include "foreach.h"
#include "resource.h"
#include <algorithm>
#include <crtdbg.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <shlwapi.h>
#include <tchar.h>
#include <vector>
#include <windows.h>
#include <Wininet.h>
using namespace std;

typedef basic_string<TCHAR> tstring;

#define ri_cast reinterpret_cast

//---------------------------
// application implementation
//---------------------------
UINT Error(TCHAR *message, TCHAR *title)
{
	_RPT0(_CRT_ERROR, message);
	return MessageBox(NULL, message, title, MB_ABORTRETRYIGNORE | MB_ICONERROR);
}

UINT GetPRM(vector<tstring> &worlds_prm)
{
	// get the executable's file name
	TCHAR file_name[MAX_PATH];
	GetModuleFileName(NULL, file_name, MAX_PATH);
	PathStripPath(file_name);
	// load the resource
	HRSRC resource_info;
	if (0 == strcmp("Perimeter.exe", file_name))
		resource_info = FindResource(NULL, MAKEINTRESOURCE(IDR_WORLD_LIST), "TEXT");
	else if (0 == strcmp("PerimeterET.exe", file_name))
		resource_info = FindResource(NULL, MAKEINTRESOURCE(IDR_WORLD_LIST_ET), "TEXT");
	else
		return IDABORT;
	HGLOBAL resource(LoadResource(NULL, resource_info));
	char *text(ri_cast<char*>(LockResource(resource)));
	istringstream text_stream(text);
	copy(
		istream_iterator<tstring>(text_stream),
		istream_iterator<tstring>(),
		inserter(worlds_prm, worlds_prm.begin()));
	return IDOK;
}

bool Ping(LPCTSTR url)
{
	const struct Pinger {
	public:
		Pinger(LPCTSTR url)
			:url_(url)
		{
			DWORD thread_id;
			HANDLE thread(CreateThread(NULL, 0, Thread, this, 0, &thread_id));
			if (WaitForSingleObject(thread, 5000))
				result_ = false;
		}
		static DWORD WINAPI Thread(LPVOID parameter)
		{
			Pinger *obj(ri_cast<Pinger*>(parameter));
			obj->result_ = (TRUE == InternetCheckConnection(obj->url_.c_str(), FLAG_ICC_FORCE_CONNECTION, 0));
			return 0;
		}
		bool Result() const
		{
			return result_;
		}
	private:
		tstring url_;
		bool result_;
	} pinger(url);
	return pinger.Result();
}

UINT GetMapList(vector<tstring> &map_list)
{
	tstring map_list_string;
	TCHAR *error_title(_T("Map Synchronization Error"));
	// check connection status
	if (!Ping(_T("http://www.rul-clan.ru/map_registration/list_maps.php")))
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
					map_list_string += buffer;
				else
					break;
			}
		}
		// wrap up
		InternetCloseHandle(url_handle);
		InternetCloseHandle(wi_handle);
	}
	// process result
	if (map_list_string.c_str() == _T("falure"))
		return Error(_T("Map could not be registered."), error_title);
	istringstream text_stream(map_list_string);
	copy(
		istream_iterator<tstring>(text_stream),
		istream_iterator<tstring>(),
		inserter(map_list, map_list.begin()));
	return IDOK;
}

UINT SavePRM(const vector<tstring> &worlds_prm, const vector<tstring> &map_list)
{
	TCHAR *error_title(_T("Map Synchronization Error"));
	// open WORLDS.PRM
	ofstream worlds_prm_file(_T("RESOURCE\\Worlds\\WORLDS.PRM"));
	if (!worlds_prm_file.is_open())
		return Error(_T("Could not open WORLDS.PRM."), error_title);
	// output data
	worlds_prm_file << (128 + map_list.size()) << "\n\n";
	foreach (const tstring &world, worlds_prm)
		worlds_prm_file << setw(0) << world << setw(24) << world << "\n";
	for (int i(0); i != 128 - worlds_prm.size(); ++i)
		worlds_prm_file << setw(0) << "*" << setw(24) << "*" << "\n";
	foreach (const tstring &world, map_list)
		worlds_prm_file << setw(0) << world << setw(24) << world << "\n";
	return IDOK;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UINT result;
	// get WORLDS.PRM
	vector<tstring> worlds_prm;
	do
	{
		worlds_prm.clear();
		result = GetPRM(worlds_prm);
	} while (IDRETRY == result);
	if (IDABORT == result)
		return 0;
	// get the list of maps
	vector<tstring> map_list;
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

