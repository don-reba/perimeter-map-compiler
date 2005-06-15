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


#pragma once

#include "error handler.h"
#include "info wnd.h"
#include "file tracker.h"
#include "preview wnd.h"
#include "project tasks.h"
#include "stat wnd.h"

#include <queue>

class MainWnd;

//-------------------------------------------------------
// project manager definition
// this class does most of the work, in a separate thread
//-------------------------------------------------------
class ProjectManager : ErrorHandler
{
// callbacks
public:
	// toggles busy state
	struct TasksLeft {
		virtual void operator() (uint task_count) = 0;
	};
// FileTracker callback implementations
private:
	class FileNotFound : public FileTracker::FileNotFound
	{
	public:
		FileNotFound(TaskData &task_data);
		void operator() (uint id, LPCTSTR path);
	private:
		TaskData &task_data_;
	};
	struct FileUpdated : FileTracker::FileUpdated
	{
		FileUpdated(ProjectManager &project_manager);
		void operator() (const IdsType &ids);
		ProjectManager &project_manager_;
	};
// type definitions
private:
	typedef std::queue<Task*> QueueType;
// construction/destruction
public:
	ProjectManager(
		MainWnd    &main_wnd,
		InfoWnd    &info_wnd,
		PreviewWnd &preview_wnd,
		StatWnd    &stat_wnd
	);
	~ProjectManager();
// interface
public:
	// initialization
	bool Initialize();
	void Close();
	// project management
	void CreateProject(LPCTSTR folder_path, LPCTSTR map_name, SIZE map_size);
	void OpenProject(LPCTSTR project_path, bool new_project = false);
	// shrub management
	void PackShrub();
	void UnpackShrub(LPCTSTR shrub_path);
	// map management
	void InstallMap();
	// miscellaneous
	void SaveThumbnail();
	// data management
	void ReloadFiles(bool reload_heightmap, bool reload_texture);
	// settings
	void UpdateSettings();
// internal function
private:
	static DWORD WINAPI ProcessorThread(LPVOID parameter);
	void AddTask(Task *task);
// data
private:
	// threading
	HANDLE           processor_thread_;
	CRITICAL_SECTION processor_section_;
	bool             stop_processing_;
	QueueType        tasks_;
	// windows
	InfoWnd    &info_wnd_;
	PreviewWnd &preview_wnd_;
	StatWnd    &stat_wnd_;
	// project
	tstring      folder_path_;
	FileTracker  tracker_;
	ProjectState project_state_;
// callback implementation instances
private:
	FileUpdated  file_updated_;
	FileNotFound file_not_found_;
// callbacks
public:
	TasksLeft *tasks_left_;
};
