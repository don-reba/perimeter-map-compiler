#pragma once
#include "data.h"
#include "preference data.h"
#include "viewer.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>
#include <stack>
#include <list>

class CPreview : public CViewer
{
public:
	// data that persists through sessions and can be changed at the user's whim
	struct CSerializable : public CPreferenceData
	{
		CSerializable(CPreview *parent);
		// interface
		virtual void Save(string file_name);
		virtual void Load(string file_name);
		virtual void Update();
		// data
		D3DCOLOR zero_layer_colour;
		float    threshold;
	protected:
		CPreview *parent;
	};
protected:
	// map segment for rendering
	struct CSection
	{
		IDirect3DTexture9 *texture;
		int vertex_offset;
		int face_count;
	};
	// custom vertex
	struct CVertex
	{
		FLOAT x, y, z;
		FLOAT u, v;
		static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX2;
	};
	// zero layer vertex
	struct CColouredVertex
	{
		FLOAT x, y, z;
		D3DCOLOR color;
		static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
	};
	// simple triangulation vertex
	struct CSimpleVertex
	{
		float x, y, z;
	};
	// triangulation element
	struct CTE
	{
		CTE() :
			l_constrained(false),
			r_constrained(false),
			b_constrained(false),
			t_constrained(false)
		{}
		struct CConstraint
		{
			inline float ConstrainedVal(int position) const
			{
				_ASSERTE(position > start);
				_ASSERTE(position < start + length);
				_ASSERTE(position != start);
				_ASSERTE(position != start + length);
				return s_val + (f_val - s_val) * (position - start) / (float)length;
			}
			short start, length; // direction is implied
			float s_val, f_val;
		};
		// data
		bool l_constrained;
		bool r_constrained;
		bool b_constrained;
		bool t_constrained;
		CConstraint l_constraint;
		CConstraint r_constraint;
		CConstraint b_constraint;
		CConstraint t_constraint;
		int index;
	};
	// billboard structure
	struct CBillboard
	{
		CBillboard();
		void CalculateAngle(float eye_x, float eye_y);
		D3DXVECTOR2 position;
		FLOAT       angle;
		IDirect3DVertexBuffer9 *vertices;
	};
	// lightweight wrapper for IDirect3DVertexBuffer9
	struct CVertexBuffer
	{
		CVertexBuffer() : _vertices(NULL), _face_count(0) {}
		CVertex *_vertices;
		UINT _face_count;
	};
public:
	// construction/destruction
	CPreview(
		CData<CStaticArray<BYTE> > *heightmap,
		CData<CPalettedTexture>    *texture,
		CData<CMapInfo>            *map_info);
	~CPreview(void);
public:
	// interface
	void Create(HWND hWndParent, const RECT &window_rect, HWND hButton, const TCHAR * const ini_path);
	void Destroy();
	virtual void Update(int caller);
	virtual void ToggleVisibility(bool show);
	virtual CViewer::WndSaveInfo GetSaveInfo();
protected:
	// window functions
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL   OnCaptureChanged(HWND hWnd, HWND capture_reciever);
	BOOL   OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
	BOOL   OnDestroy(HWND hWnd);
	BOOL   OnEraseBkgnd(HWND hWnd, HDC hDC);
	BOOL   OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
	BOOL   OnKeyDown(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
	BOOL   OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
	BOOL   OnLButtonUp(HWND hWnd, int x, int y, UINT keyFlags);
	BOOL   OnMouseMove(HWND hWnd, int x, int y, UINT codeHitTest);
	BOOL   OnMouseWheel(HWND hWnd, int xPos, int yPos, int zDelta, UINT fwKeys);
	BOOL   OnPaint(HWND hWnd);
	BOOL   OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg);
	BOOL   OnShowWindow(HWND hWnd, BOOL fShow, UINT status);
	BOOL   OnSize(HWND hWnd, UINT state, int cx, int cy);
	// DX functions
	void InitializeDevice();
	void Render();
	// data processing functions
	static DWORD WINAPI HeightmapThreadProc(LPVOID lpParameter);
	static DWORD WINAPI MapInfoThreadProc(LPVOID lpParameter);
	static DWORD WINAPI TextureThreadProc(LPVOID lpParameter);
	static DWORD WINAPI UsageTracker(LPVOID lpParameter);
	// utility functions
	void    ActivateTerrainVBs();
	void    BuildFrameMarker(CBillboard *marker, D3DCOLOR marker_colour);
	void    BuildZeroLayerVB();
	void    BuildVB();
	void    DeleteTerainVBs();
	float   Flatness(int h1, int h2, int v1, int v2, int c, int r) const;
	void    InitializeTextures();
	void    InitializeVB(vector<CSimpleVertex> &vertices);
	void    MakeViewMatrix();
	void    MakeProjectiveMatrix();
	void    PopWorldMatrix();
	void    PushWorldMatrix(const D3DXMATRIX &matrix);
	void    ReleaseTerrainVBs();
	HRESULT SetMeshStreamSource(std::list<CVertexBuffer>::iterator vb);
	void    SplitBuffer(std::list<CVertexBuffer>::iterator vb);
	void    ToggleWaitCursor(bool on);
	void    Triangulate(vector<CSimpleVertex> &vertices);
	void    UpdateData();
	void    Error(string message) const;
	// math
	inline int log2(unsigned int n) const;
	inline int exp2(unsigned int n) const;
protected:
	// window
	HWND hWnd;
	HWND hButton;
	RECT window_rect;
	bool is_visible;
	LONG is_busy; // LONG for use of interlocked operations
	LONG heightmap_update_pending;
	LONG texture_update_pending;
	LONG map_info_update_pending;
	// camera
	float       angle_h;
	float       angle_v;
	float       radius;
	D3DXVECTOR3 target;
	D3DXVECTOR3 old_target;
	D3DXMATRIX  view_matrix;
	D3DXMATRIX  proj_matrix;
	POINT       mouse_click_pos;
	bool        mouse_captured;
	// data
	CData<CStaticArray<BYTE> > *heightmap;
	CData<CPalettedTexture>    *texture;
	CData<CMapInfo>            *map_info;
	string tmp_file_path;
	HANDLE tmp_file;
	FLOAT zero_plast;
	SIZE  map_size;
	bool  heightmap_valid;
	bool  map_info_valid;
	bool  texture_valid;
	// multithreading
	HANDLE heightmap_thread;
	HANDLE map_info_thread;
	HANDLE texture_thread;
	CRITICAL_SECTION billboard_section;
	CRITICAL_SECTION matrix_section;
	CRITICAL_SECTION texture_section;
	CRITICAL_SECTION vb_section;
	CRITICAL_SECTION zero_layer_vb_section;
	clock_t          last_vb_access;
	bool             track_usage;
	// DX variables
	IDirect3D9             *d3d;
	IDirect3DDevice9       *device;
	IDirect3DVertexBuffer9 *vb;
	IDirect3DVertexBuffer9 *zero_layer_vb;
	IDirect3DVertexBuffer9 *active_vb;
	CVertexBuffer          *active_vb_copy;
	std::list<CVertexBuffer> terrain_vbs;
	vector<CSection> sections;
	CBillboard       billboards[5];
	std::stack<D3DXMATRIX> world_matrix_stack;
	bool     wireframe_mode;
	FLOAT    world_stretch;
	D3DCOLOR zero_layer_colour;
public:
	// serializable data
	CSerializable settings;
};
