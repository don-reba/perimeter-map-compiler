//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// ? Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// ? Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// ? Neither the name of Don Reba nor the names of his contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------


#pragma once

#include "panel wnd.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>
#include <stack>

//-----------------
// supporting types
//-----------------

struct SimpleVertex
{
	float x, y, z;
};

//----------------------
// message-related types
//----------------------

struct TextureAllocation
{
	D3DXCOLOR **bitmaps_; // allocated by DX
	uint        x_count_;
	uint        y_count_;
	uint        width_;
	uint        height_;
};

//---------------------
// some custom messages
//---------------------

enum
{
	WM_USR_STATE_CHANGED = WM_APP,
	WM_USR_PROJECT_CHANGED,
	WM_USR_SET_TERRAIN,
	WM_USR_TEXTURE_ALLOCATE,
	WM_USR_TEXTURE_COMMIT
};

inline void SendSetTerrain(HWND hwnd, vector<SimpleVertex> &vertices) {
	SendMessage(hwnd, WM_USR_SET_TERRAIN, ri_cast<WPARAM>(&vertices), 0L);
}
inline TextureAllocation *SendTextureAllocate(HWND hwnd) {
	return ri_cast<TextureAllocation*>(SendMessage(hwnd, WM_USR_TEXTURE_ALLOCATE, 0u, 0L));
}
inline void SendTextureCommit(HWND hwnd) {
	SendMessage(hwnd, WM_USR_TEXTURE_COMMIT, 0u, 0l);
}

template <>
struct Msg<WM_USR_STATE_CHANGED> : Msg_
{
	Msg(WndMsg &msg) : Msg_(msg) {}
	int Type() const {
		return static_cast<int>(lprm_);
	}
};

template <>
struct Msg<WM_USR_PROJECT_CHANGED> : Msg_
{
	Msg(WndMsg &msg) : Msg_(msg) {}
};

template <>
struct Msg<WM_USR_SET_TERRAIN> : Msg_
{
	Msg(WndMsg &msg) : Msg_(msg) {}
	vector<SimpleVertex>* GetVertices() const {
		return ri_cast<vector<SimpleVertex>*>(wprm_);
	}
};

template <>
struct Msg<WM_USR_TEXTURE_ALLOCATE> : Msg_
{
	Msg(WndMsg &msg) : Msg_(msg) {}
	void SetResult(TextureAllocation *allocation) {
		result_ = ri_cast<LRESULT>(allocation);
	}
};

template <>
struct Msg<WM_USR_TEXTURE_COMMIT> : Msg_
{
	Msg(WndMsg &msg) : Msg_(msg) {}
};

//--------------------------------
// 3D map preview panel definition
//--------------------------------

class PreviewWnd : public PanelWindow
{
// nested types
private:
	struct Billboard
	{
		Billboard();
		void CalculateAngle(float eye_x, float eye_y);
		FLOAT                   angle_;
		D3DXVECTOR2             position_;
		IDirect3DVertexBuffer9 *vertices_;
	};
	struct ColouredVertex
	{
		ColouredVertex(FLOAT x, FLOAT y, FLOAT z, D3DCOLOR color) : x(x), y(y), z(z), color(color) {}
		FLOAT x, y, z;
		D3DCOLOR color;
		static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
	};
	class Marker : public Billboard
	{
	public:
		Marker(D3DCOLOR colour);
		D3DCOLOR GetColour();
		bool      Highlight(bool on);
		bool      Initialize(IDirect3DDevice9 *device);
		void      Release();
		bool      Set(D3DXVECTOR2 position);
		void      SetColour(D3DCOLOR colour);
	private:
		D3DXCOLOR colour_;
		bool      highlighted_;
	};
	typedef vector<Marker> Markers;
	// map segment for rendering
	struct Section
	{
		IDirect3DTexture9 *texture_;
		uint vertex_offset_;
		uint face_count_;
	};
	struct TerrainVertex
	{
		FLOAT x, y, z;
		FLOAT u, v;
		static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX1;
	};
// construction/destruction
public:
	PreviewWnd();
	~PreviewWnd();
// interface
public:
	bool Create(HWND parent_wnd, const RECT &window_rect);
	void HighlightMarker(size_t marker_index);
	void ProjectChanged();
	void ProjectDataChanged(int type);
	void UpdateSettings();
// message handlers
private:
	void OnCaptureChanged (Msg<WM_CAPTURECHANGED>       &msg);
	void OnDestroy        (Msg<WM_DESTROY>              &msg);
	void OnExitSizeMove   (Msg<WM_EXITSIZEMOVE>         &msg);
	void OnInitDialog     (Msg<WM_INITDIALOG>           &msg);
	void OnKeyDown        (Msg<WM_KEYDOWN>              &msg);
	void OnLButtonDown    (Msg<WM_LBUTTONDOWN>          &msg);
	void OnLButtonUp      (Msg<WM_LBUTTONUP>            &msg);
	void OnMouseMove      (Msg<WM_MOUSEMOVE>            &msg);
	void OnMouseWheel     (Msg<WM_MOUSEWHEEL>           &msg);
	void OnPaint          (Msg<WM_PAINT>                &msg);
	void OnProjectChanged (Msg<WM_USR_PROJECT_CHANGED>  &msg);
	void OnSetTerrain     (Msg<WM_USR_SET_TERRAIN>      &msg);
	void OnStateChanged   (Msg<WM_USR_STATE_CHANGED>    &msg);
	void OnTextureAllocate(Msg<WM_USR_TEXTURE_ALLOCATE> &msg);
	void OnTextureCommit  (Msg<WM_USR_TEXTURE_COMMIT>   &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
private:
	void BuildZeroLayerVB();
	bool CreateDevice();
	bool InitializeDevice();
	void MakeProjectiveMatrix();
	void MakeViewMatrix();
	void PopWorldMatrix();
	void PushWorldMatrix(const D3DXMATRIX &matrix);
	void Render();
	void SetTitle(uint polycount);
// data
private:
	// DX variables
	IDirect3D9       *d3d_;
	IDirect3DDevice9 *device_;
	// DX resources
	Markers                 markers_;
	IDirect3DVertexBuffer9 *terrain_vb_;
	std::stack<D3DXMATRIX>  world_matrix_stack_;
	FLOAT                   world_stretch_;
	IDirect3DVertexBuffer9 *zero_layer_vb_;
	vector<Section>         sections_;
	// state
	FLOAT     fog_start_;
	FLOAT     fog_end_;
	D3DXCOLOR fog_colour_;
	bool      is_empty_;
	SIZE      map_size_;
	bool      wireframe_mode_;
	FLOAT     zero_level_;
	D3DXCOLOR zero_layer_colour_;
	bool      textures_valid_;
	bool      terrain_valid_;
	// camera
	float       angle_h_;
	float       angle_v_;
	float       radius_;
	D3DXVECTOR3 target_;
	D3DXMATRIX  view_matrix_;
	D3DXMATRIX  proj_matrix_;
	// mouse
	D3DXVECTOR3 old_target_;
	POINT       mouse_click_pos_;
	bool        mouse_captured_;
	// synchronization
	CRITICAL_SECTION state_section_;
};