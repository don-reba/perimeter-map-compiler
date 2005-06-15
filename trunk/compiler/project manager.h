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
