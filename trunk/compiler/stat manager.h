#pragma once
#include "data.h"
#include "viewer.h"

class CStatManager : public CViewer
{
public:
	CStatManager(
		CData<CStaticArray<BYTE> > *heightmap,
		CData<CPalettedTexture>    *texture,
		CData<CMapInfo>            *map_info);
	~CStatManager(void);
public:
	// interface
	void Create(HWND hWndParent, const RECT &window_rect, HWND hButton);
	void Destroy();
	virtual void Update(int caller);
	virtual void ToggleVisibility(bool show);
	virtual CViewer::WndSaveInfo GetSaveInfo();
protected:
	// window functions
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HBRUSH OnColorStatic(HWND hWnd, HDC hdc, HWND hwndChild, int type);
	BOOL   OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
	BOOL   OnDestroy(HWND hWnd);
	BOOL   OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
	BOOL   OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg);
	BOOL   OnShowWindow(HWND hWnd, BOOL fShow, UINT status);
	// data processing functions
	static DWORD WINAPI HeightmapThreadProc(LPVOID lpParameter);
	static DWORD WINAPI TextureThreadProc(LPVOID lpParameter);
	// math
	inline int exp2(unsigned int n) const;
protected:
	// utility functions
	void ToggleWaitCursor(bool on);
	void UpdateData();
protected:
	// data
	CData<CStaticArray<BYTE> > *heightmap;
	CData<CPalettedTexture>    *texture;
	CData<CMapInfo>            *map_info;
	HBRUSH average_color;
	LONG heightmap_update_pending;
	LONG texture_update_pending;
	// multithreading
	HANDLE heightmap_thread;
	HANDLE texture_thread;
	// window
	HWND hWnd;
	HWND hButton;
	RECT window_rect;
	bool is_visible;
	LONG is_busy; // LONG for use of interlocked operations
};
