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


#include "StdAfx.h"

#include "app data.h"
#include "main wnd.h"
#include "project data.h"
#include "project manager.h"
#include "task common.h"

//-------------------------------
// project manager implementation
//-------------------------------

ProjectManager::ProjectManager(
	MainWnd    &main_wnd,
	InfoWnd    &info_wnd,
	PreviewWnd &preview_wnd,
	StatWnd    &stat_wnd)
	:ErrorHandler       (main_wnd.hwnd_)
	,processor_thread_  (NULL)
	,info_wnd_          (info_wnd)
	,preview_wnd_       (preview_wnd)
	,project_state_     (PS_INACTIVE)
	,stat_wnd_          (stat_wnd)
	,tracker_           (main_wnd.hwnd_, file_updated_, file_not_found_)
	,file_updated_      (*this)
	,file_not_found_    (main_wnd.hwnd_)
	,zero_level_changed_(*this)
{
	InitializeCriticalSection(&processor_section_);
}

ProjectManager::~ProjectManager()
{
	// stop the tracker
	tracker_.Stop();
	// shut down the processor thread
	if (NULL != processor_thread_)
	{
		{
			AutoCriticalSection acs(&processor_section_);
			stop_processing_ = true;
		}
		ResumeThread(processor_thread_);
		const DWORD timeout(8000); // 8 seconds
		// apply force if necessary
		if (WAIT_TIMEOUT == WaitForSingleObject(processor_thread_, timeout))
		{
			_RPT0(_CRT_WARN, _T("The processing thread has been forcefully terminated./n"));
			TerminateThread(processor_thread_, 1);
		}
	}
	CloseHandle(processor_thread_);
	DeleteCriticalSection(&processor_section_);
	// clean the queue
	while (!tasks_.empty())
	{
		delete tasks_.front();
		tasks_.pop();
	}
	// save project data
	if (PS_PROJECT == project_state_)
	{
		TCHAR folder_path[MAX_PATH];
		PathCombine(folder_path, folder_path_.c_str(), MacroProjectData(ID_MAP_NAME).c_str());
		PathAddExtension(folder_path, _T(".pmproj"));
		SSProjectData::Save(folder_path);
	}
}

bool ProjectManager::Initialize()
{
	// create the processor thread
	stop_processing_ = false;
	DWORD thread_id;
	processor_thread_ = CreateThread(NULL, 0, ProcessorThread, this, 0, &thread_id);
	if (NULL == processor_thread_)
	{
		MacroDisplayError(_T("Background processing thread could not be created."));
		return false;
	}
#ifdef _DEBUG
	if (!SetThreadPriority(processor_thread_, THREAD_PRIORITY_BELOW_NORMAL))
	{
		MacroDisplayError(_T("SetThreadPriority failed\n"));
		return false;
	}
#else
	SetThreadPriority(processor_thread_, THREAD_PRIORITY_BELOW_NORMAL);
#endif
	return true;
}

void ProjectManager::Close()
{
	if (PS_INACTIVE == project_state_)
		return;
	if (PS_PROJECT == project_state_)
		tracker_.Stop();
	if (PS_SHRUB == project_state_)
		AddTask(new FreeProjectDataTask());
	// save project data
	if (PS_PROJECT == project_state_)
	{
		TCHAR folder_path[MAX_PATH];
		PathCombine(folder_path, folder_path_.c_str(), MacroProjectData(ID_MAP_NAME).c_str());
		PathAddExtension(folder_path, _T(".pmproj"));
		SSProjectData::Save(folder_path);
	}
	// set defaults
	MacroProjectData(ID_CUSTOM_HARDNESS)   = false;
	MacroProjectData(ID_CUSTOM_SKY)        = false;
	MacroProjectData(ID_CUSTOM_SURFACE)    = false;
	MacroProjectData(ID_CUSTOM_ZERO_LAYER) = false;
	// change project state indicator
	project_state_ = PS_INACTIVE;
}

void ProjectManager::CreateProject(LPCTSTR folder_path, LPCTSTR map_name, SIZE map_size, HWND main_hwnd)
{
	Close();
	// create the default project file
	MacroProjectData(ID_MAP_NAME)    = map_name;
	MacroProjectData(ID_POWER_X)     = log2(map_size.cx);
	MacroProjectData(ID_POWER_Y)     = log2(map_size.cy);
	MacroProjectData(ID_FOG_START)   = 8192;
	MacroProjectData(ID_FOG_END)     = 8192;
	MacroProjectData(ID_FOG_COLOUR)  = 0L;
	MacroProjectData(ID_SP_0).x      = map_size.cx / 2 - 1;
	MacroProjectData(ID_SP_0).y      = map_size.cy / 2 - 1;
	MacroProjectData(ID_SP_1).x      = map_size.cx / 4 - 1;
	MacroProjectData(ID_SP_1).y      = map_size.cy / 4 - 1;
	MacroProjectData(ID_SP_2).x      = map_size.cx - 1 - MacroProjectData(ID_SP_1).x;
	MacroProjectData(ID_SP_2).y      = MacroProjectData(ID_SP_1).y;
	MacroProjectData(ID_SP_3).x      = MacroProjectData(ID_SP_2).x;
	MacroProjectData(ID_SP_3).y      = map_size.cy - 1 - MacroProjectData(ID_SP_1).y;
	MacroProjectData(ID_SP_4).x      = MacroProjectData(ID_SP_1).x;
	MacroProjectData(ID_SP_4).y      = MacroProjectData(ID_SP_3).y;
	MacroProjectData(ID_CUSTOM_HARDNESS)   = false;
	MacroProjectData(ID_CUSTOM_SKY)        = false;
	MacroProjectData(ID_CUSTOM_SURFACE)    = false;
	MacroProjectData(ID_CUSTOM_ZERO_LAYER) = false;
	TCHAR path[MAX_PATH];
	_tcscpy(path, folder_path);
	PathAddBackslash(path);
	PathCombine(path, path, map_name);
	PathAddExtension(path, _T(".pmproj"));
	SSProjectData::Save(path);
	// open the project
	OpenProject(path, main_hwnd, true);
}

void ProjectManager::OpenProject(LPCTSTR project_path, HWND main_hwnd, bool new_project)
{
	Close();
	project_state_ = PS_PROJECT;
	// load project data
	SSProjectData::Load(project_path);
	SSProjectData::Output();
	// get the folder path
	{
		TCHAR folder_path[MAX_PATH];
		_tcscpy(folder_path, project_path);
		PathRemoveFileSpec(folder_path);
		folder_path_ = folder_path;
	}
	// enqueue the task of updating project data
	{
		LPCTSTR map_name(MacroProjectData(ID_MAP_NAME).c_str());
		SIZE map_size = {
			exp2(MacroProjectData(ID_POWER_X)),
			exp2(MacroProjectData(ID_POWER_Y))
		};
		AddTask(new UpdateDataTask(
			map_name,
			folder_path_.c_str(),
			map_size,
			project_state_,
			MacroAppData(ID_FAST_TEXTURE_QUANTIZATION),
			MacroAppData(ID_ENABLE_LIGHTING),
			MacroAppData(ID_THRESHOLD),
			MacroAppData(ID_DISPLAY_HARDNESS),
			MacroAppData(ID_DISPLAY_TEXTURE),
			MacroAppData(ID_DISPLAY_ZERO_LAYER),
			TaskCommon::MapInfo::LoadFromGlobal()));
	}
	// create default files
	if (new_project)
		AddTask(new CreateDefaultFilesTask(error_hwnd_));
	// update panels
	AddTask(new ChangeProjectTask(info_wnd_, preview_wnd_));
	// initialize the tracker, setting the last write time
	//  in a way that would schedule the files for immediate update
	FILETIME null_last_write;
	ZeroMemory(&null_last_write, sizeof(null_last_write)); // set to January 1, 1601 (UTC)
	tracker_.SetDatum(RS_HEIGHTMAP, _T("heightmap.bmp"), null_last_write);
	tracker_.SetDatum(RS_TEXTURE,   _T("texture.bmp"),   null_last_write);
	if (MacroProjectData(ID_CUSTOM_HARDNESS))
		tracker_.SetDatum(RS_HARDNESS, _T("hardness.bmp"), null_last_write);
	if (MacroProjectData(ID_CUSTOM_ZERO_LAYER))
		tracker_.SetDatum(RS_ZERO_LAYER, _T("zero layer.bmp"), null_last_write);
	//if (MacroProjectData(ID_CUSTOM_SURFACE))
	//	tracker_.SetDatum(RS_SURFACE, _T("surface.bmp"), null_last_write);
	//if (MacroProjectData(ID_CUSTOM_SKY))
	//	tracker_.SetDatum(RS_SKY, _T("sky.bmp"), null_last_write);
	AddTask(new NotifyProjectOpenTask(main_hwnd));
}

void ProjectManager::PackShrub()
{
	if (PS_PROJECT == project_state_)
		AddTask(new LoadProjectDataTask(error_hwnd_));
	AddTask(new PackShrubTask(
		MacroProjectData(ID_CUSTOM_HARDNESS),
		MacroProjectData(ID_CUSTOM_SKY),
		MacroProjectData(ID_CUSTOM_SURFACE),
		MacroProjectData(ID_CUSTOM_ZERO_LAYER),
		error_hwnd_));
	if (PS_PROJECT == project_state_)
		AddTask(new FreeProjectDataTask());
}

#define MacroXmlToPos(num)                                                                        \
	text_node = node_handle.FirstChildElement("starting_position_" #num "_x").FirstChild().Text(); \
	if (NULL != text_node)                                                                         \
		MacroProjectData(ID_SP_##num).x = atoi(text_node->Value());                                 \
	text_node = node_handle.FirstChildElement("starting_position_" #num "_y").FirstChild().Text(); \
	if (NULL != text_node)                                                                         \
		MacroProjectData(ID_SP_##num).y = atoi(text_node->Value())

void ProjectManager::UnpackShrub(LPCTSTR shrub_path, HWND main_hwnd)
{
	Close();
	// TODO: move the following into a task
	// get the folder path
	{
		TCHAR folder_path[MAX_PATH];
		_tcscpy(folder_path, shrub_path);
		PathRemoveFileSpec(folder_path);
		folder_path_ = folder_path;
	}
	// load the packed Biboorat from the shrub file
	BYTE *compressed_buffer;
	const size_t file_size(TaskCommon::LoadFile(shrub_path, compressed_buffer, *this));
	// check that the file has an acceptable format
	{
		BYTE word[4];
		// check the magic number
		CopyMemory(word, compressed_buffer, 4);
#ifdef PRE_RELEASE
		if (word[0] != 'S' || word[1] != 'H' || word[2] != 'R' || word[3] != 'B')
#else // PRE_RELEASE
		if (word[0] != 'S' || word[1] != 'H' || word[2] != 'R' || word[3] != 0)
#endif // PRE_RELEASE
		{
			MacroDisplayError("the file is not a valid shrub");
			delete [] compressed_buffer;
			return;
		}
		// check the compression format
		CopyMemory(word, compressed_buffer + 4, 3);
		if (word[0] != 'B' || word[1] != 'Z' || word[2] != '2')
		{
			MacroDisplayError("the shrub has an unfamiliar format");
			delete [] compressed_buffer;
			return;
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
				MacroDisplayError(_T("Insufficient memory for decompression."));
				break;
			case BZ_OUTBUFF_FULL:
				MacroDisplayError(_T("Outbuffer full. The shrub file is corrupt."));
				break;
			case BZ_DATA_ERROR:
				MacroDisplayError(_T("The shrub file is corrupt."));
				break;
			case BZ_DATA_ERROR_MAGIC:
				MacroDisplayError(_T("Magic number mismatch. The shrub file is corrupt."));
				break;
			case BZ_UNEXPECTED_EOF:
				MacroDisplayError(_T("Unexpected end of file during decompression."));
				break;
			default:
				MacroDisplayError(_T("BZ2_bzBuffToBuffDecompress failed"));
			};
			delete [] buffer;
			delete [] compressed_buffer;
			return;
		}
	}
	delete [] compressed_buffer;
	// read in xml information
	TiXmlDocument *doc(new TiXmlDocument);
	size_t xml_string_size(strlen(ri_cast<char*>(buffer)) + 1);
	doc->Parse(ri_cast<char*>(buffer)); // possible buffer overflow
	// get the content node
	TiXmlElement *content_node(doc->FirstChildElement("content"));
	if (NULL == content_node)
	{
		MacroDisplayError("The shrub has an unfamiliar format.");
		delete [] buffer;
		return;
	}
	// set project state
	project_state_ = PS_SHRUB;
	// read custom resource use information
	TiXmlNode *text_node;
	text_node = content_node->FirstChildElement("hardness");
	if (NULL != text_node)
		MacroProjectData(ID_CUSTOM_HARDNESS) = true;
	text_node = content_node->FirstChildElement("zero_layer");
	if (NULL != text_node)
		MacroProjectData(ID_CUSTOM_ZERO_LAYER) = true;
	text_node = content_node->FirstChildElement("sky");
	if (NULL != text_node)
		MacroProjectData(ID_CUSTOM_SKY) = true;
	text_node = content_node->FirstChildElement("surface");
	if (NULL != text_node)
		MacroProjectData(ID_CUSTOM_SURFACE) = true;
	// read in map information
	TiXmlHandle node_handle(content_node->FirstChildElement("map_info"));
	// map name
	text_node = node_handle.FirstChildElement("map_name").FirstChild().Text();
	if (NULL != text_node)
	{
		MacroProjectData(ID_MAP_NAME) = text_node->Value();
		SetWindowText(main_hwnd, MacroProjectData(ID_MAP_NAME).c_str());
	}
	else
	{
		MacroDisplayError(_T("The shrub has an unfamiliar format."));
		delete [] buffer;
		return;
	}
	// power x
	text_node = node_handle.FirstChildElement("map_power_x").FirstChild().Text();
	if (NULL != text_node)
		MacroProjectData(ID_POWER_X) = atoi(text_node->Value());
	else
	{
		MacroDisplayError(_T("The shrub has an unfamiliar format."));
		delete [] buffer;
		return;
	}
	// power y
	text_node = node_handle.FirstChildElement("map_power_y").FirstChild().Text();
	if (NULL != text_node)
		MacroProjectData(ID_POWER_Y) = atoi(text_node->Value());
	else
	{
		MacroDisplayError(_T("The shrub has an unfamiliar format."));
		delete [] buffer;
		return;
	}
	// zero_plast
	text_node = node_handle.FirstChildElement("zero_plast").FirstChild().Text();
	if (NULL != text_node)
		MacroProjectData(ID_ZERO_LEVEL) = atoi(text_node->Value());
	// fog start
	text_node = node_handle.FirstChildElement("fog_start").FirstChild().Text();
	if (NULL != text_node)
		MacroProjectData(ID_FOG_START) = atoi(text_node->Value());
	// fog end
	text_node = node_handle.FirstChildElement("fog_end").FirstChild().Text();
	if (NULL != text_node)
		MacroProjectData(ID_FOG_END) = atoi(text_node->Value());
	// fog colour
	text_node = node_handle.FirstChildElement("fog_colour").FirstChild().Text();
	if (NULL != text_node)
		MacroProjectData(ID_FOG_COLOUR) = atol(text_node->Value());
	// starting positons
	MacroXmlToPos(0);
	MacroXmlToPos(1);
	MacroXmlToPos(2);
	MacroXmlToPos(3);
	MacroXmlToPos(4);
	// update panels
	AddTask(new ChangeProjectTask(info_wnd_, preview_wnd_, true));
	// enqueue the task of updating project data
	{
		LPCTSTR map_name(MacroProjectData(ID_MAP_NAME).c_str());
		SIZE map_size = {
			exp2(MacroProjectData(ID_POWER_X)),
			exp2(MacroProjectData(ID_POWER_Y))
		};
		AddTask(new UpdateDataTask(
			map_name,
			folder_path_.c_str(),
			map_size,
			project_state_,
			MacroAppData(ID_FAST_TEXTURE_QUANTIZATION),
			MacroAppData(ID_ENABLE_LIGHTING),
			MacroAppData(ID_THRESHOLD),
			MacroAppData(ID_DISPLAY_HARDNESS),
			MacroAppData(ID_DISPLAY_TEXTURE),
			MacroAppData(ID_DISPLAY_ZERO_LAYER),
			TaskCommon::MapInfo::LoadFromGlobal()));
	}
	// enqueue the task of unpacking the rest of the data
	AddTask(new UnpackShrubTask(
		doc,
		buffer + xml_string_size,
		buffer,
		info_wnd_,
		preview_wnd_,
		stat_wnd_,
		*this,
		error_hwnd_));
	AddTask(new UpdatePanelsTask(
		info_wnd_,
		preview_wnd_,
		stat_wnd_,
		*this,
		error_hwnd_));
}

void ProjectManager::OnProjectOpen(HWND main_hwnd)
{
	SetWindowText(main_hwnd, MacroProjectData(ID_MAP_NAME).c_str());
	tracker_.Start(folder_path_.c_str(), &file_updated_);
}

void ProjectManager::OnProjectUnpacked(HWND main_hwnd)
{
}

void ProjectManager::OnResourceNotFound(Resource id)
{
	switch (id)
	{
	case RS_HARDNESS:
		tracker_.EnableDatum(id, false);
		MacroProjectData(ID_CUSTOM_HARDNESS) = false;
		break;
	case RS_ZERO_LAYER:
		tracker_.EnableDatum(id, false);
		MacroProjectData(ID_CUSTOM_ZERO_LAYER) = false;
	// TODO: complete
	}
}

void ProjectManager::InstallMap()
{
	if (PS_PROJECT == project_state_)
		AddTask(new LoadProjectDataTask(error_hwnd_));
	AddTask(new InstallMapTask(error_hwnd_, MacroAppData(ID_PERIMETER_PATH)));
	if (PS_PROJECT == project_state_)
		AddTask(new FreeProjectDataTask());
}

void ProjectManager::SaveThumbnail()
{
	IdsType ids(resource_count);
	ids[RS_HEIGHTMAP] = true;
	ids[RS_TEXTURE]   = true;
	if (PS_PROJECT == project_state_)
		AddTask(new LoadProjectDataTask(ids, error_hwnd_));
	AddTask(new SaveThumbTask(error_hwnd_));
	if (PS_PROJECT == project_state_)
		AddTask(new FreeProjectDataTask());
}

void ProjectManager::ReloadFiles(const IdsType &ids)
{
	if (ids.none())
		return;
	LPCTSTR map_name(MacroProjectData(ID_MAP_NAME).c_str());
	SIZE map_size = {
		exp2(MacroProjectData(ID_POWER_X)),
		exp2(MacroProjectData(ID_POWER_Y))
	};
	AddTask(new UpdateDataTask(
		map_name,
		folder_path_.c_str(),
		map_size,
		project_state_,
		MacroAppData(ID_FAST_TEXTURE_QUANTIZATION),
		MacroAppData(ID_ENABLE_LIGHTING),
		MacroAppData(ID_THRESHOLD),
		MacroAppData(ID_DISPLAY_HARDNESS),
		MacroAppData(ID_DISPLAY_TEXTURE),
		MacroAppData(ID_DISPLAY_ZERO_LAYER),
		TaskCommon::MapInfo::LoadFromGlobal()));
	if (project_state_ == PS_PROJECT)
		AddTask(new LoadProjectDataTask(ids, error_hwnd_));
	AddTask(new UpdatePanelsTask(
		info_wnd_,
		preview_wnd_,
		stat_wnd_,
		*this,
		error_hwnd_));
	if (project_state_ == PS_PROJECT)
		AddTask(new FreeProjectDataTask());
}

void ProjectManager::CreateResouce(Resource id)
{
	FILETIME null_last_write;
	ZeroMemory(&null_last_write, sizeof(null_last_write)); // set to January 1, 1601 (UTC)
	switch (id)
	{
	case RS_HARDNESS:
		tracker_.SetDatum(RS_HARDNESS, _T("hardness.bmp"), null_last_write);
		break;
	case RS_ZERO_LAYER:
		tracker_.SetDatum(RS_ZERO_LAYER, _T("zero_layer.bmp"), null_last_write);
		break;
	}
	AddTask(new CreateResourceTask(id, error_hwnd_));
	IdsType ids;
	ids[id] = true;
	SIZE map_size = {
		exp2(MacroProjectData(ID_POWER_X)),
		exp2(MacroProjectData(ID_POWER_Y))
	};
	AddTask(new UpdateDataTask(
		MacroProjectData(ID_MAP_NAME).c_str(),
		folder_path_.c_str(),
		map_size,
		project_state_,
		MacroAppData(ID_FAST_TEXTURE_QUANTIZATION),
		MacroAppData(ID_ENABLE_LIGHTING),
		MacroAppData(ID_THRESHOLD),
		MacroAppData(ID_DISPLAY_HARDNESS),
		MacroAppData(ID_DISPLAY_TEXTURE),
		MacroAppData(ID_DISPLAY_ZERO_LAYER),
		TaskCommon::MapInfo::LoadFromGlobal()));
}

void ProjectManager::UpdateSettings()
{
	SIZE map_size = {
			exp2(MacroProjectData(ID_POWER_X)),
			exp2(MacroProjectData(ID_POWER_Y))
		};
	AddTask(new UpdateDataTask(
		MacroProjectData(ID_MAP_NAME).c_str(),
		folder_path_.c_str(),
		map_size,
		project_state_,
		MacroAppData(ID_FAST_TEXTURE_QUANTIZATION),
		MacroAppData(ID_ENABLE_LIGHTING),
		MacroAppData(ID_THRESHOLD),
		MacroAppData(ID_DISPLAY_HARDNESS),
		MacroAppData(ID_DISPLAY_TEXTURE),
		MacroAppData(ID_DISPLAY_ZERO_LAYER),
		TaskCommon::MapInfo::LoadFromGlobal()));
}

DWORD WINAPI ProjectManager::ProcessorThread(LPVOID parameter)
{
	for (;;)
	{
		Task *task;
		// interract with the ProjectManager object
		{
			ProjectManager *obj(ri_cast<ProjectManager*>(parameter));
			AutoCriticalSection acs(&obj->processor_section_);
			// processing should be suspended if the corrsponding flag is set
			if (obj->stop_processing_)
				break;
			// transmit the number of tasks left
			(*obj->tasks_left_)(obj->tasks_.size());
			// processing should be suspended if the queue is empty
			if (obj->tasks_.empty())
			{
				acs.Leave();
				SuspendThread(GetCurrentThread());
				continue;
			}
			// get the current task
			task = obj->tasks_.front();
			obj->tasks_.pop();
		}
		// perform the task
		// this is a very convenient place for catching exceptions
		try
		{
			AutoCriticalSection(&Task::task_data_.section_);
			(*task)();
		}
		catch (std::bad_alloc)
		{
			ProjectManager *obj(ri_cast<ProjectManager*>(parameter));
			AutoCriticalSection acs(&obj->processor_section_);
			obj->MacroDisplayError(_T("There was not enough memory to carry out a task.\nThe following queued tasks have been cancelled."));
			while (!obj->tasks_.empty())
			{
				delete obj->tasks_.front();
				obj->tasks_.pop();
			}
		}
		delete task;
	}
	return 0;
}

void ProjectManager::AddTask(Task *task)
{
	_RPT0(_CRT_WARN, typeid(*task).name());
	_RPT0(_CRT_WARN, "\n");
	{
		AutoCriticalSection acs(&processor_section_);
		tasks_.push(task);
		(*tasks_left_)(tasks_.size());
	}
	ResumeThread(processor_thread_);
}

//--------------------------------------------
// ProjectManager::FileNotFound implementation
//--------------------------------------------

ProjectManager::FileNotFound::FileNotFound(HWND &main_hwnd)
	:main_hwnd_(main_hwnd)
{}

void ProjectManager::FileNotFound::operator() (Resource id, LPCTSTR path)
{
	PostResourceNotFound(main_hwnd_, id);
}

//-------------------------------------------
// ProjectManager::FileUpdated implementation
//-------------------------------------------

ProjectManager::FileUpdated::FileUpdated(ProjectManager &project_manager)
	:project_manager_(project_manager)
{}

void ProjectManager::FileUpdated::operator() (const IdsType &ids)
{
	project_manager_.AddTask(new LoadProjectDataTask(project_manager_.error_hwnd_));
	project_manager_.AddTask(new UpdatePanelsTask(
		project_manager_.info_wnd_,
		project_manager_.preview_wnd_,
		project_manager_.stat_wnd_,
		project_manager_,
		project_manager_.error_hwnd_));
	project_manager_.AddTask(new FreeProjectDataTask());
}

//------------------------------------------------
// ProjectManager::ZeroLevelChanged implementation
//------------------------------------------------



ProjectManager::ZeroLevelChanged::ZeroLevelChanged(ProjectManager &project_manager)
	:project_manager_(project_manager)
{}

void ProjectManager::ZeroLevelChanged::operator() ()
{
	if (!MacroProjectData(ID_CUSTOM_HARDNESS))
		return;
	IdsType ids;
	ids.set();
	project_manager_.ReloadFiles(ids);
}
