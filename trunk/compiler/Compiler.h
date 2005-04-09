#pragma once

#include "preferences.h"
#include "project dialog.h"
#include "project manager.h"
#include "map manager.h"

//------------------------------
// main application window class
//------------------------------

class CCompiler
{
protected:
	enum
	{
		IDC_STAT_MANAGER_BTN = 1000,
		IDC_PREVIEW_BTN      = 1001,
		IDC_INFO_MANAGER_BTN = 1002
	};
public:
	// lifetime
	CCompiler(HINSTANCE hInstance);
	~CCompiler();
	// window functions
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	string FolderDlg(HWND hWNd, TCHAR *title);
	BOOL   InitInstance(int nCmdShow);
	ATOM   MyRegisterClass();
	void   OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify);
	BOOL   OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);
	void   OnDestroy(HWND hWnd);
	BOOL   OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg);
	string OpenDlg(HWND hWnd, TCHAR *title, TCHAR *filter);
	void   ToggleWaitCursor(bool on);
public:
	// constants
	static const int MAX_LOADSTRING = 100;
	// data
	HWND            hWnd;                          // main window handle
	HINSTANCE       hInstance;                     // current instance
	TCHAR           szTitle[MAX_LOADSTRING];       // The title bar text
	TCHAR           szWindowClass[MAX_LOADSTRING]; // the main window class name
	CProjectManager project;                       // responsible for project management
	CPreferences    preferences;                   // responsible for the "Preferences" dialog
	CMapManager     map_manager;                   // responsible for map management
	HWND            active_shrub;                  // active shrub icon
	HWND            inactive_shrub;                // inactive shrub icon
	LONG            is_busy;                       // LONG for use of interlocked operations
	bool            initialization_succeeded;      // prevent serialization after unsuccessful initialization
};

#include "resource.h"
