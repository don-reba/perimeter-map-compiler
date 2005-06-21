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
		FileDatum() : active_(false) {}
		bool     active_;
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