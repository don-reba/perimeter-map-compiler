#include "StdAfx.h"
#include "resource.h"
#include "stat manager.h"
#include <process.h>

CStatManager::CStatManager(
	CData<CStaticArray<BYTE> > *heightmap,
	CData<CPalettedTexture>    *texture,
	CData<CMapInfo>            *map_info) :
	heightmap        (heightmap),
	texture          (texture),
	map_info         (map_info),
	hWnd             (NULL),
	hButton          (NULL),
	texture_thread   (NULL),
	heightmap_thread (NULL),
	is_busy          (false),
	heightmap_update_pending(false),
	texture_update_pending(false)
{
	average_color = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
}

CStatManager::~CStatManager(void)
{
	CloseHandle(texture_thread);
	CloseHandle(heightmap_thread);
}

void CStatManager::Create(HWND hWndParent, const RECT &window_rect, HWND hButton)
{
	this->hButton = hButton;
	hWnd = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_STAT_DLG),
		hWndParent,
		DialogProc,
		ri_cast<LPARAM>(this));
	DWORD swp_uflags(SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(hWnd, NULL, window_rect.left, window_rect.top, 0, 0, swp_uflags);
	this->window_rect = window_rect;
	is_visible = false;
}

void CStatManager::Destroy()
{
	DestroyWindow(hWnd);
}

struct RLess
{
	inline bool operator () (COLORREF a, COLORREF b) const
	{
		return GetRValue(a) < GetRValue(b);
	}
};
struct GLess
{
	inline bool operator () (COLORREF a, COLORREF b) const
	{
		return GetGValue(a) < GetGValue(b);
	}
};
struct BLess
{
	inline bool operator () (COLORREF a, COLORREF b) const
	{
		return GetBValue(a) < GetBValue(b);
	}
};

void CStatManager::Update(int caller)
{
	// remember which data need updating
	if (!heightmap_update_pending)
		InterlockedExchange(&heightmap_update_pending, heightmap->IsCurrentlyUpdated());
	if (!texture_update_pending)
		InterlockedExchange(&texture_update_pending,   texture->IsCurrentlyUpdated());
	// Does a tree make a noise in the woods if there is no one there to hear it?
	if (!is_visible)
		return;
	UpdateData();
}

void CStatManager::ToggleVisibility(bool show)
{
	ShowWindow(hWnd, show ? SW_SHOW : SW_HIDE);
	UpdateData();
}

CViewer::WndSaveInfo CStatManager::GetSaveInfo()
{
	if (hWnd)
		GetWindowRect(hWnd, &window_rect);
	CViewer::WndSaveInfo wsi;
	wsi.is_visible = is_visible;
	wsi.name = _T("stat_wnd");
	wsi.rect = window_rect;
	return wsi;
}

INT_PTR CALLBACK CStatManager::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CStatManager *obj = ri_cast<CStatManager*>(GetWindowLong(hWnd, DWL_USER));
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_CTLCOLORSTATIC, obj->OnColorStatic);
		HANDLE_MSG(hWnd, WM_COMMAND,        obj->OnCommand);
		HANDLE_MSG(hWnd, WM_DESTROY,        obj->OnDestroy);
		HANDLE_MSG(hWnd, WM_INITDIALOG,     obj->OnInitDialog);
		HANDLE_MSG(hWnd, WM_SETCURSOR,      obj->OnSetCursor);
		HANDLE_MSG(hWnd, WM_SHOWWINDOW,     obj->OnShowWindow);
	}
	return FALSE;
}

// WM_CTLCOLORSTATIC handler
HBRUSH CStatManager::OnColorStatic(HWND hWnd, HDC hdc, HWND hwndChild, int type)
{
	if (hwndChild == GetDlgItem(hWnd, IDC_AVE_COLOUR))
		return average_color;
	return FALSE;
}

// HANDLE_WM_COMMAND handler
BOOL CStatManager::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDCANCEL:
		ToggleVisibility(false);
		return TRUE;
	}
	return FALSE;
}

// WM_DESTROY handler
BOOL CStatManager::OnDestroy(HWND hWnd)
{
	GetWindowRect(hWnd, &window_rect);
	DestroyWindow(hWnd);
	this->hWnd = NULL;
	return TRUE;
}

// WM_INITDIALOG handler
BOOL CStatManager::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	// save pointer to this class in window user data
	SetWindowLong(hWnd, DWL_USER, lParam);
	return TRUE;
}

// WM_SETCURSOR handler
BOOL CStatManager::OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
	SetCursor(is_busy ? LoadCursor(NULL, IDC_APPSTARTING) : ri_cast<HCURSOR>(GetClassLong(hWnd, GCL_HCURSOR)));
	return TRUE;
}

// WM_SHOWWINDOW handler
BOOL CStatManager::OnShowWindow(HWND hWnd, BOOL fShow, UINT status)
{
	is_visible = (fShow == TRUE);
	Button_SetCheck(hButton, (TRUE == fShow) ? BST_CHECKED : BST_UNCHECKED);
	return TRUE;
}

// process the heightmap data
DWORD WINAPI CStatManager::HeightmapThreadProc(LPVOID lpParameter)
{
	clock_t start_time(clock());
	CStatManager *obj = ri_cast<CStatManager*>(lpParameter);
	obj->ToggleWaitCursor(true);
	const CStaticArray<BYTE> &data(obj->heightmap->SignOutConst());
	// calculate the average height
	// ASSUME map size is no larger than 2^28x2^28
	__int64 sum(0);
	int n(0);
	const BYTE* i(data.ptr);
	const BYTE* end(i + data.length);
	for (; i != end; ++i)
		if (*i != 0)
		{
			sum += *i;
			++n;
		}
	if (0 == n)
		sum = 0;
	else
		sum /= n;
	_ASSERTE(sum < 256);
	// wrap up
	SetDlgItemInt(obj->hWnd, IDC_AVE_HEIGHT, static_cast<UINT>(sum), false);
	InvalidateRect(GetDlgItem(obj->hWnd, IDC_AVE_HEIGHT), NULL, true);
	obj->heightmap->SignIn();
	obj->ToggleWaitCursor(false);
	clock_t end_time(clock());
	_RPT1(_CRT_WARN, "CStatManager::HeightmapThreadProc: %i ticks\n", end_time - start_time);
	return 0;
}

// process the texture data
DWORD WINAPI CStatManager::TextureThreadProc(LPVOID lpParameter)
{
	clock_t start_time(clock());
	CStatManager *obj = ri_cast<CStatManager*>(lpParameter);
	obj->ToggleWaitCursor(true);
	const CPalettedTexture &data(obj->texture->SignOutConst());
	const COLORREF * const palette_begin(data.palette);
	const BYTE * const texture_begin(data.ptr);
	const BYTE * const texture_end  (texture_begin + data.length);
	const BYTE *       texture_iter;
	const size_t num_colors(256);
	int color_count[num_colors];
	const int *color_iter;
	int half_size(0);
	int sum;
	// get the null pixels of the heightmap
	bool *null_pixels(NULL);
	const bool *null_pixels_iter;
	{
		// sign out the heightmap
		const CStaticArray<BYTE> &heightmap_data(obj->heightmap->SignOutConst());
		// allocate memory
		null_pixels = new bool[data.length];
		// initialize iterators
		const BYTE * heightmap_iter(heightmap_data.ptr);
		      bool * null_pixels_iter(null_pixels);
		// get map information
		SIZE map_size;
		{
			const CMapInfo &map_data(obj->map_info->SignOutConst());
			map_size.cx = obj->exp2(map_data.map_power_x);
			map_size.cy = obj->exp2(map_data.map_power_y);
			obj->map_info->SignIn();
			_ASSERTE(data.length == static_cast<size_t>(map_size.cx * map_size.cy));
		}
		// get the null pixels
		for (int r(0); r != map_size.cx; ++r)
		{
			for (int c(0); c != map_size.cy; ++c)
				*null_pixels_iter++ = *heightmap_iter++ == 0;
			++heightmap_iter;
		}
		obj->heightmap->SignIn();
		// count the number of non-null pixels
		{
			null_pixels_iter = null_pixels;
			const bool * const null_pixels_end(null_pixels + data.length);
			while (null_pixels_iter != null_pixels_end)
				if (!*null_pixels_iter++)
					++half_size;
			if (0 == half_size)
			{
				obj->texture->SignIn();
				return 0;
			}
			half_size /= 2;
		}
	}
	// count the number of occurences of each value of blue
	ZeroMemory(color_count, num_colors * sizeof(int));
	null_pixels_iter = null_pixels;
	for (texture_iter = texture_begin; texture_iter != texture_end; ++texture_iter)
		if (!*null_pixels_iter++)
			++color_count[GetBValue(palette_begin[*texture_iter])];
	// find the median blue
	BYTE median_b;
	sum = 0;
	color_iter = color_count;
	do
	{
		_ASSERTE(color_iter != color_count + num_colors);
		if (*color_iter != 0)
			sum += *color_iter;
		++color_iter;
	} while (sum < half_size);
	median_b = static_cast<BYTE>(color_iter - color_count);
	// count the number of occurences of each value of green
	ZeroMemory(color_count, num_colors * sizeof(int));
	null_pixels_iter = null_pixels;
	for (texture_iter = texture_begin; texture_iter != texture_end; ++texture_iter)
		if (!*null_pixels_iter++)
			++color_count[GetGValue(palette_begin[*texture_iter])];
	// find the median green
	BYTE median_g;
	sum = 0;
	color_iter = color_count;
	do
	{
		_ASSERTE(color_iter != color_count + num_colors);
		if (*color_iter != 0)
			sum += *color_iter;
		++color_iter;
	} while (sum < half_size);
	median_g = static_cast<BYTE>(color_iter - color_count);
	// count the number of occurences of each value of green
	ZeroMemory(color_count, num_colors * sizeof(int));
	null_pixels_iter = null_pixels;
	for (texture_iter = texture_begin; texture_iter != texture_end; ++texture_iter)
		if (!*null_pixels_iter++)
			++color_count[GetRValue(palette_begin[*texture_iter])];
	delete [] null_pixels;
	// find the median red
	BYTE median_r;
	sum = 0;
	color_iter = color_count;
	do
	{
		_ASSERTE(color_iter != color_count + num_colors);
		if (*color_iter != 0)
			sum += *color_iter;
		++color_iter;
	} while (sum < half_size);
	median_r = static_cast<BYTE>(color_iter - color_count);
	// wrap up
	obj->texture->SignIn();
	DeleteObject(obj->average_color);
	_ASSERTE(sizeof(LONG) == sizeof(HBRUSH));
	InterlockedExchange(
		ri_cast<LONG*>(&obj->average_color),
		ri_cast<LONG>(CreateSolidBrush(RGB(median_r, median_g, median_b))));
	InvalidateRect(GetDlgItem(obj->hWnd, IDC_AVE_COLOUR), NULL, TRUE);
	obj->ToggleWaitCursor(false);
	clock_t end_time(clock());
	_RPT1(_CRT_WARN, "CStatManager::TextureThreadProc: %i ticks\n", end_time - start_time);
	return 0;}

// toggle wait cursor
void CStatManager::ToggleWaitCursor(bool on)
{
	static int count(0);
	if (on)
	{
		if (0 == count)
		{
			InterlockedExchange(&is_busy, true);
			SendMessage(hWnd, WM_SETCURSOR, 0L, NULL);
		}
		++count;
	}
	else
		--count;
	if (0 == count)
	{
		InterlockedExchange(&is_busy, false);
		SendMessage(hWnd, WM_SETCURSOR, 0L, NULL);
	}
}

void CStatManager::UpdateData()
{
	if (heightmap_update_pending)
	{
		InterlockedExchange(&heightmap_update_pending, false);
		if (NULL != heightmap_thread)
		{
			WaitForSingleObject(heightmap_thread, INFINITE);
			CloseHandle(heightmap_thread);
		}
		heightmap_thread = CreateThread(NULL, 0, HeightmapThreadProc, this, 0, NULL);
	}
	if (texture_update_pending)
	{
		InterlockedExchange(&texture_update_pending, false);
		if (NULL != texture_thread)
		{
			WaitForSingleObject(texture_thread, INFINITE);
			CloseHandle(texture_thread);
		}
		texture_thread = CreateThread(NULL, 0, TextureThreadProc, this, 0, NULL);
	}
}

inline int CStatManager::exp2(unsigned int n) const
{
	int e(1);
	while (0 != n)
	{
		e <<= 1;
		--n;
	}
	return e;
}