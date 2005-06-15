#pragma once

#include "pmc wnd.h"

//-----------------------------------
// project creation dialog definition
//-----------------------------------

class CreateProjectDlg : public PMCWindow
{
// interface
public:
	INT_PTR DoModal(HWND parent_wnd);
public:
	tstring map_name_;
	SIZE    map_size_;
// message handlers
private:
	// window
	void OnCommand   (Msg<WM_COMMAND>    &msg);
	void OnInitDialog(Msg<WM_INITDIALOG> &msg);
	// command
	void OnCancel(Msg<WM_COMMAND> &msg);
	void OnOk    (Msg<WM_COMMAND> &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
private:
	bool ExchangeData();
};


























/**
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
};*/
