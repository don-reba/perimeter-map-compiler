#pragma once
#include "data.h"
#include "viewer.h"

class CInfoManager : public CViewer
{
public:
	CInfoManager(CData<CMapInfo> *map_info, int id);
	~CInfoManager();
public:
	// interface
	void Create(HWND hWndParent, const RECT &window_rect, HWND hButton);
	void Destroy();
	virtual void Update(int caller);
	virtual void ToggleVisibility(bool show);
	virtual CViewer::WndSaveInfo GetSaveInfo();
	void SetReadOnly(bool on);
protected:
	// window functions
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL   OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
	HBRUSH OnColorStatic(HWND hWnd, HDC hdc, HWND hwndChild, int type);
	BOOL   OnDestroy(HWND hWnd);
	BOOL   OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
	BOOL   OnShowWindow(HWND hWnd, BOOL fShow, UINT status);
	// math
	inline int exp2(unsigned int n) const;
protected:
	// data
	int id;
	CData<CMapInfo> *map_info;
	HBRUSH fog_colour;
	COLORREF custom_colors[16];
	// window
	HWND hWnd;
	HWND hButton;
	RECT window_rect;
	bool is_visible;
	bool read_only;
};
