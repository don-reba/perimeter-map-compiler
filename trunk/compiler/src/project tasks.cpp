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
// • Neither the name of Don Reba nor the names of his contributors may be used
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

#include "resource.h"
#include "btdb.h"
#include "info wnd.h"
#include "main wnd.h"
#include "preview wnd.h"
#include "project manager.h"
#include "project tasks.h"
#include "resource management.h"
#include "script creator.h"
#include "stat wnd.h"
#include "xml creator.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <set>
#include <shlobj.h>
#include <sstream>

#include <loki/ScopeGuard.h>

using namespace RsrcMgmt;
using namespace TaskCommon;

//----------------------------------------
// TaskData implementation
//----------------------------------------

TaskData::TaskData(SaveCallback::SaveHandler *save_handler, const HWND &error_hwnd)
	:um_hardness_  (error_hwnd)
	,um_heightmap_ (error_hwnd)
	,um_script_    (error_hwnd)
	,um_sky_       (error_hwnd)
	,um_surface_   (error_hwnd)
	,um_texture_   (error_hwnd)
	,um_zero_layer_(error_hwnd)
	,hardness_  (um_hardness_,   manager_)
	,heightmap_ (um_heightmap_,  manager_)
	,script_    (um_script_,     manager_)
	,sky_       (um_sky_,        manager_)
	,surface_   (um_surface_,    manager_)
	,texture_   (um_texture_,    manager_)
	,zero_layer_(um_zero_layer_, manager_)
	,manager_(error_hwnd)
{
	manager_.AddResource(&hardness_  );
	manager_.AddResource(&heightmap_ );
	manager_.AddResource(&script_    );
	manager_.AddResource(&script_    );
	manager_.AddResource(&sky_       );
	manager_.AddResource(&surface_   );
	manager_.AddResource(&texture_   );
	manager_.AddResource(&zero_layer_);
	um_hardness_.SetOnSave  (save_handler);
	um_heightmap_.SetOnSave (save_handler);
	um_script_.SetOnSave    (save_handler);
	um_sky_.SetOnSave       (save_handler);
	um_surface_.SetOnSave   (save_handler);
	um_texture_.SetOnSave   (save_handler);
	um_zero_layer_.SetOnSave(save_handler);
}

void TaskData::SetResourceManagerEnabled(bool enable)
{
	manager_.SetEnabled(enable);
}

//--------------------------------------
// CreateDefaultFilesTask implementation
//--------------------------------------

CreateDefaultFilesTask::CreateDefaultFilesTask(const HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
{}

void CreateDefaultFilesTask::operator() (TaskData &data)
{
	using namespace Loki;
	TCHAR path[MAX_PATH];
	::PathCombine(path, data.project_folder_.c_str(), _T("heightmap.png"));
	::DeleteFile(path);
	{
		Heightmap::info_t &info(data.heightmap_.GetInfo());
		info.path_ = path;
		info.size_ = data.map_size_;
		Heightmap &heightmap(data.heightmap_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
		heightmap.MakeDefault();
		heightmap.Save();
	}
	::PathCombine(path, data.project_folder_.c_str(), _T("texture.png"));
	::DeleteFile(path);
	{
		Texture::info_t &info(data.texture_.GetInfo());
		info.path_              = path;
		info.size_              = data.map_size_;
		info.fast_quantization_ = true;
		Texture &texture(data.texture_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.texture_, &TaskResource<Texture>::CheckIn);
		texture.MakeDefault();
		texture.Save();
	}
}

//----------------------------------
// ChangeProjectTask implemenatation
//----------------------------------

ChangeProjectTask::ChangeProjectTask(PreviewWnd &preview_wnd)
:preview_wnd_(preview_wnd)
{}

void ChangeProjectTask::operator() (TaskData &data)
{
	preview_wnd_.ProjectChanged();
}

//----------------------------------
// CreateResourceTask implementation
//----------------------------------

CreateResourceTask::CreateResourceTask(uint id, const HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
	,id_(id)
{}

void CreateResourceTask::operator() (TaskData &data)
{
	using namespace Loki;
	TCHAR path[MAX_PATH];
	const TCHAR * const folder_path(data.project_folder_.c_str());
	switch (id_)
	{
	case RS_HARDNESS:
		::PathCombine(path, folder_path, _T("hardness.bmp"));
		if (INVALID_FILE_ATTRIBUTES == ::GetFileAttributes(path))
		{
			Hardness::info_t &info(data.hardness_.GetInfo());
			info.path_ = path;
			info.size_ = data.map_size_;
			Hardness &hardness(data.hardness_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.hardness_, &TaskResource<Hardness>::CheckIn);
			hardness.MakeDefault();
			hardness.Save();
		}
		break;
	case RS_ZERO_LAYER:
		::PathCombine(path, folder_path, _T("zero layer.bmp"));
		if (INVALID_FILE_ATTRIBUTES == ::GetFileAttributes(path))
		{
			ZeroLayer::info_t &info(data.zero_layer_.GetInfo());
			info.path_ = path;
			info.size_ = data.map_size_;
			ZeroLayer &zero_layer(data.zero_layer_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.zero_layer_, &TaskResource<ZeroLayer>::CheckIn);
			zero_layer.MakeDefault();
			zero_layer.Save();
		}
		break;
	case RS_SKY:
		::PathCombine(path, folder_path, _T("sky.bmp"));
		if (INVALID_FILE_ATTRIBUTES == ::GetFileAttributes(path))
		{
			Sky::info_t &info(data.sky_.GetInfo());
			info.path_ = path;
			Sky &sky(data.sky_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.sky_, &TaskResource<Sky>::CheckIn);
			sky.MakeDefault();
			sky.Save();
		}
		break;
	case RS_SURFACE:
		::PathCombine(path, folder_path, _T("surface.bmp"));
		if (INVALID_FILE_ATTRIBUTES == ::GetFileAttributes(path))
		{
			Surface::info_t &info(data.surface_.GetInfo());
			info.path_ = path;
			Surface &surface(data.surface_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.surface_, &TaskResource<Surface>::CheckIn);
			surface.MakeDefault();
			surface.Save();
		}
		break;
	}
}

//--------------------------------
// ImportScriptTask implementation
//--------------------------------

ImportScriptTask::ImportScriptTask(LPCTSTR script_path, LPCTSTR xml_path)
	:script_(script_path)
	,xml_   (xml_path)
{}

void ImportScriptTask::operator() (TaskData &data)
{
	XmlCreator xml_creator;
	{
		XmlCreator::LoadResult result(xml_creator.LoadFromFile(script_.c_str()));
		if (!result.success_)
		{
			tstringstream stream;
			stream << _T("Script could not be imported.\n");
			stream << _T("Only ") << result.chars_consumed_ << _T(" characters have been read.");
			throw TaskException(stream.str().c_str());
		}
	}
	std::ofstream file(xml_.c_str());
	if (!file)
	{
		vector<TCHAR> buffer_v(MAX_PATH);
		TCHAR *buffer(&buffer_v[0]);
		_tcscpy(buffer, xml_.c_str());
		PathStripPath(buffer);
		tstring error_message("Script could not be imported.\nFailed to create \"");
		error_message += buffer;
		error_message += "\".";
		throw TaskException(error_message.c_str());
	}
	xml_creator.Read(file);
}

//--------------------------------
// InstallShrubTask implementation
//--------------------------------

InstallMapTask::InstallMapTask(
	const HWND &hwnd,
	LPCTSTR     install_path,
	uint        version,
	bool        custom_zero_layer,
	bool        rename_to_unregistered)
	:ErrorHandler           (hwnd)
	,hwnd_                  (hwnd)
	,install_path_          (install_path)
	,version_               (version)
	,custom_zero_layer_     (custom_zero_layer)
	,rename_to_unregistered_(rename_to_unregistered)
{}

void InstallMapTask::operator() (TaskData &data)
{
	using namespace Loki;
	// get the heightmap
	Heightmap &heightmap(data.heightmap_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
	// get the texture
	Texture &texture(data.texture_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.texture_, &TaskResource<Texture>::CheckIn);
	// initialize variables
	TCHAR str        [MAX_PATH];
	TCHAR folder_path[MAX_PATH];
	bool  overwriting(false);
	// get the name to use for stuff that needs a name
	// this is the map name for a registered map, and "UNREGISTERED" otherwise
	// ASSUME shrubs are registered
	tstring folder_name(
		(PS_SHRUB == data.project_state_ || !rename_to_unregistered_)
		? data.map_name_
		: _T("UNREGISTERED"));
	// create a directory for the map
	{
		// create path
		PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Worlds"));
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
				if (PS_SHRUB == data.project_state_ && IDCANCEL == MessageBox(
					hwnd_,
					_T("Map with this name already exists. Continuing will overwrite its contents."),
					_T("Warning"),
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
		Lightmap lightmap(error_hwnd_);
		lightmap.Create(heightmap);
		::PathCombine(str, folder_path, _T("map.tga"));
		SIZE size = { 128, 128 };
		SaveThumb(heightmap, lightmap, texture, str, size, *this);
	}
	// create and save world.ini
	{
		MapInfo &map_info(data.map_info_);
		::PathCombine(str, folder_path, _T("world.ini"));
		std::ofstream world_ini(str, std::ios_base::binary | std::ios_base::out);
		world_ini << map_info.GenerateWorldIni();
	}
	// create and save output.vmp
	{
		ZeroLayer &zero_layer(data.zero_layer_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.zero_layer_, &TaskResource<ZeroLayer>::CheckIn);
		::PathCombine(str, folder_path, _T("output.vmp"));
		SaveVMP(heightmap, texture, custom_zero_layer_ ? NULL : &zero_layer, str, *this);
	}
	// create and save inDam.act
	::PathCombine(str, folder_path, _T("inDam.act"));
	SavePalette(texture, str, *this);
	// append Texts.btdb
	{
		tstring language;
		// get the language of the distribution
		{
			::PathCombine(str, install_path_.c_str(), "Perimeter.ini");
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
		::PathCombine(str, install_path_.c_str(), "RESOURCE\\LocData");
		::PathCombine(str, str, language.c_str());
		::PathCombine(str, str, "Text\\Texts.btdb");
		AppendBTDB(str, folder_name.c_str(), data.project_state_, data.map_name_);
	}
	// append WORLDS.PRM
	::PathCombine(str, install_path_.c_str(), "RESOURCE\\Worlds\\WORLDS.PRM");
	AppendWorldsPrm(str, folder_name.c_str(), version_);
	// save up.tga
	::PathCombine(str, folder_path, _T("up.tga"));
	{
		Sky &sky(data.sky_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.sky_, &TaskResource<Sky>::CheckIn);
		sky.SaveAs(str);
	}
	// leveledSurfaceTexture.tga
	::PathCombine(str, folder_path, _T("leveledSurfaceTexture.tga"));
	{
		Surface &surface(data.surface_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.surface_, &TaskResource<Surface>::CheckIn);
		surface.SaveAs(str);
	}
	// create and save hardness.bin
	::PathCombine(str, folder_path, _T("leveledSurfaceTexture.tga"));
	{
		Hardness &hardness(data.hardness_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.hardness_, &TaskResource<Hardness>::CheckIn);
		hardness.SaveAs(str);
	}
	// generate mission files
	{
		MapInfo &map_info(data.map_info_);
		switch (version_)
		{
		case 1:
			::PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle"));
			::PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
			SaveMission(map_info, folder_path, folder_name.c_str(), false);
			::PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
			::PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
			SaveMission(map_info, folder_path, folder_name.c_str(), false);
			::PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
			::PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
			SaveMission(map_info, folder_path, folder_name.c_str(), false);
			break;
		case 2:
			::PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle"));
			::PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
			SaveMission2(map_info, folder_path, folder_name.c_str(), false);
			::PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
			::PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
			SaveMission2(map_info, folder_path, folder_name.c_str(), false);
			::PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
			::PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
			SaveMission2(map_info, folder_path, folder_name.c_str(), false);
			break;
		default:
			DebugBreak();
		}
	}
}

// set the appropriate entry of Texts.btdb to the name of the map
void InstallMapTask::AppendBTDB(
	LPCTSTR      path,
	LPCTSTR      folder_name,
	ProjectState project_state,
	tstring      map_name)
{
	// set a prefix for the map name
	const char * const prefix((PS_SHRUB == project_state) ? "" : "[U]");
	const size_t prefix_size = strlen(prefix);
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
void InstallMapTask::SaveMission(
	MapInfo map_info,
	LPCTSTR path,
	LPCTSTR folder_name,
	bool    survival)
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
	// save
	SaveSPG(map_info, str, folder_name, survival, *this);
	// create the ".sph" file
	_tcscpy(str, path);
	PathAddExtension(str, _T(".sph"));
	SaveSPH(str, folder_name, survival, *this);
}

// v 1.02
void InstallMapTask::SaveMission2(
	MapInfo map_info,
	LPCTSTR path,
	LPCTSTR folder_name,
	bool    survival)
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
	// save
	SaveSPG2(map_info, str, folder_name, survival, *this);
}

//-----------------------------------
// LoadProjectDataTask implementation
//-----------------------------------

//LoadProjectDataTask::LoadProjectDataTask(const IdsType  &ids, HWND &error_hwnd)
//	:ErrorHandler(error_hwnd)
//	,ids_        (ids)
//{}
//
//LoadProjectDataTask::LoadProjectDataTask(HWND &error_hwnd)
//	:ErrorHandler(error_hwnd)
//{
//	ids_.set();
//}
//
//void LoadProjectDataTask::operator() (TaskData &data)
//{
//	// preconditions
//	_ASSERTE(NULL == data.hardness_);
//	_ASSERTE(NULL == data.heightmap_);
//	_ASSERTE(NULL == data.script_);
//	_ASSERTE(NULL == data.sky_);
//	_ASSERTE(NULL == data.surface_);
//	_ASSERTE(NULL == data.texture_);
//	_ASSERTE(NULL == data.zero_layer_);
//	// locals
//	TCHAR path[MAX_PATH];
//	// load
//	const TCHAR * const folder_path(data.project_folder_.c_str());
//	if (ids_[RS_ZERO_LAYER])
//	{
//		data.zero_layer_ = new ZeroLayer(data.map_size_, error_hwnd_);
//		data.zero_layer_->Load(PathCombine(path, folder_path, data.file_names_[RS_ZERO_LAYER].c_str()));
//	}
//	if (ids_[RS_HARDNESS])
//	{
//		data.hardness_ = new Hardness(data.map_size_, error_hwnd_);
//		data.hardness_->Load(PathCombine(path, folder_path, data.file_names_[RS_HARDNESS].c_str()));
//	}
//	if (ids_[RS_HEIGHTMAP])
//	{
//		data.heightmap_ = LoadHeightmap(
//			data.map_size_,
//			*this,
//			PathCombine(path, folder_path, data.file_names_[RS_HEIGHTMAP].c_str()),
//			ids_[RS_ZERO_LAYER] ? data.zero_layer_ : NULL,
//			data.map_info_.zero_level_);
//	}
//	if (ids_[RS_SCRIPT])
//	{
//		data.script_ = new Script(error_hwnd_);
//		if (!data.script_->Load(PathCombine(path, folder_path, data.file_names_[RS_SCRIPT].c_str())))
//		{
//			delete data.script_;
//			data.script_ = NULL;
//		}
//	}
//	if (ids_[RS_SKY])
//	{
//		data.sky_ = new Sky(error_hwnd_);
//		data.sky_->Load(PathCombine(path, folder_path, data.file_names_[RS_SKY].c_str()));
//	}
//	if (ids_[RS_SURFACE])
//	{
//		data.surface_ = new Surface(error_hwnd_);
//		data.surface_->Load(PathCombine(path, folder_path, data.file_names_[RS_SURFACE].c_str()));
//	}
//	if (ids_[RS_TEXTURE])
//	{
//		data.texture_ = new Texture(data.map_size_, error_hwnd_);
//		data.texture_->Load(
//			PathCombine(path, folder_path, data.file_names_[RS_TEXTURE].c_str()),
//			data.fast_quantization_);
//	}
//}

//-----------------------------------------
// NotifyResourceCreatedTask implementation
//-----------------------------------------

NotifyResourceCreatedTask::NotifyResourceCreatedTask(Resource id, HWND main_hwnd)
	:id_       (id)
	,main_hwnd_(main_hwnd)
{}

void NotifyResourceCreatedTask::operator() (TaskData &data)
{
	SendResourceCreated(main_hwnd_, id_);
}

//-------------------------------------
// NotifyProjectOpenTask implementation
//-------------------------------------

NotifyProjectOpenTask::NotifyProjectOpenTask(HWND main_hwnd)
	:main_hwnd_(main_hwnd)
{}

void NotifyProjectOpenTask::operator() (TaskData &data)
{
	SendProjectOpen(main_hwnd_);
}

//-----------------------------------------
// NotifyProjectUnpackedTask implementation
//-----------------------------------------

NotifyProjectUnpackedTask::NotifyProjectUnpackedTask(HWND main_hwnd)
	:main_hwnd_(main_hwnd)
{}

void NotifyProjectUnpackedTask::operator() (TaskData &data)
{
	SendProjectUnpacked(main_hwnd_);
}

//-----------------------------
// PackShrubTask implementation
//-----------------------------

PackShrubTask::PackShrubTask(
	bool        custom_hardness,
	bool        custom_sky,
	bool        custom_surface,
	bool        custom_zero_layer,
	bool        use_registration,
	const HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
	,custom_hardness_  (custom_hardness)
	,custom_sky_       (custom_sky)
	,custom_surface_   (custom_surface)
	,custom_zero_layer_(custom_zero_layer)
	,use_registration_ (use_registration)
{}

void PackShrubTask::operator() (TaskData &data)
{
	using namespace Loki;
	// check out hardness
	Hardness  &hardness(data.hardness_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.hardness_, &TaskResource<Hardness>::CheckIn);
	// check out heightmap
	Heightmap &heightmap (data.heightmap_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
	// check out sky
	Sky &sky(data.sky_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.sky_, &TaskResource<Sky>::CheckIn);
	// check out surface
	Surface &surface(data.surface_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.surface_, &TaskResource<Surface>::CheckIn);
	// check out texture
	Texture &texture(data.texture_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.texture_, &TaskResource<Texture>::CheckIn);
	// check out zero layer
	ZeroLayer &zero_layer(data.zero_layer_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.zero_layer_, &TaskResource<ZeroLayer>::CheckIn);
	// calculate the checksum and register the map
	{
		DWORD   checksum(0);
		BYTE   *checksum_data;
		size_t  size;
		// heightmap
		switch (heightmap.bpp_)
		{
		case 8:
			{
				checksum_data = heightmap.data8_;
				size = heightmap.info_.size_.cx * heightmap.info_.size_.cy;
				checksum = CalculateChecksum(checksum_data, size, checksum);
			} break;
		case 16:
			{
				checksum_data = ri_cast<BYTE*>(heightmap.data16_);
				size = heightmap.info_.size_.cx * heightmap.info_.size_.cy * 2;
				checksum = CalculateChecksum(checksum_data, size, checksum);
			} break;
		}
		// texture
		checksum_data = texture.indices_;
		size = texture.info_.size_.cx * texture.info_.size_.cy;
		checksum = CalculateChecksum(checksum_data, size, checksum);
		checksum_data = ri_cast<BYTE*>(texture.palette_);
		size = 0x100 * sizeof(COLORREF);
		checksum = CalculateChecksum(checksum_data, size, checksum);
		// hardness
		if (custom_hardness_)
		{
			checksum_data = hardness.data_;
			size = hardness.info_.size_.cx * hardness.info_.size_.cy;
			checksum = CalculateChecksum(checksum_data, size, checksum);
		}
		// map info
		{
			data.map_info_.GetRawData(&checksum_data, &size);
			checksum = CalculateChecksum(checksum_data, size, checksum);
			delete [] checksum_data;
			// register the map
			if (use_registration_)
				if (!RegisterMap(data.map_name_.c_str(), checksum, *this))
					return;
		}
		// script
		{
			Script &script(data.script_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.script_, &TaskResource<Script>::CheckIn);
			std::stringstream stream;
			stream << script.doc_;
			string str(stream.str());
			checksum_data = ri_cast<BYTE*>(&str[0]);
			size = str.size();
			checksum = CalculateChecksum(checksum_data, size, checksum);
		}
		// sky
		if (custom_sky_)
		{
			checksum_data = ri_cast<BYTE*>(sky.pixels_);
			size = sky.size_.cx * sky.size_.cy * sizeof(COLORREF);
			checksum = CalculateChecksum(checksum_data, size, checksum);
		}
		// surface
		if (custom_surface_)
		{
			checksum_data = surface.indices_;
			size = surface.size_.cx * surface.size_.cy;
			checksum = CalculateChecksum(checksum_data, size, checksum);
			checksum_data = ri_cast<BYTE*>(surface.palette_);
			size = 0x100 * sizeof(COLORREF);
			checksum = CalculateChecksum(checksum_data, size, checksum);
		}
		// zero layer
		if (custom_zero_layer_)
		{
			_ASSERTE(0 == zero_layer.data_.size() % 8);
			checksum_data = ri_cast<BYTE*>(&zero_layer.data_._Myvec[0]);
			size = zero_layer.data_.size() / 8;
			checksum = CalculateChecksum(checksum_data, size, checksum);
		}
	}
	// initialise an XML metadata document
	TiXmlDocument doc;
	TiXmlNode *content_node(doc.InsertEndChild(TiXmlElement("content")));
	// allocate a buffer for the data
	size_t buffer_size;
	{
		// compute the sum of the maximum sizes for each resource used
		vector<size_t> sizes;
		const size_t map_factor (data.map_size_.cx * data.map_size_.cy);
		sizes.push_back(map_factor);                            // heightmap
		sizes.push_back(map_factor + 0x100 * sizeof(COLORREF)); // texture
		sizes.push_back(map_factor / 8);                        // mask
		if (custom_hardness_)
			sizes.push_back(hardness.info_.size_.cx * hardness.info_.size_.cy);
		if (custom_sky_)
			sizes.push_back(sky.size_.cx * sky.size_.cy * sizeof(COLORREF));
		if (custom_surface_)
			sizes.push_back(surface.size_.cx * surface.size_.cy + 0x100 * sizeof(COLORREF));
		if (custom_zero_layer_)
			sizes.push_back(map_factor / 8);
		buffer_size = std::accumulate(sizes.begin(), sizes.end(), 0);
	}
	BYTE *buffer(new BYTE[buffer_size]);
	// add packed data
	{
		BYTE *buffer_iter(buffer);
		vector<bool> mask;
		data.map_info_.Pack(*content_node->InsertEndChild(TiXmlElement("map_info")));
		{
			Script &script(data.script_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.script_, &TaskResource<Script>::CheckIn);
			script.Pack(*content_node->InsertEndChild(TiXmlElement("script")));
		}
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
		if (custom_hardness_)
			buffer_iter += hardness.Pack(
				*content_node->InsertEndChild(TiXmlElement("hardness")),
				buffer_iter,
				buffer,
				mask);
		if (custom_sky_)
			buffer_iter += sky.Pack(
				*content_node->InsertEndChild(TiXmlElement("sky")),
				buffer_iter,
				buffer);
		if (custom_surface_)
			buffer_iter += surface.Pack(
				*content_node->InsertEndChild(TiXmlElement("surface")),
				buffer_iter,
				buffer);
		if (custom_zero_layer_)
			buffer_iter += zero_layer.Pack(
				*content_node->InsertEndChild(TiXmlElement("zero_layer")),
				buffer_iter,
				buffer);
		// calculate how much of the buffer was used
		buffer_size = static_cast<size_t>(buffer_iter - buffer);
	}
	// augment the buffer with metadata
	{
		{
			std::ofstream dbg_file("shrub xml.xml");
			dbg_file << doc;
		}
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
	PathCombine(path, data.project_folder_.c_str(), data.map_name_.c_str());
	PathAddExtension(path, _T(".shrub"));
	SaveMemToFile(path, compressed_buffer, compressed_buffer_size, *this);
	delete [] compressed_buffer;
	// reassure the user
	MessageBox(
		error_hwnd_,
		_T("The map has been successfully packed, and saved to the project folder."),
		_T("Success"),
		MB_ICONINFORMATION | MB_OK); // HACK: error handle used for messaging
}

//-----------------------------
// SaveThumbTask implementation
//-----------------------------

SaveThumbTask::SaveThumbTask(const HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
{}

void SaveThumbTask::operator() (TaskData &data)
{
	using namespace Loki;
	// check out heightmap
	Heightmap &heightmap (data.heightmap_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
	// check out texture
	Texture &texture(data.texture_.CheckOut());
	LOKI_ON_BLOCK_EXIT_OBJ(data.texture_, &TaskResource<Texture>::CheckIn);
	// create the lightmap
	Lightmap lightmap(error_hwnd_);
	lightmap.Create(heightmap);
	TCHAR path[MAX_PATH];
	PathCombine(path, data.project_folder_.c_str(), _T("thumbnail.png"));
	SIZE size = { 256, 256 };
	SaveThumb(heightmap, lightmap, texture, path, size, *this);
}

//---------------------------------------------
// SetResourceManagerEnabledTask implementation
//---------------------------------------------

SetResourceManagerEnabledTask::SetResourceManagerEnabledTask(bool enable)
	:enable_(enable)
{}

void SetResourceManagerEnabledTask::operator() (TaskData &data)
{
	data.SetResourceManagerEnabled(enable_);
}

//------------------------------
// UnpackShrubTask implemenation
//------------------------------

UnpackShrubTask::UnpackShrubTask(
		TiXmlDocument *document,
		BYTE          *buffer,
		BYTE          *initial_offset,
		const HWND    &error_hwnd)
	:ErrorHandler(error_hwnd)
	,buffer_        (buffer)
	,initial_offset_(initial_offset)
	,document_      (document)
{}

UnpackShrubTask::~UnpackShrubTask()
{
	delete document_;
	delete [] initial_offset_;
}

void UnpackShrubTask::operator() (TaskData &data)
{
	using namespace Loki;
	TiXmlElement *content_node(document_->FirstChildElement("content"));
	if (NULL == content_node)
		return;
	// unpack map info
	data.map_info_.Unpack(*content_node->FirstChildElement("map_info"));
	// unpack the heightmap
	vector<bool> mask;
	{
		Heightmap &heightmap (data.heightmap_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
		heightmap.Unpack(
			content_node->FirstChildElement("heightmap"),
			buffer_,
			mask);
	}
	// unpack the texture
	{
		Texture &texture(data.texture_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.texture_, &TaskResource<Texture>::CheckIn);
		texture.Unpack(content_node->FirstChildElement("texture"), buffer_);
	}
	// unpack the hardness map
	{
		TiXmlElement *node(content_node->FirstChildElement("hardness"));
		if (NULL != node)
		{
		Hardness  &hardness(data.hardness_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.hardness_, &TaskResource<Hardness>::CheckIn);
			hardness.Unpack(node, buffer_, mask);
		}
	}
	// unpack the script
	{
		TiXmlElement *node(content_node->FirstChildElement("script"));
		if (NULL != node)
		{
			Script &script(data.script_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.script_, &TaskResource<Script>::CheckIn);
			script.Unpack(*node);
		}
	}
	// unpack the sky texture
	{
		TiXmlElement *node(content_node->FirstChildElement("sky"));
		if (NULL != node)
		{
			Sky &sky(data.sky_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.sky_, &TaskResource<Sky>::CheckIn);
			sky.Unpack(node, buffer_);
		}
	}
	// unpack the surface texture
	{
		TiXmlElement *node(content_node->FirstChildElement("surface"));
		if (NULL != node)
		{
			Surface &surface(data.surface_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.surface_, &TaskResource<Surface>::CheckIn);
			surface.Unpack(node, buffer_);
		}
	}
	// unpack the zero layer
	{
		TiXmlElement *node(content_node->FirstChildElement("zero_layer"));
		if (NULL != node)
		{
			ZeroLayer &zero_layer(data.zero_layer_.CheckOut());
			LOKI_ON_BLOCK_EXIT_OBJ(data.zero_layer_, &TaskResource<ZeroLayer>::CheckIn);
			zero_layer.Unpack(node, buffer_);
		}
	}
}

//--------------------------------------------------
// SetProjectDataTask implementation
//--------------------------------------------------

UpdateDataTask::UpdateDataTask(
	LPCTSTR        map_name,
	LPCTSTR        project_folder,
	SIZE           map_size,
	ProjectState   project_state,
	bool           fast_quantization,
	bool           enable_lighting,
	float          mesh_threshold,
	bool           display_hardness,
	bool           display_texture,
	bool           display_zero_layer,
	tstring        file_names[resource_count],
	const MapInfo &map_info)
	:display_hardness_      (display_hardness)
	,display_texture_       (display_texture)
	,display_zero_layer_    (display_zero_layer)
	,enable_lighting_       (enable_lighting)
	,fast_quantization_     (fast_quantization)
	,map_info_              (map_info)
	,map_name_              (map_name)
	,map_size_              (map_size)
	,mesh_threshold_        (mesh_threshold)
	,project_folder_        (project_folder)
	,project_state_         (project_state)
{
	for (uint i(0); i != resource_count; ++i)
		file_names_[i] = file_names[i];
}

void UpdateDataTask::operator() (TaskData &data)
{
	data.display_hardness_       = display_hardness_;
	data.display_texture_        = display_texture_;
	data.display_zero_layer_     = display_zero_layer_;
	data.enable_lighting_        = enable_lighting_;
	data.fast_quantization_      = fast_quantization_;
	data.map_info_               = map_info_;
	data.map_name_               = map_name_;
	data.map_size_               = map_size_;
	data.mesh_threshold_         = mesh_threshold_;
	data.project_folder_         = project_folder_;
	data.project_state_          = project_state_;
	for (uint i(0); i != resource_count; ++i)
		data.file_names_[i] = file_names_[i];
	// set resource info
	vector<TCHAR> path_v(MAX_PATH);
	TCHAR *path(&path_v[0]);
	// hardness
	{
		Hardness::info_t &info(data.hardness_.GetInfo());
		// path
		PathCombine(path, data.project_folder_.c_str(), data.file_names_[RS_HARDNESS].c_str());
		info.path_ = path;
		// size
		info.size_ = data.map_size_;
	}
	// heightmap
	{
		Heightmap::info_t &info(data.heightmap_.GetInfo());
		// path
		PathCombine(path, data.project_folder_.c_str(), data.file_names_[RS_HEIGHTMAP].c_str());
		info.path_ = path;
		// size
		info.size_ = data.map_size_;
		// zero level
		info.zero_level_ = data.map_info_.zero_level_;
	}
	// script
	{
		Script::info_t &info(data.script_.GetInfo());
		// path
		PathCombine(path, data.project_folder_.c_str(), data.file_names_[RS_SCRIPT].c_str());
		info.path_ = path;
	}
	// sky
	{
		Sky::info_t &info(data.sky_.GetInfo());
		// path
		PathCombine(path, data.project_folder_.c_str(), data.file_names_[RS_SKY].c_str());
		info.path_ = path;
	}
	// surface
	{
		Surface::info_t &info(data.surface_.GetInfo());
		// path
		PathCombine(path, data.project_folder_.c_str(), data.file_names_[RS_SURFACE].c_str());
		info.path_ = path;
	}
	// texture
	{
		Texture::info_t &info(data.texture_.GetInfo());
		// fast quantization
		info.fast_quantization_ = data.fast_quantization_;
		// path
		PathCombine(path, data.project_folder_.c_str(), data.file_names_[RS_TEXTURE].c_str());
		info.path_ = path;
		// size
		info.size_ = data.map_size_;
	}
	// zero layer
	{
		ZeroLayer::info_t &info(data.zero_layer_.GetInfo());
		// path
		PathCombine(path, data.project_folder_.c_str(), data.file_names_[RS_ZERO_LAYER].c_str());
		info.path_ = path;
		// size
		info.size_ = data.map_size_;
	}
}

//-------------------------------
// UpdatePanelTask implementation
//-------------------------------

UpdatePanelTask::UpdatePanelTask(
	IdsType         ids,
	PanelWindow    &wnd,
	ProjectManager &project_manager,
	update_t        update,
	const HWND     &error_hwnd)
	:ErrorHandler(error_hwnd)
	,ids_            (ids)
	,wnd_            (wnd)
	,project_manager_(project_manager)
	,update_         (update)
{}

void UpdatePanelTask::operator() (TaskData &data)
{
	if (wnd_.IsVisible())
		UpdatePanel(ids_, wnd_, data);
	else
		Enqueue();
}

void UpdatePanelTask::Enqueue()
{
	OnPanelVisible &on_panel_visible(GetOnPanelVisible());
	delegate_t   delegate(&on_panel_visible, &OnPanelVisible::operator());
	connection_t connection(wnd_.on_show_ += delegate);
	on_panel_visible.Set(
		ids_,
		&project_manager_,
		update_,
		connection);
}

//-----------------------------------------------
// UpdatePanelTask::OnPanelVisible implementation
//-----------------------------------------------

UpdatePanelTask::OnPanelVisible::OnPanelVisible()
	:project_manager_(NULL)
{}

void UpdatePanelTask::OnPanelVisible::Set(
	IdsType         ids,
	ProjectManager *project_manager,
	update_t        update,
	connection_t    connection)
{
	ids_ |= ids;
	project_manager_ = project_manager;
	update_          = update;
	connection_      = connection;
}

void UpdatePanelTask::OnPanelVisible::operator() ()
{
	(project_manager_->*update_)(ids_);
	ids_.reset();
	connection_.disconnect();
}

//---------------------------------
// UpdateInfoWndTask implementation
//---------------------------------

UpdateInfoWndTask::OnPanelVisible UpdateInfoWndTask::on_panel_visible_;

UpdateInfoWndTask::UpdateInfoWndTask(
	IdsType         ids,
	InfoWnd        &stat_wnd,
	ProjectManager &project_manager,
	const HWND     &error_hwnd)
	:UpdatePanelTask(ids, stat_wnd, project_manager, &ProjectManager::UpdateStatWnd, error_hwnd)
{}

void UpdateInfoWndTask::UpdatePanel(
	IdsType      ids,
	PanelWindow &wnd,
	TaskData    &data)
{}

UpdateInfoWndTask::OnPanelVisible& UpdateInfoWndTask::GetOnPanelVisible()
{
	return on_panel_visible_;
}

//------------------------------------
// UpdatePreviewWndTask implementation
//------------------------------------

UpdatePreviewWndTask::OnPanelVisible UpdatePreviewWndTask::on_panel_visible_;

UpdatePreviewWndTask::UpdatePreviewWndTask(
	IdsType         ids,
	PreviewWnd     &stat_wnd,
	ProjectManager &project_manager,
	const HWND     &error_hwnd)
	:UpdatePanelTask(ids, stat_wnd, project_manager, &ProjectManager::UpdateStatWnd, error_hwnd)
{}

void UpdatePreviewWndTask::UpdatePanel(
	IdsType      ids,
	PanelWindow &wnd,
	TaskData    &data)
{
	using namespace Loki;
	PreviewWnd &preview_wnd(ri_cast<PreviewWnd&>(wnd));
	// create the lightmap, if necessary
	Lightmap lightmap(error_hwnd_);
	if (data.enable_lighting_)
	{
		Heightmap &heightmap (data.heightmap_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
		lightmap.Create(heightmap);
	}
	// update the heightmap
	if (ids[RS_HEIGHTMAP])
	{
		Heightmap &heightmap (data.heightmap_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
		vector<SimpleVertex> vertices;
		Triangulate(heightmap, vertices, data.mesh_threshold_);
		SendSetTerrain(preview_wnd.hwnd_, vertices);
	}
	// update the texture
	if (data.display_texture_)
	{
		if (ids[RS_TEXTURE])
		{
			TextureAllocation *allocation(SendTextureAllocate(preview_wnd.hwnd_));
			if (NULL != allocation)
			{
				Texture &texture(data.texture_.CheckOut());
				LOKI_ON_BLOCK_EXIT_OBJ(data.texture_, &TaskResource<Texture>::CheckIn);
				CreateTextures(texture, *allocation, lightmap, data.enable_lighting_);
				SendTextureCommit(preview_wnd.hwnd_);
				delete allocation;
			}
		}
	}
	else if (data.display_hardness_)
	{
		if (ids[RS_HARDNESS])
		{
			TextureAllocation *allocation(SendTextureAllocate(preview_wnd.hwnd_));
			if (NULL != allocation)
			{
				Hardness  &hardness(data.hardness_.CheckOut());
				LOKI_ON_BLOCK_EXIT_OBJ(data.hardness_, &TaskResource<Hardness>::CheckIn);
				CreateTextures(hardness, *allocation, lightmap, data.enable_lighting_);
				SendTextureCommit(preview_wnd.hwnd_);
				delete allocation;
			}
		}
	}
	else if (data.display_zero_layer_)
	{
		if (ids[RS_ZERO_LAYER])
		{
			TextureAllocation *allocation(SendTextureAllocate(preview_wnd.hwnd_));
			if (NULL != allocation)
			{
				ZeroLayer &zero_layer(data.zero_layer_.CheckOut());
				LOKI_ON_BLOCK_EXIT_OBJ(data.zero_layer_, &TaskResource<ZeroLayer>::CheckIn);
				CreateTextures(zero_layer, *allocation, lightmap, data.enable_lighting_);
				SendTextureCommit(preview_wnd.hwnd_);
				delete allocation;
			}
		}
	}
}

UpdatePreviewWndTask::OnPanelVisible& UpdatePreviewWndTask::GetOnPanelVisible()
{
	return on_panel_visible_;
}

//---------------------------------
// UpdateStatWndTask implementation
//---------------------------------

UpdateStatWndTask::OnPanelVisible UpdateStatWndTask::on_panel_visible_;

UpdateStatWndTask::UpdateStatWndTask(
	IdsType         ids,
	StatWnd        &stat_wnd,
	ProjectManager &project_manager,
	const HWND     &error_hwnd)
	:UpdatePanelTask(ids, stat_wnd, project_manager, &ProjectManager::UpdateStatWnd, error_hwnd)
{}

void UpdateStatWndTask::UpdatePanel(
	IdsType      ids,
	PanelWindow &wnd,
	TaskData    &data)
{
	using namespace Loki;
	StatWnd &stat_wnd(ri_cast<StatWnd&>(wnd));
	if (ids[RS_HEIGHTMAP])
	{
		Heightmap &heightmap (data.heightmap_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
		stat_wnd.SetAverageHeight(AverageHeight(heightmap));
	}
	if (ids[RS_TEXTURE])
	{
		Heightmap &heightmap (data.heightmap_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.heightmap_, &TaskResource<Heightmap>::CheckIn);
		Texture &texture(data.texture_.CheckOut());
		LOKI_ON_BLOCK_EXIT_OBJ(data.texture_, &TaskResource<Texture>::CheckIn);
		stat_wnd.SetAverageColour(AverageColour(texture, heightmap));
	}
}

UpdateStatWndTask::OnPanelVisible& UpdateStatWndTask::GetOnPanelVisible()
{
	return on_panel_visible_;
}
