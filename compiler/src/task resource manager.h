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


#include "error handler.h"
#include "task resource.h"

#include <deque>
#include <vector>

class TaskResourceManager : public ErrorHandler
{
// types
private:
	struct QueueEntry
	{
		LARGE_INTEGER time_;
		uint          index_;
		QueueEntry(LARGE_INTEGER time, uint index) : time_(time), index_(index) {}
		bool operator == (uint index) const { return index_ == index; }
	};
	typedef std::deque<QueueEntry>           queue_t;
	typedef std::vector<ManagedTaskResource*> store_t;
// interface
public:
	TaskResourceManager(HWND hwnd);
	~TaskResourceManager();
	void AddResource(ManagedTaskResource *resource);
	void OnCheckOut(int id);
	void OnCheckIn (int id);
	void SetEnabled(bool enable);
// implementation
private:
	void Run();
private:
	static uint __stdcall RunProxy(void *obj);
// data
private:
	bool             is_enabled_;
	bool             is_exiting_;
	CRITICAL_SECTION section_;
	queue_t          queue_;
	HANDLE           thread_;
	HANDLE           timer_;
	store_t          resources_;
	LARGE_INTEGER    infinite_filetime_;
// constants
private:
	static const int exit_delay_   = 6  * 1000;     // 6  seconds
	static const int unload_delay_ = 4  * 10000000; // 4  seconds
	static const int wait_timeout_ = 16 * 1000;     // 16 seconds
};
