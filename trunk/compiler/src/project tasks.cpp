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

#include "../resource.h"
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
	:hardness_  (NULL)
	,heightmap_ (NULL)
	,script_    (NULL)
	,sky_       (NULL)
	,surface_   (NULL)
	,texture_   (NULL)
	,zero_layer_(NULL)
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
	const TCHAR * const folder_path(task_data_.project_folder_.c_str());
	switch (id_)
	{
	case RS_HARDNESS:
		PathCombine(path, folder_path, _T("hardness.bmp"));
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(path))
		{
			Hardness hardness(task_data_.map_size_, error_hwnd_);
			hardness.MakeDefault();
			hardness.Save(path);
		}
		break;
	case RS_ZERO_LAYER:
		PathCombine(path, folder_path, _T("zero layer.bmp"));
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(path))
		{
			ZeroLayer zero_layer(task_data_.map_size_, error_hwnd_);
			zero_layer.MakeDefault();
			zero_layer.Save(path);
		}
		break;
	case RS_SKY:
		PathCombine(path, folder_path, _T("sky.bmp"));
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(path))
		{
			Sky sky(error_hwnd_);
			sky.MakeDefault();
			sky.Save(path);
		}
		break;
	case RS_SURFACE:
		PathCombine(path, folder_path, _T("surface.bmp"));
		if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(path))
		{
			Surface surface(error_hwnd_);
			surface.MakeDefault();
			surface.Save(path);
		}
		break;
	}
}

//-----------------------------------
// FreeProjectDataTask implementation
//-----------------------------------

void FreeProjectDataTask::operator() ()
{
	delete task_data_.hardness_;
	delete task_data_.heightmap_;
	delete task_data_.script_;
	delete task_data_.sky_;
	delete task_data_.surface_;
	delete task_data_.texture_;
	delete task_data_.zero_layer_;
	task_data_.hardness_   = NULL;
	task_data_.heightmap_  = NULL;
	task_data_.script_     = NULL;
	task_data_.sky_        = NULL;
	task_data_.surface_    = NULL;
	task_data_.texture_    = NULL;
	task_data_.zero_layer_ = NULL;
}

//--------------------------------
// ImportScriptTask implementation
//--------------------------------

ImportScriptTask::ImportScriptTask(LPCTSTR script_path, LPCTSTR xml_path)
	:script_(script_path)
	,xml_   (xml_path)
{}

void ImportScriptTask::operator() ()
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
	HWND &hwnd,
	LPCTSTR install_path,
	uint version,
	bool rename_to_unregistered)
	:ErrorHandler           (hwnd)
	,hwnd_                  (hwnd)
	,install_path_          (install_path)
	,version_               (version)
	,rename_to_unregistered_(rename_to_unregistered)
{}

void InstallMapTask::operator() ()
{
	_ASSERTE(task_data_.heightmap_ != NULL);
	_ASSERTE(task_data_.texture_   != NULL);
	Heightmap &heightmap (*task_data_.heightmap_);
	Texture   &texture   (*task_data_.texture_);
	TCHAR      str        [MAX_PATH];
	TCHAR      folder_path[MAX_PATH];
	bool       overwriting(false);
	// get the name to use for stuff that needs a name
	// this is the map name for a registered map, and "UNREGISTERED" otherwise
	// ASSUME shrubs are registered
	tstring folder_name(
		(PS_SHRUB == task_data_.project_state_ || !rename_to_unregistered_)
		? task_data_.map_name_
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
				if (PS_SHRUB == task_data_.project_state_ && IDCANCEL == MessageBox(
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
	SaveVMP(heightmap, texture, task_data_.zero_layer_, str, *this);
	// create and save inDam.act
	PathCombine(str, folder_path, _T("inDam.act"));
	SavePalette(texture, str, *this);
	// append Texts.btdb
	{
		tstring language;
		// get the language of the distribution
		{
			PathCombine(str, install_path_.c_str(), "Perimeter.ini");
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
		PathCombine(str, install_path_.c_str(), "RESOURCE\\LocData");
		PathCombine(str, str, language.c_str());
		PathCombine(str, str, "Text\\Texts.btdb");
		AppendBTDB(str, folder_name.c_str());
	}
	// append WORLDS.PRM
	PathCombine(str, install_path_.c_str(), "RESOURCE\\Worlds\\WORLDS.PRM");
	AppendWorldsPrm(str, folder_name.c_str(), version_);
	// save up.tga
	PathCombine(str, folder_path, _T("up.tga"));
	if (NULL != task_data_.sky_)
		task_data_.sky_->Save(str);
	else
	{
		Sky sky(error_hwnd_);
		sky.MakeDefault();
		sky.Save(str);
	}
	// leveledSurfaceTexture.tga
	PathCombine(str, folder_path, _T("leveledSurfaceTexture.tga"));
	if (NULL != task_data_.sky_)
		task_data_.surface_->Save(str);
	else
	{
		Surface surface(error_hwnd_);
		surface.MakeDefault();
		surface.Save(str);
	}
	// create and save hardness.bin
	if (NULL != task_data_.hardness_)
	{
		PathCombine(str, folder_path, _T("hardness.bin"));
		SaveHardness(str, task_data_.hardness_->data_, task_data_.map_size_, *this);
	}
	// generate mission files
	switch (version_)
	{
	case 1:
		PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission(folder_path, folder_name.c_str(), false);
		break;
	case 2:
		PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission2(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Multiplayer"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission2(folder_path, folder_name.c_str(), false);
		PathCombine(folder_path, install_path_.c_str(), _T("RESOURCE\\Battle\\SURVIVAL"));
		PathCombine(folder_path, folder_path, folder_name.c_str()); // WARN: not compatible with Unicode
		SaveMission2(folder_path, folder_name.c_str(), false);
		break;
	default:
		DebugBreak();
	}
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
	if (NULL != task_data_.script_)
	{
		std::ofstream spg_file(str);
		ScriptCreator script_creator(spg_file);
		if (!script_creator.Create(task_data_.script_->doc_))
			SaveSPG(task_data_.map_info_, str, folder_name, survival, *this);
	}
	else
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
	if (NULL != task_data_.script_)
	{
		std::ofstream spg_file(str);
		ScriptCreator script_creator(spg_file);
		if (!script_creator.Create(task_data_.script_->doc_))
			SaveSPG2(task_data_.map_info_, str, folder_name, survival, *this);
	}
	else
		SaveSPG2(task_data_.map_info_, str, folder_name, survival, *this);
}

//-----------------------------------
// LoadProjectDataTask implementation
//-----------------------------------

LoadProjectDataTask::LoadProjectDataTask(const IdsType  &ids, HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
	,ids_        (ids)
{}

LoadProjectDataTask::LoadProjectDataTask(HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
{
	ids_.set();
}

void LoadProjectDataTask::operator() ()
{
	// preconditions
	_ASSERTE(NULL == task_data_.hardness_);
	_ASSERTE(NULL == task_data_.heightmap_);
	_ASSERTE(NULL == task_data_.script_);
	_ASSERTE(NULL == task_data_.sky_);
	_ASSERTE(NULL == task_data_.surface_);
	_ASSERTE(NULL == task_data_.texture_);
	_ASSERTE(NULL == task_data_.zero_layer_);
	// locals
	TCHAR path[MAX_PATH];
	// load
	const TCHAR * const folder_path(task_data_.project_folder_.c_str());
	if (ids_[RS_ZERO_LAYER])
	{
		task_data_.zero_layer_ = new ZeroLayer(task_data_.map_size_, error_hwnd_);
		task_data_.zero_layer_->Load(PathCombine(path, folder_path, task_data_.file_names_[RS_ZERO_LAYER].c_str()));
	}
	if (ids_[RS_HARDNESS])
	{
		task_data_.hardness_ = new Hardness(task_data_.map_size_, error_hwnd_);
		task_data_.hardness_->Load(PathCombine(path, folder_path, task_data_.file_names_[RS_HARDNESS].c_str()));
	}
	if (ids_[RS_HEIGHTMAP])
	{
		task_data_.heightmap_ = LoadHeightmap(
			task_data_.map_size_,
			*this,
			PathCombine(path, folder_path, task_data_.file_names_[RS_HEIGHTMAP].c_str()),
			ids_[RS_ZERO_LAYER] ? task_data_.zero_layer_ : NULL,
			task_data_.map_info_.zero_level_);
	}
	if (ids_[RS_SCRIPT])
	{
		task_data_.script_ = new Script(error_hwnd_);
		if (!task_data_.script_->Load(PathCombine(path, folder_path, task_data_.file_names_[RS_SCRIPT].c_str())))
		{
			delete task_data_.script_;
			task_data_.script_ = NULL;
		}
	}
	if (ids_[RS_SKY])
	{
		task_data_.sky_ = new Sky(error_hwnd_);
		task_data_.sky_->Load(PathCombine(path, folder_path, task_data_.file_names_[RS_SKY].c_str()));
	}
	if (ids_[RS_SURFACE])
	{
		task_data_.surface_ = new Surface(error_hwnd_);
		task_data_.surface_->Load(PathCombine(path, folder_path, task_data_.file_names_[RS_SURFACE].c_str()));
	}
	if (ids_[RS_TEXTURE])
	{
		task_data_.texture_ = new Texture(task_data_.map_size_, error_hwnd_);
		task_data_.texture_->Load(
			PathCombine(path, folder_path, task_data_.file_names_[RS_TEXTURE].c_str()),
			task_data_.fast_quantization_);
	}
}

//-----------------------------------------
// NotifyResourceCreatedTask implementation
//-----------------------------------------

NotifyResourceCreatedTask::NotifyResourceCreatedTask(Resource id, HWND main_hwnd)
	:id_       (id)
	,main_hwnd_(main_hwnd)
{}

void NotifyResourceCreatedTask::operator() ()
{
	SendResourceCreated(main_hwnd_, id_);
}

//-------------------------------------
// NotifyProjectOpenTask implementation
//-------------------------------------

NotifyProjectOpenTask::NotifyProjectOpenTask(HWND main_hwnd)
	:main_hwnd_(main_hwnd)
{}

void NotifyProjectOpenTask::operator() ()
{
	SendProjectOpen(main_hwnd_);
}

//-----------------------------------------
// NotifyProjectUnpackedTask implementation
//-----------------------------------------

NotifyProjectUnpackedTask::NotifyProjectUnpackedTask(HWND main_hwnd)
	:main_hwnd_(main_hwnd)
{}

void NotifyProjectUnpackedTask::operator() ()
{
	SendProjectUnpacked(main_hwnd_);
}

//-----------------------------
// PackShrubTask implementation
//-----------------------------

PackShrubTask::PackShrubTask(
	bool custom_hardness,
	bool custom_sky,
	bool custom_surface,
	bool custom_zero_layer,
	bool use_registration,
	HWND &error_hwnd)
	:ErrorHandler(error_hwnd)
	,custom_hardness_  (custom_hardness)
	,custom_sky_       (custom_sky)
	,custom_surface_   (custom_surface)
	,custom_zero_layer_(custom_zero_layer)
	,use_registration_ (use_registration)
{}

void PackShrubTask::operator() ()
{
	_ASSERTE(NULL != task_data_.heightmap_);
	_ASSERTE(NULL != task_data_.texture_);
	_ASSERTE(NULL != task_data_.hardness_);
	_ASSERTE(NULL != task_data_.sky_);
	_ASSERTE(NULL != task_data_.surface_);
	_ASSERTE(NULL != task_data_.zero_layer_);
	Hardness  &hardness  (*task_data_.hardness_);
	Heightmap &heightmap (*task_data_.heightmap_);
	Sky       &sky       (*task_data_.sky_);
	Surface   &surface   (*task_data_.surface_);
	Texture   &texture   (*task_data_.texture_);
	ZeroLayer &zero_layer(*task_data_.zero_layer_);
	// calculate the checksum and register the map
	{
		DWORD   checksum(0);
		BYTE   *data;
		size_t  size;
		// heightmap
		switch (heightmap.GetBpp())
		{
		case 8:
			{
				const Heightmap8 &heightmap8(ri_cast<const Heightmap8&>(heightmap));
				data = heightmap8.data_;
				size = heightmap8.size_.cx * heightmap8.size_.cy;
				checksum = CalculateChecksum(data, size, checksum);
			} break;
		case 16:
			{
				const Heightmap16 &heightmap16(ri_cast<const Heightmap16&>(heightmap));
				data = ri_cast<BYTE*>(heightmap16.data_);
				size = heightmap16.size_.cx * heightmap16.size_.cy * 2;
				checksum = CalculateChecksum(data, size, checksum);
			} break;
		}
		// texture
		data = texture.indices_;
		size = texture.size_.cx * texture.size_.cy;
		checksum = CalculateChecksum(data, size, checksum);
		data = ri_cast<BYTE*>(texture.palette_);
		size = 0x100 * sizeof(COLORREF);
		checksum = CalculateChecksum(data, size, checksum);
		// hardness
		if (custom_hardness_)
		{
			data = hardness.data_;
			size = hardness.size_.cx * hardness.size_.cy;
			checksum = CalculateChecksum(data, size, checksum);
		}
		// map info
		{
			task_data_.map_info_.GetRawData(&data, &size);
			checksum = CalculateChecksum(data, size, checksum);
			delete [] data;
			// register the map
			if (use_registration_)
				if (!RegisterMap(task_data_.map_name_.c_str(), checksum, *this))
					return;
		}
		// script
		if (NULL != task_data_.script_)
		{
			Script &script(*task_data_.script_);
			std::stringstream stream;
			stream << script.doc_;
			string str(stream.str());
			data = ri_cast<BYTE*>(&str[0]);
			size = str.size();
			checksum = CalculateChecksum(data, size, checksum);
		}
		// sky
		if (custom_sky_)
		{
			data = ri_cast<BYTE*>(sky.pixels_);
			size = sky.size_.cx * sky.size_.cy * sizeof(COLORREF);
			checksum = CalculateChecksum(data, size, checksum);
		}
		// surface
		if (custom_surface_)
		{
			data = surface.indices_;
			size = surface.size_.cx * surface.size_.cy;
			checksum = CalculateChecksum(data, size, checksum);
			data = ri_cast<BYTE*>(surface.palette_);
			size = 0x100 * sizeof(COLORREF);
			checksum = CalculateChecksum(data, size, checksum);
		}
		// zero layer
		if (custom_zero_layer_)
		{
			_ASSERTE(0 == zero_layer.data_.size() % 8);
			data = ri_cast<BYTE*>(&zero_layer.data_._Myvec[0]);
			size = zero_layer.data_.size() / 8;
			checksum = CalculateChecksum(data, size, checksum);
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
		const size_t map_factor (task_data_.map_size_.cx * task_data_.map_size_.cy);
		sizes.push_back(map_factor);                            // heightmap
		sizes.push_back(map_factor + 0x100 * sizeof(COLORREF)); // texture
		sizes.push_back(map_factor / 8);                        // mask
		if (custom_hardness_)
			sizes.push_back(hardness.size_.cx * hardness.size_.cy);
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
		task_data_.map_info_.Pack(*content_node->InsertEndChild(TiXmlElement("map_info")));
		if (NULL != task_data_.script_)
			task_data_.script_->Pack(*content_node->InsertEndChild(TiXmlElement("script")));
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
	PathCombine(path, task_data_.project_folder_.c_str(), task_data_.map_name_.c_str());
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

//------------------------------
// UnpackShrubTask implemenation
//------------------------------

UnpackShrubTask::UnpackShrubTask(
		TiXmlDocument  *document,
		BYTE           *buffer,
		BYTE           *initial_offset,
		InfoWnd        &info_wnd,
		PreviewWnd     &preview_wnd,
		StatWnd        &stat_wnd,
		ProjectManager &project_manager,
		HWND           &error_hwnd)
	:ErrorHandler(error_hwnd)
	,buffer_         (buffer)
	,initial_offset_ (initial_offset)
	,document_       (document)
	,info_wnd_       (info_wnd)
	,preview_wnd_    (preview_wnd)
	,project_manager_(project_manager)
	,stat_wnd_       (stat_wnd)
{}

UnpackShrubTask::~UnpackShrubTask()
{
	delete document_;
	delete [] initial_offset_;
}

void UnpackShrubTask::operator() ()
{
	TiXmlElement *content_node(document_->FirstChildElement("content"));
	if (NULL == content_node)
		return;
	// unpack map info
	task_data_.map_info_.Unpack(*content_node->FirstChildElement("map_info"));
	// unpack the heightmap
	vector<bool> mask;
	{
		_ASSERTE(NULL == task_data_.heightmap_);
		task_data_.heightmap_ = UnpackHeightmap(
			task_data_.map_size_,
			*this,
			content_node->FirstChildElement("heightmap"),
			buffer_,
			mask);
		_ASSERTE(NULL != task_data_.heightmap_);
	}
	// unpack the texture
	{
		_ASSERTE(NULL == task_data_.texture_);
		task_data_.texture_ = new Texture(task_data_.map_size_, error_hwnd_);
		Texture &texture(*task_data_.texture_);
		texture.Unpack(content_node->FirstChildElement("texture"), buffer_);
	}
	// unpack the hardness map
	{
		_ASSERTE(NULL == task_data_.hardness_);
		TiXmlElement *node(content_node->FirstChildElement("hardness"));
		if (NULL != node)
		{
			task_data_.hardness_ = new Hardness(task_data_.map_size_, error_hwnd_);
			Hardness &hardness(*task_data_.hardness_);
			hardness.Unpack(node, buffer_, mask);
		}
	}
	// unpack the script
	{
		_ASSERTE(NULL == task_data_.script_);
		TiXmlElement *node(content_node->FirstChildElement("script"));
		if (NULL != node)
		{
			task_data_.script_ = new Script(error_hwnd_);
			Script &script(*task_data_.script_);
			if (!script.Unpack(*node))
			{
				delete task_data_.script_;
				task_data_.script_ = NULL;
			}
		}
	}
	// unpack the sky texture
	{
		_ASSERTE(NULL == task_data_.sky_);
		TiXmlElement *node(content_node->FirstChildElement("sky"));
		if (NULL != node)
		{
			task_data_.sky_ = new Sky(error_hwnd_);
			Sky &sky(*task_data_.sky_);
			sky.Unpack(node, buffer_);
		}
	}
	// unpack the surface texture
	{
		_ASSERTE(NULL == task_data_.surface_);
		TiXmlElement *node(content_node->FirstChildElement("surface"));
		if (NULL != node)
		{
			task_data_.surface_ = new Surface(error_hwnd_);
			Surface &surface(*task_data_.surface_);
			surface.Unpack(node, buffer_);
		}
	}
	// unpack the zero layer
	{
		_ASSERTE(NULL == task_data_.zero_layer_);
		TiXmlElement *node(content_node->FirstChildElement("zero_layer"));
		if (NULL != node)
		{
			task_data_.zero_layer_ = new ZeroLayer(task_data_.map_size_, error_hwnd_);
			ZeroLayer &zero_layer(*task_data_.zero_layer_);
			zero_layer.Unpack(node, buffer_);
		}
	}
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
	ids.set(RS_SKY,     false);
	ids.set(RS_SURFACE, false);
	project_manager_.ReloadFiles(ids);
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

void UpdateDataTask::operator() ()
{
	task_data_.display_hardness_       = display_hardness_;
	task_data_.display_texture_        = display_texture_;
	task_data_.display_zero_layer_     = display_zero_layer_;
	task_data_.enable_lighting_        = enable_lighting_;
	task_data_.fast_quantization_      = fast_quantization_;
	task_data_.map_info_               = map_info_;
	task_data_.map_name_               = map_name_;
	task_data_.map_size_               = map_size_;
	task_data_.mesh_threshold_         = mesh_threshold_;
	task_data_.project_folder_         = project_folder_;
	task_data_.project_state_          = project_state_;
	for (uint i(0); i != resource_count; ++i)
		task_data_.file_names_[i] = file_names_[i];
}

//--------------------------------
// UpdatePanelsTask implementation
//--------------------------------

UpdatePanelsTask::UpdatePanelsTask(
	InfoWnd        &info_wnd,
	PreviewWnd     &preview_wnd,
	StatWnd        &stat_wnd,
	ProjectManager &project_manager,
	HWND           &error_hwnd)
	:ErrorHandler    (error_hwnd)
	,info_wnd_       (info_wnd)
	,preview_wnd_    (preview_wnd)
	,stat_wnd_       (stat_wnd)
	,project_manager_(project_manager)
{}

void UpdatePanelsTask::operator() ()
{
	// what should be loaded
	IdsType ids(resource_count);
	ids[RS_HARDNESS]   = (NULL != task_data_.hardness_);
	ids[RS_HEIGHTMAP]  = (NULL != task_data_.heightmap_);
	ids[RS_TEXTURE]    = (NULL != task_data_.texture_);
	ids[RS_ZERO_LAYER] = (NULL != task_data_.zero_layer_);
	if (ids.none())
		return;
	// initialize resources
	Hardness  &hardness   (*task_data_.hardness_);
	Heightmap &heightmap  (*task_data_.heightmap_);
	Lightmap   lightmap   (task_data_.map_size_, error_hwnd_);
	Texture   &texture    (*task_data_.texture_);
	ZeroLayer &zero_layer (*task_data_.zero_layer_);
	if (task_data_.enable_lighting_)
		lightmap.Create(heightmap);
	// which panels should be updated
	bool preview_wnd_visible(preview_wnd_.IsVisible());
	bool stat_wnd_visible   (stat_wnd_.IsVisible());
	// stat wnd
	if (stat_wnd_visible)
	{
		if (ids[RS_HEIGHTMAP])
			stat_wnd_.SetAverageHeight(AverageHeight(heightmap));
		if (ids[RS_TEXTURE])
			stat_wnd_.SetAverageColour(AverageColour(texture, heightmap));
	}
	else
		stat_wnd_.SetVisibilityNotification(new OnPanelVisible(project_manager_));
	// preview wnd
	if (preview_wnd_visible)
	{
		// update the heightmap
		if (ids[RS_HEIGHTMAP])
			switch (heightmap.GetBpp())
			{
			case 8:
				{
					vector<SimpleVertex> vertices;
					Triangulate(ri_cast<const Heightmap8&>(heightmap), vertices, task_data_.mesh_threshold_);
					SendSetTerrain(preview_wnd_.hwnd_, vertices);
				} break;
			case 16:
				{
					vector<SimpleVertex> vertices;
					{
						Heightmap8 *heightmap8(ri_cast<Heightmap16&>(heightmap));
						Triangulate(*heightmap8, vertices, task_data_.mesh_threshold_);
						delete heightmap8;
					}
					SendSetTerrain(preview_wnd_.hwnd_, vertices);
				} break;
			}
		// update the texture
		if (task_data_.display_texture_)
		{
			if (ids[RS_TEXTURE])
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
		else if (task_data_.display_hardness_)
		{
			if (ids[RS_HARDNESS])
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
		else if (task_data_.display_zero_layer_)
		{
			if (ids[RS_ZERO_LAYER])
			{
				TextureAllocation *allocation(SendTextureAllocate(preview_wnd_.hwnd_));
				if (NULL != allocation)
				{
					CreateTextures(zero_layer, *allocation, lightmap, task_data_.enable_lighting_);
					SendTextureCommit(preview_wnd_.hwnd_);
					delete allocation;
				}
			}
		}
	}
	else
		preview_wnd_.SetVisibilityNotification(new OnPanelVisible(project_manager_));
}

//-----------------------------------------------
// UpdatePanelsTask::OnPanelVisible implemenation
//-----------------------------------------------

UpdatePanelsTask::OnPanelVisible::OnPanelVisible(ProjectManager &project_manager)
	:project_manager_(project_manager)
{}

void UpdatePanelsTask::OnPanelVisible::operator ()(bool on)
{
	IdsType ids;
	ids.set();
	ids.set(RS_SKY,     false);
	ids.set(RS_SURFACE, false);
	project_manager_.ReloadFiles(ids);
}
