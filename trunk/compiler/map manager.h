#pragma once
#include "preview.h"

class CPreview;

class CMapManager
{
public:
	CMapManager();
	~CMapManager();
public:
	// interface
	void Create(HWND hWndParent);
protected:
	// window functions
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL   OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
	BOOL   OnDestroy(HWND hWnd);
	BOOL   OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
	// utility
protected:
	void Error(string message) const;
	bool GetInstallPath(string &install_path) const;
protected:
	// window
	HWND hWnd;
};
