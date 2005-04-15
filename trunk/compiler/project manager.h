#pragma once

#include "data.h"
#include "info manager.h"
#include "preference data.h"
#include "preview.h"
#include "stat manager.h"
#include <map>
#include <jasper\jasper.h>
#include <FreeImage\Wrapper\FreeImagePlus\FreeImagePlus.h>
#include <tinyxml/tinyxml.h>

class CProjectManager
{
public:
	// view window creation attributes
	struct WndAttributes
	{
		RECT rect;
		HWND button;
		bool is_visible;
	};
	// data that persists through sessions and can be changed at the user's whim
	struct CSerializable : public CPreferenceData
	{
		CSerializable(CProjectManager *parent);
		// interface
		void Save(string file_name);
		void Load(string file_name);
		void Update();
		// data
		bool texture_colour_quality;
		bool enable_lighting;
	private:
		CProjectManager *parent;
	};

public:
	CProjectManager(void);
	~CProjectManager(void);

public:
	// interface
	void Create(
		HWND  hWnd,
		const WndAttributes &stat_manager_attributes,
		const WndAttributes &preview_attributes,
		const WndAttributes &info_manager_attributes,
		const TCHAR * const ini_path);
	void Destroy();
	void NewProject(string project_folder, SIZE map_size, string map_name);
	void OpenProject(string project_file);
	void Pack();
	bool Unpack(string shrub_file);
	void Install();
	void SaveMapThumb();
	void ToggleStatManager(bool show);
	void TogglePreview(bool show);
	void ToggleInfoManager(bool show);
	vector<CViewer::WndSaveInfo> GetSaveInfo();
	CPreview::CSerializable *GetPreviewSettings();
	CSerializable *GetProjectSettings();
	void SaveSettings(string ini_path);
protected:
	// static functions
	static VOID CALLBACK CheckFiles(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static DWORD WINAPI HeightmapLoadThreadProc(LPVOID lpParameter);
	static DWORD WINAPI TextureLoadThreadProc(LPVOID lpParameter);
private:
	// utility functions
	void   AppendBTDB(const TCHAR *path, const char *folder_name);
	void   AppendWorldsPrm(const TCHAR *path, const TCHAR *folder_name) const;
	void   CleanUp();
	void   CreateHeightmapThread();
	void   CreateTextureThread();
	void   CreateLightMap(const BYTE *heightmap, SIZE size);
	DWORD  CalculateChecksum();
	void   DefaultHeightmap();
	void   DefaultMapInfo();
	void   DefaultTexture();
	void   Error(string message) const;
	void   Extrapolate(int *pix, int w, int h) const;
	int    GenerateId() const;
	bool   GetInstallPath(string &install_path) const;
	DWORD  LoadFile(const TCHAR *name, BYTE *&pBuffer) const;
	void   LoadHeightmap();
	void   LoadTexture();
	int    PackHeightmap(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask);
	void   PackMapInfo(TiXmlNode &node);
	int    PackTexture(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask);
	bool   RegisterMap();
	void   SaveHeightmap(const TCHAR *path);
	void   SaveMapInfo(const TCHAR *path);
	bool   SaveMemToFile(const TCHAR *path, const BYTE *buffer, DWORD size) const;
	void   SaveMission(const TCHAR *path, const TCHAR *folder_name, bool survival);
	void   SavePalette(const TCHAR *path);
	void   SaveShrub(const TCHAR *path, const BYTE *buffer, DWORD size) const;
	void   SaveSPG(const TCHAR *path, const TCHAR *folder_name, const bool survival);
	void   SaveSPH(const TCHAR *path, const TCHAR *folder_name, const bool survival);
	void   SaveTexture(const TCHAR *path);
	bool   SaveThumb(const TCHAR *path, SIZE size);
	void   SaveVMP(const TCHAR *path);
	void   StackBlur(int *pix, int w, int h, int radius) const;
	void   UnpackHeightmap(TiXmlNode *node, BYTE *buffer);
	bool   UnpackMapInfo(TiXmlNode *node);
	void   UnpackTexture(TiXmlNode *node, BYTE *buffer);
	// math
	inline int exp2(unsigned int n) const;
	inline int log2(unsigned int n) const;
protected:
	// main application framework
	CData<CStaticArray<BYTE> > heightmap;
	CData<CPalettedTexture>    texture;
	CData<CMapInfo>            map_info;
	CData<CStaticArray<BYTE> > lightmap;
	CInfoManager info_manager;
	CPreview     preview;
	CStatManager stat_manager;
	// window information
	HWND hWnd;
	RECT stat_window_rect;
	// multithreading
	HANDLE heightmap_thread;
	HANDLE texture_thread;
	// project settings
	size_t map_placement;
	enum { PS_INACTIVE, PS_PROJECT, PS_SHRUB } project_state;
	// times the files were last written to
	FILETIME heightmap_time;
	FILETIME texture_time;
	// timer for keeping track of file's status
	UINT_PTR file_timer;
	// flag to prevent resource tracking when needed
	LONG track_resources;
protected:
// serializable settings
	CSerializable settings;
};
