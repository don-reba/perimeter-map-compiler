#include "stdafx.h"

#include "file tracker.h"

//---------------------------
// FileTracker implementation
//---------------------------

FileTracker::FileTracker(HWND &hwnd, FileUpdated &file_updated, FileNotFound &file_not_found)
	:ErrorHandler   (hwnd)
	,file_updated_  (&file_updated)
	,file_not_found_(&file_not_found)
	,is_valid_      (true)
{
	InitializeCriticalSection(&tracker_section_); // really, if this fails, likely nothing else will work
	stop_tracking_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == stop_tracking_event_)
	{
		MacroDisplayError(_T("The tracking event could not be created."));
		is_valid_ = false;
	}
}

FileTracker::~FileTracker()
{
	DeleteCriticalSection(&tracker_section_);
}

void FileTracker::AddData(uint id, LPCTSTR file_name, const FILETIME &last_write)
{
	AutoCriticalSection acs(&tracker_section_);
	FileDatum datum;
	datum.file_name_  = file_name;
	datum.last_write_ = last_write;
	files_[id] = datum;
}

bool FileTracker::Start(LPCTSTR folder_path, FileTracker::FileUpdated *file_updated)
{
	if (!is_valid_)
	{
		_RPT0(_CRT_WARN, "FileTracker::Create called on an invalid file tracker");
		return false;
	}
	file_updated_ = file_updated;
	folder_path_  = folder_path;
	// create the processor thread
	ResetEvent(stop_tracking_event_);
	DWORD thread_id;
	tracker_thread_ = CreateThread(NULL, 0, TrackerThread, this, 0, &thread_id);
	if (NULL == tracker_thread_)
	{
		MacroDisplayError(_T("File tracking thread could not be created."));
		return false;
	}
	return true;
}

void FileTracker::Stop()
{
	// shut down the processor thread
	if (NULL != tracker_thread_)
	{
		{
			AutoCriticalSection acs(&tracker_section_);
			SetEvent(stop_tracking_event_);
		}
		const DWORD timeout(8000); // 8 seconds
		// apply force if necessary
		if (WAIT_TIMEOUT == WaitForSingleObject(tracker_thread_, timeout))
		{
			_RPT0(_CRT_WARN, _T("The tracker thread has been forcefully terminated./n"));
			TerminateThread(tracker_thread_, 1);
		}
	}
	CloseHandle(tracker_thread_);
}

DWORD WINAPI FileTracker::TrackerThread(LPVOID parameter)
{
	FileTracker *obj(ri_cast<FileTracker*>(parameter));
	// initialize the handle array to wait on
	const size_t handle_count(2);
	HANDLE handles[handle_count] =
	{
		FindFirstChangeNotification(
			obj->folder_path_.c_str(),
			FALSE,
			FILE_NOTIFY_CHANGE_LAST_WRITE),
		obj->stop_tracking_event_
	};
	if (INVALID_HANDLE_VALUE == handles[0])
	{
		obj->MacroDisplayError(_T("Could not begin to track files."));
		return 1;
	}
	// initial file check
	{
		AutoCriticalSection acs(&obj->tracker_section_);
		obj->CheckFiles();
	}
	// file check cycle
	for (;;)
	{
		DWORD wait_result(WaitForMultipleObjects(handle_count, handles, FALSE, INFINITE));
		_ASSERTE(wait_result != WAIT_TIMEOUT);
		if (WAIT_OBJECT_0 + 1 == wait_result)
			break;
		{
			AutoCriticalSection acs(&obj->tracker_section_);
			obj->CheckFiles();
		}
		FindNextChangeNotification(handles[0]);
	}
	return 0;
}

void FileTracker::CheckFiles()
{
	// check which files where changed
	IdsType ids;
	for (uint i(0); i != resource_count; ++i)
		ids[i] = WasUpdated(files_[i], i);
	// use the callback
	(*file_updated_)(ids);
}

bool FileTracker::WasUpdated(FileDatum &datum, uint id)
{
	// create the full path to the file
	TCHAR path[MAX_PATH];
	PathCombine(path, folder_path_.c_str(), datum.file_name_.c_str());
	// open the file
	HANDLE file(INVALID_HANDLE_VALUE);
	const DWORD retry_interval(1000); // milliseconds
	const uint  try_count(8);
	uint try_num(0);
	for (; try_num != try_count; ++try_num)
	{
		file = CreateFile(
			path,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (INVALID_HANDLE_VALUE == file)
		{
			if (ERROR_FILE_NOT_FOUND == GetLastError())
				(*file_not_found_)(id, path);
			else
				Sleep(retry_interval);
		}
		else
			break;
	}
	if (INVALID_HANDLE_VALUE == file || try_count == try_num)
		return false;
	// retrieve the last write time
	FILETIME last_write;
	GetFileTime(file, NULL, NULL, &last_write);
	CloseHandle(file);
	// check if the file has been written to since the last write
	bool was_updated(0 > CompareFileTime(&datum.last_write_, &last_write));
	datum.last_write_ = last_write;
	return was_updated;
}