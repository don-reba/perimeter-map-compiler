//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// • Neither the name of Don Reba nor the names of its contributors may be used
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


#include "stdafx.h"

#include "app data.h"
#include "preview wnd.h"
#include "project data.h"
#include "resource.h"

#include <sstream>

//----------
// constants
//----------

const int texture_width (1024);
const int texture_height(1024);

//----------------------------
// additional message crackers
//----------------------------

#define SendStateChanged(hwnd, type) ((int)SNDMSG((hwnd), WM_USR_STATE_CHANGED,   0u, (LPARAM)(type)))
#define SendProjectChanged(hwnd)     ((int)SNDMSG((hwnd), WM_USR_PROJECT_CHANGED, 0u, 0l))

//------------------------------------
// 3D map preview panel implementation
//------------------------------------

PreviewWnd::PreviewWnd()
	:d3d_              (NULL)
	,device_           (NULL)
	,fog_start_        (0)
	,fog_end_          (0)
	,fog_colour_       (static_cast<DWORD>(0L))
	,is_empty_         (true)
	,mouse_captured_   (false)
	,textures_valid_   (false)
	,terrain_vb_       (NULL)
	,terrain_valid_    (false)
	,wireframe_mode_   (false)
	,world_stretch_    (1.3f)
	,zero_layer_colour_(D3DCOLOR_ARGB(0x28, 0x00, 0x00, 0x00))
	,zero_layer_vb_    (NULL)
{
	// initialize DirectGraphics
	d3d_ = Direct3DCreate9(D3D_SDK_VERSION);
	if (NULL == d3d_)
		MacroDisplayError(_T("Direct3D9 could not be initialized."));
	// initialize world matrix stack
	{
		D3DXMATRIX identity_matrix;
		D3DXMatrixIdentity(&identity_matrix);
		world_matrix_stack_.push(identity_matrix);
	}
}

PreviewWnd::~PreviewWnd()
{
	// release DirectGraphics
	d3d_->Release();
}

bool PreviewWnd::Create(HWND parent_wnd, const RECT &window_rect)
{
	hwnd_ = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_PREVIEW_DLG),
		parent_wnd,
		DlgProc<PreviewWnd>,
		ri_cast<LPARAM>(this));
	if (NULL == hwnd_)
	{
		MacroDisplayError(_T("The preview window could not be created."));
		return false;
	}
	SetRect(window_rect);
	SetWindowPos(
		hwnd_,
		NULL,
		window_rect.left,
		window_rect.top,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		SWP_NOACTIVATE | SWP_NOZORDER);
	InitializeDevice();
	return true;
}

void PreviewWnd::OnCaptureChanged(Msg<WM_CAPTURECHANGED> &msg)
{
	mouse_captured_ = false;
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void PreviewWnd::OnDestroy(Msg<WM_DESTROY> &msg)
{
	// release the terrain VB
	if (NULL != terrain_vb_)
		terrain_vb_->Release();
	// release marker VBs
	foreach (Marker &marker, markers_)
		marker.Release();
	// release zero layer VB
	if (NULL != zero_layer_vb_)
		zero_layer_vb_->Release();
	// release the textures
	if (textures_valid_)
		foreach(Section &section, sections_)
			section.texture_->Release();
	// release the device
	if (NULL != device_)
		device_->Release();
}

void PreviewWnd::OnExitSizeMove(Msg<WM_EXITSIZEMOVE> &msg)
{
	InitializeDevice();
	Render();
}

void PreviewWnd::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	msg.result_  = TRUE;
	msg.handled_ = true;
	if (!InitializeDevice())
	{
		EndDialog(hwnd_, 0);
		return;
	}
	// initialize zero_layer_colour_
	zero_layer_colour_ = D3DCOLOR_ARGB(
		MacroAppData(ID_ZERO_LAYER_OPACITY),
		GetRValue(MacroAppData(ID_ZERO_LAYER_COLOUR)),
		GetGValue(MacroAppData(ID_ZERO_LAYER_COLOUR)),
		GetBValue(MacroAppData(ID_ZERO_LAYER_COLOUR)));
	// initialize the frame marker VBs
	markers_.push_back(Marker(D3DCOLOR_ARGB(0xFF, 0xFF, 0x00, 0x00))); // player 0
	markers_.push_back(Marker(D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00))); // player 1
	markers_.push_back(Marker(D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00))); // player 2
	markers_.push_back(Marker(D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00))); // player 3
	markers_.push_back(Marker(D3DCOLOR_ARGB(0xFF, 0x00, 0xFF, 0x00))); // player 4
	foreach (Marker &marker, markers_)
		if (!marker.Initialize(device_))
		{
			DestroyWindow(hwnd_);
			return;
		}
	// initialize the zero layer VB
	_ASSERTE(NULL == zero_layer_vb_);
	if (FAILED(device_->CreateVertexBuffer(
		4 * sizeof(ColouredVertex),
		0,
		ColouredVertex::FVF,
		D3DPOOL_MANAGED,
		&zero_layer_vb_,
		NULL)))
	{
		MacroDisplayError(_T("Zero layer could not be created."));
		return;
	}
}

void PreviewWnd::OnKeyDown(Msg<WM_KEYDOWN> &msg)
{
	// constants
	const float angle_h_delta(D3DX_PI / 64.0f);
	const float angle_v_delta(D3DX_PI / 128.0f);
	const float angle_h_min(0.0f);
	const float angle_h_max(2 * D3DX_PI);
	const float angle_v_min(0.0f);
	const float angle_v_max(D3DX_PI / 2.0f - 0.0001f);
	const float displace_delta(16.0f);
	const float radius_delta(0.8f);
	// key handling
	switch (msg.VKey())
	{
	case VK_TAB:
		{
			if (wireframe_mode_)
			{
				wireframe_mode_ = false;
				device_->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
				device_->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			}
			else
			{
				wireframe_mode_ = true;
				device_->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
				device_->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			}
			Render();
		} break;
	case VK_DELETE:
		{
			// rotate left
			angle_h_ += angle_h_delta;
			if (angle_h_ >= angle_h_max)
				angle_h_ -= angle_h_max;
			MakeViewMatrix();
			Render();
		} break;
	case VK_NEXT:
		{
			// rotate right
			angle_h_ -= angle_h_delta;
			if (angle_h_ < angle_h_min)
				angle_h_ += angle_h_max;
			MakeViewMatrix();
			Render();
		} break;
	case VK_HOME:
		{
			// rotate down
			float new_angle = angle_v_ + angle_v_delta;
			if (new_angle > angle_v_max)
				new_angle = angle_v_max;
			angle_v_ = new_angle;
			MakeViewMatrix();
			Render();
		} break;
	case VK_END:
		{
			// rotate up
			float new_angle = angle_v_ - angle_v_delta;
			if (new_angle < angle_v_min)
				new_angle = angle_v_min;
			angle_v_ = new_angle;
			MakeViewMatrix();
			Render();
		} break;
	case VK_ADD:
		{
			radius_ *= radius_delta;
			if (radius_ <= 0.0f)
				radius_ = 0.1f;
			MakeViewMatrix();
			MakeProjectiveMatrix();
			Render();
		} break;
	case VK_SUBTRACT:
		{
			radius_ /= radius_delta;
			MakeViewMatrix();
			MakeProjectiveMatrix();
			Render();
		} break;
	case VK_LEFT:
		{
			const float proj(cos(angle_v_) * radius_);
			float dir_x(cos(angle_h_) * proj);
			float dir_y(sin(angle_h_) * proj);
			const float dir_length(sqrt(dir_x * dir_x + dir_y * dir_y));
			dir_x /= dir_length;
			dir_y /= dir_length;
			target_.x = target_.x - dir_y * displace_delta;
			target_.y = target_.y + dir_x * displace_delta;
			MakeViewMatrix();
			Render();
		} break;
	case VK_RIGHT:
		{
			const float proj(cos(angle_v_) * radius_);
			float dir_x(cos(angle_h_) * proj);
			float dir_y(sin(angle_h_) * proj);
			const float dir_length(sqrt(dir_x * dir_x + dir_y * dir_y));
			dir_x /= dir_length;
			dir_y /= dir_length;
			target_.x = target_.x + dir_y * displace_delta;
			target_.y = target_.y - dir_x * displace_delta;
			MakeViewMatrix();
			Render();
		} break;
	case VK_UP:
		{
			const float proj(cos(angle_v_) * radius_);
			float dir_x(cos(angle_h_) * proj);
			float dir_y(sin(angle_h_) * proj);
			const float dir_length(sqrt(dir_x * dir_x + dir_y * dir_y));
			dir_x /= dir_length;
			dir_y /= dir_length;
			target_.x = target_.x - dir_x * displace_delta;
			target_.y = target_.y - dir_y * displace_delta;
			MakeViewMatrix();
			Render();
		} break;
	case VK_DOWN:
		{
			const float proj(cos(angle_v_) * radius_);
			float dir_x(cos(angle_h_) * proj);
			float dir_y(sin(angle_h_) * proj);
			const float dir_length(sqrt(dir_x * dir_x + dir_y * dir_y));
			dir_x /= dir_length;
			dir_y /= dir_length;
			target_.x = target_.x + dir_x * displace_delta;
			target_.y = target_.y + dir_y * displace_delta;
			MakeViewMatrix();
			Render();
		} break;
	}
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void PreviewWnd::OnLButtonDown(Msg<WM_LBUTTONDOWN> &msg)
{
	SetCapture(hwnd_);
	mouse_captured_  = true;
	mouse_click_pos_ = msg.Position();
	old_target_      = target_;
	msg.result_      = FALSE;
	msg.handled_     = true;
}

void PreviewWnd::OnLButtonUp(Msg<WM_LBUTTONUP> &msg)
{
	ReleaseCapture();
	mouse_captured_ = false;
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void PreviewWnd::OnMouseMove(Msg<WM_MOUSEMOVE> &msg)
{
	if (mouse_captured_)
	{
		RECT rect;
		GetClientRect(hwnd_, &rect);
		const float width(static_cast<float>(rect.right - rect.left));
		const float height(static_cast<float>(rect.bottom - rect.top));
		const float shift_x(map_size_.cx * ((msg.Position().x - mouse_click_pos_.x) / width));
		const float shift_y(map_size_.cy * ((msg.Position().y - mouse_click_pos_.y) / height));
		const float proj(cos(angle_v_) * radius_);
		float dir_x(cos(angle_h_) * proj);
		float dir_y(sin(angle_h_) * proj);
		const float dir_length(sqrt(dir_x * dir_x + dir_y * dir_y));
		dir_x /= dir_length;
		dir_y /= dir_length;
		target_.x = old_target_.x - dir_y * shift_x - dir_x * shift_y;
		target_.y = old_target_.y + dir_x * shift_x - dir_y * shift_y;
		MakeViewMatrix();
		Render();
		msg.result_  = FALSE;
		msg.handled_ = true;
	}
}

void PreviewWnd::OnMouseWheel(Msg<WM_MOUSEWHEEL> &msg)
{
	radius_ *= pow(0.9f, static_cast<int>(msg.WheelDelta()) / 120);
	if (radius_ <= 0.0f)
		radius_ = 0.1f;
	MakeViewMatrix();
	MakeProjectiveMatrix();
	Render();
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void PreviewWnd::OnPaint(Msg<WM_PAINT> &msg)
{
	Render();
	msg.result_ = FALSE;
	msg.handled_ = true;
}

void PreviewWnd::OnProjectChanged(Msg<WM_USR_PROJECT_CHANGED> &msg)
{
	// rememeber map size
	map_size_.cx = exp2(MacroProjectData(ID_POWER_X));
	map_size_.cy = exp2(MacroProjectData(ID_POWER_Y));
	// resize sections
	if (textures_valid_)
	{
		foreach(Section &section, sections_)
		{
			section.texture_->Release();
			section.texture_ = NULL;
		}
	}
	sections_.resize((map_size_.cx / texture_width) * (map_size_.cy / texture_height));
	// reset camera
	target_.x = map_size_.cx / 2.0f;
	target_.y = map_size_.cy / 2.0f;
	target_.z = 0;
	angle_h_ = D3DX_PI / 2;
	angle_v_ = D3DX_PI / 4.0f;
	radius_ = static_cast<float>(__max(map_size_.cx, map_size_.cy));
	// set frame markers
	{
		_ASSERTE(markers_.size() >= 5);
		markers_[0].Set(D3DXVECTOR2(
			static_cast<FLOAT>(MacroProjectData(ID_SP_0).x),
			static_cast<FLOAT>(MacroProjectData(ID_SP_0).y)));
		markers_[1].Set(D3DXVECTOR2(
			static_cast<FLOAT>(MacroProjectData(ID_SP_1).x),
			static_cast<FLOAT>(MacroProjectData(ID_SP_1).y)));
		markers_[2].Set(D3DXVECTOR2(
			static_cast<FLOAT>(MacroProjectData(ID_SP_2).x),
			static_cast<FLOAT>(MacroProjectData(ID_SP_2).y)));
		markers_[3].Set(D3DXVECTOR2(
			static_cast<FLOAT>(MacroProjectData(ID_SP_3).x),
			static_cast<FLOAT>(MacroProjectData(ID_SP_3).y)));
		markers_[4].Set(D3DXVECTOR2(
			static_cast<FLOAT>(MacroProjectData(ID_SP_4).x),
			static_cast<FLOAT>(MacroProjectData(ID_SP_4).y)));
	}
	// set fog parameters
	{
		fog_start_  = static_cast<FLOAT>(MacroProjectData(ID_FOG_START));
		fog_end_    = static_cast<FLOAT>(MacroProjectData(ID_FOG_END));
		fog_colour_ = D3DCOLOR_ARGB(
			0,
			GetRValue(MacroProjectData(ID_FOG_COLOUR)),
			GetGValue(MacroProjectData(ID_FOG_COLOUR)),
			GetBValue(MacroProjectData(ID_FOG_COLOUR)));
		device_->SetRenderState(D3DRS_FOGCOLOR, fog_colour_);
		device_->SetRenderState(D3DRS_FOGSTART, *(ri_cast<DWORD*>(&fog_start_)));
		device_->SetRenderState(D3DRS_FOGEND,   *(ri_cast<DWORD*>(&fog_end_)));
	}
	// set zero level
	zero_level_ = static_cast<FLOAT>(MacroProjectData(ID_ZERO_LEVEL));
	BuildZeroLayerVB();
	MakeViewMatrix();
	MakeProjectiveMatrix();
	// render
	textures_valid_ = false;
	terrain_valid_  = false;
	is_empty_       = false;
	Render();
	msg.handled_ = true;
}

void PreviewWnd::OnSetTerrain(Msg<WM_USR_SET_TERRAIN> &msg)
{
	vector<SimpleVertex> &vertices(*msg.GetVertices());
	// handle the case of vertices.size() == 0
	if (0 == vertices.size())
	{
		terrain_valid_ = false;
		return;
	}
	// preconditions
	_ASSERTE(vertices.size() % 6 == 0); // should be true after Triangulate is done
	_ASSERTE(!sections_.empty());
	// reset face counts
	foreach (Section &section, sections_)
		section.face_count_ = 0;
	// count the number of faces in each section
	// assume that each quad consists of two faces and lies in one section
	const int textures_per_row(map_size_.cx / texture_width);
	{
		vector<SimpleVertex>::const_iterator i(vertices.begin());
		const vector<SimpleVertex>::const_iterator end(vertices.end());
		for (;  i != end; i += 6)
		{
			const int section_x(static_cast<int>(i->x / texture_width));
			const int section_y(static_cast<int>(i->y / texture_height));
			const int section_index(section_y * textures_per_row + section_x);
			sections_[section_index].face_count_ += 2;
		}
	}
	// generate vertex offsets
	{
		vector<Section>::iterator i(sections_.begin() + 1);
		const vector<Section>::const_iterator end(sections_.end());
		for (; i != end; ++i)
			i->vertex_offset_ = (i - 1)->face_count_ * 3;
		sections_[0].vertex_offset_ = 0;
		for (i = sections_.begin() + 1; i != end; ++i)
			i->vertex_offset_ += (i - 1)->vertex_offset_;
	}
	// initialize the vertex buffer
	{
		// create a new buffer
		TerrainVertex *vb;
		if (NULL != terrain_vb_)
			terrain_vb_->Release();
		HRESULT result(device_->CreateVertexBuffer(
			vertices.size() * sizeof(TerrainVertex),
			0,
			TerrainVertex::FVF,
			D3DPOOL_MANAGED,
			&terrain_vb_,
			NULL));
		if (D3D_OK != result)
		{
			if (D3DERR_OUTOFVIDEOMEMORY == result)
				MacroDisplayError(_T("There was not enough video memory to create the terrain.\nOptimize the map, or raise mesh threshold level in Preferences."));
			else
				MacroDisplayError(_T("Terrain could not be created."));
			return;
		}
		if (FAILED(terrain_vb_->Lock(0, 0, ri_cast<void**>(&vb), 0L)))
		{
			MacroDisplayError("Terrain buffer could not be locked.");
			terrain_vb_->Release();
			return;
		}
		// initialize section iterators to vertex offsets
		vector<int> section_iters(sections_.size());
		for (size_t i(0); i != sections_.size(); ++i)
			section_iters[i] = sections_[i].vertex_offset_;
		// sort the vertices by texture
		{
			vector<SimpleVertex>::const_iterator quad(vertices.begin());
			const vector<SimpleVertex>::const_iterator end(vertices.end());
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
					const SimpleVertex &simple_vertex(*(quad + i));
					TerrainVertex &vertex(vb[offset++]);
					vertex.x = simple_vertex.x;
					vertex.y = simple_vertex.y;
					vertex.z = simple_vertex.z;
					vertex.u = (vertex.x - section_x * texture_width)  / texture_width;
					vertex.v = (vertex.y - section_y * texture_height) / texture_height;
				}
				section_iters[section_index] = offset;
			}
		}
		terrain_vb_->Unlock();
	}
	terrain_valid_ = true;
	SetTitle(vertices.size() / 3);
	Render();
}

void PreviewWnd::OnStateChanged(Msg<WM_USR_STATE_CHANGED> &msg)
{
	switch (msg.Type())
	{
	case ProjectData::ID_SP_0:
		{
			markers_.at(0).Set(D3DXVECTOR2(
				static_cast<FLOAT>(MacroProjectData(ID_SP_0).x),
				static_cast<FLOAT>(MacroProjectData(ID_SP_0).y)));
		} break;
	case ProjectData::ID_SP_1:
		{
			markers_.at(1).Set(D3DXVECTOR2(
				static_cast<FLOAT>(MacroProjectData(ID_SP_1).x),
				static_cast<FLOAT>(MacroProjectData(ID_SP_1).y)));
		} break;
	case ProjectData::ID_SP_2:
		{
			markers_.at(2).Set(D3DXVECTOR2(
				static_cast<FLOAT>(MacroProjectData(ID_SP_2).x),
				static_cast<FLOAT>(MacroProjectData(ID_SP_2).y)));
		} break;
	case ProjectData::ID_SP_3:
		{
			markers_.at(3).Set(D3DXVECTOR2(
				static_cast<FLOAT>(MacroProjectData(ID_SP_3).x),
				static_cast<FLOAT>(MacroProjectData(ID_SP_3).y)));
		} break;
	case ProjectData::ID_SP_4:
		{
			markers_.at(4).Set(D3DXVECTOR2(
				static_cast<FLOAT>(MacroProjectData(ID_SP_4).x),
				static_cast<FLOAT>(MacroProjectData(ID_SP_4).y)));
		} break;
	case ProjectData::ID_ZERO_LEVEL:
		{
			zero_level_ = static_cast<FLOAT>(MacroProjectData(ID_ZERO_LEVEL));
		} break;
	case ProjectData::ID_FOG_START:
		{
			fog_start_ = static_cast<FLOAT>(MacroProjectData(ID_FOG_START));
			device_->SetRenderState(D3DRS_FOGSTART, *ri_cast<DWORD*>(&fog_start_));
		} break;
	case ProjectData::ID_FOG_END:
		{
			fog_start_ = static_cast<FLOAT>(MacroProjectData(ID_FOG_END));
			device_->SetRenderState(D3DRS_FOGEND, *ri_cast<DWORD*>(&fog_end_));
		} break;
	case ProjectData::ID_FOG_COLOUR:
		{
			fog_colour_ = D3DCOLOR_ARGB(
				0,
				GetRValue(MacroProjectData(ID_FOG_COLOUR)),
				GetGValue(MacroProjectData(ID_FOG_COLOUR)),
				GetBValue(MacroProjectData(ID_FOG_COLOUR)));
			device_->SetRenderState(D3DRS_FOGCOLOR, fog_colour_);
		} break;
	}
	MakeProjectiveMatrix();
	MakeViewMatrix();
	Render();
	msg.handled_ = true;
}

void PreviewWnd::OnTextureAllocate(Msg<WM_USR_TEXTURE_ALLOCATE> &msg)
{
	textures_valid_ = false;
		TextureAllocation *allocation(new TextureAllocation);
	allocation->width_   = texture_width;
	allocation->height_  = texture_height;
	allocation->x_count_ = map_size_.cx / texture_width;
	allocation->y_count_ = map_size_.cy / texture_height;
	const size_t texture_count(allocation->x_count_ * allocation->y_count_);
	allocation->bitmaps_ = new D3DXCOLOR*[texture_count];
	for (size_t i(0), size(sections_.size()); i != size; ++i)
	{
		if (NULL != sections_[i].texture_)
			sections_[i].texture_->Release();
		if (D3D_OK != device_->CreateTexture(
			texture_width,
			texture_height,
			0,
			D3DUSAGE_AUTOGENMIPMAP,
			D3DFMT_A8R8G8B8,
			D3DPOOL_MANAGED,
         &sections_[i].texture_,
			NULL))
		{
			MacroDisplayError(_T("Texture could not be created."));
			msg.SetResult(NULL);
			return;
		}
		D3DLOCKED_RECT rect;
		if (
			FAILED(sections_[i].texture_->LockRect(0, &rect, NULL, 0)) ||
			rect.Pitch != 4 * texture_width)
		{
			MacroDisplayError(_T("Texture could not be locked."));
			msg.SetResult(NULL);
			return;
		};
		allocation->bitmaps_[i] = static_cast<D3DXCOLOR*>(rect.pBits);
	}
	msg.SetResult(allocation);
	msg.handled_ = true;
}

void PreviewWnd::OnTextureCommit(Msg<WM_USR_TEXTURE_COMMIT> &msg)
{
	foreach(Section &section, sections_)
	{
		section.texture_->UnlockRect(0);
		section.texture_->PreLoad(); // WARN: NVIDIA seems to ignore this...
	}
	textures_valid_= true;
	Render();
}

void PreviewWnd::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&PreviewWnd::OnDestroy,
		&PreviewWnd::OnExitSizeMove,
		&PreviewWnd::OnInitDialog,
		&PreviewWnd::OnKeyDown,
		&PreviewWnd::OnLButtonDown,
		&PreviewWnd::OnLButtonUp,
		&PreviewWnd::OnMouseMove,
		&PreviewWnd::OnMouseWheel,
		&PreviewWnd::OnPaint,
		&PreviewWnd::OnProjectChanged,
		&PreviewWnd::OnSetTerrain,
		&PreviewWnd::OnStateChanged,
		&PreviewWnd::OnTextureAllocate,
		&PreviewWnd::OnTextureCommit
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

void PreviewWnd::BuildZeroLayerVB()
{
	ColouredVertex *vertices;
	if (FAILED(zero_layer_vb_->Lock(0, 0, ri_cast<void**>(&vertices), 0L)))
		MacroDisplayError(_T("Zero layer vertex buffer could not be locked."));
	vertices[0].x     = 0.0f;
	vertices[0].y     = 0.0f;
	vertices[0].z     = 0.0f;
	vertices[0].color = zero_layer_colour_;
	vertices[1].x     = static_cast<FLOAT>(map_size_.cx);	
	vertices[1].y     = 0.0f;
	vertices[1].z     = 0.0f;
	vertices[1].color = zero_layer_colour_;
	vertices[2].x     = static_cast<FLOAT>(0);	
	vertices[2].y     = static_cast<FLOAT>(map_size_.cy);
	vertices[2].z     = 0.0f;
	vertices[2].color = zero_layer_colour_;
	vertices[3].x     = static_cast<FLOAT>(map_size_.cx);	
	vertices[3].y     = static_cast<FLOAT>(map_size_.cy);
	vertices[3].z     = 0.0f;
	vertices[3].color = zero_layer_colour_;
	zero_layer_vb_->Unlock();
}

bool PreviewWnd::InitializeDevice()
{
	HRESULT result;
	// get window dimensions
	RECT client_rect;
	GetClientRect(hwnd_, &client_rect);
	// define presentation parameters
	D3DPRESENT_PARAMETERS d3d_params;
	ZeroMemory(&d3d_params, sizeof(d3d_params));
	d3d_params.AutoDepthStencilFormat = D3DFMT_D16;
	d3d_params.BackBufferCount        = 1;
	d3d_params.BackBufferHeight       = client_rect.bottom;
	d3d_params.BackBufferWidth        = client_rect.right;
	d3d_params.EnableAutoDepthStencil = TRUE;
	d3d_params.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	d3d_params.Windowed               = true;
	// create or reset the device
	if (NULL == device_)
	{
		result = d3d_->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			hwnd_,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3d_params,
			&device_);
		if (D3D_OK != result)
		{
			MacroDisplayError(_T("Direct3D9 device could not be acquired."));
			device_ = NULL;
			DestroyWindow(hwnd_);
			return false;
		}
	}
	else
	{
		// wait for the device_ to become accessible
		for (bool quit(false); !quit;)
		{
			result = device_->TestCooperativeLevel();
			switch (result)
			{
			case D3D_OK:
			case D3DERR_DEVICENOTRESET:
				quit = true;
				break;
			case D3DERR_DRIVERINTERNALERROR:
				MacroDisplayError(_T("Internal Driver Error on IDirect3DDevice9::TestCooperativeLevel."));
				DestroyWindow(hwnd_);
				return false;
			default:
				Sleep(500);
			}
		}
		// reset the device_
		result = device_->Reset(&d3d_params);
		if ((D3D_OK != result))
		{
			MacroDisplayError(_T("Direct3DDevice9 could not be reset."));
			DestroyWindow(hwnd_);
			return false;
		}
		// set some states
		device_->SetRenderState(D3DRS_FOGCOLOR, fog_colour_);
		device_->SetRenderState(D3DRS_FOGSTART, *ri_cast<DWORD*>(&fog_start_));
		device_->SetRenderState(D3DRS_FOGEND,   *ri_cast<DWORD*>(&fog_end_));
	}
	// configure the device_
	device_->SetSamplerState(0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP);
	device_->SetSamplerState(0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP);
	device_->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	device_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device_->SetRenderState(D3DRS_ZENABLE,          D3DZB_TRUE);
	device_->SetRenderState(D3DRS_LIGHTING,         FALSE);
	device_->SetRenderState(D3DRS_AMBIENT,          0x0000FF00);
	device_->SetRenderState(D3DRS_CULLMODE,         D3DCULL_CCW);
	device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device_->SetRenderState(D3DRS_SRCBLEND,         D3DBLEND_SRCALPHA);
	device_->SetRenderState(D3DRS_DESTBLEND,        D3DBLEND_INVSRCALPHA);
	device_->SetRenderState(D3DRS_FOGENABLE,        TRUE);
	device_->SetRenderState(D3DRS_FOGTABLEMODE,     D3DFOG_LINEAR);
	// set wireframe mode if necessary
	if (wireframe_mode_)
	{
		device_->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
		device_->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	}
	else
	{
		device_->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
		device_->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	}
	MakeViewMatrix();
	MakeProjectiveMatrix();
	return true;
}

void PreviewWnd::MakeProjectiveMatrix()
{
	// get window size
	RECT client_rect;
	GetClientRect(hwnd_, &client_rect);
	// set the projection matrix
	int dim = __max(map_size_.cx, map_size_.cy);
	D3DXMatrixPerspectiveFovLH(
		&proj_matrix_, D3DX_PI / 3.0f,
		static_cast<FLOAT>(client_rect.right) / static_cast<FLOAT>(client_rect.bottom),
		__max(10.0f, radius_ - dim),
		radius_ + dim);
}

void PreviewWnd::MakeViewMatrix()
{
	D3DXVECTOR3 eye; // used in both the view and the billboard angle calculations
	// create the view matrix
	float proj(cos(angle_v_) * radius_);
	eye = D3DXVECTOR3(cos(angle_h_) * proj, sin(angle_h_) * proj, sin(angle_v_) * radius_);
	eye += target_;
	D3DXVECTOR3 up(-eye.x, -eye.y, 1.0f);
	up += target_;
	D3DXMatrixLookAtLH(&view_matrix_, &eye, &target_, &up);
	// set billboard angles
	foreach (Marker &marker, markers_)
		marker.CalculateAngle(eye.x, eye.y);
}

void PreviewWnd::PopWorldMatrix()
{
	_ASSERTE(!world_matrix_stack_.empty());
	world_matrix_stack_.pop();
	device_->SetTransform(D3DTS_WORLD, &world_matrix_stack_.top());
}

void PreviewWnd::ProjectChanged()
{
	SendProjectChanged(hwnd_);
}

void PreviewWnd::HighlightMarker(size_t marker_index)
{
	_ASSERTE(marker_index < markers_.size());
	if (marker_index >= markers_.size())
		return;
	for (size_t i(0); i != markers_.size(); ++i)
		markers_[i].Highlight(i == marker_index);
	Render();
}

void PreviewWnd::ProjectDataChanged(int type)
{
	SendStateChanged(hwnd_, type);
}

void PreviewWnd::UpdateSettings()
{
	zero_layer_colour_ = D3DCOLOR_ARGB(
		MacroAppData(ID_ZERO_LAYER_OPACITY),
		GetRValue(MacroAppData(ID_ZERO_LAYER_COLOUR)),
		GetGValue(MacroAppData(ID_ZERO_LAYER_COLOUR)),
		GetBValue(MacroAppData(ID_ZERO_LAYER_COLOUR)));
	BuildZeroLayerVB();
	Render();
}

void PreviewWnd::PushWorldMatrix(const D3DXMATRIX &matrix)
{
	_ASSERTE(!world_matrix_stack_.empty());
	D3DXMATRIX new_matrix;
	D3DXMatrixMultiply(&new_matrix, &world_matrix_stack_.top(), &matrix);
	world_matrix_stack_.push(new_matrix);
	device_->SetTransform(D3DTS_WORLD, &new_matrix);
}

void PreviewWnd::Render()
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
	{
		_RPT0(_CRT_WARN, _T("Warning. Recursion in PreviewWnd::Render.\n"));
		return;
	}
	// check preconditions
	if (NULL == device_)
		return;
	// render
	HRESULT result;
	bool repeat_render(true);
	while (repeat_render)
	{
		repeat_render = false;
		// clear the device
		if (FAILED(device_->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x003A6EA5, 1.0f, 0)))
			MacroDisplayError(_T("IDirect3DDevice9::Clear failed."));
		if (!is_empty_)
		{
			// set the matrices
			if (FAILED(device_->SetTransform(D3DTS_VIEW, &view_matrix_)))
				MacroDisplayError(_T("IDirect3DDevice9::SetTransform failed."));
			if (FAILED(device_->SetTransform(D3DTS_PROJECTION, &proj_matrix_)))
				MacroDisplayError(_T("IDirect3DDevice9::SetTransform failed."));
			// render the scene
			if (!FAILED(device_->BeginScene()))
			{
				// stretch the scene
				D3DXMATRIX stretch_matrix;
				D3DXMatrixScaling(&stretch_matrix, 1.0f, 1.0f, world_stretch_);
				StackedMatrix stacked_stretch_matrix(*this, stretch_matrix);
				// render terrain
				if (textures_valid_ && terrain_valid_)
				{
					// prepare for rendering
					device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
					device_->SetRenderState(D3DRS_ZWRITEENABLE,     TRUE);
					device_->SetFVF(TerrainVertex::FVF);
					if (D3D_OK != device_->SetStreamSource(0, terrain_vb_, 0, sizeof(TerrainVertex)))
					{
						MacroDisplayError(_T("IDIrect3DDevice9::SetStreamSource failed."));
						DestroyWindow(hwnd_);
						return;
					}
					// render the map by sections
					foreach (Section &section, sections_)
					{
						if (0 == section.face_count_)
							continue;
						if (D3D_OK != device_->SetTexture(0, section.texture_))
						{
							MacroDisplayError(_T("IDIrect3DDevice9::SetTexture failed."));
							DestroyWindow(hwnd_);
							return;
						}
						result = device_->DrawPrimitive(
							D3DPT_TRIANGLELIST,
							section.vertex_offset_,
							section.face_count_);
						if (D3D_OK != result)
						{
							if (D3DERR_OUTOFVIDEOMEMORY != result)
							{
								MacroDisplayError(_T("IDIrect3DDevice9::DrawPrimitive failed."));
								DestroyWindow(hwnd_);
								return;
							}
							MacroDisplayError(_T("There was not enough video memory to render map preview.\nOptimize the map, or raise mesh threshold level in Preferences."));
							if (FAILED(device_->EndScene()))
								MacroDisplayError(_T("IDirect3DDevice9::EndScene failed"));
							break;
						}
					}
				}
				// prepare for alphablended rendering
				device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device_->SetRenderState(D3DRS_ZWRITEENABLE,     FALSE);
				device_->SetTexture(0, NULL);
				// render the zero level
				{
					device_->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
					device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
					// set zero layer height
					D3DXMATRIX zl_height_matrix;
					D3DXMatrixTranslation(&zl_height_matrix, 0, 0, (zero_level_ + 0.5f) * world_stretch_);
					StackedMatrix stacked_zlr_height_matrix(*this, zl_height_matrix);
					// render
					device_->SetFVF(ColouredVertex::FVF);
					if (D3D_OK != device_->SetTexture(0, NULL))
						MacroDisplayError(_T("IDirect3DDevice9::SetTexture failed."));
					if (D3D_OK != device_->SetStreamSource(0, zero_layer_vb_, 0, sizeof(ColouredVertex)))
						MacroDisplayError(_T("IDirect3DDevice9::SetStreamSource failed."));
					if (D3D_OK != device_->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2))
						MacroDisplayError(_T("IDirect3DDevice9::DrawPrimitive failed."));
				}
				// render the frame markers
				device_->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
				device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				foreach(const Marker &marker, markers_)
				{
					// set marker position
					D3DXMATRIX frame_marker_matrix;
					{
						D3DXMATRIX frame_marker_rotation;
						D3DXMatrixRotationZ(&frame_marker_rotation, marker.angle_);
						D3DXMatrixTranslation(
							&frame_marker_matrix,
							marker.position_.x,
							marker.position_.y,
							0);
						D3DXMatrixMultiply(&frame_marker_matrix, &frame_marker_rotation, &frame_marker_matrix);
					}
					StackedMatrix stacked_frame_marker_matrix(*this, frame_marker_matrix);
					// render the marker
					if (D3D_OK != device_->SetStreamSource(0, marker.vertices_, 0, sizeof(ColouredVertex)))
					{
						MacroDisplayError(_T("IDirect3DDevice9::SetStreamSource failed."));
						break;
					}
					if (D3D_OK != device_->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 5))
					{
						MacroDisplayError(_T("IDirect3DDevice9::DrawPrimitive failed."));
						break;
					}
				}
				// end the scene
				if (FAILED(device_->EndScene()))
					MacroDisplayError(_T("IDirect3DDevice9::EndScene failed."));
			}
			else
				MacroDisplayError(_T("IDirect3DDevice9::BeginScene failed."));
		}
		// present or reset
		result = device_->Present(NULL, NULL, NULL, NULL);
		switch(result)
		{
		case D3DERR_DEVICELOST:
			InitializeDevice();
			repeat_render = true;
			break;
		case D3DERR_DRIVERINTERNALERROR:
			MacroDisplayError(_T("Internal driver error when rendering."));
			DestroyWindow(hwnd_);
			return;
		case D3DERR_INVALIDCALL:
			MacroDisplayError("IDirect3D9::Present - invalid call.");
			DestroyWindow(hwnd_);
			return;
		}
	}
}

void PreviewWnd::SetTitle(uint polycount)
{
	tstringstream stream;
	stream << _T("Map Preview (") << polycount << _T(" polygons)");
	SetWindowText(hwnd_, stream.str().c_str());
}

//-------------------------------------
// PreviewWnd::Billboard implementation
//-------------------------------------

PreviewWnd::Billboard::Billboard()
	:vertices_(NULL)
{}

void PreviewWnd::Billboard::CalculateAngle(float eye_x, float eye_y)
{
	angle_ = atan2(eye_y - position_.y, eye_x - position_.x) - D3DX_PI / 2;
}

//----------------------------------
// PreviewWnd::Marker implementation
//----------------------------------

PreviewWnd::Marker::Marker(D3DCOLOR colour)
	:colour_     (colour)
	,highlighted_(false)
{}

D3DCOLOR PreviewWnd::Marker::GetColour()
{
	return colour_;
}

bool PreviewWnd::Marker::Highlight(bool on)
{
	_ASSERTE(NULL != vertices_);
	// set vertices
	ColouredVertex *vertices;
	if (FAILED(vertices_->Lock(0, 0, ri_cast<void**>(&vertices), 0L)))
		return false;
	const D3DCOLOR opaque(on ? 0xFFFFFFFF : (colour_ | 0xFF000000));
	vertices[2].color = opaque;
	vertices[4].color = opaque;
	vertices_->Unlock();
	highlighted_ = on;
	return true;
}

bool PreviewWnd::Marker::Initialize(IDirect3DDevice9 *device)
{
	if (NULL != vertices_)
		vertices_->Release();
	return SUCCEEDED(device->CreateVertexBuffer(
		7 * sizeof(ColouredVertex),
		0,
		ColouredVertex::FVF,
		D3DPOOL_MANAGED,
		&vertices_,
		NULL));
}

void PreviewWnd::Marker::Release()
{
	if (NULL != vertices_)
		vertices_->Release();
}

bool PreviewWnd::Marker::Set(D3DXVECTOR2 position)
{
	_ASSERTE(NULL != vertices_);
	// set position
	position_ = position;
	// set vertices
	ColouredVertex *vertices;
	if (FAILED(vertices_->Lock(0, 0, ri_cast<void**>(&vertices), 0L)))
		return false;
	const FLOAT radius(4.0f);
	const FLOAT height(512.0f);
	const D3DCOLOR clear  (colour_ & D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0xFF));
	const D3DCOLOR opaque (highlighted_ ? 0xFFFFFFFF : (colour_ | 0xFF000000));
	vertices[0] = ColouredVertex(-radius,     0.0f, 0.0f,   clear);
	vertices[1] = ColouredVertex(-radius,     0.0f, height, 0L);
	vertices[2] = ColouredVertex(-radius / 4, 0.0f, 0.0f,   opaque);
	vertices[3] = ColouredVertex(0.0f,        0.0f, height, 0L);
	vertices[4] = ColouredVertex(radius / 4,  0.0f, 0.0f,   opaque);
	vertices[5] = ColouredVertex(radius,      0.0f, height, 0L);
	vertices[6] = ColouredVertex(radius,      0.0f, 0.0f,   clear);
	vertices_->Unlock();
	return true;
}

void PreviewWnd::Marker::SetColour(D3DCOLOR colour)
{
	colour_ = colour;
}
