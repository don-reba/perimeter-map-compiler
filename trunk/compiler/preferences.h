#pragma once
#include "preview.h"
#include "project manager.h"

class CPreview;

class CPreferences
{
public:
	CPreferences(
		CPreview::CSerializable *preview_settings,
		CProjectManager::CSerializable *project_settings);
	~CPreferences();
public:
	// interface
	void Create(HWND hWndParent);
protected:
	// window functions
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL   OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
	HBRUSH OnColorStatic(HWND hWnd, HDC hdc, HWND hwndChild, int type);
	BOOL   OnDestroy(HWND hWnd);
	BOOL   OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
	// utility functions
	void Apply();
protected:
	// data
	CPreview::CSerializable *preview_settings;
	CProjectManager::CSerializable *project_settings;
	// window
	HWND     hWnd;
	COLORREF custom_colors[16];
	COLORREF zero_plast_colour;
	HBRUSH   zero_plast_colour_brush;
};
