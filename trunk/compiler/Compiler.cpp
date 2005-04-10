// Compiler.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "about.h"
#include "Compiler.h"
#include "shlobj.h"

//----------------------------
// additional message crackers
//----------------------------

#undef Static_SetIcon
#define Static_SetIcon(hwndCtl, icon_id) ((void)SNDMSG((hwndCtl), STM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(icon_id), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)))

//-----------
// button ids
//-----------

const int IDC_STAT_MANAGER_BTN(1000);
const int IDC_PREVIEW_BTN     (1001);
const int IDC_INFO_MANAGER_BTN(1002);

//--------
// WinMain
//--------

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	InitCommonControls();

	MSG msg;
	HACCEL hAccelTable;

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_COMPILER);

	// create the main window
	CCompiler compiler(hInstance);
	compiler.MyRegisterClass();
	if (FALSE == compiler.InitInstance(nCmdShow))
		return 0;


	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//-------------------------
// CCompiler implementation
//-------------------------

CCompiler::CCompiler(HINSTANCE hInstance) :
	hInstance                (hInstance),
	project                  (),
	preferences              (project.GetPreviewSettings(), project.GetProjectSettings()),
	is_busy                  (false),
	initialization_succeeded (false)
{
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_COMPILER, szWindowClass, MAX_LOADSTRING);
}

CCompiler::~CCompiler()
{
}

// main window's message pump
LRESULT CALLBACK CCompiler::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// prevent processing before the dialog is fully initialized
	{
		static bool initialized(false);
		if (WM_CREATE == message)
		{
			initialized = true;
		}
		if (false == initialized)
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	// get the object pointer
	CCompiler *obj;
	if (WM_CREATE != message)
		obj = ri_cast<CCompiler*>(GetWindowLong(hWnd, GWL_USERDATA));
	else
		obj = ri_cast<CCompiler*>(ri_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
	// process the message
	switch (message)
	{
		HANDLE_MSG(hWnd, WM_CREATE,    obj->OnCreate);
		HANDLE_MSG(hWnd, WM_COMMAND,   obj->OnCommand);
		HANDLE_MSG(hWnd, WM_DESTROY,   obj->OnDestroy);
		HANDLE_MSG(hWnd, WM_SETCURSOR, obj->OnSetCursor);
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
// select a directory for a new project folder via the standard BrowseForFolder dialog
string CCompiler::FolderDlg(HWND hWnd, TCHAR *title)
{
	// open the dialog
	TCHAR strFolderName[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner = hWnd;
	bi.pszDisplayName = strFolderName;
	bi.lpszTitle = title;
	bi.ulFlags = BIF_USENEWUI | BIF_VALIDATE;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	//  get path
	if (NULL != pidl && SHGetPathFromIDList(pidl, strFolderName))
		return strFolderName;
	return "";
}

BOOL CCompiler::InitInstance(int nCmdShow)
{
	const size_t max_str_size(64);
	TCHAR str[max_str_size];
	// get the path to the initialisation file
	// it has the same name as the executible, but with an "ini" extension
	TCHAR ini_path[MAX_PATH];
	GetModuleFileName(NULL, ini_path, MAX_PATH);
	PathRemoveExtension(ini_path);
	PathAddExtension(ini_path, ".ini");
	// window constants
	const SIZE button_sz = { 32 + 4, 32 + 4 };
	const SIZE shrub_sz  = { 64,     64 };
	const SIZE space_sz  = { 8,      4 };
	const SIZE margin_sz = { 8,      8 };
	const SIZE client_sz = {
		margin_sz.cx + button_sz.cx+ space_sz.cy + shrub_sz.cx + margin_sz.cx,
		margin_sz.cy * 2 + button_sz.cy * 3 + space_sz.cy * 2 };
	// get dektop size ratio
	float desktop_ratio;
	{
		
		RECT desktop_rect;
		SIZE old_size, new_size;
		// get old resolution
		GetPrivateProfileString(
			_T("windows"),
			_T("resolution_h"),
			_T("1024"),
			str,
			max_str_size,
			ini_path);
		old_size.cx = _ttol(str);
		GetPrivateProfileString(
			_T("windows"),
			_T("resolution_v"),
			_T("768"),
			str,
			max_str_size,
			ini_path);
		old_size.cy = _ttol(str);
		// get new resolution
		GetWindowRect(GetDesktopWindow(), &desktop_rect);
		new_size.cx = desktop_rect.right - desktop_rect.left;
		new_size.cy = desktop_rect.bottom - desktop_rect.top;
		// calculate ratio
		desktop_ratio = static_cast<float>(new_size.cx) / old_size.cx;
		
	}
	// get the window placement
	DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	POINT window_pos;
	SIZE  window_size;
	{
		// position
		GetPrivateProfileString(
			_T("windows"),
			_T("main_wnd_rect"),
			_T("61 26 187 202"),
			str,
			max_str_size,
			ini_path);
		_stscanf(
			str,
			_T("%li %li %li %li"),
			&window_pos.x,
			&window_pos.y,
			&window_size.cx,
			&window_size.cy);
		window_pos.x = static_cast<LONG>(desktop_ratio * window_pos.x);
		window_pos.y = static_cast<LONG>(desktop_ratio * window_pos.y);
		// size
		RECT window_rect = { 0, 0, client_sz.cx, client_sz.cy };
		AdjustWindowRect(&window_rect, window_style, TRUE);
		window_size.cx = window_rect.right  - window_rect.left;
		window_size.cy = window_rect.bottom - window_rect.top;
	}
	// create the main window
   hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		window_style,
		window_pos.x,
		window_pos.y,
		window_size.cx,
		window_size.cy,
		NULL,
		NULL,
		hInstance,
		ri_cast<LPVOID>(this));
   if (!hWnd)
	{
      return FALSE;
	}
	// create the shrub icon
	active_shrub = CreateWindow(
		_T("STATIC"),
		NULL,
		WS_CHILD | SS_ICON,
		margin_sz.cx + button_sz.cx + space_sz.cx,
		max(0, (client_sz.cy - shrub_sz.cy) / 2),
		shrub_sz.cx,
		shrub_sz.cy,
		hWnd,
		NULL,
		hInstance,
		NULL);
	Static_SetIcon(active_shrub, IDI_SHRUB_ACTIVE);
	inactive_shrub = CreateWindow(
		_T("STATIC"),
		NULL,
		WS_CHILD | WS_VISIBLE | SS_ICON,
		margin_sz.cx + button_sz.cx + space_sz.cx,
		max(0, (client_sz.cy - shrub_sz.cy) / 2),
		shrub_sz.cx,
		shrub_sz.cy,
		hWnd,
		NULL,
		hInstance,
		NULL);
	Static_SetIcon(inactive_shrub, IDI_SHRUB_INACTIVE);
	// get the stat manager rectangle
	CProjectManager::WndAttributes stat_manager_attributes;
	{
		// rectangle
		GetPrivateProfileString(_T("windows"), _T("stat_wnd_rect"), _T("51 220 189 300"), str, max_str_size, ini_path);
		_stscanf(
			str,
			_T("%li %li %li %li"),
			&stat_manager_attributes.rect.left,
			&stat_manager_attributes.rect.top,
			&stat_manager_attributes.rect.right,
			&stat_manager_attributes.rect.bottom);
		stat_manager_attributes.rect.left   = static_cast<LONG>(desktop_ratio * stat_manager_attributes.rect.left);
		stat_manager_attributes.rect.right  = static_cast<LONG>(desktop_ratio * stat_manager_attributes.rect.right);
		stat_manager_attributes.rect.bottom = static_cast<LONG>(desktop_ratio * stat_manager_attributes.rect.bottom);
		stat_manager_attributes.rect.top    = static_cast<LONG>(desktop_ratio * stat_manager_attributes.rect.top);
		// visibility
		GetPrivateProfileString(_T("windows"), _T("stat_wnd_is_visible"), _T("true"), str, 6, ini_path);
		stat_manager_attributes.is_visible = (0 == _tcscmp(str, _T("true")));
	}
	// create the stat manager button
	stat_manager_attributes.button = CreateWindow(
		_T("BUTTON"),
		_T("sm"),
		WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_ICON,
		margin_sz.cx,
		margin_sz.cy,
		button_sz.cx,
		button_sz.cy,
		hWnd,
		ri_cast<HMENU>(IDC_STAT_MANAGER_BTN),
		hInstance,
		NULL);
	SendMessage(stat_manager_attributes.button, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
	// get the preview rectangle
	CProjectManager::WndAttributes preview_attributes;
	{
		// rectangle
		GetPrivateProfileString(_T("windows"), _T("preview_wnd_rect"), _T("200 23 1007 735"), str, max_str_size, ini_path);
		_stscanf(
			str,
			_T("%li %li %li %li"),
			&preview_attributes.rect.left,
			&preview_attributes.rect.top,
			&preview_attributes.rect.right,
			&preview_attributes.rect.bottom);
		preview_attributes.rect.left   = static_cast<LONG>(desktop_ratio * preview_attributes.rect.left);
		preview_attributes.rect.right  = static_cast<LONG>(desktop_ratio * preview_attributes.rect.right);
		preview_attributes.rect.bottom = static_cast<LONG>(desktop_ratio * preview_attributes.rect.bottom);
		preview_attributes.rect.top    = static_cast<LONG>(desktop_ratio * preview_attributes.rect.top);
		// visibility
		GetPrivateProfileString(_T("windows"), _T("preview_wnd_is_visible"), _T("true"), str, 6, ini_path);
		preview_attributes.is_visible = (_tcscmp(str, "true") == 0);
	}
	// create the preview button
	preview_attributes.button = CreateWindow(
		_T("BUTTON"),
		_T("mp"),
		WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_ICON,
		margin_sz.cx,
		margin_sz.cy + button_sz.cy + space_sz.cy,
		button_sz.cx,
		button_sz.cy,
		hWnd,
		ri_cast<HMENU>(IDC_PREVIEW_BTN),
		hInstance,
		NULL);
	SendMessage(preview_attributes.button, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MP), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
	// get the info manager rectangle
	CProjectManager::WndAttributes info_manager_attributes;
	{
		// rectangle
		GetPrivateProfileString(_T("windows"), _T("info_wnd_rect"), _T("1 322 189 623"), str, max_str_size, ini_path);
		_stscanf(
			str,
			_T("%li %li %li %li"),
			&info_manager_attributes.rect.left,
			&info_manager_attributes.rect.top,
			&info_manager_attributes.rect.right,
			&info_manager_attributes.rect.bottom);
		info_manager_attributes.rect.left   = static_cast<LONG>(desktop_ratio * info_manager_attributes.rect.left);
		info_manager_attributes.rect.right  = static_cast<LONG>(desktop_ratio * info_manager_attributes.rect.right);
		info_manager_attributes.rect.bottom = static_cast<LONG>(desktop_ratio * info_manager_attributes.rect.bottom);
		info_manager_attributes.rect.top    = static_cast<LONG>(desktop_ratio * info_manager_attributes.rect.top);
		// visibility
		GetPrivateProfileString(_T("windows"), _T("info_wnd_is_visible"), _T("true"), str, 6, ini_path);
		info_manager_attributes.is_visible = (_tcscmp(str, "true") == 0);
	}
	// create the stat manager button
	info_manager_attributes.button = CreateWindow(
		_T("BUTTON"),
		_T("im"),
		WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_ICON,
		margin_sz.cx,
		margin_sz.cy + button_sz.cy * 2 + space_sz.cy * 2,
		button_sz.cx,
		button_sz.cy,
		hWnd,
		ri_cast<HMENU>(IDC_INFO_MANAGER_BTN),
		hInstance,
		NULL);
	SendMessage(info_manager_attributes.button, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_IM), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
	// create the project
	project.Create(hWnd, stat_manager_attributes, preview_attributes, info_manager_attributes, ini_path);
	// show all viewers
	project.ToggleStatManager(stat_manager_attributes.is_visible);
	project.TogglePreview(preview_attributes.is_visible);
	project.ToggleInfoManager(info_manager_attributes.is_visible);
	// initialize COM
	CoInitialize(NULL);
	// show the window
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
	initialization_succeeded = true;
   return TRUE;
}

ATOM CCompiler::MyRegisterClass()
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = (WNDPROC)WndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = hInstance;
	wcex.hIcon         = LoadIcon(hInstance, (LPCTSTR)IDI_COMPILER);
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
	wcex.lpszMenuName  = (LPCTSTR)IDC_COMPILER;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm       = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_COMPILER);

	return RegisterClassEx(&wcex);
}

// WM_COMMAND message handler
void CCompiler::OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case ID_FILE_NEWPROJECT:
		{
			CProjectDlg dlg;
			if (IDOK == dlg.DoModal(hInstance, hWnd))
			{
				string project_folder(FolderDlg(hWnd, "Choose a folder for the new project's files."));
				if (!project_folder.empty())
				{
					ToggleWaitCursor(true);
					// create the project
					project.NewProject(project_folder, dlg.map_size, dlg.map_name);
					// change menu state
					EnableMenuItem(GetMenu(hWnd), ID_FILE_PACKSHRUB,   MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_INSTALLSHRUB, MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_OPENPROJECT, MF_DISABLED | MF_GRAYED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_NEWPROJECT,  MF_DISABLED | MF_GRAYED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_UNPACKSHRUB, MF_DISABLED | MF_GRAYED);
					ToggleWaitCursor(false);
				}
			}
		} break;
	case ID_FILE_OPENPROJECT:
		{
			string project_file(OpenDlg(hWnd, "Open Project", "Map Project Files (*.pmproj)\0*.pmproj\0"));
			if (!project_file.empty())
			{
				ToggleWaitCursor(true);
				// open the project
				project.OpenProject(project_file);
				// change menu state
				EnableMenuItem(GetMenu(hWnd), ID_FILE_PACKSHRUB,   MF_ENABLED);
				EnableMenuItem(GetMenu(hWnd), ID_FILE_INSTALLSHRUB, MF_ENABLED);
				EnableMenuItem(GetMenu(hWnd), ID_FILE_OPENPROJECT, MF_DISABLED | MF_GRAYED);
				EnableMenuItem(GetMenu(hWnd), ID_FILE_NEWPROJECT,  MF_DISABLED | MF_GRAYED);
				EnableMenuItem(GetMenu(hWnd), ID_FILE_UNPACKSHRUB, MF_DISABLED | MF_GRAYED);
				ToggleWaitCursor(false);
			}
		} break;
	case ID_FILE_PACKSHRUB:
		if (IDCANCEL == MessageBox(
			hWnd,
			_T("The map will be automatically registered. Note that this requires connection to the internet.\nIt will not be possible to register this map again under the same name if it is modified.\nProceed?"),
			_T("Warning"),
			MB_OKCANCEL | MB_ICONWARNING))
			break;
		ToggleWaitCursor(true);
		project.Pack();
		ToggleWaitCursor(false);
		break;
	case ID_FILE_UNPACKSHRUB:
		{
			string shrub_file(OpenDlg(hWnd, _T("Unpack Shrub"), _T("Shrub Files (*.shrub)\0*.shrub\0")));
			if (!shrub_file.empty())
			{
				// unpack the shrub
				ToggleWaitCursor(true);
				if (project.Unpack(shrub_file))
				{
					// change menu state
					EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVEPROJECT,  MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_INSTALLSHRUB, MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_OPENPROJECT,  MF_DISABLED | MF_GRAYED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_NEWPROJECT,   MF_DISABLED | MF_GRAYED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_PACKSHRUB,    MF_DISABLED | MF_GRAYED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_UNPACKSHRUB,  MF_DISABLED | MF_GRAYED);
					// change the shrub icon
					ShowWindow(active_shrub, SW_SHOW);
					ShowWindow(inactive_shrub, SW_HIDE);
				}
				ToggleWaitCursor(false);
			}
		} break;
	case ID_FILE_INSTALLSHRUB:
		{
			ToggleWaitCursor(true);
			// install the shrub as a map
			project.Install();
			ToggleWaitCursor(false);
		} break;
	case ID_FILE_MANAGEMAPS:
		{
			map_manager.Create(hWnd);
		} break;
	case ID_TOOLS_PREFERENCES:
		preferences.Create(hWnd);
		break;
	case IDM_ABOUT:
		{
			CAbout about_dlg;
			about_dlg.DoModal(hInstance, hWnd);
		} break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	case IDC_STAT_MANAGER_BTN:
		project.ToggleStatManager(Button_GetCheck(hwndCtl) == BST_CHECKED);
		break;
	case IDC_PREVIEW_BTN:
		project.TogglePreview(Button_GetCheck(hwndCtl) == BST_CHECKED);
		break;
	case IDC_INFO_MANAGER_BTN:
		project.ToggleInfoManager(Button_GetCheck(hwndCtl) == BST_CHECKED);
		break;
	default:
		FORWARD_WM_COMMAND(hWnd, id, hwndCtl, codeNotify, DefWindowProc);
	}
}

// WM_CREATE message handler
BOOL CCompiler::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
	// save pointer to this class in window user data
	SetWindowLong(hWnd, GWL_USERDATA, ri_cast<LONG>(lpCreateStruct->lpCreateParams));
	return TRUE; // required by the default message cracker
}

// WM_DESTROY message handler
void CCompiler::OnDestroy(HWND hWnd)
{
	if (initialization_succeeded)
	{
		// query information to save
		RECT desktop_rect;
		GetWindowRect(GetDesktopWindow(), &desktop_rect);
		vector<CViewer::WndSaveInfo> save_info(project.GetSaveInfo());
		{
			// add information for the main window
			CViewer::WndSaveInfo info;
			info.name = _T("main_wnd");
			GetWindowRect(hWnd, &info.rect);
			save_info.push_back(info);
		}
		// get the path to the initialisation file
		// it has the same name as the executible, but with an "ini" extension
		TCHAR ini_path[MAX_PATH];
		GetModuleFileName(NULL, ini_path, MAX_PATH);
		PathRemoveExtension(ini_path);
		PathAddExtension(ini_path, _T(".ini"));
		// save the information
		TCHAR str[64];
		// save desktop width
		_ltot(desktop_rect.right - desktop_rect.left, str, 10);
		WritePrivateProfileString(_T("windows"), _T("resolution_h"), str, ini_path);
		// save desktop height
		_ltot(desktop_rect.bottom - desktop_rect.top, str, 10);
		WritePrivateProfileString(_T("windows"), _T("resolution_v"), str, ini_path);
		for (size_t i = 0; i != save_info.size(); ++i)
		{
			// rectangle
			_stprintf(
				str,
				"%li %li %li %li",
				save_info[i].rect.left,
				save_info[i].rect.top,
				save_info[i].rect.right,
				save_info[i].rect.bottom);
			WritePrivateProfileString(
				_T("windows"),
				(save_info[i].name + _T("_rect")).c_str(),
				str,
				ini_path);
			// visibility
			if (i + 1 != save_info.size())
				WritePrivateProfileString(
					_T("windows"),
					(save_info[i].name + "_is_visible").c_str(),
					save_info[i].is_visible ? "true" : "false",
					ini_path);
		}
		// save preview settings
		project.SaveSettings(ini_path);
	}
	// quit
	project.Destroy();
	PostQuitMessage(0);
}

// WM_SETCURSOR handler
BOOL CCompiler::OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
	if (hWnd)
		return FORWARD_WM_SETCURSOR(hWnd, hWndCursor, codeHitTest, msg, DefWindowProc);
	SetCursor(is_busy ? LoadCursor(NULL, IDC_APPSTARTING) : ri_cast<HCURSOR>(GetClassLong(hWnd, GCL_HCURSOR)));
	return TRUE;
}

// toggle wait cursor
void CCompiler::ToggleWaitCursor(bool on)
{
	static int count(0);
	if (on)
	{
		if (0 == count)
		{
			InterlockedExchange(&is_busy, true);
			PostMessage(hWnd, WM_SETCURSOR, 0L, NULL);
		}
		++count;
	}
	else
		--count;
	if (0 == count)
	{
		InterlockedExchange(&is_busy, false);
		PostMessage(hWnd, WM_SETCURSOR, 0L, NULL);
	}
}

// open a project file via the standard Open dialog
string CCompiler::OpenDlg(HWND hWnd, TCHAR *title, TCHAR *filter)
{
	TCHAR strFileName[MAX_PATH] = { 0 };
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = strFileName;
	ofn.nMaxFile = sizeof(strFileName);
	ofn.lpstrTitle = title;
	ofn.lpstrFilter = filter;
	ofn.Flags =
		OFN_LONGNAMES     |
		OFN_FILEMUSTEXIST |
		OFN_READONLY      |
		OFN_NOCHANGEDIR   |
		OFN_HIDEREADONLY;
	if (false != GetOpenFileName(&ofn))
		return strFileName;
	return "";
}
