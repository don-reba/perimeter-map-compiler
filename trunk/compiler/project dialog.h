#pragma once

class CProjectDlg
{
public:
	CProjectDlg() { map_size.cx = map_size.cy = 5; }
	~CProjectDlg() {}
public:
	INT_PTR DoModal(HINSTANCE hInstance, HWND hWnd);
protected:
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
	BOOL OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
public:
	SIZE   map_size;
	string map_name;
};
