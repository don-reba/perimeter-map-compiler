#include "StdAfx.h"
#include "preview.h"
#include "resource.h"
#include <cmath>
using std::list;

//----------
// constants
//----------

const int texture_width (1024);
const int texture_height(1024);

//------------------------
// CPreview implementation
//------------------------

CPreview::CPreview(
	CData<CStaticArray<BYTE> > *heightmap,
	CData<CPalettedTexture>    *texture,
	CData<CMapInfo>            *map_info,
	CData<CStaticArray<BYTE> > *lightmap)
	:heightmap        (heightmap)
	,texture          (texture)
	,map_info         (map_info)
	,lightmap         (lightmap)
	,heightmap_valid  (false)
	,map_info_valid   (false)
	,texture_valid    (false)
	,settings         (this)
	,hWnd             (NULL)
	,hButton          (NULL)
	,device           (NULL)
	,zero_layer_vb    (NULL)
	,mouse_captured   (false)
	,wireframe_mode   (false)
	,heightmap_thread (NULL)
	,map_info_thread  (NULL)
	,texture_thread   (NULL)
	,is_busy          (false)
	,world_stretch    (1.3f)
	,active_vb        (NULL)
	,last_vb_access   (0)
	,track_usage      (true)
	,tmp_file         (INVALID_HANDLE_VALUE)
	,heightmap_update_pending (false)
	,texture_update_pending   (false)
	,map_info_update_pending  (false)
{
	// set map size to a value a project is guaranteed to not have
	map_size.cx = 0;
	map_size.cy = 0;
	// initialize DirectX
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (NULL == d3d)
		Error(_T("Direct3DCreate9 failed"));
	// initialize critical sections
	InitializeCriticalSection(&billboard_section);
	InitializeCriticalSection(&matrix_section);
	InitializeCriticalSection(&texture_section);
	InitializeCriticalSection(&vb_section);
	InitializeCriticalSection(&zero_layer_vb_section);
	// initialize world matrix stack
	{
		D3DXMATRIX identity_matrix;
		D3DXMatrixIdentity(&identity_matrix);
		world_matrix_stack.push(identity_matrix);
	}
	// set default zero layer colour
	zero_layer_colour   = D3DCOLOR_ARGB(0x28, 0x00, 0x00, 0x00);
}

CPreview::~CPreview(void)
{
	d3d->Release();
}

void CPreview::Create(HWND hWndParent, const RECT &window_rect, HWND hButton, const TCHAR * const ini_path)
{
	this->hButton = hButton;
	// load settings
	settings.Load(ini_path);
	// create the window
	CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_PREVIEW_DLG),
		hWndParent,
		DialogProc,
		ri_cast<LPARAM>(this));
	SetWindowPos(
		hWnd,
		NULL,
		window_rect.left,
		window_rect.top,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		SWP_NOACTIVATE | SWP_NOZORDER);
	this->window_rect = window_rect;
	is_visible = false;
}

void CPreview::Destroy()
{
	if (NULL != heightmap_thread)
	{
		WaitForSingleObject(heightmap_thread, INFINITE);
		CloseHandle(heightmap_thread);
		heightmap_thread = NULL;
	}
	if (NULL != map_info_thread)
	{
		WaitForSingleObject(map_info_thread, INFINITE);
		CloseHandle(map_info_thread);
		map_info_thread = NULL;
	}
	if (NULL != texture_thread)
	{
		WaitForSingleObject(texture_thread, INFINITE);
		CloseHandle(texture_thread);
		texture_thread = NULL;
	}
	{
		track_usage = false;
		CAutoCriticalSection auto_vb_section(&vb_section);
		DeleteTerainVBs();
		CloseHandle(tmp_file);
	}
	DestroyWindow(hWnd);
}

void CPreview::Update(int caller)
{
	// remember which data need updating
	if (!heightmap_update_pending)
		InterlockedExchange(&heightmap_update_pending, heightmap->IsCurrentlyUpdated());
	if (!texture_update_pending)
		InterlockedExchange(&texture_update_pending,   texture->IsCurrentlyUpdated());
	if (!map_info_update_pending)
		InterlockedExchange(&map_info_update_pending,  map_info->IsCurrentlyUpdated());
	// Does a tree make a noise in the woods if there is no one there to hear it?
	if (!is_visible)
		return;
	UpdateData();
}

void CPreview::ToggleVisibility(bool show)
{
	ShowWindow(hWnd, show ? SW_SHOW : SW_HIDE);
	UpdateData();
}


CViewer::WndSaveInfo CPreview::GetSaveInfo()
{
	if (hWnd)
		GetWindowRect(hWnd, &window_rect);
	CViewer::WndSaveInfo wsi;
	wsi.is_visible = is_visible;
	wsi.name = _T("preview_wnd");
	wsi.rect = window_rect;
	return wsi;
}

INT_PTR CALLBACK CPreview::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CPreview *obj(ri_cast<CPreview*>((uMsg != WM_INITDIALOG) ? GetWindowLong(hWnd, DWL_USER) : lParam));
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_CAPTURECHANGED, obj->OnCaptureChanged);
		HANDLE_MSG(hWnd, WM_COMMAND,        obj->OnCommand);
		HANDLE_MSG(hWnd, WM_DESTROY,        obj->OnDestroy);
		HANDLE_MSG(hWnd, WM_ERASEBKGND,     obj->OnEraseBkgnd);
		HANDLE_MSG(hWnd, WM_INITDIALOG,     obj->OnInitDialog);
		HANDLE_MSG(hWnd, WM_KEYDOWN,        obj->OnKeyDown);
		HANDLE_MSG(hWnd, WM_LBUTTONDOWN,    obj->OnLButtonDown);
		HANDLE_MSG(hWnd, WM_LBUTTONUP,      obj->OnLButtonUp);
		HANDLE_MSG(hWnd, WM_MOUSEMOVE,      obj->OnMouseMove);
		HANDLE_MSG(hWnd, WM_MOUSEWHEEL,     obj->OnMouseWheel);
		HANDLE_MSG(hWnd, WM_PAINT,          obj->OnPaint);
		HANDLE_MSG(hWnd, WM_SETCURSOR,      obj->OnSetCursor);
		HANDLE_MSG(hWnd, WM_SHOWWINDOW,     obj->OnShowWindow);
		HANDLE_MSG(hWnd, WM_SIZE,           obj->OnSize);
	}
	return FALSE;
}

// WM_CAPTURECHANGED handler
BOOL CPreview::OnCaptureChanged(HWND hWnd, HWND capture_reciever)
{
	mouse_captured = false;
	return FALSE;
}

// WM_COMMAND handler
BOOL CPreview::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
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
BOOL CPreview::OnDestroy(HWND hWnd)
{
	GetWindowRect(hWnd, &window_rect);
	PostQuitMessage(0);
	this->hWnd = NULL;
	return TRUE;
}

// WM_ERASEBKGND handler
BOOL CPreview::OnEraseBkgnd(HWND hWnd, HDC hDC)
{
	return TRUE;
}

// WM_INITDIALOG handler
BOOL CPreview::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	SetWindowLong(hWnd, DWL_USER, lParam);
	this->hWnd = hWnd;
	InitializeDevice();
	return TRUE;
}

// WM_KEYDOWN handler
BOOL CPreview::OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	// constants
	const float angle_h_delta(D3DX_PI / 64.0f);
	const float angle_v_delta(D3DX_PI / 128.0f);
	const float angle_h_min(0.0f);
	const float angle_h_max(2 * D3DX_PI);
	const float angle_v_min(0.0f);
	const float angle_v_max(D3DX_PI / 2.0f - 0.0001f);
	// key handling
	switch (vk)
	{
	case VK_TAB:
		{
			if (wireframe_mode)
			{
				wireframe_mode = false;
				device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
				device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			}
			else
			{
				wireframe_mode = true;
				device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
				device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			}
			Render();
		} break;
	case VK_DELETE:
		{
			// rotate left
			angle_h += angle_h_delta;
			if (angle_h >= angle_h_max)
				angle_h -= angle_h_max;
			MakeViewMatrix();
			Render();
		} break;
	case VK_NEXT:
		{
			// rotate right
			angle_h -= angle_h_delta;
			if (angle_h < angle_h_min)
				angle_h += angle_h_max;
			MakeViewMatrix();
			Render();
		} break;
	case VK_HOME:
		{
			// rotate down
			float new_angle = angle_v + angle_v_delta;
			if (new_angle > angle_v_max)
				new_angle = angle_v_max;
			angle_v = new_angle;
			MakeViewMatrix();
			Render();
		} break;
	case VK_END:
		{
			// rotate up
			float new_angle = angle_v - angle_v_delta;
			if (new_angle < angle_v_min)
				new_angle = angle_v_min;
			angle_v = new_angle;
			MakeViewMatrix();
			Render();
		} break;
	case VK_ADD:
		{
			radius *= 0.9f;
			if (radius <= 0.0f)
				radius = 0.1f;
			MakeViewMatrix();
			MakeProjectiveMatrix();
			Render();
		} break;
	case VK_SUBTRACT:
		{
			radius /= 0.9f;
			MakeViewMatrix();
			MakeProjectiveMatrix();
			Render();
		} break;
	}
	return FALSE;
}

// WM_LBUTTONDOWN handler
BOOL CPreview::OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	if (TRUE == fDoubleClick)
		return TRUE;
	SetCapture(hWnd);
	mouse_captured = true;
	mouse_click_pos.x = x;
	mouse_click_pos.y = y;
	old_target = target;
	return FALSE;
}

// WM_LBUTTON handler
BOOL CPreview::OnLButtonUp(HWND hWnd, int x, int y, UINT keyFlags)
{
	ReleaseCapture();
	return FALSE;
}

// WM_MOUSEMOVE handler
BOOL CPreview::OnMouseMove(HWND hWnd, int x, int y, UINT codeHitTest)
{
	if (mouse_captured)
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		const float width(static_cast<float>(rect.right - rect.left));
		const float height(static_cast<float>(rect.bottom - rect.top));
		const float shift_x(map_size.cx * ((x - mouse_click_pos.x) / width));
		const float shift_y(map_size.cy * ((y - mouse_click_pos.y) / height));
		const float proj(cos(angle_v) * radius);
		float dir_x(cos(angle_h) * proj);
		float dir_y(sin(angle_h) * proj);
		const float dir_length(sqrt(dir_x * dir_x + dir_y * dir_y));
		dir_x /= dir_length;
		dir_y /= dir_length;
		target.x = old_target.x - dir_y * shift_x - dir_x * shift_y;
		target.y = old_target.y + dir_x * shift_x - dir_y * shift_y;
		MakeViewMatrix();
		Render();
	}
	return FALSE;
}

// WM_MOUSEWHEEL handler
BOOL CPreview::OnMouseWheel(HWND hWnd, int xPos, int yPos, int zDelta, UINT fwKeys)
{
	radius *= pow(0.9f, zDelta / 120);
	if (radius <= 0.0f)
		radius = 0.1f;
	MakeViewMatrix();
	MakeProjectiveMatrix();
	Render();
	return FALSE;
}

// WM_PAINT handler
BOOL CPreview::OnPaint(HWND hWnd)
{
	Render();
	return FALSE;
}

// WM_SETCURSOR handler
BOOL CPreview::OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
	if (hWnd)
		return FALSE;
	SetCursor(is_busy ? LoadCursor(NULL, IDC_APPSTARTING) : ri_cast<HCURSOR>(GetClassLong(hWnd, GCL_HCURSOR)));
	return TRUE;
}

// WM_SHOWWINDOW handler
BOOL CPreview::OnShowWindow(HWND hWnd, BOOL fShow, UINT status)
{
	is_visible = (fShow == TRUE);
	Button_SetCheck(hButton, (TRUE == fShow) ? BST_CHECKED : BST_UNCHECKED);
	return TRUE;
}

// WM_SIZE handler
BOOL CPreview::OnSize(HWND hWnd, UINT state, int cx, int cy)
{
	InitializeDevice();
	if (map_info_valid)
		map_info->Update(0);
	Render();
	return FALSE;
}

// initialize D3D device and reset managed resources
void CPreview::InitializeDevice()
{
	HRESULT result;
	// get window dimensions
	RECT client_rect;
	GetClientRect(hWnd, &client_rect);
	// define presentation parameters
	D3DPRESENT_PARAMETERS d3d_params;
	ZeroMemory(&d3d_params, sizeof(d3d_params));
	d3d_params.Windowed = true;
	d3d_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d_params.EnableAutoDepthStencil = TRUE;
	d3d_params.AutoDepthStencilFormat = D3DFMT_D16;
	d3d_params.BackBufferWidth = client_rect.right;
	d3d_params.BackBufferHeight = client_rect.bottom;
	// create or reset the device
	if (NULL == device)
	{
		result = d3d->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			hWnd,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3d_params,
			&device);
		if (D3D_OK != result)
		{
			Error(_T("IDirect3D9::CreateDevice failed"));
			device = NULL;
			DestroyWindow(hWnd);
			return;
		}
	}
	else
	{
		if (NULL != active_vb)
			active_vb->Release();
		result = device->Reset(&d3d_params);
		if ((D3D_OK != result))
		{
			Error(_T("IDirect3DDevice9::Reset failed"));
			device = NULL;
			DestroyWindow(hWnd);
			return;
		}
	}
	// configure the device
	device->SetSamplerState(0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC);
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetRenderState(D3DRS_ZENABLE,  D3DZB_TRUE);
	device->SetRenderState(D3DRS_LIGHTING, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_FOGENABLE,    TRUE);
	device->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
	// set wireframe mode if needed
	if (wireframe_mode)
	{
		device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
		device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	}
	else
	{
		device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
		device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	}
}

// main render function
// designed for handling virtually unlimited detalization of the terrain mesh
void CPreview::Render()
{
	// prevent recursion
	static int recursion_count(0);
	const struct CCounter
	{
		CCounter(int *count) : count(count) { ++*count; }
		~CCounter() { --*count; }
	protected:
		int *count;
	} recursion_counter(&recursion_count);
	if (recursion_count > 1)
		return;
	// check preconditions
	if (NULL == device)
		return;
	// render
	bool repeat_render;
	do
	{
		repeat_render = false;
		HRESULT result;
		// clear the device
		if (FAILED(device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x003a6ea5, 1.0f, 0)))
			Error(_T("IDirect3DDevice9::Clear failed"));
		// set the matrices
		{
			CAutoCriticalSection auto_matrix_section(&matrix_section);
			if (FAILED(device->SetTransform(D3DTS_VIEW, &view_matrix)))
				Error(_T("IDirect3DDevice9::SetTransform failed"));
			if (FAILED(device->SetTransform(D3DTS_PROJECTION, &proj_matrix)))
				Error(_T("IDirect3DDevice9::SetTransform failed"));
		}
		CAutoCriticalSection auto_vb_section(&vb_section);
		CAutoCriticalSection auto_texture_section(&texture_section);
		CAutoCriticalSection auto_zero_layer_vb_section(&zero_layer_vb_section);
		// render the scene
		if (!FAILED(device->BeginScene()))
		{
			// stretch the scene
			{
				D3DXMATRIX stretch_matrix;
				D3DXMatrixScaling(&stretch_matrix, 1.0f, 1.0f, world_stretch);
				PushWorldMatrix(stretch_matrix);
			}
			if (texture_valid && heightmap_valid)
			// render terrain
			{
				// check preconditions
				_ASSERTE(!terrain_vbs.empty());
				_ASSERTE(!sections.empty());
				// prepare for rendering
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
				device->SetFVF(CVertex::FVF);
				// initialize vertex buffer iterators
						list<CVertexBuffer>::iterator       vb_i  (terrain_vbs.begin());
				const list<CVertexBuffer>::const_iterator vb_end(terrain_vbs.end());
				// initialise section iterators
						vector<CSection>::const_iterator sections_i  (sections.begin());
				const vector<CSection>::const_iterator sections_end(sections.end());
				// set initial remainder values
				UINT vb_remainder     (vb_i->_face_count);
				UINT section_remainder(sections_i->face_count);
				// set initial stream source and texture
				result = SetMeshStreamSource(vb_i);
				if (D3D_OK != result)
				{
					if (D3DERR_OUTOFVIDEOMEMORY != result)
					{
						PopWorldMatrix();
						DestroyWindow(hWnd);
						return;
					}
					// restart
					repeat_render = true;
					PopWorldMatrix();
					// end the scene
					if (FAILED(device->EndScene()))
						Error(_T("IDirect3DDevice9::EndScene failed"));
					continue;
				}
				if (D3D_OK != device->SetTexture(0, sections_i->texture))
				{
					Error(_T("IDIrect3DDevice9::SetTexture failed"));
					PopWorldMatrix();
					DestroyWindow(hWnd);
					return;
				}
				// main terrain render loop
				while (sections_i != sections_end)
				{
					_ASSERTE(vb_i != vb_end);
					UINT faces_to_draw;
					bool advanced_section(false);
					UINT vb_offet(vb_i->_face_count - vb_remainder);
					if (vb_remainder >= section_remainder)
					// if the buffer is sufficient to draw the current section, draw it
					{
						faces_to_draw = section_remainder;
						advanced_section = true;
						vb_remainder -= faces_to_draw;
					}
					else
					// otherwise draw as much as possible
					{
						faces_to_draw = vb_remainder;
						section_remainder -= faces_to_draw;
					}
					// actually draw now
					if (faces_to_draw > 0)
					{
						result = device->DrawPrimitive(
							D3DPT_TRIANGLELIST,
							vb_offet * 3,
							faces_to_draw);
						if (D3D_OK != result)
						{
							// check if the cause of failure was not memory shortage
							if (D3DERR_OUTOFVIDEOMEMORY != result)
							{
								Error(_T("IDirect3DDevice9::DrawPrimitive failed"));
								PopWorldMatrix();
								DestroyWindow(hWnd);
								return;
							}
							// restart
							repeat_render = true;
							break;
						}
					}
					// set new stream or texture
					if (advanced_section)
					{
						++sections_i;
						if (sections_i != sections_end)
						{
							section_remainder = sections_i->face_count;
							// set texture to the next section's
							if (D3D_OK != device->SetTexture(0, sections_i->texture))
							{
								Error(_T("IDIrect3DDevice9::SetTexture failed"));
								PopWorldMatrix();
								DestroyWindow(hWnd);
								return;
							}
						}
					}
					else
					{
						++vb_i;
						if (vb_i != vb_end)
						{
							vb_remainder = vb_i->_face_count;
							// set stream to the next vertex buffer's
							result = SetMeshStreamSource(vb_i);
							if (D3D_OK != result)
							{
								if (D3DERR_OUTOFVIDEOMEMORY != result)
								{
									PopWorldMatrix();
									DestroyWindow(hWnd);
									return;
								}
								// restart
								repeat_render = true;
								break;
							}
						}
					}
				}
				if (repeat_render)
				{
					PopWorldMatrix();
					// end the scene
					if (FAILED(device->EndScene()))
						Error(_T("IDirect3DDevice9::EndScene failed"));
					continue;
				}
			}
			if (map_info_valid)
			{
				// set zero layer height
				{
					D3DXMATRIX zero_layer_height;
					D3DXMatrixTranslation(&zero_layer_height, 0, 0, zero_plast);
					PushWorldMatrix(zero_layer_height);
				}
				// render the zero level
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device->SetFVF(CColouredVertex::FVF);
				device->SetTexture(0, NULL);
				if (D3D_OK != device->SetStreamSource(0, zero_layer_vb, 0, sizeof(CColouredVertex)))
					Error(_T("IDirect3DDevice9::SetStreamSource failed"));
				if (D3D_OK != device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2))
					Error(_T("IDirect3DDevice9::DrawPrimitive failed"));
				// pop heightmap matrix
				PopWorldMatrix();
				// render the frame markers
				{
					CAutoCriticalSection auto_billboard_section(&billboard_section);
					for (int i(0); i != 5; ++i)
					{
						// set marker position
						{
							D3DXMATRIX frame_marker_rotation;
							D3DXMatrixRotationZ(&frame_marker_rotation, billboards[i].angle);
							D3DXMATRIX frame_marker_position;
							D3DXMatrixTranslation(
								&frame_marker_position,
								billboards[i].position.x,
								billboards[i].position.y,
								0);
							D3DXMatrixMultiply(&frame_marker_position, &frame_marker_rotation, &frame_marker_position);
							PushWorldMatrix(frame_marker_position);
						}
						if (D3D_OK != device->SetStreamSource(0, billboards[i].vertices, 0, sizeof(CColouredVertex)))
							Error(_T("IDirect3DDevice9::SetStreamSource failed"));
						if (D3D_OK != device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 6))
							Error(_T("IDirect3DDevice9::DrawPrimitive failed"));
						// pop marker position matrix
						PopWorldMatrix();
					}
				}
			}
			// pop world stretch matrix
			PopWorldMatrix();
			// end the scene
			if (FAILED(device->EndScene()))
				Error(_T("IDirect3DDevice9::EndScene failed"));
		}
		else
			Error(_T("IDirect3DDevice9::BeginScene failed"));
		// present or reset
		result = device->Present(NULL, NULL, NULL, NULL);
		switch(result)
		{
		case D3DERR_DEVICELOST:
			InitializeDevice();
			repeat_render = true;
			break;
		case D3DERR_DRIVERINTERNALERROR:
			Error(_T("internal driver error when rendering"));
			DestroyWindow(hWnd);
			return;
		case D3DERR_INVALIDCALL:
			Error("InvalidCall");
			DestroyWindow(hWnd);
			return;
		}
	} while (repeat_render);
}

DWORD WINAPI CPreview::HeightmapThreadProc(LPVOID lpParameter)
{
	// start up
	CPreview *obj = ri_cast<CPreview*>(lpParameter);
	obj->ToggleWaitCursor(true);
	if (!obj->map_info_valid)
		SuspendThread(GetCurrentThread());
	clock_t start_time(clock());
	// release the vertex buffers
	obj->ReleaseTerrainVBs();
	// create a representation of the map
	obj->sections.resize((obj->map_size.cx / texture_width) * (obj->map_size.cy / texture_height));
	obj->BuildVB();
	// redraw
	InvalidateRect(obj->hWnd, NULL, TRUE);
	// wrap up
	obj->ToggleWaitCursor(false);
	clock_t end_time(clock());
	_RPT1(_CRT_WARN, "CPreview::HeightmapThreadProc: %i ticks\n", end_time - start_time);
	return 0;
}

DWORD WINAPI CPreview::MapInfoThreadProc(LPVOID lpParameter)
{
	// start up
	clock_t start_time(clock());
	CPreview *obj = ri_cast<CPreview*>(lpParameter);
	obj->ToggleWaitCursor(true);
	// check out map info
	const CMapInfo &map_data(obj->map_info->SignOutConst());
	// set temporary file path
	{
		TCHAR path[MAX_PATH];
		_tcscpy(path, map_data.folder_path.c_str());
		PathCombine(path, path, _T("~tmp_preview_data"));
		obj->tmp_file_path = path;
	}
	// store billboard positions
	{
		CAutoCriticalSection auto_billboard_section(&obj->billboard_section);
		for (int i(0); i != 5; ++i)
		{
			obj->billboards[i].position.x = static_cast<FLOAT>(map_data.start_pos[i].x);
			obj->billboards[i].position.y = static_cast<FLOAT>(map_data.start_pos[i].y);
		}
	}
	// work with size old_map_size(map_size);
	SIZE old_map_size(obj->map_size);
	obj->map_size.cx = obj->exp2(map_data.map_power_x);
	obj->map_size.cy = obj->exp2(map_data.map_power_y);
	if (old_map_size.cx != obj->map_size.cx || old_map_size.cy != obj->map_size.cy)
	{
		// reset camera
		CAutoCriticalSection auto_matrix_section(&obj->matrix_section);
		obj->target.x = obj->map_size.cx / 2.0f;
		obj->target.y = obj->map_size.cy / 2.0f;
		obj->target.z = 0;
		obj->angle_h = D3DX_PI / 2;
		obj->angle_v = D3DX_PI / 4.0f;
		obj->radius = static_cast<float>(max(obj->map_size.cx, obj->map_size.cy));
		obj->MakeViewMatrix();
		obj->MakeProjectiveMatrix();
	}
	// initialize vertex buffers
	if (!obj->map_info_valid)
	{
		// zero layer
		CAutoCriticalSection auto_zero_layer_vb_section(&obj->zero_layer_vb_section);
		_ASSERTE(NULL == obj->zero_layer_vb);
		if (FAILED(obj->device->CreateVertexBuffer(
			4 * sizeof(CColouredVertex),
			0,
			CColouredVertex::FVF,
			D3DPOOL_MANAGED,
			&obj->zero_layer_vb,
			NULL)))
			obj->Error(_T("IDirect3DDevice9::CreateVertexBuffer failed"));
		obj->zero_layer_vb->PreLoad();
		obj->BuildZeroLayerVB();
		// frame markers
		{
			CAutoCriticalSection auto_billboard_section(&obj->billboard_section);
			for (int i(0); i != 5; ++i)
			{
				// initialize buffer
				_ASSERTE(NULL == obj->billboards[i].vertices);
				if (FAILED(obj->device->CreateVertexBuffer(
					8 * sizeof(CColouredVertex),
					0,
					CColouredVertex::FVF,
					D3DPOOL_MANAGED,
					&obj->billboards[i].vertices,
					NULL)))
					obj->Error(_T("IDirect3DDevice9::CreateVertexBuffer failed"));
				// determine colour
				D3DCOLOR marker_colour;
				if (0 == i)
					marker_colour = D3DCOLOR_ARGB(0xFF, 0xFF, 0x00, 0x00);
				else
					marker_colour = D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00);
				// fill the vertex buffer
				obj->BuildFrameMarker(obj->billboards + i, marker_colour);
			}
		}
	}
	// store zero layer height
	obj->zero_plast = map_data.zero_plast * obj->world_stretch;
	// set fog parameters
	{
		FLOAT fog_start(static_cast<FLOAT>(map_data.fog_start));
		FLOAT fog_end  (static_cast<FLOAT>(map_data.fog_end));
		obj->device->SetRenderState(
			D3DRS_FOGCOLOR,
			D3DCOLOR_ARGB(0, GetRValue(map_data.fog_colour), GetGValue(map_data.fog_colour), GetBValue(map_data.fog_colour)));
		obj->device->SetRenderState(D3DRS_FOGSTART, *(ri_cast<DWORD*>(&fog_start)));
		obj->device->SetRenderState(D3DRS_FOGEND,   *(ri_cast<DWORD*>(&fog_end)));
		obj->map_info_valid = true;
	}
	// check in map info
	obj->map_info->SignIn();
	// resume the other threads
	ResumeThread(obj->heightmap_thread);
	ResumeThread(obj->texture_thread);
	// redraw
	obj->MakeViewMatrix();
	InvalidateRect(obj->hWnd, NULL, TRUE);
	// wrap up
	obj->ToggleWaitCursor(false);
	clock_t end_time(clock());
	_RPT1(_CRT_WARN, "CPreview::TextureThreadProc: %i ticks\n", end_time - start_time);
	return 0;
}

DWORD WINAPI CPreview::TextureThreadProc(LPVOID lpParameter)
{
	// start up
	clock_t start_time(clock());
	CPreview *obj = ri_cast<CPreview*>(lpParameter);
	if (!obj->map_info_valid)
		SuspendThread(GetCurrentThread());
	obj->ToggleWaitCursor(true);
	// set up for rendering
	obj->InitializeTextures();
	// redraw
	InvalidateRect(obj->hWnd, NULL, TRUE);
	// wrap up
	obj->ToggleWaitCursor(false);
	clock_t end_time(clock());
	_RPT1(_CRT_WARN, "CPreview::TextureThreadProc: %i ticks\n", end_time - start_time);
	return 0;
}

DWORD WINAPI CPreview::UsageTracker(LPVOID lpParameter)
{
	const time_to_wait(16 * 1000); // in milliseconds
	const time_to_sleep(1 * 1000); // in milliseconds
	CPreview *obj = ri_cast<CPreview*>(lpParameter);
	while (obj->track_usage)
	{
		// check if data is in memory or if Render was called recently
		if (INVALID_HANDLE_VALUE != obj->tmp_file || clock() - obj->last_vb_access <= time_to_wait)
		// if so - sleep
			Sleep(time_to_sleep);
		else
		// else - move data to disk
		{
			_RPT0(_CRT_WARN, "• writing file\n");
			CAutoCriticalSection auto_vb_section(&obj->vb_section);
			// create file
			{
				const DWORD dwDesiredAccess       = GENERIC_READ | GENERIC_WRITE;
				const DWORD dwShareMode           = FILE_SHARE_READ;
				const DWORD dwCreationDisposition = CREATE_ALWAYS;
				const DWORD dwFlagsAndAttributes  = /*FILE_ATTRIBUTE_TEMPORARY | */FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_DELETE_ON_CLOSE;
				if (INVALID_HANDLE_VALUE != obj->tmp_file)
					CloseHandle(obj->tmp_file);
				obj->tmp_file = CreateFile(
					obj->tmp_file_path.c_str(),
					dwDesiredAccess,
					dwShareMode,
					NULL,
					dwCreationDisposition,
					dwFlagsAndAttributes,
					NULL);
				if (INVALID_HANDLE_VALUE == obj->tmp_file)
				{
					obj->Error(_T("Temporary file could not be created. Performance will be compromised."));
					return 0;
				}
			}
			// write data to the file
			{
				      list<CVertexBuffer>::const_iterator i(obj->terrain_vbs.begin());
				const list<CVertexBuffer>::const_iterator end(obj->terrain_vbs.end());
				for (; i != end; ++i)
				{
					DWORD bytes_to_write;
					DWORD num_bytes_written;
					// write vertices
					bytes_to_write = i->_face_count * 3 * sizeof(CVertex);
					if (FALSE == WriteFile(
						obj->tmp_file,
						i->_vertices,
						bytes_to_write,
						&num_bytes_written,
						NULL) || bytes_to_write != num_bytes_written)
					{
						obj->Error(_T("Temporary file could not be written to. Performance will be compromised."));
						return 0;
					}
				}
			}
			// delete buffers
			obj->DeleteTerainVBs();
		}
	}
	return 0;
}

void CPreview::ActivateTerrainVBs()
{
	if (INVALID_HANDLE_VALUE != tmp_file)
	{
		_RPT0(_CRT_WARN, "• reading file\n");
		SetFilePointer(tmp_file, 0, NULL, FILE_BEGIN);
		      list<CVertexBuffer>::iterator       i(terrain_vbs.begin());
		const list<CVertexBuffer>::const_iterator end(terrain_vbs.end());
		DWORD num_bytes_read;
		DWORD bytes_to_read;
		for (; i != end; ++i)
		{
			// read the number of faces
			bytes_to_read = i->_face_count * 3 * sizeof(CVertex);
			i->_vertices = new CVertex[i->_face_count * 3];
			_ASSERTE(i->_vertices != NULL);
			if (FALSE == ReadFile(
				tmp_file,
				i->_vertices,
				bytes_to_read,
				&num_bytes_read,
				NULL) || bytes_to_read != num_bytes_read)
			{
				Error(_T("Temporary file could not be read from."));
				break;
			}
		}
		CloseHandle(tmp_file);
		tmp_file = INVALID_HANDLE_VALUE;
	}
}

void CPreview::BuildFrameMarker(CBillboard *marker, D3DCOLOR marker_colour)
{
	CColouredVertex *vertices;
	if (FAILED(marker->vertices->Lock(0, 0, ri_cast<void**>(&vertices), D3DLOCK_DISCARD)))
		Error(_T("IDirect3DVertexBuffer9::Lock failed"));
	const FLOAT radius(6.0f);
	const FLOAT height(512.0f);
	vertices[0].x     = -radius;
	vertices[0].y     = 0.0f;
	vertices[0].z     = 0.0f;
	vertices[0].color = marker_colour & D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0xFF);
	vertices[1].x     = -radius;
	vertices[1].y     = 0.0f;
	vertices[1].z     = height;
	vertices[1].color = marker_colour & D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0xFF);
	vertices[2].x     = -radius / 4;
	vertices[2].y     = 0.0f;
	vertices[2].z     = 0.0f;
	vertices[2].color = marker_colour | D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);
	vertices[3].x     = 0.0f;
	vertices[3].y     = 0.0f;
	vertices[3].z     = height;
	vertices[3].color = marker_colour & D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0xFF);
	vertices[4].x     = radius / 4;
	vertices[4].y     = 0.0f;
	vertices[4].z     = 0.0f;
	vertices[4].color = marker_colour | D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);
	vertices[5].x     = 0.0f;
	vertices[5].y     = 0.0f;
	vertices[5].z     = height;
	vertices[5].color = marker_colour & D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0xFF);
	vertices[6].x     = radius;
	vertices[6].y     = 0.0f;
	vertices[6].z     = 0.0f;
	vertices[6].color = marker_colour & D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0xFF);
	vertices[7].x     = radius;
	vertices[7].y     = 0.0f;
	vertices[7].z     = height;
	vertices[7].color = marker_colour & D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0xFF);
	zero_layer_vb->Unlock();
}

void CPreview::BuildZeroLayerVB()
{
	settings.SignOut();
	CColouredVertex *vertices;
	if (FAILED(zero_layer_vb->Lock(0, 0, ri_cast<void**>(&vertices), D3DLOCK_DISCARD)))
		Error(_T("IDirect3DVertexBuffer9::Lock failed"));
	vertices[0].x     = 0.0f;
	vertices[0].y     = 0.0f;
	vertices[0].z     = 0.0f;
	vertices[0].color = settings.zero_layer_colour;
	vertices[1].x     = static_cast<FLOAT>(map_size.cx);	
	vertices[1].y     = 0.0f;
	vertices[1].z     = 0.0f;
	vertices[1].color =  settings.zero_layer_colour;
	vertices[2].x     = static_cast<FLOAT>(0);	
	vertices[2].y     = static_cast<FLOAT>(map_size.cy);
	vertices[2].z     = 0.0f;
	vertices[2].color =  settings.zero_layer_colour;
	vertices[3].x     = static_cast<FLOAT>(map_size.cx);	
	vertices[3].y     = static_cast<FLOAT>(map_size.cy);
	vertices[3].z     = 0.0f;
	vertices[3].color =  settings.zero_layer_colour;
	zero_layer_vb->Unlock();
	settings.SignIn();
}

void CPreview::BuildVB()
{
	vector<CSimpleVertex> vertices;
	Triangulate(vertices);
	// change window title
	{
		TCHAR poly_num_str[16];
		_itot(vertices.size() / 3, poly_num_str, 10);
		string window_text = "Map Preview (";
		window_text.append(poly_num_str);
		window_text.append(" polygons)");
		SetWindowText(hWnd, window_text.c_str());
	}
	// set up for rendering
	InitializeVB(vertices);
}

void CPreview::DeleteTerainVBs()
{
			list<CVertexBuffer>::iterator       i(terrain_vbs.begin());
	const list<CVertexBuffer>::const_iterator end(terrain_vbs.end());
	for (; i != end; ++i)
		if (NULL != i->_vertices)
		{
			delete [] i->_vertices;
			i->_vertices = NULL;
		}
}

float CPreview::Flatness(int h1, int h2, int v1, int v2, int c, int r) const
{
	float dx;
	float dy;
	float c_dy;
	float length;
	float cnst;
	// project c onto h2 - h1
	dx   = static_cast<float>(r + r);
	dy   = static_cast<float>(h2 - h1);
	c_dy = static_cast<float>(c - h1);
	length = sqrt(dx * dx + dy * dy);
	dx /= length;
	dy /= length;
	cnst = (r * dx + c_dy * dy) / (dx * dx + dy * dy);
	float h_proj_x = r - cnst * dx;
	float h_proj_y = c_dy - cnst * dy;
	// project c onto v2 - v1
	dx   = static_cast<float>(r + r);
	dy   = static_cast<float>(v2 - v1);
	c_dy = static_cast<float>(c - v1);
	length = sqrt(dx * dx + dy * dy);
	dx /= length;
	dy /= length;
	cnst = (r * dx + c_dy * dy) / (dx * dx + dy * dy);
	float v_proj_x = r - cnst * dx;
	float v_proj_y = c_dy - cnst * dy;
	// compute the length of the distance between the projections
	float proj_dy = v_proj_y + h_proj_y;
	float distance = h_proj_x * h_proj_x + v_proj_x * v_proj_x + proj_dy * proj_dy;
	return distance;
}

// initialize the textures
void CPreview::InitializeTextures()
{
	// check preconditions
	_ASSERTE(map_size.cx >= texture_width);
	_ASSERTE(map_size.cy >= texture_height);
	_ASSERTE(map_size.cx % texture_width  == 0);
	_ASSERTE(map_size.cy % texture_height == 0);
	// get lightmap data, if necessary
	const bool enable_lighting(settings.enable_lighting);
	const BYTE *lightmap_ptr;
	if (enable_lighting)
	{
		while (!lightmap->IsValid())
			Sleep(200);
		lightmap_ptr = lightmap->SignOutConst().ptr;
	}
	// compute constants and variables
	const CPalettedTexture &texture_data(texture->SignOutConst());
	const COLORREF *palette(texture_data.palette);
	const int x_num(map_size.cx / texture_width);
	const int y_num(map_size.cy / texture_height);
	int texture_index(0);
	int initial_offset(0);
	// main loop
	CAutoCriticalSection auto_texture_section(&texture_section);
	for (int texture_y(0); texture_y != y_num; ++texture_y)
	{
		for (int texture_x(0); texture_x != x_num; ++texture_x)
		{
			// create the texture and get its data
			if (FAILED(device->CreateTexture(
				texture_width,
				texture_height,
				0,
				D3DUSAGE_AUTOGENMIPMAP,
				D3DFMT_A8R8G8B8,
				D3DPOOL_MANAGED,
            &sections[texture_index].texture,
				NULL)))
			{
				Error(_T("IDirect3DDevice9::CreateTexture failed"));
				return;
			}
			D3DLOCKED_RECT rect;
			if (FAILED(sections[texture_index].texture->LockRect(0, &rect, NULL, 0)) ||
				 rect.Pitch != 4 * texture_width)
			{
				Error(_T("IDirect3DTexture9::LockRect failed"));
				return;
			};
			BYTE *texture_iterator(ri_cast<BYTE*>(rect.pBits));
			// copy texture data
			int offset(initial_offset);
			for (int y(0); y != texture_height; ++y)
			{
				for (int x(0); x != texture_width; ++x)
				{
					const COLORREF colour(palette[texture_data.ptr[offset]]);
					if (!enable_lighting)
					{
						texture_iterator[3] = 0xFF; // alpha
						texture_iterator[2] = GetRValue(colour);
						texture_iterator[1] = GetGValue(colour);
						texture_iterator[0] = GetBValue(colour);
					}
					else
					{
						float f_light(static_cast<float>(lightmap_ptr[offset]));
						f_light = (f_light + 128.0f) / 255.0f;
						texture_iterator[3] = 0xFF; // alpha
						texture_iterator[2] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetRValue(colour) * f_light)));
						texture_iterator[1] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetGValue(colour) * f_light)));
						texture_iterator[0] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetBValue(colour) * f_light)));
					}
					++offset;
					texture_iterator += 4;
				}
				offset += map_size.cx - texture_width;
			}
			if (FAILED(sections[texture_index].texture->UnlockRect(0)))
			{
				Error(_T("IDirect3DTexture9::UnlockRect failed"));
				return;
			};
			sections[texture_index].texture->PreLoad();
			// prepare for the next cycle
			initial_offset += texture_width;
			++texture_index;
		}
		initial_offset += map_size.cx * (texture_height - 1);
	}
	// check in resources
	texture->SignIn();
	texture_valid = true;
	if (enable_lighting)
		lightmap->SignIn();
}

// sort the vertices by texture and initialize the vertex buffer
void CPreview::InitializeVB(vector<CSimpleVertex> &vertices)
{
	// handle the case of vertices.size() == 0
	if (0 == vertices.size())
	{
		heightmap_valid = false;
		return;
	}
	// preconditions
	_ASSERTE(vertices.size() % 6 == 0); // should be true after Triangulate is done
	_ASSERTE(!sections.empty());
	// reset face counts
	{
		vector<CSection>::iterator i(sections.begin());
		const vector<CSection>::const_iterator end(sections.end());
		for (; i != end; ++i)
			i->face_count = 0;
	}
	// count the number of faces in each section
	// assume that each quad consists of two faces and lies in one section
	const int textures_per_row(map_size.cx / texture_width);
	{
		vector<CSimpleVertex>::const_iterator i(vertices.begin());
		const vector<CSimpleVertex>::const_iterator end(vertices.end());
		for (;  i != end; i += 6)
		{
			const int section_x(static_cast<int>(i->x / texture_width));
			const int section_y(static_cast<int>(i->y / texture_height));
			const int section_index(section_y * textures_per_row + section_x);
			sections[section_index].face_count += 2;
		}
	}
	// generate vertex offsets
	{
		vector<CSection>::iterator i(sections.begin() + 1);
		const vector<CSection>::const_iterator end(sections.end());
		for (; i != end; ++i)
			i->vertex_offset = (i - 1)->face_count * 3;
		sections[0].vertex_offset = 0;
		for (i = sections.begin() + 1; i != end; ++i)
			i->vertex_offset += (i - 1)->vertex_offset;
	}
	// initialize the vertex buffer
	{
		CAutoCriticalSection auto_vb_section(&vb_section);
		// create a new buffer
		CVertexBuffer vb;
		vb._face_count = vertices.size() / 3;
		vb._vertices = new CVertex[vertices.size()];
		// initialize section iterators to vertex offsets
		vector<int> section_iters(sections.size());
		for (size_t i(0); i != sections.size(); ++i)
			section_iters[i] = sections[i].vertex_offset;
		// sort the vertices by texture
		{
			vector<CSimpleVertex>::const_iterator quad(vertices.begin());
			const vector<CSimpleVertex>::const_iterator end(vertices.end());
			for (; quad != end; quad += 6)
			{
				// find postition to insert the quad into
				const int section_x(static_cast<int>(quad->x / texture_width));
				const int section_y(static_cast<int>(quad->y / texture_height));
				const int section_index(section_y * textures_per_row + section_x);
				int offset(section_iters[section_index]);
				// insert each vertex of the quad
				for (int i(0); i != 6; ++i)
				{
					const CSimpleVertex &simple_vertex(*(quad + i));
					CVertex &vertex(vb._vertices[offset++]);
					vertex.x = simple_vertex.x;
					vertex.y = simple_vertex.y;
					vertex.z = simple_vertex.z;
					vertex.u = (vertex.x - section_x * texture_width)  / texture_width;
					vertex.v = (vertex.y - section_y * texture_height) / texture_height;
				}
				section_iters[section_index] = offset;
			}
		}
		CloseHandle(tmp_file);
		tmp_file = INVALID_HANDLE_VALUE;
		DeleteTerainVBs();
		terrain_vbs.push_back(vb);
		last_vb_access = clock();
	}
	// set up for rendering
	active_vb       = NULL;
	active_vb_copy  = NULL;
	if (!heightmap_valid)
	{
		DWORD thread_id;
		CreateThread(NULL, 0, UsageTracker, this, 0, &thread_id);
	}
	heightmap_valid = true;
}

void CPreview::MakeViewMatrix()
{
	D3DXVECTOR3 eye; // used in both the view and the billboard angle calculations
	// create the view matrix
	{
		CAutoCriticalSection auto_matrix_section(&matrix_section);
		float proj(cos(angle_v) * radius);
		eye = D3DXVECTOR3(cos(angle_h) * proj, sin(angle_h) * proj, sin(angle_v) * radius);
		eye += target;
		D3DXVECTOR3 up(-eye.x, -eye.y, 1.0f);
		up += target;
		D3DXMatrixLookAtLH(&view_matrix, &eye, &target, &up);
	}
	// set billboard angles
	{
		CAutoCriticalSection auto_billboard_section(&billboard_section);
		for (int i(0); i != 5; ++i)
			billboards[i].CalculateAngle(eye.x, eye.y);
	}
}

void CPreview::MakeProjectiveMatrix()
{
	// get window size
	RECT client_rect;
	GetClientRect(hWnd, &client_rect);
	// set the projection matrix
	int dim = max(map_size.cx, map_size.cy);
	D3DXMatrixPerspectiveFovLH(
		&proj_matrix, D3DX_PI / 3.0f,
		static_cast<FLOAT>(client_rect.right / client_rect.bottom),
		max(10.0f, radius - dim),
		radius + dim);
}

void CPreview::PopWorldMatrix()
{
	_ASSERTE(!world_matrix_stack.empty());
	world_matrix_stack.pop();
	device->SetTransform(D3DTS_WORLD, &world_matrix_stack.top());
}

void CPreview::PushWorldMatrix(const D3DXMATRIX &matrix)
{
	_ASSERTE(!world_matrix_stack.empty());
	D3DXMATRIX new_matrix;
	D3DXMatrixMultiply(&new_matrix, &world_matrix_stack.top(), &matrix);
	device->SetTransform(D3DTS_WORLD, &new_matrix);
	world_matrix_stack.push(new_matrix);
}

void CPreview::ReleaseTerrainVBs()
{
	heightmap_valid = false;
	list<CVertexBuffer>::iterator i(terrain_vbs.begin());
	const list<CVertexBuffer>::const_iterator end(terrain_vbs.end());
	for (; i != end; ++i)
		if (NULL != i->_vertices)
			delete [] i->_vertices;
	terrain_vbs.clear();
}

HRESULT CPreview::SetMeshStreamSource(list<CVertexBuffer>::iterator vb)
{
	HRESULT result;
	// if the given buffer is not in video memory, load it
	if (&*vb != &*active_vb_copy)
	{
		if (NULL != active_vb)
			active_vb->Release();
		const size_t bytes_in_buffer(vb->_face_count * 3 * sizeof(CVertex));
		result = device->CreateVertexBuffer(
			bytes_in_buffer,
			D3DUSAGE_WRITEONLY,
			CVertex::FVF,
			D3DPOOL_DEFAULT,
			&active_vb,
			NULL);
		if (D3D_OK != result)
		{
			// check if the cause of failure was not memory shortage
			if (D3DERR_OUTOFVIDEOMEMORY != result)
			{
				Error(_T("IDirect3DDevice9::SetStreamSource failed"));
				return result;
			}
			SplitBuffer(vb);
			return result;
		}
		// make sure the vertex data is in system memory
		ActivateTerrainVBs();
		// feed the vertex data into the buffer
		CVertex *vertex_array;
		result = active_vb->Lock(0, 0, ri_cast<void**>(&vertex_array), D3DLOCK_DISCARD);
		if (D3D_OK != result)
		{
			Error(_T("IDirect3DVertexBuffer9::Lock failed"));
			return result;
		}
		CopyMemory(vertex_array, vb->_vertices, bytes_in_buffer);
		// unlock the buffer
		active_vb->Unlock();
		// remember which copy is active
		active_vb_copy = &*vb;
	}
	// set stream source to the buffer in memory
	result = device->SetStreamSource(0, active_vb, 0, sizeof(CVertex));
	if (D3D_OK != result)
	{
		Error(_T("IDirect3DDevice9::SetStreamSource failed"));
		return result;
	}
	last_vb_access = clock();
	return D3D_OK;
}

void CPreview::SplitBuffer(std::list<CVertexBuffer>::iterator vb)
{
	// load buffers from the file
	ActivateTerrainVBs();
	// create new buffer #1
	CVertexBuffer vb1;
	vb1._face_count = vb->_face_count / 2;
	vb1._vertices   = new CVertex[vb1._face_count * 3];
	// create new buffer #2
	CVertexBuffer vb2;
	vb2._face_count = vb->_face_count - vb1._face_count;
	vb2._vertices   = new CVertex[vb2._face_count * 3];
	// copy half of the vertices into the first vertex buffer
	CopyMemory(
		vb1._vertices,
		vb->_vertices,
		vb1._face_count * 3 * sizeof(CVertex));
	// copy the remaining vertices into the second vertex buffer
	CopyMemory(
		vb2._vertices,
		vb->_vertices + vb1._face_count * 3,
		vb2._face_count * 3 * sizeof(CVertex));
	// insert the new buffers into the list in place of the old one
	delete [] vb->_vertices;
	terrain_vbs.insert(terrain_vbs.insert(terrain_vbs.erase(vb), vb2), vb1);
	last_vb_access = clock();
}

void CPreview::ToggleWaitCursor(bool on)
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

// generate an optimized triangle list from the heightmap
void CPreview::Triangulate(vector<CSimpleVertex> &vertices)
{
	const CStaticArray<BYTE> &heightmap_data(heightmap->SignOutConst());
	int n;
	// check preconditions
	{
		const CMapInfo &map_data(map_info->SignOutConst());
		n = min(map_data.map_power_x, map_data.map_power_y);
		_ASSERTE(n >= 2); // algorithm requirement
		_ASSERTE(heightmap_data.length == static_cast<size_t>((map_size.cx + 1) * (map_size.cy + 1)));
		_ASSERTE(map_data.map_power_x < 14);
		_ASSERTE(map_data.map_power_y < 14);
		_ASSERTE(map_data.map_power_x == map_data.map_power_x); // TODO: add the capability of handling non-square maps
		map_info->SignIn();
	}
	// allocate and initialize storage for the algorithm
	vector<bool> *regions(new vector<bool>[n - 1]);
	{
		int num(2);
		for (int i(n - 2); i != -1; --i)
		{
			regions[i].resize(num * num);
			num *= 2;
		}
	}
	// determine flatness of map regions of increasing size
	{
		settings.SignOut();
		const float threshold(settings.threshold * settings.threshold); // settings really set sqrt(threshold)
		settings.SignIn();
		int index(0);
		// base case (k = 1)
		for (int xi(0); xi != map_size.cx / 2; ++xi)
		{
			for (int yi(0); yi != map_size.cy / 2; ++yi)
			{
				int offset = xi * (map_size.cx + 1) * 2 + (map_size.cx + 1) + yi * 2 + 1;
				// stop if any of the underlying points are zero
				for (int x(-1); x != 2; ++x)
					for (int y(-1); y != 2; ++y)
						if (heightmap_data.ptr[offset + y * (map_size.cx + 1) + x] == 0)
							goto end;
				// check if the region is flat enough
				if (Flatness(
					(int)heightmap_data.ptr[offset - 1],
					(int)heightmap_data.ptr[offset + 1],
					(int)heightmap_data.ptr[offset - map_size.cx - 1],
					(int)heightmap_data.ptr[offset + map_size.cx + 1],
					(int)heightmap_data.ptr[offset],
					1) < threshold)
					regions[0][index] = true;
			end:
				++index;
			}
		}
		// general case
		for (int k(2); k != n; ++k)
		{
			index = 0;
			int num(exp2(n - k));
			for (int xi(0); xi != num; ++xi)
			{
				for (int yi(0); yi != num; ++yi, ++index)
				{
					// stop if any of the underlying regions are false
					int prev_offset(2 * (2 * num * xi + yi));
					if (
						!regions[k - 2][prev_offset]           ||
						!regions[k - 2][prev_offset + 1]       ||
						!regions[k - 2][prev_offset + num * 2] ||
						!regions[k - 2][prev_offset + num * 2 + 1])
						continue;
					// check if the region is flat enough
					int radius(exp2(k - 1)); // horizontal distance from center to the edge
					int side(radius + radius); // length of a side of the region
					int offset((map_size.cx + 1) * (side * xi + radius) + side * yi + radius);
					if (Flatness(
						(int)heightmap_data.ptr[offset - radius],
						(int)heightmap_data.ptr[offset + radius],
						(int)heightmap_data.ptr[offset - (map_size.cx + 1) * radius],
						(int)heightmap_data.ptr[offset + (map_size.cx + 1) * radius],
						(int)heightmap_data.ptr[offset],
						radius) < threshold)
					{
						regions[k - 1][index] = true;
						// set the underlying regions to false
						regions[k - 2][prev_offset]               = false;
						regions[k - 2][prev_offset + 1]           = false;
						regions[k - 2][prev_offset + num * 2]     = false;
						regions[k - 2][prev_offset + num * 2 + 1] = false;
					}
				}
			}
		}
		// build the mesh
		{
			// we only need information for two levels at a time,
			//  so a recursive approach would be wasteful
			vector<CTE>  queue1;
			vector<CTE>  queue2;
			vector<CTE> *current(&queue1);
			vector<CTE> *queued (&queue2);
			vector<CTE> *temp_array;
			// add the first elements to the current queue
			for (int i(0); i != 4; ++i)
			{
				CTE te;
				te.index = i;
				current->push_back(te);
			}
			short num; // number of regions per row or column of the heightmap
			// heightmap values underlying the region
			float tl_val, tr_val;
			float bl_val, br_val;
			// corresponding heightmap values
			CSimpleVertex tl_vert, tr_vert;
			CSimpleVertex bl_vert, br_vert;
			// cartesian components of an index
			short xi, yi;
			// iterate through the hierarchy
			for (int k(n - 2); k != 0; --k)
			{
				num = static_cast<short>(exp2(n - k - 1));
				short side(static_cast<short>(exp2(k + 1))); // length of the region's side
				vector<CTE>::const_iterator te_i(current->begin());
				const vector<CTE>::const_iterator te_end(current->end());
				for (; te_i != te_end; ++te_i)
				{
					const CTE te(*te_i);
					// split the index into cartesian coordinates
					xi = static_cast<short>(te.index % num);
					yi = static_cast<short>(te.index / num);
					// calculate sides of the region
					short l_coord(static_cast<short>(xi * side));
					short r_coord(static_cast<short>(l_coord + side));
					short b_coord(static_cast<short>(yi * side));
					short t_coord(static_cast<short>(b_coord + side));
					// calculate heightmap values at the corners of the region
					{
						BYTE *offset(heightmap_data.ptr + side * ((map_size.cx + 1) * yi + xi));
						bl_val = *offset;
						br_val = *(offset + side);
						offset += side * (map_size.cx + 1);
						tl_val = *offset;
						tr_val = *(offset + side);
					}
					// constrain the heightmap values, if necessary
					// bottom left
					if (te.l_constrained && te.l_constraint.start != b_coord)
						bl_val = te.l_constraint.ConstrainedVal(b_coord);
					if (te.b_constrained && te.b_constraint.start != l_coord)
						bl_val = te.b_constraint.ConstrainedVal(l_coord);
					// bottom right
					if (te.r_constrained && te.r_constraint.start != b_coord)
						br_val = te.r_constraint.ConstrainedVal(b_coord);
					if (te.b_constrained && (te.b_constraint.start + te.b_constraint.length) != r_coord)
						br_val = te.b_constraint.ConstrainedVal(r_coord);
					// top left
					if (te.l_constrained && (te.l_constraint.start + te.l_constraint.length) != t_coord)
						tl_val = te.l_constraint.ConstrainedVal(t_coord);
					if (te.t_constrained && te.t_constraint.start != l_coord)
						tl_val = te.t_constraint.ConstrainedVal(l_coord);
					// top right
					if (te.r_constrained && (te.r_constraint.start + te.r_constraint.length) != t_coord)
						tr_val = te.r_constraint.ConstrainedVal(t_coord);
					if (te.t_constrained && (te.t_constraint.start + te.t_constraint.length) != r_coord)
						tr_val = te.t_constraint.ConstrainedVal(r_coord);
					// if the region is flat, add it to the triangulation and continue
					if (regions[k][te.index])
					{
						// bottom left vertex
						bl_vert.x = l_coord;
						bl_vert.y = b_coord;
						bl_vert.z = bl_val;
						// bottom right vertex
						br_vert.x = r_coord;
						br_vert.y = b_coord;
						br_vert.z = br_val;
						// top left vertex
						tl_vert.x = l_coord;
						tl_vert.y = t_coord;
						tl_vert.z = tl_val;
						// top right vertex
						tr_vert.x = r_coord;
						tr_vert.y = t_coord;
						tr_vert.z = tr_val;
						// add the vertices to the triangulation (counter-clockwise)
						// TODO: try using indexes
						vertices.push_back(bl_vert);
						vertices.push_back(br_vert);
						vertices.push_back(tr_vert);
						vertices.push_back(bl_vert);
						vertices.push_back(tr_vert);
						vertices.push_back(tl_vert);
						continue;
					}
					// check if each of the region's neighbours exists (relies on shortcircuiting)
					bool l_constrained((xi != 0)       && regions[k][te.index - 1]);
					bool r_constrained((xi != num - 1) && regions[k][te.index + 1]);
					bool b_constrained((yi != 0)       && regions[k][te.index - num]);
					bool t_constrained((yi != num - 1) && regions[k][te.index + num]);
					// locate subregions
					CTE bl_te, br_te, tl_te, tr_te;
					{
						int offset(4 * te.index - 2 * xi);
						bl_te.index = offset;
						br_te.index = offset + 1;
						tl_te.index = offset + num * 2;
						tr_te.index = offset + num * 2 + 1;
					}
					// copy constraints
					if (te.l_constrained) // left
					{
						_ASSERTE(!l_constrained);
						bl_te.l_constrained = tl_te.l_constrained = true;
						bl_te.l_constraint  = tl_te.l_constraint  = te.l_constraint;
					}
					if (te.r_constrained) // right
					{
						_ASSERTE(!r_constrained);
						br_te.r_constrained = tr_te.r_constrained = true;
						br_te.r_constraint  = tr_te.r_constraint  = te.r_constraint;
					}
					if (te.b_constrained) // bottom
					{
						_ASSERTE(!b_constrained);
						bl_te.b_constrained = br_te.b_constrained = true;
						bl_te.b_constraint  = br_te.b_constraint  = te.b_constraint;
					}
					if (te.t_constrained) // top
					{
						_ASSERTE(!t_constrained);
						tl_te.t_constrained = tr_te.t_constrained = true;
						tl_te.t_constraint  = tr_te.t_constraint  = te.t_constraint;
					}
					// add the constraints
					if (l_constrained) // left
					{
						// constrain the bottom left region and the top left region
						_ASSERTE(bl_te.l_constrained == false);
						_ASSERTE(tl_te.l_constrained == false);
						bl_te.l_constrained       = tl_te.l_constrained       = true;
						bl_te.l_constraint.start  = tl_te.l_constraint.start  = b_coord;
						bl_te.l_constraint.length = tl_te.l_constraint.length = side;
						bl_te.l_constraint.s_val  = tl_te.l_constraint.s_val  = bl_val;
						bl_te.l_constraint.f_val  = tl_te.l_constraint.f_val  = tl_val;
					}
					if (r_constrained) // right
					{
						// constrain the bottom right region and the top right region
						_ASSERTE(br_te.r_constrained == false);
						_ASSERTE(tr_te.r_constrained == false);
						br_te.r_constrained       = tr_te.r_constrained       = true;
						br_te.r_constraint.start  = tr_te.r_constraint.start  = b_coord;
						br_te.r_constraint.length = tr_te.r_constraint.length = side;
						br_te.r_constraint.s_val  = tr_te.r_constraint.s_val  = br_val;
						br_te.r_constraint.f_val  = tr_te.r_constraint.f_val  = tr_val;
					}
					if (b_constrained) // bottom
					{
						// constrain the bottom left region and the bottom right region
						_ASSERTE(bl_te.b_constrained == false);
						_ASSERTE(br_te.b_constrained == false);
						bl_te.b_constrained       = br_te.b_constrained       = true;
						bl_te.b_constraint.start  = br_te.b_constraint.start  = l_coord;
						bl_te.b_constraint.length = br_te.b_constraint.length = side;
						bl_te.b_constraint.s_val  = br_te.b_constraint.s_val  = bl_val;
						bl_te.b_constraint.f_val  = br_te.b_constraint.f_val  = br_val;
					}
					if (t_constrained) // top
					{
						// constrain the top left region and the top right region
						_ASSERTE(tl_te.t_constrained == false);
						_ASSERTE(tr_te.t_constrained == false);
						tl_te.t_constrained       = tr_te.t_constrained       = true;
						tl_te.t_constraint.start  = tr_te.t_constraint.start  = l_coord;
						tl_te.t_constraint.length = tr_te.t_constraint.length = side;
						tl_te.t_constraint.s_val  = tr_te.t_constraint.s_val  = tl_val;
						tl_te.t_constraint.f_val  = tr_te.t_constraint.f_val  = tr_val;
					}
					// enqueue the subregions
					queued->push_back(bl_te);
					queued->push_back(br_te);
					queued->push_back(tl_te);
					queued->push_back(tr_te);
				}
				// switch queues
				current->clear();
				temp_array = current;
				current = queued;
				queued = temp_array;
			}
			// handle the special case of k = 0
			num = static_cast<short>(map_size.cx / 2);
			vector<CTE>::const_iterator te_i(current->begin());
			const vector<CTE>::const_iterator te_end(current->end());
			for (; te_i != te_end; ++te_i)
			{
				const CTE te(*te_i);
				// split the index into cartesian coordinates
				xi = static_cast<short>(te.index % num);
				yi = static_cast<short>(te.index / num);
				// calculate sides of the region
				short l_coord(xi + xi);
				short r_coord(l_coord + 2);
				short b_coord(yi + yi);
				short t_coord(b_coord + 2);
				// calculate heightmap values at the corners of the region
				BYTE *offset(heightmap_data.ptr + 2 * ((map_size.cx + 1) * yi + xi));
				bl_val = *offset;
				br_val = *(offset + 2);
				offset += 2 * (map_size.cx + 1);
				tl_val = *offset;
				tr_val = *(offset + 2);
				// constrain the heightmap values, if necessary
				// bottom left
				if (te.l_constrained && te.l_constraint.start != b_coord)
					bl_val = te.l_constraint.ConstrainedVal(b_coord);
				if (te.b_constrained && te.b_constraint.start != l_coord)
					bl_val = te.b_constraint.ConstrainedVal(l_coord);
				// bottom right
				if (te.r_constrained && te.r_constraint.start != b_coord)
					br_val = te.r_constraint.ConstrainedVal(b_coord);
				if (te.b_constrained && (te.b_constraint.start + te.b_constraint.length) != r_coord)
					br_val = te.b_constraint.ConstrainedVal(r_coord);
				// top left
				if (te.l_constrained && (te.l_constraint.start + te.l_constraint.length) != t_coord)
					tl_val = te.l_constraint.ConstrainedVal(t_coord);
				if (te.t_constrained && te.t_constraint.start != l_coord)
					tl_val = te.t_constraint.ConstrainedVal(l_coord);
				// top right
				if (te.r_constrained && (te.r_constraint.start + te.r_constraint.length) != t_coord)
					tr_val = te.r_constraint.ConstrainedVal(t_coord);
				if (te.t_constrained && (te.t_constraint.start + te.t_constraint.length) != r_coord)
					tr_val = te.t_constraint.ConstrainedVal(r_coord);
				// if the region is flat, add it to the triangulation and continue
				if (regions[k][te.index])
				{
					// bottom left vertex
					bl_vert.x = l_coord;
					bl_vert.y = b_coord;
					bl_vert.z = bl_val;
					// bottom right vertex
					br_vert.x = r_coord;
					br_vert.y = b_coord;
					br_vert.z = br_val;
					// top left vertex
					tl_vert.x = l_coord;
					tl_vert.y = t_coord;
					tl_vert.z = tl_val;
					// top right vertex
					tr_vert.x = r_coord;
					tr_vert.y = t_coord;
					tr_vert.z = tr_val;
					// add the vertices to the triangulation (counter-clockwise)
					// TODO: try using indexes
					vertices.push_back(bl_vert);
					vertices.push_back(br_vert);
					vertices.push_back(tr_vert);
					vertices.push_back(bl_vert);
					vertices.push_back(tr_vert);
					vertices.push_back(tl_vert);
					continue;
				}
				// shift "offset" to the region's center
				offset -= map_size.cx;
				// find the middle point
				float center(*offset);
				// if any point under the region is zero, do not triangulate it
				//  this is justified because inner regions will not have zero subregions
				//  and boundary regions are not as important
				if (
					*offset                     == 0 ||
					*(offset + 1)               == 0 ||
					*(offset - 1)               == 0 ||
					*(offset + map_size.cx)     == 0 ||
					*(offset + map_size.cx + 2) == 0 ||
					*(offset - map_size.cx - 2) == 0 ||
					*(offset - map_size.cx)     == 0)
					continue;
				// bottom left subregion
				// bottom left vertex
				bl_vert.x = l_coord;
				bl_vert.y = b_coord;
				bl_vert.z = bl_val;
				// bottom right vertex
				br_vert.x = l_coord + 1.0f;
				br_vert.y = b_coord;
				br_vert.z = (bl_val + br_val) / 2.0f;
				// top left vertex
				tl_vert.x = l_coord;
				tl_vert.y = b_coord + 1.0f;
				tl_vert.z = (bl_val + tl_val) / 2.0f;
				// top right vertex
				tr_vert.x = l_coord + 1.0f;
				tr_vert.y = b_coord + 1.0f;
				tr_vert.z = center;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
				// bottom right subregion
				// bottom left vertex
				bl_vert.x = l_coord + 1.0f;
				bl_vert.y = b_coord;
				bl_vert.z = (bl_val + br_val) / 2.0f;
				// bottom right vertex
				br_vert.x = r_coord;
				br_vert.y = b_coord;
				br_vert.z = br_val;
				// top left vertex
				tl_vert.x = l_coord + 1.0f;
				tl_vert.y = b_coord + 1.0f;
				tl_vert.z = center;
				// top right vertex
				tr_vert.x = r_coord;
				tr_vert.y = b_coord + 1.0f;
				tr_vert.z = (br_val + tr_val) / 2.0f;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
				// top left subregion
				// bottom left vertex
				bl_vert.x = l_coord;
				bl_vert.y = b_coord + 1.0f;
				bl_vert.z = (bl_val + tl_val) / 2.0f;
				// bottom right vertex
				br_vert.x = l_coord + 1.0f;
				br_vert.y = b_coord + 1.0f;
				br_vert.z = center;
				// top left vertex
				tl_vert.x = l_coord;
				tl_vert.y = t_coord;
				tl_vert.z = tl_val;
				// top right vertex
				tr_vert.x = l_coord + 1.0f;
				tr_vert.y = t_coord;
				tr_vert.z = (tl_val + tr_val) / 2.0f;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
				// top right subregion
				// bottom left vertex
				bl_vert.x = l_coord + 1.0f;
				bl_vert.y = b_coord + 1.0f;
				bl_vert.z = center;
				// bottom right vertex
				br_vert.x = r_coord;
				br_vert.y = b_coord + 1.0f;
				br_vert.z = (br_val + tr_val) / 2.0f;
				// top left vertex
				tl_vert.x = l_coord + 1.0f;
				tl_vert.y = t_coord;
				tl_vert.z = (tl_val + tr_val) / 2.0f;
				// top right vertex
				tr_vert.x = r_coord;
				tr_vert.y = t_coord;
				tr_vert.z = tr_val;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
			}
		}
	}
	heightmap->SignIn();
	delete [] regions;
}

void CPreview::UpdateData()
{
	sections.resize((map_size.cx / texture_width) * (map_size.cy / texture_height));
	// start processing the heightmap
	if (heightmap_update_pending)
	{
		InterlockedExchange(&heightmap_update_pending, false);
		if (NULL != heightmap_thread)
		{
			WaitForSingleObject(heightmap_thread, INFINITE);
			CloseHandle(heightmap_thread);
		}
		DWORD thread_id;
		heightmap_thread = CreateThread(NULL, 0, HeightmapThreadProc, this, 0, &thread_id);
		if (heightmap_thread == NULL)
			Error(_T("CreateThread failed"));
	}
	// start processing the texture
	if (texture_update_pending)
	{
		InterlockedExchange(&texture_update_pending, false);
		if (NULL != texture_thread)
		{
			WaitForSingleObject(texture_thread, INFINITE);
			CloseHandle(texture_thread);
		}
		DWORD thread_id;
		texture_thread = CreateThread(NULL, 0, TextureThreadProc, this, 0, &thread_id);
		if (texture_thread == NULL)
			Error(_T("CreateThread failed"));
	}
	// start processing map information
	if (map_info_update_pending)
	{
		InterlockedExchange(&map_info_update_pending, false);
		if (NULL != map_info_thread)
		{
			WaitForSingleObject(map_info_thread, INFINITE);
			CloseHandle(map_info_thread);
		}
		DWORD thread_id;
		map_info_thread = CreateThread(NULL, 0, MapInfoThreadProc, this, 0, &thread_id);
		if (map_info_thread == NULL)
			Error(_T("CreateThread failed"));
	}
}

// generates an assertion in debug mode and a warning in release mode
void CPreview::Error(string message) const
{
	string new_message = _T("CPreview: \n");
	new_message.append(message);
	_RPT0(_CRT_ERROR, new_message.c_str());
	MessageBox(hWnd, new_message.c_str(), NULL, MB_OK | MB_ICONERROR);
}

inline int CPreview::log2(unsigned int n) const
{
	int l(-1);
	while (0 != n)
	{
		n >>= 1;
		++l;
	}
	return l;
}

inline int CPreview::exp2(unsigned int n) const
{
	int e(1);
	while (0 != n)
	{
		e <<= 1;
		--n;
	}
	return e;
}

//------------------------------------
// CPreview::CBillboard implementation
//------------------------------------

CPreview::CBillboard::CBillboard() :
	vertices(NULL)
{}

void CPreview::CBillboard::CalculateAngle(float eye_x, float eye_y)
{
	angle = atan2(eye_y - position.y, eye_x - position.x) - D3DX_PI / 2;
}

//--------------------------------------
// CPreview::CSerializable implementaion
//--------------------------------------

CPreview::CSerializable::CSerializable(CPreview *parent)
 :parent(parent)
{}

void CPreview::CSerializable::Save(string file_name)
{
	const TCHAR * const section_name(_T("Map Preview Settings"));
	const size_t buffer_size(256);
	TCHAR buffer[buffer_size];
	CAutoCriticalSection auto_critical_section(&critical_section);
	// threshold
	_stprintf(buffer, _T("%.2f"), threshold);
	WritePrivateProfileString(section_name, _T("threshold"), buffer, file_name.c_str());
	// opacity
	_stprintf(buffer, _T("%i"), static_cast<int>(zero_layer_colour >> 24));
	WritePrivateProfileString(section_name, _T("opacity"), buffer, file_name.c_str());
	// colour
	_stprintf(
		buffer,
		_T("%i %i %i"),
		static_cast<int>((zero_layer_colour & 0x00FF0000) >> 16),
		static_cast<int>((zero_layer_colour & 0x0000FF00) >> 8 ),
		static_cast<int>((zero_layer_colour & 0x000000FF) >> 0 ));
	WritePrivateProfileString(section_name, _T("color"), buffer, file_name.c_str());
}

void CPreview::CSerializable::Load(string file_name)
{
	const TCHAR * const section_name(_T("Map Preview Settings"));
	const size_t buffer_size(256);
	TCHAR buffer[buffer_size];
	CAutoCriticalSection auto_critical_section(&critical_section);
	// some initialization
	zero_layer_colour = 0L;
	// threshold
	{
		float raw_threshold;
		GetPrivateProfileString(section_name, _T("threshold"), _T("3"), buffer, buffer_size, file_name.c_str());
		_stscanf(buffer, "%f", &raw_threshold);
		threshold = __max(0, __min(8.0f, raw_threshold));
	}
	// opacity
	{
		int raw_opacity;
		GetPrivateProfileString(section_name, _T("opacity"), _T("40"), buffer, buffer_size, file_name.c_str());
		_stscanf(buffer, "%i", &raw_opacity);
		raw_opacity = __max(0, __min(255, raw_opacity));
		zero_layer_colour |= raw_opacity << 24;
	}
	// colour
	{
		int raw_r, raw_g, raw_b;
		GetPrivateProfileString(section_name, _T("color"), "0 0 0", buffer, buffer_size, file_name.c_str());
		_stscanf(buffer, "%i %i %i", &raw_r, &raw_g, &raw_b);
		raw_r = __max(0, __min(255, raw_r));
		raw_g = __max(0, __min(255, raw_g));
		raw_b = __max(0, __min(255, raw_b));
		zero_layer_colour |= raw_r << 16 | raw_g << 8 | raw_b;
	}
	// enable_lighting
	{
		const TCHAR * const section_name(_T("Project Settings")); // HACK
		GetPrivateProfileString(
			section_name,
			_T("enable lighting"),
			_T("true"),
			buffer,
			buffer_size,
			file_name.c_str());
		enable_lighting = (0 == _tcscmp(buffer, _T("true")));
	}
}

void CPreview::CSerializable::Update()
{
	if (parent->map_info_valid)
		parent->BuildZeroLayerVB();
	if (parent->heightmap_valid)
	{
		parent->ReleaseTerrainVBs();
		parent->BuildVB();
	}
	if (parent->texture_valid)
		parent->InitializeTextures();
	parent->Render();
}