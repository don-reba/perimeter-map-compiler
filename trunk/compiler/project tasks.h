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


//---------------------------------------------------
// Tasks are basically delayed function calls.
// Arguments are passed through constructors, stored,
// and then used in the operator() implementation.
//---------------------------------------------------

#pragma once

#include "error handler.h"
#include "preview wnd.h"
#include "task common.h"

#include <bitset>
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>

class InfoWnd;
class PreviewWnd;
class ProjectManager;
class StatWnd;

namespace TaskCommon
{
	struct Heightmap;
	struct Lightmap;
	struct Texture;
}

//--------------------------------
// the possible process states
// these affect execution of tasks
//--------------------------------

enum ProjectState
{
	PS_INACTIVE,
	PS_PROJECT,
	PS_SHRUB
};

//--------------------------------------
// resouce types that can be manipulated
//--------------------------------------

enum Resource
{
	RS_HEIGHTMAP,
	RS_TEXTURE,
	RS_HARDNESS,
	RS_ZERO_LAYER,
	RS_SURFACE,
	RS_SKY,
	resource_count
};
typedef std::bitset<resource_count> IdsType;

//----------------------------------------------------
// thread-safe task environment
// synchronization is left up to the user of the tasks
//----------------------------------------------------

struct TaskData
{
public:
	TaskData();
	~TaskData();
public:
	tstring             map_name_;
	tstring             project_folder_;
	SIZE                map_size_;
	ProjectState        project_state_;
	bool                fast_quantization_;
	bool                enable_lighting_;
	float               mesh_threshold_;
	bool                display_hardness_;
	bool                display_texture_;
	bool                display_zero_layer_;
	TaskCommon::MapInfo map_info_;
	CRITICAL_SECTION    section_;
	// cached resources
	TaskCommon::Hardness  *hardness_;
	TaskCommon::Heightmap *heightmap_;
	TaskCommon::Sky       *sky_;
	TaskCommon::Surface   *surface_;
	TaskCommon::Texture   *texture_;
	TaskCommon::ZeroLayer *zero_layer_;
};

//---------------------------------------------------------------------------------
// base class for the family of functors responcible for main application functions
//---------------------------------------------------------------------------------

class Task
{
public:
	virtual ~Task() {}
public:
	virtual void operator() () = 0;
public:
	static TaskData task_data_;
};

//--------------------------------------------------
// creates a default heightmap and a default texture
//--------------------------------------------------

class CreateDefaultFilesTask : public Task, public ErrorHandler
{
public:
	CreateDefaultFilesTask(HWND &error_hwnd);
public:
	void operator() ();
};

//-----------------------
// loads and caches files
//-----------------------

class CacheProjectDataTask : public Task, public ErrorHandler
{
public:
	CacheProjectDataTask(HWND &error_hwnd);
	void operator() ();
};

//----------------------------------------------
// notifies windows that the project has changed
//----------------------------------------------

class ChangeProjectTask : public Task
{
public:
	ChangeProjectTask(InfoWnd &info_wnd, PreviewWnd &preview_wnd, bool read_only = false);
	void operator() ();
private:
	InfoWnd    &info_wnd_;
	PreviewWnd &preview_wnd_;
	bool        read_only_;
};

//----------------------------------
// creates an optional resource file
//----------------------------------

class CreateResourceTask : public Task, public ErrorHandler
{
public:
	CreateResourceTask(uint id, HWND error_hwnd);
	void operator() ();
private:
	uint id_;
};

//-------------------
// frees cached files
//-------------------

class FreeProjectDataTask : public Task
{
public:
	void operator() ();
};

//-----------------
// installs a Shrub
//-----------------

class InstallMapTask : public Task, public ErrorHandler
{
// construction/destruction
public:
	InstallMapTask(HWND &hwnd, tstring &perimeter_path);
// Task interface
public:
	void operator() ();
// internal functioni
private:
	void AppendBTDB(LPCTSTR path, LPCTSTR folder_name);
	void AppendWorldsPrm(LPCTSTR path, LPCTSTR folder_name, uint version);
	void SaveMission (LPCTSTR path, LPCTSTR folder_name, bool survival);
	void SaveMission2(LPCTSTR path, LPCTSTR folder_name, bool survival);
// data
private:
	HWND    hwnd_;
	tstring perimeter_path_;
};

//------------
// loads files
//------------

class LoadProjectDataTask : public Task, public ErrorHandler
{
public:
	LoadProjectDataTask(const IdsType  &ids, HWND &error_hwnd);
	LoadProjectDataTask(HWND &error_hwnd); // loads all data
	void operator() ();
private:
	IdsType ids_;
};

//----------------------------------------
// notifies that a project has been opened
//----------------------------------------
class NotifyProjectOpenTask : public Task
{
public:
	NotifyProjectOpenTask(HWND main_hwnd);
	void operator() ();
private:
	HWND main_hwnd_;
};

//------------------------------------------
// notifies that a project has been unpacked
//------------------------------------------
class NotifyProjectUnpackedTask : public Task
{
public:
	NotifyProjectUnpackedTask(HWND main_hwnd);
	void operator() ();
private:
	HWND main_hwnd_;
};

//--------------
// packs a shrub
//--------------

class PackShrubTask : public Task, public ErrorHandler
{
public:
	PackShrubTask(
		bool custom_hardness,
		bool custom_sky,
		bool custom_surface,
		bool custom_zero_layer,
		HWND &error_hwnd);
	void operator() ();
private:
	bool custom_hardness_;
	bool custom_sky_;
	bool custom_surface_;
	bool custom_zero_layer_;
};

//-----------------------------------
// saves a thumbnail image of the map
//-----------------------------------

class SaveThumbTask : public Task, public ErrorHandler
{
public:
	SaveThumbTask(HWND &error_hwnd);
	void operator() ();
};

//--------------------------------------------
// the correct way of changing task_data_
// these changes have to be performed in order
//--------------------------------------------

class UpdateDataTask : public Task
{
public:
	UpdateDataTask(
		LPCTSTR      map_name,
		LPCTSTR      project_folder,
		SIZE         map_size,
		ProjectState project_state,
		bool         fast_quantization,
		bool         enable_lighting,
		float        mesh_threshold,
		bool         display_hardness,
		bool         display_texture,
		bool         display_zero_layer,
		const TaskCommon::MapInfo &map_info);
	void operator() ();
private:
	tstring      map_name_;
	tstring      project_folder_;
	SIZE         map_size_;
	ProjectState project_state_;
	bool         fast_quantization_;
	bool         enable_lighting_;
	float        mesh_threshold_;
	bool         display_hardness_;
	bool         display_texture_;
	bool         display_zero_layer_;
	TaskCommon::MapInfo map_info_;
};

//---------------------------------------
// updates panels to reflect data changes
//---------------------------------------

class UpdatePanelsTask : public Task, public ErrorHandler
{
// nested types
private:
	struct OnPanelVisible : PanelWindow::ToggleVisibility
	{
		OnPanelVisible(ProjectManager &project_manager);
		void operator() (bool on);
	private:
		ProjectManager &project_manager_;
	};
// construction/destruction
public:
	UpdatePanelsTask(
		InfoWnd        &info_wnd,
		PreviewWnd     &preview_wnd,
		StatWnd        &stat_wnd,
		ProjectManager &project_manager,
		HWND           &error_hwnd);
// Task interface
public:
	void operator() ();
// data
private:
	InfoWnd        &info_wnd_;
	PreviewWnd     &preview_wnd_;
	ProjectManager &project_manager_;
	StatWnd        &stat_wnd_;
};

//----------------
// unpacks a Shrub
//----------------

class UnpackShrubTask : public Task, public ErrorHandler
{
// nested types
private:
	struct OnPanelVisible : PanelWindow::ToggleVisibility
	{
		OnPanelVisible(ProjectManager &project_manager);
		void operator() (bool on);
	private:
		ProjectManager &project_manager_;
	};
// construction/destruction
public:
	UnpackShrubTask(
		TiXmlDocument  *document,
		BYTE           *buffer,
		BYTE           *initial_offset,
		InfoWnd        &info_wnd,
		PreviewWnd     &preview_wnd,
		StatWnd        &stat_wnd,
		ProjectManager &project_manager,
		HWND           &error_hwnd);
	~UnpackShrubTask();
// Task interface
public:
	void operator() ();
// data
private:
	TiXmlDocument  *document_;
	BYTE           *buffer_;
	InfoWnd        &info_wnd_;
	BYTE           *initial_offset_;
	PreviewWnd     &preview_wnd_;
	ProjectManager &project_manager_;
	StatWnd        &stat_wnd_;
};
