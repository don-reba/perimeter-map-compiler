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

#include "btdb.h"
#include "info wnd.h"
#include "preview wnd.h"
#include "project manager.h"
#include "project tasks.h"
#include "resource.h"
#include "resource management.h"
#include "stat wnd.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <set>
#include <shlobj.h>
#include <sstream>

using namespace RsrcMgmt;
using namespace TaskCommon;

//------------------------
// static member instances
//------------------------

TaskData Task::task_data_;

//----------------------------------------
// TaskData implementation
//----------------------------------------

TaskData::TaskData()
{
	InitializeCriticalSection(&section_);
}

TaskData::~TaskData()
{
	DeleteCriticalSection(&section_);
}

//--------------------------------------
// CreateDefaultFilesTask implementation
//--------------------------------------

CreateDefaultFilesTask::CreateDefaultFilesTask(HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
{}

void CreateDefaultFilesTask::operator() ()
{
	TCHAR path[MAX_PATH];
	PathCombine(path, task_data_.project_folder_.c_str(), _T("heightmap.bmp"));
	DeleteFile(path);
	SaveHeightmap(path, NULL, task_data_.map_size_, *this);
	PathCombine(path, task_data_.project_folder_.c_str(), _T("texture.bmp"));
	DeleteFile(path);
	SaveTexture(path, NULL, NULL, task_data_.map_size_, *this);
}

//--------------------------------------------------
// SetProjectDataTask implementation
//--------------------------------------------------

UpdateDataTask::UpdateDataTask(
	LPCTSTR      map_name,
	LPCTSTR      project_folder,
	SIZE         map_size,
	ProjectState project_state,
	bool         fast_quantization,
	bool         enable_lighting,
	float        mesh_threshold,
	bool        display_texture)
	:map_name_         (map_name)
	,project_folder_   (project_folder)
	,map_size_         (map_size)
	,project_state_    (project_state)
	,fast_quantization_(fast_quantization)
	,enable_lighting_  (enable_lighting)
	,mesh_threshold_   (mesh_threshold)
	,display_texture_  (display_texture)
{}

void UpdateDataTask::operator() ()
{
	task_data_.map_name_          = map_name_;
	task_data_.project_folder_    = project_folder_;
	task_data_.map_size_          = map_size_;
	task_data_.project_state_     = project_state_;
	task_data_.fast_quantization_ = fast_quantization_;
	task_data_.enable_lighting_   = enable_lighting_;
	task_data_.mesh_threshold_    = mesh_threshold_;
	task_data_.display_texture_   = display_texture_;
	task_data_.map_info_.Load();
}

//-----------------------------------
// CacheProjectDataTask implemenation
//-----------------------------------

CacheProjectDataTask::CacheProjectDataTask(HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
{}

void CacheProjectDataTask::operator() ()
{
	_ASSERTE(NULL == task_data_.hardness_);
	_ASSERTE(NULL == task_data_.heightmap_);
	_ASSERTE(NULL == task_data_.texture_);
	task_data_.hardness_  = new Hardness (task_data_.map_size_, error_hwnd_);
	task_data_.heightmap_ = new Heightmap(task_data_.map_size_, error_hwnd_);
	task_data_.texture_   = new Texture  (task_data_.map_size_, error_hwnd_);
	TCHAR path[MAX_PATH];
	task_data_.hardness_->Load(PathCombine(
		path, task_data_.project_folder_.c_str(),
		_T("hardness.bmp")));
	task_data_.heightmap_->Load(PathCombine(
		path, task_data_.project_folder_.c_str(),
		_T("heightmap.bmp")));
	task_data_.texture_->Load(
		PathCombine(path, task_data_.project_folder_.c_str(), _T("texture.bmp")),
		task_data_.fast_quantization_);
}

//-----------------------------------
// FreeProjectDataTask implementation
//-----------------------------------

void FreeProjectDataTask::operator() ()
{
	_ASSERTE(NULL != task_data_.hardness_);
	_ASSERTE(NULL != task_data_.heightmap_);
	_ASSERTE(NULL != task_data_.texture_);
	delete task_data_.hardness_;
	delete task_data_.heightmap_;
	delete task_data_.texture_;
	task_data_.hardness_  = NULL;
	task_data_.heightmap_ = NULL;
	task_data_.texture_   = NULL;
}

//-----------------------------------
// LoadProjectDataTask implementation
//-----------------------------------

LoadProjectDataTask::LoadProjectDataTask(
	const IdsType  &ids,
	InfoWnd        &info_wnd,
	PreviewWnd     &preview_wnd,
	StatWnd        &stat_wnd,
	ProjectManager &project_manager,
	HWND           &error_hwnd)
	:ErrorHandler    (error_hwnd)
	,ids_            (ids)
	,info_wnd_       (info_wnd)
	,preview_wnd_    (preview_wnd)
	,project_manager_(project_manager)
	,stat_wnd_       (stat_wnd)
{}

void LoadProjectDataTask::operator() ()
{
	_ASSERTE(resource_count == ids_.size());
	TCHAR path[MAX_PATH];
	Hardness  hardness (task_data_.map_size_, error_hwnd_);
	Heightmap heightmap(task_data_.map_size_, error_hwnd_);
	Lightmap  lightmap (task_data_.map_size_, error_hwnd_);
	Texture   texture  (task_data_.map_size_, error_hwnd_);
	bool preview_wnd_visible(preview_wnd_.IsVisible());
	bool stat_wnd_visible   (stat_wnd_.IsVisible());
	// load data
	bool load_hardness;
	bool load_heightmap;
	bool load_lightmap;
	bool load_texture;
	{
		// set dependencies
		load_hardness = ids_[RS_HARDNESS];
		load_texture = ids_[RS_TEXTURE];
		load_lightmap =
			preview_wnd_visible && (
				 task_data_.display_texture_ && load_texture ||
				!task_data_.display_texture_ && load_hardness);
		load_heightmap = ids_[RS_HEIGHTMAP] || load_lightmap || stat_wnd_visible && ids_[RS_TEXTURE];
		// load
		if (load_hardness)
			hardness.Load(PathCombine(path, task_data_.project_folder_.c_str(), _T("hardness.bmp")));
		if (load_heightmap)
			heightmap.Load(PathCombine(path, task_data_.project_folder_.c_str(), _T("heightmap.bmp")));
		if (load_lightmap)
			lightmap.Create(heightmap);
		if (load_texture)
			texture.Load(
				PathCombine(path, task_data_.project_folder_.c_str(), _T("texture.bmp")),
				task_data_.fast_quantization_);
	}
	// update windows
	{
		// stat wnd
		if (stat_wnd_visible)
		{
			if (load_heightmap)
				stat_wnd_.SetAverageHeight(AverageHeight(heightmap));
			if (load_texture)
				stat_wnd_.SetAverageColour(AverageColour(texture, heightmap));
		}
		else
			stat_wnd_.SetVisibilityNotification(new OnPanelVisible(ids_, project_manager_));
		// preview wnd
		if (preview_wnd_visible)
		{
			// update the heightmap
			if (ids_[RS_HEIGHTMAP])
			{
				vector<SimpleVertex> vertices;
				Triangulate(heightmap, vertices, task_data_.mesh_threshold_);
				SendSetTerrain(preview_wnd_.hwnd_, vertices);
			}
			// update the texture
			if (task_data_.display_texture_)
			{
				if (load_texture)
				{
					TextureAllocation *allocation(SendTextureAllocate(preview_wnd_.hwnd_));
					if (NULL != allocation)
					{
						CreateTextures(texture, *allocation, lightmap, task_data_.enable_lighting_);
						SendTextureCommit(preview_wnd_.hwnd_);
						delete allocation;
					}
				}
			}
			else
			{
				if (load_hardness)
				{
					TextureAllocation *allocation(SendTextureAllocate(preview_wnd_.hwnd_));
					if (NULL != allocation)
					{
						CreateTextures(hardness, *allocation, lightmap, task_data_.enable_lighting_);
						SendTextureCommit(preview_wnd_.hwnd_);
						delete allocation;
					}
				}
			}
		}
		else
			preview_wnd_.SetVisibilityNotification(new OnPanelVisible(ids_, project_manager_));
	}
}

//--------------------------------------------------
// LoadProjectDataTask::OnPanelVisible implemenation
//--------------------------------------------------

LoadProjectDataTask::OnPanelVisible::OnPanelVisible(const IdsType &ids, ProjectManager &project_manager)
	:ids_(ids)
	,project_manager_(project_manager)
{}

void LoadProjectDataTask::OnPanelVisible::operator ()(bool on)
{
	project_manager_.ReloadFiles(ids_);
}

//-----------------------------
// PackShrubTask implementation
//-----------------------------

PackShrubTask::PackShrubTask(HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
{}

void PackShrubTask::operator() ()
{
	_ASSERTE(NULL != task_data_.heightmap_);
	_ASSERTE(NULL != task_data_.texture_);
	Heightmap &heightmap(*task_data_.heightmap_);
	Texture   &texture  (*task_data_.texture_);
	// calculate the checksum and register the map
	{
		DWORD   checksum(0);
		BYTE   *data;
		size_t  size;
		// heightmap
		data = heightmap.data_;
		size = heightmap.size_.cx * heightmap.size_.cy;
		checksum = CalculateChecksum(data, size, checksum);
		// texture
		data = texture.indices_;
		size = texture.size_.cx * texture.size_.cy;
		checksum = CalculateChecksum(data, size, checksum);
		data = ri_cast<BYTE*>(texture.palette_);
		size = 768;
		checksum = CalculateChecksum(data, size, checksum);
		// map info
		task_data_.map_info_.GetRawData(&data, &size);
		checksum = CalculateChecksum(data, size, checksum);
		delete [] data;
		// register the map
		if (!RegisterMap(task_data_.map_name_.c_str(), checksum, *this))
			return;
	}
	// initialise an XML metadata document
	TiXmlDocument doc;
	TiXmlNode *content_node(doc.InsertEndChild(TiXmlElement("content")));
	// allocate a buffer for the data
	size_t buffer_size;
	{
		const size_t heightmap_size(task_data_.map_size_.cx * task_data_.map_size_.cy);
		const size_t mask_size     (task_data_.map_size_.cx * task_data_.map_size_.cy / 8);
		const size_t texture_size  (task_data_.map_size_.cx * task_data_.map_size_.cy);
		const size_t palette_size  (256);
		buffer_size = heightmap_size + mask_size + texture_size + palette_size;
	}
	BYTE *buffer(new BYTE[buffer_size]);
	// add heightmap, texture and map information
	{
		BYTE *buffer_iter(buffer);
		vector<bool> mask;
		task_data_.map_info_.Pack(*content_node->InsertEndChild(TiXmlElement("map_info")));
		buffer_iter += heightmap.Pack(
			*content_node->InsertEndChild(TiXmlElement("heightmap")),
			buffer_iter,
			buffer,
			mask);
		buffer_iter += texture.Pack(
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
		MacroDisplayError(_T("BZ2_bzBuffToBuffCompress failed"));
		delete [] buffer;
		delete [] compressed_buffer;
		return;
	}
	delete [] buffer;
	compressed_buffer_size += header_size;
	// record header
	{
#ifdef PRE_RELEASE
		char header[8] = { 'S', 'H', 'R', 'B', 'B', 'Z', '2', 0 };
#else // PRE_RELEASE
		char header[8] = { 'S', 'H', 'R', 0,   'B', 'Z', '2', 0 };
#endif // PRE_RELEASE
		CopyMemory(compressed_buffer, header, 8);
		CopyMemory(compressed_buffer + 8, &buffer_size, 4);
	}
	// save the packed Biboorat into a shrub file
	TCHAR path[MAX_PATH];
	PathCombine(path, task_data_.project_folder_.c_str(), task_data_.map_name_.c_str());
	PathAddExtension(path, _T(".shrub"));
	SaveMemToFile(path, compressed_buffer, compressed_buffer_size, *this);
	delete [] compressed_buffer;
	// reassure the user
	MessageBox(
		error_hwnd_,
		_T("The map has been successfully packed, and saved to the project folder."),
		_T("Success"),
		MB_ICONINFORMATION | MB_OK);
}

//------------------------------
// UnpackShrubTask implemenation
//------------------------------

UnpackShrubTask::UnpackShrubTask(
		TiXmlDocument  *document,
		BYTE           *buffer,
		InfoWnd        &info_wnd,
		PreviewWnd     &preview_wnd,
		StatWnd        &stat_wnd,
		ProjectManager &project_manager,
		HWND           &error_hwnd)
	:ErrorHandler(error_hwnd)
	,buffer_         (buffer)
	,document_       (document)
	,info_wnd_       (info_wnd)
	,preview_wnd_    (preview_wnd)
	,project_manager_(project_manager)
	,stat_wnd_       (stat_wnd)
{}

UnpackShrubTask::~UnpackShrubTask()
{
	delete document_;
}

void UnpackShrubTask::operator() ()
{
	TiXmlElement *content_node(document_->FirstChildElement("content"));
	if (NULL == content_node)
		return;
	// unpack the heightmap
	_ASSERTE(NULL == task_data_.heightmap_);
	task_data_.heightmap_ = new Heightmap(task_data_.map_size_, error_hwnd_);
	Heightmap &heightmap(*task_data_.heightmap_);
	heightmap.Unpack(content_node->FirstChildElement("heightmap"), buffer_);
	// unpack the texture
	_ASSERTE(NULL == task_data_.texture_);
	task_data_.texture_ = new Texture(task_data_.map_size_, error_hwnd_);
	Texture &texture(*task_data_.texture_);
	texture.Unpack(content_node->FirstChildElement("texture"), buffer_);
	// stat wnd
	if (stat_wnd_.IsVisible())
	{
		stat_wnd_.SetAverageHeight(AverageHeight(heightmap));
		stat_wnd_.SetAverageColour(AverageColour(texture, heightmap));
	}
	else
		stat_wnd_.SetVisibilityNotification(new OnPanelVisible(project_manager_));
	// preview wnd
	if (preview_wnd_.IsVisible())
	{
		Lightmap lightmap(task_data_.map_size_, error_hwnd_);
		if (task_data_.enable_lighting_)
			lightmap.Create(heightmap);
		{
			TextureAllocation *allocation(SendTextureAllocate(preview_wnd_.hwnd_));
			if (NULL != allocation)
			{
				CreateTextures(texture, *allocation, lightmap, task_data_.enable_lighting_);
				SendTextureCommit(preview_wnd_.hwnd_);
				delete allocation;
			}
		}
		{
			vector<SimpleVertex> vertices;
			Triangulate(heightmap, vertices, task_data_.mesh_threshold_);
			SendSetTerrain(preview_wnd_.hwnd_, vertices);
		}
	}
	else
		preview_wnd_.SetVisibilityNotification(new OnPanelVisible(project_manager_));
}

//----------------------------------------------
// UnpackShrubTask::OnPanelVisible implemenation
//----------------------------------------------

UnpackShrubTask::OnPanelVisible::OnPanelVisible(ProjectManager &project_manager)
	:project_manager_(project_manager)
{}

void UnpackShrubTask::OnPanelVisible::operator ()(bool on)
{
	if (!on)
		return;
	IdsType ids;
	ids.set();
	project_manager_.ReloadFiles(ids);
}

//--------------------------------
// InstallShrubTask implementation
//--------------------------------

InstallMapTask::InstallMapTask(HWND &hwnd, tstring &perimeter_path)
	:ErrorHandler   (hwnd)
	,hwnd_          (hwnd)
	,perimeter_path_(perimeter_path)
{}

void InstallMapTask::operator() ()
{
	_ASSERTE(task_data_.hardness_  != NULL);
	_ASSERTE(task_data_.heightmap_ != NULL);
	_ASSERTE(task_data_.texture_   != NULL);
	Hardness  &hardness (*task_data_.hardness_);
	Heightmap &heightmap(*task_data_.heightmap_);
	Texture   &texture  (*task_data_.texture_);
	TCHAR      str        [MAX_PATH];
	TCHAR      folder_path[MAX_PATH];
	tstring    install_path;
	bool       overwriting(false);
	uint       version(1);
	// get install path
	{
		DWORD attributes(GetFileAttributes(perimeter_path_.c_str()));
		if (INVALID_FILE_ATTRIBUTES == attributes)
		{
			if (!GetInstallPath(install_path, *this))
				return;
		}
		else
		{
			if (perimeter_path_.size() >= MAX_PATH)
			{
				MacroDisplayError(_T("The installation path is too long."));
				return;
			}
			if (0 == (FILE_ATTRIBUTE_DIRECTORY | attributes))
			{
				_tcscpy(str, perimeter_path_.c_str());
				PathRemoveFileSpec(str);
				install_path = str;
			}
			else
				install_path = perimeter_path_;
			version = 2;
		}
	}
	// get the name to use for stuff that needs a name
	// this is the map name for a registered map, and "UNREGISTERED" otherwise
	// ASSUME shrubs are registered
	tstring folder_name((PS_SHRUB == task_data_.project_state_) ? task_data_.map_name_ :  _T("UNREGISTERED"));
	// create a directory for the map
	{
		// create path
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Worlds"));
		PathCombine(folder_path, folder_path, folder_name.c_str());
		// create directory
		int result(SHCreateDirectoryEx(hwnd_, folder_path, NULL));
		switch (result)
		{
		case ERROR_SUCCESS:
			overwriting = true;
			break;
		case ERROR_ALREADY_EXISTS:
			{
				if (PS_SHRUB == task_data_.project_state_ && IDCANCEL == MessageBox(
					hwnd_,
					_T("Map with this name already exists. Continuing will overwrite its contents."),
					_T("Installation Error"),
					MB_ICONWARNING | MB_OKCANCEL))
					return;
			} break;
		default:
			MacroDisplayError(_T("SHCreateDirectoryEx failed"));
			return;
		}
	}
	// create and save map.tga
	{
		Lightmap lightmap(task_data_.map_size_, error_hwnd_);
		lightmap.Create(heightmap);
		PathCombine(str, folder_path, _T("map.tga"));
		SIZE size = { 128, 128 };
		SaveThumb(heightmap, lightmap, texture, str, size, *this);
	}
	// create and save world.ini
	{
		PathCombine(str, folder_path, _T("world.ini"));
		std::ofstream world_ini(str, std::ios_base::binary | std::ios_base::out);
		world_ini << task_data_.map_info_.GenerateWorldIni();
	}
	// create and save output.vmp
	PathCombine(str, folder_path, _T("output.vmp"));
	SaveVMP(hardness, heightmap, texture, str, *this);
	// create and save inDam.act
	PathCombine(str, folder_path, _T("inDam.act"));
	SavePalette(texture, str, *this);
	// append Texts.btdb
	{
		tstring language;
		// get the language of the distribution
		{
			PathCombine(str, install_path.c_str(), "Perimeter.ini");
			std::ifstream ini(str);
			if (!ini.is_open())
			{
				MacroDisplayError(_T("Perimeter.ini could not be opened."));
				return;
			}
			tstring line;
			tstring target(_T("DefaultLanguage="));
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
	AppendWorldsPrm(str, folder_name.c_str(), version);
	// create save up.tga from a resource
	PathCombine(str, folder_path, _T("up.tga"));
	SaveImageFromResource(IDR_UP_TX, str, *this);
	// create save leveledSurfaceTexture.tga from a resource
	PathCombine(str, folder_path, _T("leveledSurfaceTexture.tga"));
	SaveImageFromResource(IDR_SURFACE_TX, str, *this);
	// create and save hardness.bin
	PathCombine(str, folder_path, _T("hardness.bin"));
	SaveHardness(str, task_data_.hardness_->data_, task_data_.map_size_, *this);
	// generate mission files
	switch (version)
	{
	case 1:
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission(folder_path, folder_name.c_str(), false);
		break;
	case 2:
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission2(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission2(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission2(folder_path, folder_name.c_str(), false);
		break;
	default:
		DebugBreak();
	}
	// reassure the user
	/*MessageBox(
		hwnd_,
		_T("Installation has been completed successfully."),
		_T("Success"),
		MB_ICONINFORMATION | MB_OK);*/
}

// set the appropriate entry of Texts.btdb to the name of the map
void InstallMapTask::AppendBTDB(LPCTSTR path, LPCTSTR folder_name)
{
	// set a prefix for the map name
	const char * const prefix((PS_SHRUB == task_data_.project_state_) ? "" : "[U]");
	const size_t prefix_size = strlen(prefix);
	// get the name of the map
	tstring map_name(task_data_.map_name_);
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
	Btdb btdb(path);
	btdb.AddMapEntry(folder_name, map_name.c_str());
}

void InstallMapTask::AppendWorldsPrm(LPCTSTR path, LPCTSTR folder_name, uint version)
{
	std::set<tstring> worlds;
	// create a list of worlds
	{
		// read in strings from the worlds_list.txt resource
		{
			tistringstream worlds_list_stream;
			// load the list of reserved names
			{
				size_t alloc(1024); // 1 KB
				vector<TCHAR> result;
				result.resize(alloc);
				if (!UncompressResource(IDR_WORLDS_LIST, ri_cast<BYTE*>(&result[0]), alloc))
				{
					MacroDisplayError("Worlds list could not be loaded.");
					return;
				}
				worlds_list_stream.str(&result[0]);
			}
			std::copy(
				std::istream_iterator<tstring>(worlds_list_stream),
				std::istream_iterator<tstring>(),
				std::inserter(worlds, worlds.begin()));
		}
		if (2 == version)
		{
			tistringstream worlds_list_stream;
			// load the list of reserved names
			{
				size_t alloc(1024); // 1 KB
				vector<TCHAR> result;
				result.resize(alloc);
				if (!UncompressResource(IDR_WORLDS_LIST_2, ri_cast<BYTE*>(&result[0]), alloc))
				{
					MacroDisplayError("Worlds list could not be loaded.");
					return;
				}
				worlds_list_stream.str(&result[0]);
			}
			std::copy(
				std::istream_iterator<tstring>(worlds_list_stream),
				std::istream_iterator<tstring>(),
				std::inserter(worlds, worlds.begin()));
		}
		// open WORLDS.PRM
		std::ifstream prm_file(path);
		if (!prm_file.is_open())
		{
			MacroDisplayError(_T("Could not open WORLDS.PRM for reading."));
			return;
		}
		// read in strings from WORLDS.PRM
		{
			DWORD num_worlds;
			prm_file >> num_worlds;
			tstring world;
			while (prm_file)
			{
				prm_file >> world >> world;
				if (!world.empty())
					worlds.insert(world);
			}
		}
		// append the new world
		{
			tstring new_world(folder_name);
			worlds.insert(new_world);
		}
	}
	// save the list of worlds
	{
		std::ofstream prm_file(path);
		if (!prm_file.is_open())
		{
			MacroDisplayError(_T("Could not open WORLDS.PRM for writing."));
			return;
		}
		std::set<tstring>::const_iterator       i  (worlds.begin());
		const std::set<tstring>::const_iterator end(worlds.end());
		prm_file << worlds.size() << _T("\n\n");
		for (; i != end; ++i)
			prm_file << std::setw(0) << *i << std::setw(24) << *i << _T("\n");
	}
}

// v 1.01
void InstallMapTask::SaveMission(LPCTSTR path, LPCTSTR folder_name, bool survival)
{
	TCHAR str[MAX_PATH];
	// create an empty ".dat" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".dat"));
	SaveMemToFile(str, NULL, 0, *this);
	// create an empty ".gmp" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".gmp"));
	SaveMemToFile(str, NULL, 0, *this);
	// create the ".spg" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".spg"));
	SaveSPG(task_data_.map_info_, str, folder_name, survival, *this);
	// create the ".sph" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".sph"));
	SaveSPH(str, folder_name, survival, *this);
}

// v 1.02 beta
void InstallMapTask::SaveMission2(LPCTSTR path, LPCTSTR folder_name, bool survival)
{
	TCHAR str[MAX_PATH];
	// create an empty ".dat" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".dat"));
	SaveMemToFile(str, NULL, 0, *this);
	// create an empty ".gmp" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".gmp"));
	SaveMemToFile(str, NULL, 0, *this);
	// create the ".spg" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".spg"));
	SaveSPG2(task_data_.map_info_, str, folder_name, survival, *this);
}

//-----------------------------
// SaveThumbTask implementation
//-----------------------------

SaveThumbTask::SaveThumbTask(HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
{}

void SaveThumbTask::operator() ()
{
	_ASSERTE(NULL != task_data_.heightmap_);
	_ASSERTE(NULL != task_data_.texture_);
	Heightmap &heightmap(*task_data_.heightmap_);
	Texture   &texture  (*task_data_.texture_);
	Lightmap   lightmap(task_data_.map_size_, error_hwnd_);
	lightmap.Create(heightmap);
	TCHAR path[MAX_PATH];
	PathCombine(path, task_data_.project_folder_.c_str(), _T("thumbnail.png"));
	SIZE size = { 256, 256 };
	SaveThumb(heightmap, lightmap, texture, path, size, *this);
}

//----------------------------------
// ChangeProjectTask implemenatation
//----------------------------------

ChangeProjectTask::ChangeProjectTask(InfoWnd &info_wnd, PreviewWnd &preview_wnd, bool read_only)
	:info_wnd_   (info_wnd)
	,preview_wnd_(preview_wnd)
	,read_only_  (read_only)
{}

void ChangeProjectTask::operator() ()
{
	info_wnd_.Update(read_only_);
	preview_wnd_.ProjectChanged();
}

//----------------------------------
// CreateResourceTask implementation
//----------------------------------

CreateResourceTask::CreateResourceTask(uint id, HWND error_hwnd)
	:ErrorHandler(error_hwnd)
	,id_(id)
{}

void CreateResourceTask::operator() ()
{
	TCHAR path[MAX_PATH];
	switch (id_)
	{
	case RS_HARDNESS:
		PathCombine(path, task_data_.project_folder_.c_str(), _T("hardness.bmp"));
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(path))
		{
			Hardness hardness(task_data_.map_size_, error_hwnd_);
			hardness.MakeDefault();
			hardness.Save(path);
		}
		break;
	}
}
