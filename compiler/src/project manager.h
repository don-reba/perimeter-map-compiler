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

#include "error handler.h"
#include "info wnd.h"
#include "file tracker.h"
#include "preview wnd.h"
#include "project tasks.h"
#include "stat wnd.h"
#include "task resources.h"

#include <queue>
#include <boost/array.hpp>

class MainWnd;

//-------------------------------------------------------
// ProjectManager definition
// this class does most of the work, in a separate thread
// the class is not thread-safe;
//  it can only be used within the main thread
//-------------------------------------------------------
class ProjectManager : ErrorHandler, TaskCommon::SaveCallback::SaveHandler
{
// callbacks
public:
	// toggles busy state
	struct TasksLeft {
		virtual void operator() (uint task_count) = 0;
	};
// callback implementations
private:
	struct FileUpdated : FileTracker::FileUpdated
	{
		FileUpdated(ProjectManager &project_manager);
		void operator() (const IdsType &ids);
		ProjectManager &project_manager_;
	};
	struct ZeroLevelChanged : InfoWnd::ZeroLevelChanged {
		ZeroLevelChanged(ProjectManager &project_manager);
		void operator() ();
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
	void CreateProject(LPCTSTR folder_path, LPCTSTR map_name, SIZE map_size, HWND main_hwnd);
	void OpenProject(LPCTSTR project_path, HWND main_hwnd, bool new_project = false);
	// shrub management
	void PackShrub();
	bool UnpackShrub(LPCTSTR shrub_path, HWND main_hwnd);
	void OnProjectOpen(HWND main_hwnd);
	void OnProjectUnpacked(HWND main_hwnd);
	void OnResourceCreated(Resource id);
	// map management
	void InstallMap(LPCTSTR install_path, uint version);
	// miscellaneous
	void SaveThumbnail();
	// data management
	void CreateResource(Resource id, HWND main_hwnd);
	void ImportScript(LPCTSTR script_path, HWND main_hwnd);
	// panel management
	void UpdateInfoWnd   (IdsType ids);
	void UpdatePreviewWnd(IdsType ids);
	void UpdateStatWnd   (IdsType ids);
	void UpdatePanels    (IdsType ids);
	// settings
	void UpdateSettings();
// internal function
private:
	static uint __stdcall ProcessorThreadProxy(void *obj);
	void AddTask(Task *task);
	void FindFileNames();
	void ProcessorThread();
	void ChangeOpeningCount(int delta);
	// SaveHandler
	void OnSaveBegin(Resource id);
	void OnSaveEnd  (Resource id);
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
	int          opening_count_;
	tstring file_names_[resource_count];
// callback implementation instances
private:
	FileUpdated file_updated_;
public:
	ZeroLevelChanged zero_level_changed_;
// callbacks
public:
	TasksLeft *tasks_left_;
};
