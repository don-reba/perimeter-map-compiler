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
#include "task resource manager.h"

#include <process.h>

#include <algorithm>
#include <iostream>

#include <loki/ScopeGuard.h>

using namespace std;
using namespace Loki;

//-----------------------------------
// TaskResourceManager implementation
//-----------------------------------

TaskResourceManager::TaskResourceManager(HWND hwnd)
	:ErrorHandler(hwnd)
	,is_enabled_(true)
	,is_exiting_(false)
	,thread_    (NULL)
	,timer_     (NULL)
{
	infinite_filetime_.QuadPart = 0x7FFFFFFFFFFFFFFF;
	// initialize the critical section
	InitializeCriticalSection(&section_);
	// initialize the timer
	timer_ = CreateWaitableTimer(NULL, TRUE, NULL);
	if (NULL == timer_)
		MacroDisplayError("Waitable timer creation has failed.");
	SetWaitableTimer(
			timer_,              // hTimer
			&infinite_filetime_, // pDueTime
			0,                   // lPeriod
			NULL,                // pfnCompletionRoutine
			NULL,                // lpArgToCompletionRoutine
			FALSE                // fResume
			);
	// start watching
	uint thread_id;
	thread_ = ri_cast<HANDLE>(_beginthreadex(NULL, 0, &RunProxy, this, 0, &thread_id));
}

TaskResourceManager::~TaskResourceManager()
{
	is_exiting_ = true;
	LARGE_INTEGER null_time;
	null_time.QuadPart = 0;
	SetWaitableTimer(
			timer_,     // hTimer
			&null_time, // pDueTime
			0,          // lPeriod
			NULL,       // pfnCompletionRoutine
			NULL,       // lpArgToCompletionRoutine
			FALSE       // fResume
			);
	if (WAIT_TIMEOUT == WaitForSingleObject(thread_, exit_delay_))
	{
		_RPT0(_CRT_WARN, _T("The resource manager thread has been forcefully terminated./n"));
		TerminateThread(thread_, 1);
	}
	CloseHandle(thread_);
	DeleteCriticalSection(&section_);
}

// thread-safe
void TaskResourceManager::AddResource(ManagedTaskResource *resource)
{
	EnterCriticalSection(&section_);
	LOKI_ON_BLOCK_EXIT(LeaveCriticalSection, &section_);
	resource->SetID(resources_.size());
	resources_.push_back(resource);
}

void TaskResourceManager::OnCheckOut(int id)
{
	EnterCriticalSection(&section_);
	LOKI_ON_BLOCK_EXIT(LeaveCriticalSection, &section_);
	// find the entry with this id
	const queue_t::iterator i = find(queue_.begin(), queue_.end(), id);
	// remove the entry
	if (i != queue_.end())
	{
		bool is_head = (i == queue_.begin());
		queue_.erase(i);
		if (is_head)
		{
			LARGE_INTEGER time =
				queue_.empty()
				? infinite_filetime_
				: queue_.front().time_;
			int result = SetWaitableTimer(
				timer_, // hTimer
				&time,  // pDueTime
				0,      // lPeriod
				NULL,   // pfnCompletionRoutine
				NULL,   // lpArgToCompletionRoutine
				FALSE   // fResume
				);
			if (0 == result)
				MacroDisplayError("SetWaitableTimer failure.");
		}
	}
}

void TaskResourceManager::OnCheckIn(int id)
{
	EnterCriticalSection(&section_);
	LOKI_ON_BLOCK_EXIT(LeaveCriticalSection, &section_);
	// find the entry with this id
	const queue_t::iterator i = find(queue_.begin(), queue_.end(), id);
	// get the time of unloading
	LARGE_INTEGER time;
	time.QuadPart = -unload_delay_;
	// reset the timer if this is the head entry
	if (i == queue_.begin())
	{
		int result = SetWaitableTimer(
			timer_, // hTimer
			&time,  // pDueTime
			0,      // lPeriod
			NULL,   // pfnCompletionRoutine
			NULL,   // lpArgToCompletionRoutine
			FALSE   // fResume
			);
		if (0 == result)
			MacroDisplayError("SetWaitableTimer failure.");
	}
	// remove the entry if it is already in the queue
	if (i != queue_.end())
		queue_.erase(i);
	// enqueue the entry
	queue_.push_back(QueueEntry(time, id));
}

void TaskResourceManager::SetEnabled(bool enable)
{
	EnterCriticalSection(&section_);
	LOKI_ON_BLOCK_EXIT(LeaveCriticalSection, &section_);
	is_enabled_ = enable;
	foreach (ManagedTaskResource *resource, resources_)
		resource->SetEnabled(enable);
}

uint __stdcall TaskResourceManager::RunProxy(void *obj)
{
	ri_cast<TaskResourceManager*>(obj)->Run();
	return 0;
}

void TaskResourceManager::Run()
{
	for (;;)
	{
		DWORD wait_result = WaitForSingleObject(timer_, wait_timeout_);
		if (WAIT_TIMEOUT == wait_result)
		{
			EnterCriticalSection(&section_);
			LOKI_ON_BLOCK_EXIT(LeaveCriticalSection, &section_);
			// check for the exit signal
			if (is_exiting_)
				break;
			continue;
		}
		int  index;
		{
			EnterCriticalSection(&section_);
			LOKI_ON_BLOCK_EXIT(LeaveCriticalSection, &section_);
			// check for the exit signal
			if (is_exiting_)
				break;
			// retrieve data
			index = queue_.front().index_;
			queue_.pop_front();
			// prepare for sleep
			LARGE_INTEGER time =
				queue_.empty()
				? infinite_filetime_
				: queue_.front().time_;
			SetWaitableTimer(
					timer_, // hTimer
					&time,  // pDueTime
					0,      // lPeriod
					NULL,   // pfnCompletionRoutine
					NULL,   // lpArgToCompletionRoutine
					FALSE   // fResume
					);
		}
		if (is_enabled_)
			resources_[index]->Unload();
	}
	_RPT0(_CRT_WARN, "Resource Manager terminating...\n");
}
