#include "StdAfx.h"
#include "btdb.h"
#include "project manager.h"
#include "resource.h"
#include <algorithm>
#include <bzip2-1.0.2\bzlib.h>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <set>
#include <shlobj.h>
#include <sstream>
#include <Wininet.h>
using namespace std;

#undef max // conflict with numeric_limits
#undef min // conflict with numeric_limits


CProjectManager::CProjectManager(void) :
	stat_manager     (&heightmap, &texture, &map_info),
	preview          (&heightmap, &texture, &map_info, &lightmap),
	info_manager     (&map_info, GenerateId()),
	file_timer       (NULL),
	heightmap_thread (NULL),
	texture_thread   (NULL),
	hWnd             (NULL),
	project_state    (PS_INACTIVE),
	settings         (this),
	track_resources  (true)
{
	// link viewers
	heightmap.AddViewer(&preview);
	heightmap.AddViewer(&stat_manager);
	texture.AddViewer(&preview);
	texture.AddViewer(&stat_manager);
	map_info.AddViewer(&info_manager);
	map_info.AddViewer(&preview);
	// initialize JasPer
	if (0 != jas_init())
		Error("JasPer initialization failed");
	// initialize FreeImage
	FreeImage_Initialise(TRUE);
}

CProjectManager::~CProjectManager(void)
{
	CleanUp();
	// uninitialize JasPer
	jas_image_clearfmts();
	//untinitialize FreeImage
	FreeImage_DeInitialise();
}

// initialize viewer windows and the like
void CProjectManager::Create(
	HWND  hWnd,
	const WndAttributes &stat_manager_attributes,
	const WndAttributes &preview_attributes,
	const WndAttributes &info_manager_attributes,
	const TCHAR * const ini_path)
{
	this->hWnd = hWnd;
	settings.Load(ini_path);
	stat_manager.Create(
		hWnd,
		stat_manager_attributes.rect,
		stat_manager_attributes.button);
	preview.Create(
		hWnd,
		preview_attributes.rect,
		preview_attributes.button,
		ini_path);
	info_manager.Create(
		hWnd,
		info_manager_attributes.rect,
		info_manager_attributes.button);
}

// destroy viewer windows and the like
void CProjectManager::Destroy()
{
	// back up information
	if (PS_PROJECT == project_state)
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_data.Save((map_data.folder_path + map_data.map_name + _T(".pmproj")).c_str());
		map_info.SignIn();
	}
	// destroy viewers
	stat_manager.Destroy();
	preview.Destroy();
	info_manager.Destroy();
	// destroy texture
	{
		CPalettedTexture &texture_data(texture.SignOut());
		if (NULL != texture_data.ptr)
		{
			texture_data.length = 0;
			delete [] texture_data.ptr;
		}
	}
	// destroy heightmap
	{
		CStaticArray<BYTE> &heightmap_data(heightmap.SignOut());
		if (NULL != heightmap_data.ptr)
		{
			heightmap_data.length = 0;
			delete [] heightmap_data.ptr;
		}
	}
	// destroy lightmap
	{
		CStaticArray<BYTE> &lightmap_data(lightmap.SignOut());
		if (NULL != lightmap_data.ptr)
		{
			lightmap_data.length = 0;
			delete [] lightmap_data.ptr;
		}
	}
}

// create the files of the new project and load them
void CProjectManager::NewProject(string project_folder, SIZE map_size, string map_name)
{
	TCHAR path[MAX_PATH];
	// create the project file path
	PathCombine(path, project_folder.c_str(), (map_name + _T(".pmproj")).c_str());
	// initialize map info
	{
		CMapInfo &map_data(map_info.SignOut());
		map_data.map_name = map_name;
		map_data.map_power_x = log2(map_size.cx);
		map_data.map_power_y = log2(map_size.cy);
		map_data.SaveCriticalInfo(path);
		map_info.SignIn();
	}
	// open the project
	OpenProject(path);
}

void CProjectManager::OpenProject(string project_file)
{
	CleanUp();
	string folder_path;
	// get the folder path
	{
		TCHAR temp_folder_path[MAX_PATH];
		_tcscpy(temp_folder_path, project_file.c_str());
		PathRemoveFileSpec(temp_folder_path);
		PathAddBackslash(temp_folder_path);
		folder_path = temp_folder_path;
		map_info.SignOut().folder_path = folder_path;
		map_info.SignIn();
	}
	// read in map information
	{
		CMapInfo &map_data(map_info.SignOut());
		map_data.Load(project_file.c_str());
		SetWindowText(hWnd, map_data.map_name.c_str());
		map_info.SignIn();
		map_info.Update(0);
	}
	// functor for subsequent work
	const struct {
		HANDLE operator() (const TCHAR *path) const
		{
			const DWORD dwDesiredAccess       = FILE_READ_ATTRIBUTES | SYNCHRONIZE;
			const DWORD dwShareMode           = FILE_SHARE_READ | FILE_SHARE_WRITE;
			const DWORD dwCreationDisposition = OPEN_EXISTING;
			const DWORD dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
			return CreateFile(
				path,
				dwDesiredAccess,
				dwShareMode,
				NULL,
				dwCreationDisposition,
				dwFlagsAndAttributes,
				NULL);
		}
	} GetFileHandle;
	// begin tracking the files
	{
		BY_HANDLE_FILE_INFORMATION info;
		HANDLE file_handle;
		string file_path;
		// load heightmap
		file_path = folder_path + _T("heightmap.bmp");
		file_handle = GetFileHandle(file_path.c_str());
		if (INVALID_HANDLE_VALUE == file_handle)
		{
			// create a default and try again
			DefaultHeightmap();
			SaveHeightmap(file_path.c_str());
			file_handle = GetFileHandle(file_path.c_str());
			// if that does not work, complain and disable tracking
			if (INVALID_HANDLE_VALUE == file_handle)
			{
				Error("heightmap.bmp could not be opened");
				heightmap_time.dwLowDateTime  = numeric_limits<DWORD>::max();
				heightmap_time.dwHighDateTime = numeric_limits<DWORD>::max();
			}
		}
		if (INVALID_HANDLE_VALUE != file_handle)
		{
			// remember the time the texture was last written to
			GetFileInformationByHandle(file_handle, &info);
			heightmap_time = info.ftLastWriteTime;
			CloseHandle(file_handle);
		}
		// load texture
		file_path = folder_path + _T("texture.bmp");
		file_handle = GetFileHandle(file_path.c_str());
		if (INVALID_HANDLE_VALUE == file_handle)
		{
			// create a default and try again
			DefaultTexture();
			SaveTexture(file_path.c_str());
			file_handle = GetFileHandle(file_path.c_str());
			// if that does not work, complain and disable tracking
			if (INVALID_HANDLE_VALUE == file_handle)
			{
				Error("texture.bmp could not be opened");
				texture_time.dwLowDateTime  = numeric_limits<DWORD>::max();
				texture_time.dwHighDateTime = numeric_limits<DWORD>::max();
			}
		}
		if (INVALID_HANDLE_VALUE != file_handle)
		{
			// remember the time the texture was last written to
			GetFileInformationByHandle(file_handle, &info);
			texture_time = info.ftLastWriteTime;
			CloseHandle(file_handle);
		}
		// start the tracking timer
		file_timer = SetTimer(hWnd, ri_cast<UINT_PTR>(this), 1000, CheckFiles);
		if (0 == file_timer)
			Error("tracking timer could not be started");
	}
	// actually load the data now
	CreateHeightmapThread();
	CreateTextureThread();
	// set project state
	project_state = PS_PROJECT;
}

// create a shrub file in the project folder
void CProjectManager::Pack()
{
	// first, register the map
	if (!RegisterMap())
		return;
	// get dimensions and name of the map
	SIZE map_size;
	string map_name;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_name = map_data.map_name;
		map_info.SignIn();
	}
	// initialise an XML metadata document
	TiXmlDocument doc;
	TiXmlNode *content_node(doc.InsertEndChild(TiXmlElement("content")));
	// allocate a buffer for the data
	size_t buffer_size;
	{
		const size_t heightmap_size(map_size.cx * map_size.cy);
		const size_t mask_size     (map_size.cx * map_size.cy / 8);
		const size_t texture_size  (map_size.cx * map_size.cy);
		const size_t palette_size  (256);
		buffer_size = heightmap_size + mask_size + texture_size + palette_size;
	}
	BYTE *buffer(new BYTE[buffer_size]);
	// pack heightmap, texture and map information
	{
		BYTE *buffer_iter(buffer);
		vector<bool> mask;
		PackMapInfo(*content_node->InsertEndChild(TiXmlElement("map_info")));
		buffer_iter += PackHeightmap(
			*content_node->InsertEndChild(TiXmlElement("heightmap")),
			buffer_iter,
			buffer,
			mask);
		buffer_iter += PackTexture(
			*content_node->InsertEndChild(TiXmlElement("texture")),
			buffer_iter,
			buffer,
			mask);
		// calculate how much of the buffer was used
		buffer_size = static_cast<size_t>(buffer_iter - buffer);
	}
	// augment the buffer with metadata
	{
		// output the XML metadata into a string
		string xml_string;
		xml_string << doc;
		size_t xml_string_size(xml_string.size() + 1);
		// append the xml metadata to the buffer
		BYTE *augmented_buffer(new BYTE[buffer_size + xml_string_size]);
		CopyMemory(augmented_buffer, xml_string.c_str(), xml_string_size);
		CopyMemory(augmented_buffer + xml_string_size, buffer, buffer_size);
		// replace the old buffer
		delete [] buffer;
		buffer = augmented_buffer;
		buffer_size += xml_string_size;
	}
	// allocate a new buffer for the compressed data
	const size_t header_size(12);
	size_t compressed_buffer_size(static_cast<size_t>(buffer_size * 1.01f) + 600); // as specified in bzip2 manual
	BYTE *compressed_buffer(new BYTE[compressed_buffer_size + header_size]);
	// compress the shrub
	if (BZ_OK != BZ2_bzBuffToBuffCompress(
		ri_cast<char*>(compressed_buffer + header_size),
		&compressed_buffer_size,
		ri_cast<char*>(buffer),
		buffer_size,
		9,
		0,
		0))
	{
		Error(_T("BZ2_bzBuffToBuffCompress failed"));
		delete [] buffer;
		delete [] compressed_buffer;
		return;
	}
	delete [] buffer;
	compressed_buffer_size += header_size;
	// record header
	{
		char compression_format[8] = { 'S', 'H', 'R', 0, 'B', 'Z', '2', 0 };
		CopyMemory(compressed_buffer, compression_format, 8);
		CopyMemory(compressed_buffer + 8, &buffer_size, 4);
	}
	// save the packed Biboorat into a shrub file
	TCHAR path[MAX_PATH];
	PathCombine(path, map_info.SignOutConst().folder_path.c_str(), map_name.c_str());
	map_info.SignIn();
	PathAddExtension(path, _T(".shrub"));
	SaveShrub(path, compressed_buffer, compressed_buffer_size);
	delete [] compressed_buffer;
	// reassure the user
	MessageBox(
		hWnd,
		_T("The map has been successfully packed."),
		_T("Success"),
		MB_ICONINFORMATION | MB_OK);
}

bool CProjectManager::Unpack(string shrub_file)
{
	InterlockedExchange(&track_resources, false);
	// get the folder path
	{
		TCHAR temp_folder_path[MAX_PATH];
		_tcscpy(temp_folder_path, shrub_file.c_str());
		PathRemoveFileSpec(temp_folder_path);
		PathAddBackslash(temp_folder_path);
		map_info.SignOut().folder_path = temp_folder_path;
		map_info.SignIn();
	}
	// load the packed Biboorat from the shrub file
	BYTE *compressed_buffer;
	const size_t file_size(LoadFile(shrub_file.c_str(), compressed_buffer));
	// check that the file has an acceptable format
	{
		BYTE word[4];
		// check the magic number
		CopyMemory(word, compressed_buffer, 4);
		if (word[0] != 'S' || word[1] != 'H' || word[2] != 'R' || word[3] != 0)
		{
			Error("the file is not a valid shrub");
			delete [] compressed_buffer;
			InterlockedExchange(&track_resources, true);
			return false;
		}
		// check the compression format
		CopyMemory(word, compressed_buffer + 4, 3);
		if (word[0] != 'B' || word[1] != 'Z' || word[2] != '2')
		{
			Error("the shrub has an unfamiliar format");
			delete [] compressed_buffer;
			InterlockedExchange(&track_resources, true);
			return false;
		}
	}
	// read in the size of the buffer and allocate memory
	size_t buffer_size(0);
	CopyMemory(&buffer_size, compressed_buffer + 8, 4);
	BYTE *buffer(new BYTE[buffer_size]);
	// decompress the shrub
	{
		int result(BZ2_bzBuffToBuffDecompress(
			ri_cast<char*>(buffer),
			&buffer_size,
			ri_cast<char*>(compressed_buffer + 12),
			file_size - 12,
			0,
			0));
		if (BZ_OK != result)
		{
			switch (result)
			{
			case BZ_MEM_ERROR:
				Error(_T("Insufficient memory for decompression."));
				break;
			case BZ_OUTBUFF_FULL:
				Error(_T("Outbuffer full. The shrub file is corrupt."));
				break;
			case BZ_DATA_ERROR:
				Error(_T("The shrub file is corrupt."));
				break;
			case BZ_DATA_ERROR_MAGIC:
				Error(_T("Magic number mismatch. The shrub file is corrupt."));
				break;
			case BZ_UNEXPECTED_EOF:
				Error(_T("Unexpected end of file during decompression."));
				break;
			default:
				Error(_T("BZ2_bzBuffToBuffDecompress failed"));
			};
			delete [] buffer;
			delete [] compressed_buffer;
			InterlockedExchange(&track_resources, true);
			return false;
		}
	}
	delete [] compressed_buffer;
	// read in xml information
	TiXmlDocument doc;
	size_t xml_string_size(strlen(ri_cast<char*>(buffer)) + 1);
	doc.Parse(ri_cast<char*>(buffer)); // possible buffer overflow
	// get the content node
	TiXmlElement *content_node(doc.FirstChildElement("content"));
	if (NULL == content_node)
	{
		Error("the shrub has an unfamiliar format");
		delete [] buffer;
		InterlockedExchange(&track_resources, true);
		return false;
	}
	// unpack map_info, heightmap, and texture
	UnpackMapInfo(content_node->FirstChildElement("map_info"));
	UnpackHeightmap(content_node->FirstChildElement("heightmap"), buffer + xml_string_size);
	UnpackTexture(content_node->FirstChildElement("texture"), buffer + xml_string_size);
	// clean up
	delete [] buffer;
	// set project state
	project_state = PS_SHRUB;
	InterlockedExchange(&track_resources, true);
	return true;
}

void CProjectManager::Install()
{
	TCHAR str[MAX_PATH];
	TCHAR str2[MAX_PATH];
	string install_path;
	TCHAR folder_path[MAX_PATH];
	bool overwriting(false);
	if (!GetInstallPath(install_path))
		return;
	// get the name to use for stuff that needs a name
	// this is the map name for a registered map, and "UNREGISTERED" otherwise
	// ASSUME shrubs are registered
	string folder_name;
	if (PS_SHRUB == project_state)
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		folder_name = map_data.map_name;
		map_info.SignIn();
	}
	else
		folder_name = _T("UNREGISTERED");
	// create a directory for the map
	{
		// create path
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Worlds"));
		PathCombine(folder_path, folder_path, folder_name.c_str());
		// create directory
		int result(SHCreateDirectoryEx(hWnd, folder_path, NULL));
		switch (result)
		{
		case ERROR_SUCCESS:
			overwriting = true;
			break;
		case ERROR_ALREADY_EXISTS:
			{
				if (PS_SHRUB == project_state && IDCANCEL == MessageBox(
					hWnd,
					_T("Map with this name already exists. Continuing will overwrite its contents."),
					_T("Installation Error"),
					MB_ICONWARNING | MB_OKCANCEL))
					return;
			} break;
		default:
			Error(_T("SHCreateDirectoryEx failed"));
			return;
		}
	}
	// create and save map.tga
	{
		PathCombine(str, folder_path, _T("map.tga"));
		SIZE size = { 128, 128 };
		SaveThumb(str, size);
	}
	// create and save world.ini
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		PathCombine(str, folder_path, _T("world.ini"));
		ofstream world_ini(str, ios_base::binary | ios_base::out);
		world_ini << map_data.GenerateWorldIni();
		map_info.SignIn();
	}
	// create and save output.vmp
	PathCombine(str, folder_path, _T("output.vmp"));
	SaveVMP(str);
	// create and save inDam.act
	PathCombine(str, folder_path, _T("inDam.act"));
	SavePalette(str);
	// append Texts.btdb
	{
		string language;
		// get the language of the distribution
		{
			PathCombine(str, install_path.c_str(), "Perimeter.ini");
			ifstream ini(str);
			if (!ini.is_open())
			{
				Error("Perimeter.ini could not be opened.");
				return;
			}
			string line;
			string target("DefaultLanguage=");
			while (ini)
			{
				getline(ini, line);
				size_t pos(line.find(target));
				if (line.npos != pos)
				{
					language = line.substr(pos + target.size(), line.size() - target.size());
					break;
				}
			}
		}
		//GetPrivateProfileString("Game", "DefaultLanguage", "Russian--", language, language_size, str);
		PathCombine(str, install_path.c_str(), "RESOURCE\\LocData");
		PathCombine(str, str, language.c_str());
		PathCombine(str, str, "Text\\Texts.btdb");
		AppendBTDB(str, folder_name.c_str());
	}
	// append WORLDS.PRM
	PathCombine(str, install_path.c_str(), "RESOURCE\\Worlds\\WORLDS.PRM");
	AppendWorldsPrm(str, folder_name.c_str());
	// copy some files from GLETSCHER's folder
	// TODO: clean up on failure
	{
		// functor for copying a file from GLETSCHER to the map folder
		const struct CopyFromGLETSCHER_T
		{
			CopyFromGLETSCHER_T(
				const CProjectManager *obj,
				      TCHAR           *str,
				      TCHAR           *str2,
				const TCHAR           *folder_path,
				const TCHAR           *install_path)
				: obj(obj), str(str), str2(str2), folder_path(folder_path), install_path(install_path)
			{}
			inline bool operator() (const TCHAR *file_name) const
			{
				PathCombine(str2, install_path, _T("RESOURCE\\Worlds\\GLETSCHER"));
				PathCombine(str2, str2, file_name);
				PathCombine(str, folder_path, file_name);
				if (FALSE == CopyFile(str2, str, FALSE))
				{
					string msg(file_name);
					msg.append(" could not be copied. Make sure the version of your game is at least 1.01.");
					obj->Error(msg.c_str());
					return false;
				}
				return true;
			}
			const CProjectManager *obj;
			TCHAR *str;
			TCHAR *str2;
			const TCHAR *folder_path;
			const TCHAR *install_path;
		} CopyFromGLETSCHER(this, str, str2, folder_path, install_path.c_str());
		// up.tga
		if (!CopyFromGLETSCHER(_T("up.tga")))
			return;
		// leveledSurfaceTexture.tga
		if (!CopyFromGLETSCHER(_T("leveledSurfaceTexture.tga")))
			return;
		// geoLattice.bin
		if (!CopyFromGLETSCHER(_T("geoLattice.bin")))
			return;
		// inGeo.act
		if (!CopyFromGLETSCHER(_T("inGeo.act")))
			return;
	}
	// change to the mission directory
	PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Battle"));
	PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
	SaveMission(folder_path, folder_name.c_str(), false);
	PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
	PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
	SaveMission(folder_path, folder_name.c_str(), false);
	PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
	PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
	SaveMission(folder_path, folder_name.c_str(), false);
	// reassure the user
	MessageBox(
		hWnd,
		_T("Installation has been completed successfully."),
		_T("Success"),
		MB_ICONINFORMATION | MB_OK);
}

void CProjectManager::SaveMapThumb()
{
	TCHAR path[MAX_PATH];
	_tcscpy(path, map_info.SignOutConst().folder_path.c_str());
	PathCombine(path, path, "thumbnail.png");
	SIZE size = { 256, 256 };
	if (SaveThumb(path, size))
		MessageBox(hWnd, _T("The thumbnail was saved in the project folder."), _T("Success"), MB_OK);
}

void CProjectManager::ToggleStatManager(bool show)
{
	stat_manager.ToggleVisibility(show);
}

void CProjectManager::TogglePreview(bool show)
{
	preview.ToggleVisibility(show);
}

void CProjectManager::ToggleInfoManager(bool show)
{
	info_manager.ToggleVisibility(show);
}

vector<CViewer::WndSaveInfo> CProjectManager::GetSaveInfo()
{
	vector<CViewer::WndSaveInfo> save_info;
	save_info.push_back(stat_manager.GetSaveInfo());
	save_info.push_back(preview.GetSaveInfo());
	save_info.push_back(info_manager.GetSaveInfo());
	return save_info;
}

CPreview::CSerializable *CProjectManager::GetPreviewSettings()
{
	return &preview.settings;
}

CProjectManager::CSerializable *CProjectManager::GetProjectSettings()
{
	return &settings;
}

void CProjectManager::SaveSettings(string ini_path)
{
	settings.Save(ini_path);
	preview.settings.Save(ini_path);
}

// track the files
VOID CALLBACK CProjectManager::CheckFiles(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	CProjectManager* project = ri_cast<CProjectManager*>(idEvent);
	// make sure tracking is allowed
	if (!*ri_cast<bool*>(&project->track_resources))
		return;
	// get folder path
	string folder_path = project->map_info.SignOutConst().folder_path;
	project->map_info.SignIn();
	// open files
	BY_HANDLE_FILE_INFORMATION info;
	BOOL information_retrieved;
	const DWORD dwDesiredAccess       = FILE_READ_ATTRIBUTES | SYNCHRONIZE;
	const DWORD dwShareMode           = FILE_SHARE_READ | FILE_SHARE_WRITE;
	const DWORD dwCreationDisposition = OPEN_EXISTING;
	const DWORD dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
	// open the heightmap file
	HANDLE heightmap_file = CreateFile(
		(folder_path + _T("heightmap.bmp")).c_str(),
		dwDesiredAccess,
		dwShareMode,
		NULL,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		NULL);
	if (INVALID_HANDLE_VALUE != heightmap_file)
	{
		// check the time the heightmap file was last written to
		information_retrieved = GetFileInformationByHandle(heightmap_file, &info);
		CloseHandle(heightmap_file);
		if (
			TRUE == information_retrieved &&
			0 > CompareFileTime(&project->heightmap_time, &info.ftLastWriteTime))
		{
			// reset the write time on the heightmap
			project->heightmap_time = info.ftLastWriteTime;
			// update the heightmap file
			project->CreateHeightmapThread();
		}
	}
	// open texture file
	HANDLE texture_file = CreateFile(
		(folder_path + _T("texture.bmp")).c_str(),
		dwDesiredAccess,
		dwShareMode,
		NULL,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		NULL);
	if (INVALID_HANDLE_VALUE != texture_file)
	{
		// check the time the texture file was last written to
		information_retrieved = GetFileInformationByHandle(texture_file, &info);
		CloseHandle(texture_file);
		if (
			TRUE == information_retrieved &&
			0 > CompareFileTime(&project->texture_time, &info.ftLastWriteTime))
		{
			// reset the write time on the texture 
			project->texture_time = info.ftLastWriteTime;
			// update the texture file
			project->CreateTextureThread();
		}
	}
}

// load the texture file in the background
DWORD WINAPI CProjectManager::HeightmapLoadThreadProc(LPVOID lpParameter)
{
	clock_t start_time(clock());
	CProjectManager *obj = ri_cast<CProjectManager*>(lpParameter);
	obj->LoadHeightmap();
	clock_t end_time(clock());
	_RPT1(_CRT_WARN, "CProjectManager::HeightmapLoadThreadProc: %i ticks\n", end_time - start_time);
	return 0;
}

// load the texture file in the background
DWORD WINAPI CProjectManager::TextureLoadThreadProc(LPVOID lpParameter)
{
	clock_t start_time(clock());
	CProjectManager *obj = ri_cast<CProjectManager*>(lpParameter);
	obj->LoadTexture();
	clock_t end_time(clock());
	_RPT1(_CRT_WARN, "CProjectManager::TextureLoadThreadProc: %i ticks\n", end_time - start_time);
	return 0;
}

// set the appropriate entry of Texts.btdb to the name of the map
void CProjectManager::AppendBTDB(const TCHAR *path, const char *folder_name)
{
	// set a prefix for the map name
	const char * const prefix((PS_SHRUB == project_state) ? "" : "[U]");
	const size_t prefix_size = strlen(prefix);
	// get the name of the map
	string map_name(map_info.SignOutConst().map_name);
	map_info.SignIn();
	// prefix name of the map
	map_name.insert(0, prefix);
	// replace some characters in the name of the map by others
	{
		char *source("_");
		char *target(" ");
		for (size_t i(prefix_size); i != map_name.size(); ++i)
		{
			char *chr_i(strchr(source, map_name[i]));
			if (NULL != chr_i)
				map_name[i] = *(target + (chr_i - source));
		}
	}
	CBTDB btdb(path);
	btdb.AddMapEntry(folder_name, map_name.c_str());
}

void CProjectManager::AppendWorldsPrm(const TCHAR *path, const TCHAR *folder_name) const
{
	set<string> worlds;
	// create a list of worlds
	{
		// open file
		ifstream prm_file(path);
		if (!prm_file.is_open())
		{
			Error(_T("Could not open WORLDS.PRM for reading."));
			return;
		}
		// read in strings from the worlds_list.txt resource
		{
			istringstream worlds_list_stream;
			// load the list of reserved names
			{
				HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_WORLDS_LIST), "Text"));
				HGLOBAL resource(LoadResource(NULL, resource_info));
				char *text(ri_cast<char*>(LockResource(resource)));
				worlds_list_stream.str(text);
			}
			string world;
			while (worlds_list_stream)
			{
				worlds_list_stream >> world;
				if (!world.empty())
					worlds.insert(world);
			}
		}
		// read in strings from WORLDS.PRM
		{
			DWORD num_worlds;
			prm_file >> num_worlds;
			string world;
			while (prm_file)
			{
				prm_file >> world >> world;
				if (!world.empty())
					worlds.insert(world);
			}
		}
		// append the new world
		{
			string new_world(folder_name);
			worlds.insert(new_world);
		}
	}
	// save the list of worlds
	{
		ofstream prm_file(path);
		if (!prm_file.is_open())
		{
			Error(_T("Could not open WORLDS.PRM for writing."));
			return;
		}
		      set<string>::const_iterator i  (worlds.begin());
		const set<string>::const_iterator end(worlds.end());
		prm_file << worlds.size() << endl << endl;
		for (; i != end; ++i)
			prm_file << setw(0) << *i << setw(24) << *i << endl;
	}
}

void CProjectManager::CleanUp()
{
	// clean up the timer
	if (file_timer != NULL)
		KillTimer(NULL, file_timer);
	file_timer = NULL;
}

void CProjectManager::CreateHeightmapThread()
{
	if (NULL != heightmap_thread)
	{
		WaitForSingleObject(heightmap_thread, INFINITE);
		CloseHandle(heightmap_thread);
	}
	DWORD thread_id;
	heightmap_thread = CreateThread(NULL, 0, HeightmapLoadThreadProc, this, 0, &thread_id);
}

void CProjectManager::CreateTextureThread()
{
	if (NULL != texture_thread)
	{
		WaitForSingleObject(texture_thread, INFINITE);
		CloseHandle(texture_thread);
	}
	DWORD thread_id;
	texture_thread = CreateThread(NULL, 0, TextureLoadThreadProc, this, 0, &thread_id);
}

void CProjectManager::CreateLightMap(const BYTE *heightmap, SIZE size)
{
	// ASSUME the length of each row of the heightmap is size.cx + 1
	// sign out the lightmap
	CStaticArray<BYTE> &lightmap_data(lightmap.SignOut());
	// allocate new memory if necessary
	{
		const size_t new_size(size.cx * size.cy);
		if (new_size != lightmap_data.length)
		{
			delete [] lightmap_data.ptr;
			lightmap_data.ptr = new BYTE[new_size];
			lightmap_data.length = new_size;
		}
	}
	// calculate lighting
	const BYTE *i(heightmap);    // heightmap iterator
	BYTE *li(lightmap_data.ptr); // lightmap iterator
	for (LONG r(0); r != size.cy; ++r)
	{
		const BYTE * row_i(i + size.cx);
		int          high_point_z(*row_i);
		const BYTE * high_point_x(row_i);
		li += size.cx;
		while (row_i != i)
		{
			--li;
			--row_i;
			const ptrdiff_t point_x(high_point_x - row_i);
			const int       point_z(*row_i);
			if (high_point_z - point_z < point_x)
			// if the point is unshadowed
			{
				const float dx(1.0f);
				const float dy(static_cast<float>(row_i[0] - row_i[1]));
				float length(sqrt(2 * (dx * dx + dy * dy)));
				float dot((dx + dy) / length);
				*li = (BYTE)(255 * dot);
				high_point_z = point_z;
				high_point_x = row_i;
			}
			else
				*li = 0x00;
		}
		li += size.cx;
		i += size.cx + 1;
	}
	// blur the lightmap (fake soft shadows :) )
	{
		const size_t lightmap_size(size.cx * size.cy);
		int *int_lightmap(new int[lightmap_size]);
		for (size_t i(0); i != lightmap_size; ++i)
			int_lightmap[i] = lightmap_data.ptr[i];
		StackBlur(int_lightmap, size.cx, size.cy, 4);
		for (size_t i(0); i != lightmap_size; ++i)
			lightmap_data.ptr[i] = static_cast<BYTE>(int_lightmap[i]);
		delete [] int_lightmap;
	}
	// sign in the lightmap
	lightmap.SignIn();
}

DWORD CProjectManager::CalculateChecksum()
{
	const DWORD QUOTIENT(0x04c11db7);
	// generate table
	const size_t table_size(256);
	DWORD crctab[table_size];
	{
		DWORD crc;
		for (size_t i(0); i != table_size; ++i)
		{
			crc = i << 24;
			for (size_t j(0); j != 8; ++j)
			{
				if (crc & 0x80000000)
					crc = (crc << 1) ^ QUOTIENT;
				else
					crc = crc << 1;
			}
			crctab[i] = crc;
		}
	}
	// calulate checksum
	DWORD result;
	{
		const BYTE *data;
		size_t length;
		// set data to the heightmap
		{
			const CStaticArray<BYTE> &heightmap_data(heightmap.SignOutConst());
			data   = heightmap_data.ptr;
			length = heightmap_data.length;
			length -= length % 4;
		}
		// add to the checksum
		_ASSERTE(length % 4 == 0 && length >= 4);
		result = *data++ << 24;
		result |= *data++ << 16;
		result |= *data++ << 8;
		result |= *data++;
		result = ~ result;
		length -=4;
		for (size_t i(0); i != length; ++i)
			result = (result << 8 | *data++) ^ crctab[result >> 24];
		// sign in the heightmap
		heightmap.SignIn();
		// set data to the texture
		{
			const CPalettedTexture &texture_data(texture.SignOutConst());
			data = texture_data.ptr;
			length = texture_data.length;
		}
		// add to the checksum
		_ASSERTE(length % 4 == 0 && length >= 4);
		for (size_t i(0); i != length; ++i)
			result = (result << 8 | *data++) ^ crctab[result >> 24];
		// sign in the texture
		texture.SignIn();
		// set data to the texture palette
		{
			const CPalettedTexture &texture_data(texture.SignOutConst());
			data = ri_cast<const BYTE*>(texture_data.palette);
			length = 256 * 4;
		}
		// add to the checksum
		_ASSERTE(length % 4 == 0 && length >= 4);
		for (size_t i(0); i != length; ++i)
			result = (result << 8 | *data++) ^ crctab[result >> 24];
		// sign in the texture
		texture.SignIn();
		// set data to map_info
		{
			BYTE *new_data;
			length = map_info.SignOutConst().GetBinaryBlock(new_data);
			map_info.SignIn();
			data = new_data;
		}
		// add to the checksum
		_ASSERTE(length % 4 == 0 && length >= 4);
		for (size_t i(0); i != length; ++i)
			result = (result << 8 | *data++) ^ crctab[result >> 24];
		// deallocate memory allocated by GetBinaryBlock
		delete [] (data - length);
	}
	return ~result;
}

void CProjectManager::DefaultHeightmap()
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// sign out the heightmap
	CStaticArray<BYTE> &heightmap_data(heightmap.SignOut());
	// allocate new memory if necessary
	const size_t new_size((map_size.cx + 1) * (map_size.cy + 1));
	if (new_size != heightmap_data.length)
	{
		delete [] heightmap_data.ptr;
		heightmap_data.ptr = new BYTE[new_size];
		heightmap_data.length = new_size;
	}
	// fill the map with zeros
	ZeroMemory(heightmap_data.ptr, heightmap_data.length);
	// sign in the heightmap
	heightmap.SignIn();
}

void CProjectManager::DefaultMapInfo()
{
	// sign out map info
	CMapInfo &map_data(map_info.SignOut());
	// fill with default values
	map_data.Default();
	// sign in map info
	map_info.SignIn();
}

void CProjectManager::DefaultTexture()
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// sign out the texture
	CPalettedTexture &texture_data(texture.SignOut());
	// allocate new memory for the texture if necessary
	const size_t new_size(map_size.cx * map_size.cy);
	if (new_size != texture_data.length)
	{
		delete [] texture_data.ptr;
		texture_data.length = new_size;
		texture_data.ptr = new BYTE[new_size];
	}
	// colour the texture white
	ZeroMemory(texture_data.ptr, texture_data.length);
	FillMemory(texture_data.palette, 256 * sizeof(COLORREF), 0xFF);
	// sign in the texture
	texture.SignIn();
}

// add extra precision to the input by interpolation
void CProjectManager::Extrapolate(int *pix, int w, int h) const
{
	int *pix_end(pix + w * h);
	int *row_end(pix + w);
	int *v1;
	int *v2(pix);
	// main loop
	while (v2 != pix_end)
	{
		// set iterators
		v1 = v2;
		++v2;
		// find the next contour
		do ++v2; while (*v2 == *(v2 - 1) && v2 != row_end);
		if (v2 == row_end)
		{
			row_end += w;
			continue;
		}
		if (*v2 < *(v2 - 1))
			--v2;
		// set colours
		int clr1(*v1);
		int clr2(*v2);
		// set pixels between contours
		float d(static_cast<float>(v2 - v1));
		for (int *i(v1 + 1); i != v2; ++i)
		{
			float t((i - v1) / d);
			*i = static_cast<int>(clr1 * (1 - t) + clr2 * t);
		}
	}
}

// generate unique identifiers for viewers
int CProjectManager::GenerateId() const
{
	static int id = 0;
	return ++id; // 0 reserved for the project manager
}

// get Perimeter installation path from the registry
bool CProjectManager::GetInstallPath(string &install_path) const
{
	// check the registry for necessary information
	HKEY perimeter_key;
	// open perimeter's registry key
	if (ERROR_SUCCESS != RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Codemasters\\Perimeter"),
		0,
		KEY_READ,
		&perimeter_key))
	{
		MessageBox(
			hWnd,
			_T("Please make sure you have Perimeter installed."),
			_T("Installation Error"),
			MB_OK);
		return false;
	}
	// verify Perimeter version
	{
		DWORD version;
		DWORD version_length;
		DWORD version_type;
		if (ERROR_SUCCESS != RegQueryValueEx(
			perimeter_key,
			_T("Version"),
			NULL,
			&version_type,
			NULL,
			&version_length))
		{
			RegCloseKey(perimeter_key);
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		if (version_length != 4 || version_type != REG_DWORD)
		{
			RegCloseKey(perimeter_key);
			Error(_T("Wrong Version type."));
			return false;
		}
		if (ERROR_SUCCESS != RegQueryValueEx(
			perimeter_key,
			_T("Version"),
			NULL,
			NULL,
			ri_cast<BYTE*>(&version),
			&version_length))
		{
			RegCloseKey(perimeter_key);
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		if (version != 101)
		{
			RegCloseKey(perimeter_key);
			MessageBox(
				hWnd,
				_T("Incompatible Perimeter version. Please make sure you have version 1.01."),
				_T("Installation Error"),
				MB_OK);
			return false;
		}
	}
	// get path to Perimeter's installation folder
	{
		DWORD install_path_type;
		DWORD install_path_length;
		if (ERROR_SUCCESS != RegQueryValueEx(
			perimeter_key,
			_T("Install_Path"),
			NULL,
			&install_path_type,
			NULL,
			&install_path_length))
		{
			RegCloseKey(perimeter_key);
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		if (REG_SZ != install_path_type)
		{
			RegCloseKey(perimeter_key);
			Error(_T("Wrong Install_Path type."));
			return false;
		}
		TCHAR install_path_temp[MAX_PATH];
		if (ERROR_SUCCESS != RegQueryValueEx(
			perimeter_key,
			_T("Install_Path"),
			NULL,
			NULL,
			ri_cast<BYTE*>(install_path_temp),
			&install_path_length))
		{
			RegCloseKey(perimeter_key);
			Error(_T("RegQueryValueEx failed"));
			return false;
		}
		install_path = install_path_temp;
	}
	RegCloseKey(perimeter_key);
	return true;
}

// loads a file into memory
// buffer should not be initialized, but releasing it is the caller's responcibility
DWORD CProjectManager::LoadFile(const TCHAR *name, BYTE *&pBuffer) const
{
	DWORD dwDesiredAccess       = FILE_READ_DATA;
	DWORD dwShareMode           = FILE_SHARE_READ;
	DWORD dwCreationDisposition = OPEN_EXISTING;
	DWORD dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
	// open the file
	HANDLE hFile = CreateFile(
		name,
		dwDesiredAccess,
		dwShareMode,
		NULL,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;
	// querry file size
	DWORD dwFileSize = GetFileSize(hFile, NULL);
	if (INVALID_FILE_SIZE == dwFileSize)
	{
		CloseHandle(hFile);
		Error("GetFileSize failed");
		return 0;
	}
	// load file into memory
	pBuffer = new BYTE[dwFileSize];
	DWORD NumberOfBytesRead;
	if (0 == ReadFile(
		hFile,
		pBuffer,
		dwFileSize,
		&NumberOfBytesRead,
		NULL) || NumberOfBytesRead != dwFileSize)
	{
		CloseHandle(hFile);
		Error("ReadFile failed");
		return 0;
	}
	CloseHandle(hFile);
	return dwFileSize;
}

// load the heightmap into a byte array
void CProjectManager::LoadHeightmap()
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// load the heightmap image
	fipImage image;
	string heightmap_path(map_info.SignOutConst().folder_path + _T("heightmap.bmp"));
	map_info.SignIn();
	image.load(heightmap_path.c_str());
	while (FALSE == image.isValid())
	{
		Sleep(128);
		image.load(heightmap_path.c_str());
	}
	// make sure that dimensions are correct
	if (image.getWidth() != map_size.cx || image.getHeight() != map_size.cy)
	{
		Error(_T("image dimensions do not correspond with project settigns"));
		return;
	}
	// turn the image to grayscale if necessary
	if (8 != image.getBitsPerPixel())
	{
		if (FALSE == image.convertToGrayscale())
		{
			Error(_T("fipImage::convertToGrayscale failed"));
			return;
		}
	}
	// flip the image vertically
	if (FALSE == image.flipVertical())
		Error(_T("fipImage::flipVertical failed"));
	// sign out the heightmap
	CStaticArray<BYTE> &heightmap_data(heightmap.SignOut());
	// allocate new memory if necessary
	const size_t new_size((map_size.cx + 1) * (map_size.cy + 1));
	if (new_size != heightmap_data.length)
	{
		delete [] heightmap_data.ptr;
		heightmap_data.ptr = new BYTE[new_size];
		heightmap_data.length = new_size;
	}
	// read in data
	const BYTE *image_data(image.accessPixels());
	BYTE *heightmap_iter(heightmap_data.ptr);
	for (LONG r(0); r != map_size.cy; ++r)
	{
		CopyMemory(heightmap_iter, image_data, map_size.cx);
		heightmap_iter += map_size.cx;
		image_data += map_size.cx;
		// pad horizontally
		*heightmap_iter = *(image_data - 1);
		++heightmap_iter;
	}
	// pad vertically
	CopyMemory(heightmap_iter, heightmap_iter - (map_size.cx + 1), map_size.cx + 1);
	// calculate the lightmap
	if (settings.enable_lighting)
		CreateLightMap(heightmap_data.ptr, map_size);
	// wrap up
	heightmap.SignIn();
	heightmap.Update(0);
}

// load the texture into a color array
void CProjectManager::LoadTexture()
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// load the texture image
	fipImage image;
	string texture_path(map_info.SignOutConst().folder_path + "texture.bmp");
	map_info.SignIn();
	image.load(texture_path.c_str());
	while (FALSE == image.isValid())
	{
		Sleep(128);
		image.load(texture_path.c_str());
	}
	// make sure that dimensions are correct
	if (image.getWidth() != map_size.cx || image.getHeight() != map_size.cy)
	{
		Error(_T("image dimensions do not correspond with project settigns"));
		return;
	}
	// quantize the image if necessary
	if (FIC_PALETTE != image.getColorType())
	{
		settings.SignOut();
		FREE_IMAGE_QUANTIZE mode(settings.texture_colour_quality ? FIQ_NNQUANT : FIQ_WUQUANT);
		settings.SignIn();
		if (FALSE == image.colorQuantize(mode))
		{
			Error(_T("fipImage::colorQuantize failed"));
			return;
		}
	}
	// flip the image vertically
	if (FALSE == image.flipVertical())
		Error(_T("fipImage::flipVertical failed"));
	// sign out the texture
	CPalettedTexture &texture_data(texture.SignOut());
	// allocate new memory for the texture
	const size_t new_size(map_size.cx * map_size.cy);
	if (new_size != texture_data.length)
	{
		delete [] texture_data.ptr;
		texture_data.length = new_size;
		texture_data.ptr = new BYTE[new_size];
	}
	// extract the texture
	CopyMemory(texture_data.ptr, image.accessPixels(), texture_data.length);
	// extract the palette
	int palette_size(min(256, image.getPaletteSize() / 4));
	RGBQUAD *palette(image.getPalette());
	for (int i(0); i != palette_size; ++i)
		texture_data.palette[i] = RGB(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
	// wrap up
	texture.SignIn();
	texture.Update(0);
}

int CProjectManager::PackHeightmap(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask)
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	mask.reserve(map_size.cx * map_size.cy);
	// pack heightmap itself
	int encoded_size;
	{
		// create the image data matrix
		jas_matrix_t *data(jas_matrix_create(map_size.cy, map_size.cx)); // rows then columns
		{
			const CStaticArray<BYTE> heightmap_data(heightmap.SignOutConst());
			size_t index(0);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				for (LONG c(0); c != map_size.cx; ++c)
				{
					jas_matrix_set(data, r, c, heightmap_data.ptr[index]);
					mask.push_back(0 == heightmap_data.ptr[index]);
					++index;
				}
				++index; // skip padding
			}
			heightmap.SignIn();
		}
		// initialize the image component structure
		jas_image_cmptparm_t component;
		component.width  = map_size.cx;
		component.height = map_size.cy;
		component.tlx    = 0;
		component.tly    = 0;
		component.hstep  = 1;
		component.vstep  = 1;
		component.prec   = 8;
		component.sgnd   = false;
		// create the image
		jas_image_t *image(jas_image_create(1, &component, JAS_CLRSPC_SGRAY));
		if (0 == image)
		{
			Error(_T("jas_image_create failed"));
		}
		// fill the component of the image
		jas_image_setcmpttype(image, 0, 0);
		if (0 != jas_image_writecmpt(image, 0, 0, 0, map_size.cx, map_size.cy, data))
			Error(_T("jas_imagewritecmpt failed"));
		// create an output stream
		size_t buffer_size(map_size.cx * map_size.cy);
		jas_stream_t *stream = jas_stream_memopen(ri_cast<char*>(buffer), buffer_size);
		if (0 == stream)
			Error(_T("jas_stream_open failure"));
		// encode and write the image into the stream
		int format = jas_image_strtofmt("jpc");
		if (-1 == format)
			Error(_T("jas_image_strtofmt failed"));
		if (0 != jas_image_encode(image, stream, format, "mode=real rate=0.1"))
			Error(_T("jas_image_encode failed"));
		encoded_size = stream->rwcnt_;
		// clean up
		jas_stream_close(stream);
		jas_image_destroy(image);
		jas_matrix_destroy(data);
	}
	// pack the mask
	_ASSERTE(mask.size() % 8 == 0);
	const int mask_size(mask.size() / 8);
	{
		BYTE *mask_buffer(new BYTE[mask_size]);
		ZeroMemory(mask_buffer, mask_size);
		int bit_index(0);
		for (int i(0); i != mask_size; ++i)
			for (int b(0); b != 8; ++b)
				if (mask[bit_index++])
					mask_buffer[i] |= 1 << b;
		CopyMemory(buffer + encoded_size, mask_buffer, mask_size);
		delete [] mask_buffer;
	}
	// record xml metadata
	{
		char str[16];
		// compression format
		node.InsertEndChild(TiXmlElement("compression"))->InsertEndChild(TiXmlText("JPC"));
		// offset of compressed heightmap data
		_itot(buffer - initial_offset, str, 10);
		node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
		// size of compressed heightmap data
		_itot(encoded_size, str, 10);
		node.InsertEndChild(TiXmlElement("size"))->InsertEndChild(TiXmlText(str));
		// offset of the null point mask
		_itot(buffer - initial_offset + encoded_size, str, 10);
		node.InsertEndChild(TiXmlElement("mask_offset"))->InsertEndChild(TiXmlText(str));
	}
	return encoded_size + mask_size;
}

void CProjectManager::PackMapInfo(TiXmlNode &node)
{
	const CMapInfo &map_data(map_info.SignOutConst());
	map_data.SaveToXml(node);
	map_info.SignIn();
}

int CProjectManager::PackTexture(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask)
{
	// pack the texture and the palette
	const CPalettedTexture texture_data(texture.SignOut());
	const int texture_size(texture_data.length);
	const int palette_size(256 * sizeof(COLORREF));
	for (int i(0); i != texture_size; ++i)
		if (mask[i])
			texture_data.ptr[i] = 0;
	CopyMemory(buffer, texture_data.ptr, texture_size);
	CopyMemory(buffer + texture_size, texture_data.palette, palette_size);
	texture.SignIn();
	// write XML metadata
	{
		char str[16];
		// offset of compressed texture data
		_itot(buffer - initial_offset, str, 10);
		node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
		// offset of the null point mask
		_itot(buffer - initial_offset + texture_size, str, 10);
		node.InsertEndChild(TiXmlElement("palette_offset"))->InsertEndChild(TiXmlText(str));
	}
	return texture_size + palette_size;
}

bool CProjectManager::RegisterMap()
{
	// check connection status
	if (FALSE == InternetCheckConnection(
		_T("http://www.rul-clan.ru/map_registration/map_register.php"),
		FLAG_ICC_FORCE_CONNECTION,
		0))
	{
		MessageBox(
			hWnd,
			_T("Connection to the server could not be established."),
			_T("Registration Error"),
			MB_OK);
		return false;
	}
	// register map
	const size_t buffer_size(1048576);
	TCHAR *buffer(new TCHAR[buffer_size]);
	{
		// establish connection
		HINTERNET wi_handle(InternetOpen(
			_T("Perimeter Map Compiler"),
			INTERNET_OPEN_TYPE_PRECONFIG,
			NULL,
			NULL,
			0));
		if (NULL == wi_handle)
		{
			Error(_T("InternetOpen failed"));
			delete [] buffer;
			return false;
		}
		// open URL
		HINTERNET url_handle;
		{
			// sign out map info
			CMapInfo map_data(map_info.SignOutConst());
			// create the RL
			TCHAR url[MAX_PATH];
			{
				// calculate map checksum
				TCHAR checksum[16];
				ZeroMemory(checksum, 16);
				DWORD dw(CalculateChecksum());
				_ltot(dw, checksum, 16);
				// create an unsafe url
				string unsafe_url(_T("http://www.rul-clan.ru/map_registration/map_register.php?map_name="));
				unsafe_url += map_data.map_name;
				unsafe_url += _T("&map_checksum=");
				unsafe_url += checksum;
				// convert the unsafe url to canonical form
				DWORD num_chars(MAX_PATH);
				if (S_OK != UrlEscape(
					unsafe_url.c_str(),
					url,
					&num_chars,
					URL_ESCAPE_UNSAFE))
				{
					Error(_T("UrlEscape failed"));
					InternetCloseHandle(wi_handle);
					delete [] buffer;
					return false;
				}
				url[num_chars] = _T('\0');
			}
			// open the URL
			url_handle = InternetOpenUrl(wi_handle, url, NULL, 0L, 0L, 0L);
			if (NULL == url_handle)
			{
				Error(_T("InternetOpenUrl failed"));
				InternetCloseHandle(wi_handle);
				delete [] buffer;
				return false;
			}
			// sign in map info
			map_info.SignIn();
		}
		// get data
		DWORD bytes_read;
		if (FALSE == InternetReadFile(
			url_handle,
			buffer,
			buffer_size,
			&bytes_read))
		{
			Error(_T("InternetReadFile failed"));
			InternetCloseHandle(url_handle);
			InternetCloseHandle(wi_handle);
			delete [] buffer;
			return false;
		}
		buffer[bytes_read] = _T('\0');
		// wrap up
		InternetCloseHandle(url_handle);
		InternetCloseHandle(wi_handle);
	}
	// process result
	if (0 != _tcscmp(buffer, _T("success")))
	{
		const TCHAR * const error_name(_T("Registration Error"));
		if (0 == _tcscmp(buffer, _T("taken")))
			MessageBox(hWnd, _T("The name of this map is already in use."), error_name, MB_OK);
		else if (0 == _tcscmp(buffer, _T("invalid")))
			MessageBox(hWnd, _T("Invalid map name."), error_name, MB_OK);
		else if (0 == _tcscmp(buffer, _T("reg_limit_exceeded")))
			MessageBox(hWnd, _T("Registration limit for today exceeded."), error_name, MB_OK);
		else
			MessageBox(hWnd, _T("Map could not be registered."), error_name, MB_OK);
		delete [] buffer;
		return false;
	}
	delete [] buffer;
	return true;
}

void CProjectManager::SaveHeightmap(const TCHAR *path)
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// initialize the heightmap image
	fipImage image(FIT_BITMAP, static_cast<WORD>(map_size.cx), static_cast<WORD>(map_size.cy), 8);
	// sign out the heightmap
	const CStaticArray<BYTE> &heightmap_data(heightmap.SignOutConst());
	// fill the image
	{
		_ASSERTE(heightmap_data.length == static_cast<size_t>((map_size.cx + 1) * (map_size.cy + 1)));
		BYTE *image_iter(image.accessPixels());
		BYTE *heightmap_iter(heightmap_data.ptr);
		for (LONG r(0); r != map_size.cy; ++r)
		{
			CopyMemory(image_iter, heightmap_iter, map_size.cx);
			image_iter     += map_size.cx;
			heightmap_iter += map_size.cx + 1;
		}
	}
	// sign in the heightmap
	heightmap.SignIn();
	// save the image
	if (FALSE == image.save(path, BMP_DEFAULT))
		Error(_T("fipImage::save failed"));
}

void CProjectManager::SaveMapInfo(const TCHAR *path)
{
	// sign out map info
	const CMapInfo &map_data(map_info.SignOut());
	// save map info
	map_data.Save(path);
	// sign in map info
	map_info.SignIn();
}

bool CProjectManager::SaveMemToFile(const TCHAR *path, const BYTE *buffer, DWORD size) const
{
	// create the file
	DWORD dwDesiredAccess       = GENERIC_WRITE;
	DWORD dwShareMode           = 0;
	DWORD dwCreationDisposition = CREATE_ALWAYS;
	DWORD dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
	// open the file
	HANDLE hFile = CreateFile(
		path,
		dwDesiredAccess,
		dwShareMode,
		NULL,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Error(_T("CreateFile failed"));
		return false;
	}
	DWORD num_bytes_written;
	if (FALSE == WriteFile(
		hFile,
		buffer,
		size,
		&num_bytes_written,
		NULL))
	{
		CloseHandle(hFile);
		Error(_T("WriteFile failed"));
		return false;
	}
	CloseHandle(hFile);
	return true;
}

void CProjectManager::SaveMission(const TCHAR *path, const TCHAR *folder_name, bool survival)
{
	TCHAR str[MAX_PATH];
	// create an empty ".dat" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".dat"));
	SaveMemToFile(str, NULL, 0);
	// create an empty ".gmp" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".gmp"));
	SaveMemToFile(str, NULL, 0);
	// create the ".spg" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".spg"));
	SaveSPG(str, folder_name, survival);
	// create the ".sph" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".sph"));
	SaveSPH(str, folder_name, survival);
}

// save texture palette
void CProjectManager::SavePalette(const TCHAR *path)
{
	// allocate memory
	const size_t palette_size(768);
	BYTE *image_palette(new BYTE[palette_size]);
	BYTE *palette_iter(image_palette);
	// fill the palette
	const CPalettedTexture &texture_data(texture.SignOutConst());
	const COLORREF *texture_palette_iter(texture_data.palette);;
	for (int i(0); i != 256; ++i)
	{
		*palette_iter++ = GetRValue(*texture_palette_iter);
		*palette_iter++ = GetGValue(*texture_palette_iter);
		*palette_iter++ = GetBValue(*texture_palette_iter);
		++texture_palette_iter;
	}
	texture.SignIn();
	// save the palette to a file
	SaveMemToFile(path, image_palette, palette_size);
	delete [] image_palette;
}

void CProjectManager::SaveShrub(const TCHAR *path, const BYTE *buffer, DWORD size) const
{
	SaveMemToFile(path, buffer, size);
}

void CProjectManager::SaveSPG(const TCHAR *path, const TCHAR *folder_name, const bool survival)
{
	const size_t spg_alloc(8388608); // 8 MB should be enough
	size_t spg_size(spg_alloc);
	char *spg(new char[spg_size]);
	char *spg_iter1(spg), *spg_iter2;
	// make sure the allocation was successful
	if (NULL == spg)
	{
		Error("Not enough memory.");
		return;
	}
	// load the spg template
	{
		BYTE *compressed_buffer;
		// load the compressed template
		HRSRC resource_info(FindResource(
			NULL,
			MAKEINTRESOURCE(survival ? IDR_SURVIVAL_SPG : IDR_SPG),
			"BZ2"));
		HGLOBAL resource(LoadResource(NULL, resource_info));
		compressed_buffer = ri_cast<BYTE*>(LockResource(resource));
		// uncompress
		if (BZ_OK != BZ2_bzBuffToBuffDecompress(
			spg,
			&spg_size,
			ri_cast<char*>(compressed_buffer),
			SizeofResource(NULL, resource_info),
			0,
			0))
		{
			Error(_T("BZ2_bzBuffToBuffDecompress failed"));
			delete [] spg;
			return;
		}
	}
	// create an output stream
	ofstream spg_out(path, ios_base::binary | ios_base::out);
	// set map name
	{
		// get map name
		char target[] = "%folder_name%";
		spg_iter2 = strstr(spg, target);
		if (NULL == spg_iter2)
		{
			Error(_T("Perimeter Map Compiler seems to have been corrupted. Please reinstall."));
			delete [] spg;
			return;
		}
		spg_out.write(spg_iter1, spg_iter2 - spg_iter1);
		spg_out << folder_name;
		spg_iter2 += sizeof(target) - 1;
	}
	// sign out map info, as it will be needed for the following replacements
	const CMapInfo &map_data(map_info.SignOutConst());
	// set Frame positions
	const unsigned int pos_range_start(survival ? 0 : 1);
	const unsigned int pos_range_end  (survival ? 1 : 5);
	for (size_t i(pos_range_start); i != pos_range_end; ++i)
	{
		char target[] = "%frame_position_X%";
		itoa(i, target + sizeof(target) - 3, 10);
		spg_iter1 = spg_iter2;
		spg_iter2 = strstr(spg_iter2, target);
		if (NULL == spg_iter2)
		{
			Error(_T("Program seems to have been corrupted. Please reinstall."));
			map_info.SignIn();
			delete [] spg;
			return;
		}
		spg_out.write(spg_iter1, spg_iter2 - spg_iter1);
		if (survival)
			spg_out << map_data.start_pos[0].x << " " << map_data.start_pos[0].y << " " << 256;
		else
			spg_out << map_data.start_pos[i].x << " " << map_data.start_pos[i].y << " " << 256;
		spg_iter2 += sizeof(target) - 1;
	}
	// set camera positions
	for (size_t i(pos_range_start); i != pos_range_end; ++i)
	{
		char target[] = "%camera_position_X%";
		itoa(i, target + sizeof(target) - 3, 10);
		// replace both occurences
		spg_iter1 = spg_iter2;
		spg_iter2 = strstr(spg_iter2, target);
		if (NULL == spg_iter2)
		{
			Error(_T("Program seems to have been corrupted. Please reinstall."));
			map_info.SignIn();
			delete [] spg;
			return;
		}
		spg_out.write(spg_iter1, spg_iter2 - spg_iter1);
		if (survival)
			spg_out << map_data.start_pos[0].x << " " << map_data.start_pos[0].y;
		else
			spg_out << map_data.start_pos[i].x << " " << map_data.start_pos[i].y;
		spg_iter2 += sizeof(target) - 1;
	}
	// sign in map info
	map_info.SignIn();
	// output the rest of the data
	spg_out.write(spg_iter2, spg_size - (spg_iter2 - spg) - 1); // minus one for the terminating zero
	// clean up
	delete [] spg;
}

void CProjectManager::SaveSPH(const TCHAR *path, const TCHAR *folder_name, const bool survival)
{
	const size_t sph_alloc(4096); // 4 KB should be enough
	size_t sph_size(sph_alloc);
	char *sph(new char[sph_size]);
	char *sph_iter1(sph), *sph_iter2(sph);
	// make sure the allocation was successful
	if (NULL == sph)
	{
		Error("Not enough memory.");
		return;
	}
	// load the sph template
	{
		BYTE *compressed_buffer;
		// load the compressed template
		HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_SPH), "BZ2"));
		HGLOBAL resource(LoadResource(NULL, resource_info));
		compressed_buffer = ri_cast<BYTE*>(LockResource(resource));
		// uncompress
		if (BZ_OK != BZ2_bzBuffToBuffDecompress(
			sph,
			&sph_size,
			ri_cast<char*>(compressed_buffer),
			SizeofResource(NULL, resource_info),
			0,
			0))
		{
			Error(_T("BZ2_bzBuffToBuffDecompress failed"));
			delete [] sph;
			return;
		}
	}
	// create an output stream
	ofstream sph_out(path, ios_base::binary | ios_base::out);
	// set map name
	{
		char target[] = "%folder_name%";
		sph_iter1 = sph_iter2;
		sph_iter2 = strstr(sph_iter2, target);
		if (NULL == sph_iter2)
		{
			Error(_T("Program seems to have been corrupted. Please reinstall."));
			delete [] sph;
			return;
		}
		sph_out.write(sph_iter1, sph_iter2 - sph_iter1);
		sph_out << folder_name;
		sph_iter2 += sizeof(target) - 1;
	}
	// set the number of players
	{
		char target[] = "%player_count%";
		sph_iter1 = sph_iter2;
		sph_iter2 = strstr(sph_iter2, target);
		if (NULL == sph_iter2)
		{
			Error(_T("Program seems to have been corrupted. Please reinstall."));
			delete [] sph;
			return;
		}
		sph_out.write(sph_iter1, sph_iter2 - sph_iter1);
		char *player_count(survival ? "1" : "4");
		sph_out << player_count;
		sph_iter2 += sizeof(target) - 1;
	}
	// set map path
	{
		char target[] = "%folder_name%";
		sph_iter1 = sph_iter2;
		sph_iter2 = strstr(sph_iter2, target);
		if (NULL == sph_iter2)
		{
			Error(_T("Program seems to have been corrupted. Please reinstall."));
			delete [] sph;
			return;
		}
		sph_out.write(sph_iter1, sph_iter2 - sph_iter1);
		string map_name;
		if (survival)
			map_name = "survival\\\\";
		map_name += folder_name;
		sph_out << map_name;
		sph_iter2 += sizeof(target) - 1;
	}
	// output the rest of the data
	sph_out.write(sph_iter2, sph_size - (sph_iter2 - sph) - 1); // minus one for the terminating zero
	// clean up
	delete [] sph;
}

void CProjectManager::SaveTexture(const TCHAR *path)
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// initialize the texture image
	fipImage image(FIT_BITMAP, static_cast<WORD>(map_size.cx), static_cast<WORD>(map_size.cy), 8);
	// sign out the texture
	const CPalettedTexture &texture_data(texture.SignOutConst());
	// fill the image
	CopyMemory(image.accessPixels(), texture_data.ptr, texture_data.length);
	CopyMemory(image.getPalette(), texture_data.palette, 256 * 4);
	// convert the image to 24 bits (for easier editing)
	if (FALSE == image.convertTo24Bits())
		Error(_T("fipImage::convertTo24Bits failed"));
	// sign in the texture
	texture.SignIn();
	// save the image
	if (FALSE == image.save(path, BMP_SAVE_RLE))
		Error(_T("fipImage::save failed"));
}

// create a thumbnail version of the map texture and save it
bool CProjectManager::SaveThumb(const TCHAR *path, SIZE size)
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// initialize the map preview image
	fipImage image(FIT_BITMAP, static_cast<WORD>(map_size.cx), static_cast<WORD>(map_size.cy), 24);
	// if lighting data has not been initialized alread, do it now
	if (!lightmap.IsValid())
	{
		CreateLightMap(heightmap.SignOutConst().ptr, map_size);
		heightmap.SignIn();
	}
	// fill the image with data from the texture and from the lightmap
	{
		const CPalettedTexture &texture_data(texture.SignOutConst());
		const BYTE *lightmap_i(lightmap.SignOutConst().ptr);
		const BYTE *texture_i(texture_data.ptr);
		const BYTE * const texture_end(texture_i + texture_data.length);
		BYTE *image_i(image.accessPixels());
		while (texture_i != texture_end)
		{
			float f_light(static_cast<float>(*lightmap_i));
			f_light = (f_light + 128.0f) / 255.0f;
			COLORREF colour(texture_data.palette[*texture_i]);
			image_i[0] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetBValue(colour) * f_light)));
			image_i[1] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetGValue(colour) * f_light)));
			image_i[2] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetRValue(colour) * f_light)));
			image_i += 3;
			++texture_i;
			++lightmap_i;
		}
		lightmap.SignIn();
		texture.SignIn();
	}
	// paint all pixels underneath which the heightmap is zero black
	{
		const CStaticArray<BYTE> &heightmap_data(heightmap.SignOutConst());
		const BYTE *heightmap_ptr(heightmap_data.ptr);
		BYTE *image_data(image.accessPixels());
		for (int c(0); c != map_size.cx; ++c)
		{
			for (LONG r(0); r != map_size.cy; ++r)
			{
				if (0 == *heightmap_ptr++)
					image_data[0] = image_data[1] = image_data[2] = 0;
				image_data +=3;
			}
			++heightmap_ptr;
		}
		heightmap.SignIn();
	}
	// resize the image
	if (FALSE == image.rescale(
		static_cast<WORD>(size.cx),
		static_cast<WORD>(size.cy),
		FILTER_BILINEAR))
	{
		Error(_T("fipImage::rescale failed"));
		return false;
	}
	// flip the image vertically
	if (FALSE == image.flipVertical())
		Error(_T("fipImage::flipVertical failed"));
	// save the image
	if (FALSE == image.save(path))
	{
		Error(_T("fipImage::save failed"));
		return false;
	}
	return true;
}

// create the VMP file and save it
void CProjectManager::SaveVMP(const TCHAR *path)
{
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// allocate memory
	const size_t map_data_size(map_size.cx * map_size.cy);
	const size_t vmp_size(map_data_size * 4 + 20);
	BYTE *vmp(new BYTE[vmp_size]);
	BYTE *vmp_iter(vmp);
	// write the magic number
	{
		BYTE magic_number[4] = { 0x53, 0x32, 0x54, 0x30 };
		CopyMemory(vmp_iter, magic_number, 4);
		vmp_iter += 4;
	}
	// write map size
	{
		_ASSERTE(sizeof(SIZE) == 8);
		CopyMemory(vmp_iter, &map_size, 8);
		vmp_iter += 8;
	}
	// write static data
	{
		BYTE unused[8] = { 0x08, 0x82, 0x40, 0x00, 0x01, 0x00, 0x00, 0x00 };
		CopyMemory(vmp_iter, unused, 8);
		vmp_iter += 8;
	}
	// fill the first layer with zeros
	{
		ZeroMemory(vmp_iter, map_data_size);
		vmp_iter += map_data_size;
	}
	// extract necessary heightmap data
	int *int_heightmap(NULL);
	bool *null_pixels(NULL);
	{
		// sign out the heightmap
		const CStaticArray<BYTE> &heightmap_data(heightmap.SignOutConst());
		_ASSERTE(heightmap_data.length == static_cast<size_t>((map_size.cx + 1) * (map_size.cy + 1)));
		// allocate memory
		int_heightmap = new int [heightmap_data.length];
		null_pixels   = new bool[heightmap_data.length];
		// set iterators
		      int  *       int_heightmap_iter(int_heightmap);
		      bool *       null_pixels_iter(null_pixels);
		const BYTE *       heightmap_iter(heightmap_data.ptr);
		const BYTE * const heightmap_end(heightmap_iter + heightmap_data.length);
		// main loop
		while (heightmap_iter != heightmap_end)
		{
			*null_pixels_iter   = *heightmap_iter == 0;
			*int_heightmap_iter = *heightmap_iter << 5;
			++null_pixels_iter;
			++heightmap_iter;
			++int_heightmap_iter;
		}
		// sign in the heightmap
		heightmap.SignIn();
	}
	// interpolate heightmap
	StackBlur(int_heightmap, map_size.cx + 1, map_size.cy + 1, 4);
	// fill the second layer with the heightmap
	{
		const int  *int_heightmap_iter(int_heightmap);
		const bool *null_pixels_iter(null_pixels);
		for (LONG r(0); r != map_size.cy; ++r)
		{
			for (LONG c(0); c != map_size.cx; ++c)
			{
				*vmp_iter = *null_pixels_iter ? 0 : static_cast<BYTE>(*int_heightmap_iter >> 5);
				++null_pixels_iter;
				++vmp_iter;
				++int_heightmap_iter;
			}
			++int_heightmap_iter;
			++null_pixels_iter;
		}
	}
	delete [] null_pixels;
	// fill the third layer with the least significant bits of the heightmap extrapolation
	{
		int *int_heightmap_iter(int_heightmap);
		for (LONG r(0); r != map_size.cy; ++r)
		{
			for (LONG c(0); c != map_size.cx; ++c)
				*vmp_iter++ = (BYTE)(*int_heightmap_iter++ & 0x1F);
			++int_heightmap_iter;
		}
	}
	delete [] int_heightmap;
	// record the texture
	{
		const CPalettedTexture &texture_data(texture.SignOutConst());
		_ASSERTE(texture_data.length == map_data_size);
		CopyMemory(vmp_iter, texture_data.ptr, map_data_size);
		texture.SignIn();
		vmp_iter += map_data_size;
	}
	// create the file
	SaveMemToFile(path, vmp, vmp_size);
	// clean up
	delete [] vmp;
}

// Stack Blur Algorithm by Mario Klingemann <mario@quasimondo.com>
void CProjectManager::StackBlur(int *pix, int w, int h, int radius) const
{
	int wm(w - 1);
	int hm(h - 1);
	int wh(w * h);
	int div(radius + radius + 1);

	int *c(new int[wh]);
	int sum, x, y, i, p, yp, yi, yw;
	int *vmin(new int[__max(w,h)]);

	int divsum((div + 1) >> 1);
	divsum *= divsum;
	int *dv(new int[8192 * divsum]);
	for (i = 0; i < 8192 * divsum; ++i)
		dv[i] = (i / divsum);

	yw = yi = 0;

	int *stack(new int[div]);
	int stackpointer;
	int stackstart;
	int *sir;
	int rbs;
	int r1(radius + 1);
	int outsum;
	int insum;

	for (y = 0; y < h; ++y)
	{
		insum = outsum = sum = 0;
		// fill the stack and calculuate sums
		for(i = -radius; i <= radius; ++i)
		{
			p = pix[yi + min(wm, __max(i, 0))];
			stack[i + radius] = p;
			rbs = r1 - abs(i);
			sum += p * rbs;
			if (i > 0)
				insum += p;
			else
				outsum += p;
		}
		stackpointer = radius;

		for (x = 0; x < w; ++x)
		{
			c[yi] = dv[sum];
			sum -= outsum;
			stackstart = stackpointer - radius + div;
			sir = stack + stackstart % div;
			outsum -= *sir;
			if(y == 0)
				vmin[x] = min(x + radius + 1, wm);
			*sir = pix[yw + vmin[x]];
			insum += *sir;
			sum += insum;
			stackpointer = (stackpointer + 1) % div;
			sir = stack + stackpointer % div;
			outsum += *sir;
			insum -= *sir;
			++yi;
		}
		yw += w;
	}
	for (x = 0; x < w; x++)
	{
		insum = outsum = sum = 0;
		yp =- radius * w;
		for(i = -radius; i <= radius; ++i)
		{
			yi = __max(0,yp) + x;
			sir = stack + i + radius;
			*sir = c[yi];
			rbs=r1-abs(i);
			sum += c[yi] * rbs;
			if (i > 0)
				insum += *sir;
			else
				outsum += *sir;
			if (i < hm)
				yp += w;
		}
		yi = x;
		stackpointer = radius;
		for (y = 0; y < h; ++y)
		{
			pix[yi] = dv[sum];
			sum -= outsum;
			stackstart = stackpointer - radius + div;
			sir = stack + stackstart % div;
			outsum -= *sir;
			if(x==0)
				vmin[y] = min(y + r1,hm) * w;
			p = x + vmin[y];
			*sir = c[p];
			insum += *sir;
			sum += insum;
			stackpointer=(stackpointer+1)%div;
			sir = stack + stackpointer;
			outsum += *sir;
			insum -= *sir;
			yi += w;
		}
	}
	delete [] stack;
	delete [] dv;
	delete [] vmin;
	delete [] c;
}

// generates an assertion in debug mode and a warning in release mode
void CProjectManager::Error(string message) const
{
	string new_message = _T("CProjectManager: \n");
	new_message.append(message);
	_RPT0(_CRT_ERROR, new_message.c_str());
	MessageBox(hWnd, new_message.c_str(), NULL, MB_OK | MB_ICONERROR);
}

void CProjectManager::UnpackHeightmap(TiXmlNode *node, BYTE *buffer)
{
	// read in XML metadata
	size_t compressed_heightmap_size;
	BYTE *compressed_heightmap;
	BYTE *mask_buffer;
	{
		// find data
		TiXmlHandle node_handle(node);
		TiXmlText *compression_node(node_handle.FirstChildElement("compression").FirstChild().Text());
		TiXmlText *offset_node     (node_handle.FirstChildElement("offset"     ).FirstChild().Text());
		TiXmlText *size_node       (node_handle.FirstChildElement("size"       ).FirstChild().Text());
		TiXmlText *mask_offset_node(node_handle.FirstChildElement("mask_offset").FirstChild().Text());
		if (
			NULL == compression_node ||
			NULL == offset_node      ||
			NULL == size_node        ||
			NULL == mask_offset_node)
		{
			_RPT0(_CRT_WARN, "loading default heightmap\n");
			DefaultHeightmap();
			heightmap.Update(0);
			return;
		}
		// make sure the compression format matches
		if (0 != strcmp(compression_node->Value(), "JPC"))
		{
			_RPT0(_CRT_WARN, "loading default heightmap\n");
			DefaultHeightmap();
			heightmap.Update(0);
			return;
		}
		// finally parse the data
		compressed_heightmap      = buffer + atoi(offset_node->Value());
		compressed_heightmap_size = atoi(size_node->Value()); // possible buffer overflow
		mask_buffer               = buffer + atoi(mask_offset_node->Value());
	}
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	// unpack the mask
	vector<bool> mask;
	{
		int mask_size(map_size.cx * map_size.cy);
		_ASSERTE(mask_size % 8 == 0);
		mask.reserve(mask_size);
		mask_size /= 8;
		for (int i(0); i != mask_size; ++i)
			for (int b(0); b != 8; ++b)
				mask.push_back(0 != (mask_buffer[i] & 1 << b));
	}
	// unpack heightmap itself
	{
		// create an input stream
		jas_stream_t *stream(jas_stream_memopen(
			ri_cast<char*>(compressed_heightmap),
			compressed_heightmap_size));
		if (0 == stream)
		{
			Error(_T("jas_stream)open failure"));
			DefaultHeightmap();
			heightmap.Update(0);
			return;
		}
		// decode the image
		int format = jas_image_strtofmt("jpc");
		if (-1 == format)
		{
			Error(_T("jas_image_strtofmt failed"));
			DefaultHeightmap();
			heightmap.Update(0);
			return;
		}
		jas_image_t *image(jas_image_decode(stream, format, ""));
		if (0 == image)
		{
			Error(_T("jas_image_decode failed"));
			DefaultHeightmap();
			heightmap.Update(0);
			return;
		}
		// extract image data
		CStaticArray<BYTE> &heightmap_data(heightmap.SignOut());
		{
			// allocate memory, if necessary
			const size_t heightmap_size((map_size.cx + 1) * (map_size.cy + 1));
			if (heightmap_data.length != heightmap_size)
			{
				delete [] heightmap_data.ptr;
				heightmap_data.ptr = new BYTE[heightmap_size];
				heightmap_data.length = heightmap_size;
			}
			// get the data matrix
			jas_matrix_t *data_matrix(jas_matrix_create(map_size.cy, map_size.cx));
			if (0 == data_matrix)
				Error(_T("jas_matrix_create failed"));
			if (0 > jas_image_readcmpt(image, 0, 0, 0, map_size.cx, map_size.cy, data_matrix))
				Error(_T("jas_image_readcmpt failed"));
			// extract image data, with padding, and 0 where mask is true
			BYTE *data_ptr(heightmap_data.ptr);
			int mask_index(0);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				for (LONG c(0); c != map_size.cx; ++c)
					*data_ptr++ = mask[mask_index++] ? 0 : static_cast<BYTE>(jas_matrix_get(data_matrix, r, c));
				*data_ptr++ = *(data_ptr - 1); // pad horizontally
			}
			// pad vertically
			for (int c(0); c != map_size.cx + 1; ++c)
			{
				*data_ptr = *(data_ptr - map_size.cx - 1);
				++data_ptr;
			}
		}
		CreateLightMap(heightmap_data.ptr, map_size);
		heightmap.SignIn();
		heightmap.Update(0);
	}
}

bool CProjectManager::UnpackMapInfo(TiXmlNode *node)
{
	// get the necessary data
	TiXmlHandle node_handle(node);
	TiXmlText *map_name_node   (node_handle.FirstChildElement("map_name"   ).FirstChild().Text());
	TiXmlText *map_power_x_node(node_handle.FirstChildElement("map_power_x").FirstChild().Text());
	TiXmlText *map_power_y_node(node_handle.FirstChildElement("map_power_y").FirstChild().Text());
	if (
		NULL == map_name_node    ||
		NULL == map_power_x_node ||
		NULL == map_power_y_node)
	{
		Error(_T("unfamiliar shrub format"));
		return false;
	}
	// sign out map info
	CMapInfo &map_data(map_info.SignOut());
	// set critical data
	map_data.map_name = map_name_node->Value();
	map_data.map_power_x = atoi(map_power_x_node->Value());
	map_data.map_power_y = atoi(map_power_y_node->Value());
	// set window title
	SetWindowText(hWnd, map_data.map_name.c_str());
	// load the rest of the data
	map_data.LoadFromXml(node);
	// sign in and update
	map_info.SignIn();
	info_manager.SetReadOnly(true);
	map_info.Update(0);
	return true;
}

void CProjectManager::UnpackTexture(TiXmlNode *node, BYTE *buffer)
{
	// read in XML metadata
	BYTE *texture_buffer;
	BYTE *palette_buffer;
	{
		// find data
		TiXmlHandle node_handle(node);
		TiXmlText *offset_node        (node_handle.FirstChildElement("offset"        ).FirstChild().Text());
		TiXmlText *palette_offset_node(node_handle.FirstChildElement("palette_offset").FirstChild().Text());
		if (
			NULL == offset_node ||
			NULL == palette_offset_node)
		{
			_RPT0(_CRT_WARN, "loading default texture\n");
			DefaultTexture();
			texture.Update(0);
			return;
		}
		// parse the data
		texture_buffer = buffer + atoi(offset_node->Value());
		palette_buffer = buffer + atoi(palette_offset_node->Value());
	}
	// get dimensions of the map
	SIZE map_size;
	{
		const CMapInfo &map_data(map_info.SignOutConst());
		map_size.cx = exp2(map_data.map_power_x);
		map_size.cy = exp2(map_data.map_power_y);
		map_info.SignIn();
	}
	CPalettedTexture &texture_data(texture.SignOut());
	const size_t texture_size(map_size.cx * map_size.cy);
	const size_t palette_size(256 * sizeof(COLORREF));
	if (texture_data.length != texture_size)
	{
		texture_data.length = texture_size;
		delete [] texture_data.ptr;
		texture_data.ptr = new BYTE[texture_size];
	}
	CopyMemory(texture_data.palette, palette_buffer, palette_size);
	CopyMemory(texture_data.ptr,     texture_buffer, texture_size);
	texture.SignIn();
	texture.Update(0);
	return;
}

inline int CProjectManager::exp2(unsigned int n) const
{
	int e(1);
	while (0 != n)
	{
		e <<= 1;
		--n;
	}
	return e;
}

inline int CProjectManager::log2(unsigned int n) const
{
	int l(-1);
	while (0 != n)
	{
		n >>= 1;
		++l;
	}
	return l;
}

//----------------------------------------------
// CProjectManager::CSerializable implementation
//----------------------------------------------
CProjectManager::CSerializable::CSerializable(CProjectManager *parent)
 :parent(parent)
{}

void CProjectManager::CSerializable::Save(string file_name)
{
	const TCHAR * const section_name(_T("Project Settings"));
	CAutoCriticalSection auto_critical_section(&critical_section);
	// texture colour quality
	WritePrivateProfileString(
		section_name,
		_T("texture colour"),
		texture_colour_quality ? _T("quality") : _T("speed"),
		file_name.c_str());
	// enable_lighting
	WritePrivateProfileString(
		section_name,
		_T("enable lighting"),
		enable_lighting ? _T("true") : _T("false"),
		file_name.c_str());
}

void CProjectManager::CSerializable::Load(string file_name)
{
	const TCHAR * const section_name(_T("Project Settings"));
	const size_t buffer_size(16);
	TCHAR buffer[buffer_size];
	CAutoCriticalSection auto_critical_section(&critical_section);
	// texture_colour_quality
	GetPrivateProfileString(
		section_name,
		_T("texture colour"),
		_T("speed"),
		buffer,
		buffer_size,
		file_name.c_str());
	texture_colour_quality = (0 == _tcscmp(buffer, _T("quality")));
	// enable_lighting
	GetPrivateProfileString(
		section_name,
		_T("enable lighting"),
		_T("true"),
		buffer,
		buffer_size,
		file_name.c_str());
	enable_lighting = (0 == _tcscmp(buffer, _T("true")));
}

void CProjectManager::CSerializable::Update()
{
	if (PS_PROJECT == parent->project_state)
	{
		// schedule to update the heightmap soon
		parent->heightmap_time.dwLowDateTime  = numeric_limits<DWORD>::min();
		parent->heightmap_time.dwHighDateTime = numeric_limits<DWORD>::min();
		// schedule to update the texture soon
		parent->texture_time.dwLowDateTime  = numeric_limits<DWORD>::min();
		parent->texture_time.dwHighDateTime = numeric_limits<DWORD>::min();
	}
	if (PS_SHRUB == parent->project_state)
	{
		const CMapInfo &map_data(parent->map_info.SignOutConst());
		TCHAR file_path[MAX_PATH];
		_tcscpy(file_path, map_data.folder_path.c_str());
		PathCombine(file_path, file_path, map_data.map_name.c_str());
		PathAddExtension(file_path, _T(".shrub"));
		parent->map_info.SignIn();
		parent->Unpack(file_path);
	}
};