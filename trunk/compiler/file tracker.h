#pragma once

#include "error handler.h"
#include "project tasks.h"

//-----------------------------------------------
// class for tracking changes in files definition
//-----------------------------------------------

class FileTracker : ErrorHandler
{
// nested classes
public:
	struct FileUpdated {
		virtual void operator() (const IdsType &ids) = 0;
	};
	struct FileNotFound {
		virtual void operator() (uint id, LPCTSTR path) = 0;
	};
private:
	struct FileDatum {
		tstring  file_name_;
		FILETIME last_write_;
	};
// construction/destruction
public:
	FileTracker(HWND &hwnd, FileUpdated &file_updated, FileNotFound &file_not_found);
	~FileTracker();
//interface
public:
	void AddData(uint id, LPCTSTR file_name, const FILETIME &last_write);
	bool Start(LPCTSTR folder_path, FileUpdated *file_updated);
	void Stop();
// internal function
private:
	static DWORD WINAPI TrackerThread(LPVOID parameter);
	void CheckFiles();
	bool WasUpdated(FileDatum &datum, uint id);
// data
private:
	typedef FileDatum FileData[resource_count];
	bool              is_valid_;
	FileData          files_;
	HANDLE            tracker_thread_;
	HANDLE            stop_tracking_event_;
	CRITICAL_SECTION  tracker_section_;
	tstring           folder_path_;
	FileUpdated      *file_updated_;
	FileNotFound     *file_not_found_;
};