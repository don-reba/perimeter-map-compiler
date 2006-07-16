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


//---------------------------------------------------
// Tasks are basically delayed function calls.
// Arguments are passed through constructors, stored,
// and then used in the operator() implementation.
//---------------------------------------------------

#pragma once

#include "error handler.h"
#include "preview wnd.h"
#include "task common.h"
#include "task resource.h"
#include "task resource manager.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>

#include <list>

class InfoWnd;
class PreviewWnd;
class ProjectManager;
class StatWnd;

namespace TaskCommon
{
	class  Heightmap;
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

//----------------------------------------------------
// thread-safe task environment
// synchronization is left up to the user of the tasks
//----------------------------------------------------

struct TaskData
{
public:
	// interface
	TaskData(TaskCommon::SaveCallback::SaveHandler *save_handler, const HWND &error_hwnd);
	void SetResourceManagerEnabled(bool enable);
public:
	// miscelleneous data
	bool                display_hardness_;
	bool                display_texture_;
	bool                display_zero_layer_;
	bool                enable_lighting_;
	bool                fast_quantization_;
	float               mesh_threshold_;
	ProjectState        project_state_;
	SIZE                map_size_;
	TaskCommon::MapInfo map_info_;
	tstring             file_names_[resource_count];
	tstring             map_name_;
	tstring             project_folder_;
	// resources
	TaskResource<TaskCommon::Hardness>  hardness_;
	TaskResource<TaskCommon::Heightmap> heightmap_;
	TaskResource<TaskCommon::Script>    script_;
	TaskResource<TaskCommon::Sky>       sky_;
	TaskResource<TaskCommon::Surface>   surface_;
	TaskResource<TaskCommon::Texture>   texture_;
	TaskResource<TaskCommon::ZeroLayer> zero_layer_;
private:
	// unmanaged resources
	TaskCommon::Hardness  um_hardness_;
	TaskCommon::Heightmap um_heightmap_;
	TaskCommon::Script    um_script_;
	TaskCommon::Sky       um_sky_;
	TaskCommon::Surface   um_surface_;
	TaskCommon::Texture   um_texture_;
	TaskCommon::ZeroLayer um_zero_layer_;
	// manager
	TaskResourceManager manager_;
};

//---------------------
// Task exception class
//---------------------

class TaskException
{
public:
	TaskException(LPCTSTR msg) : msg_(msg) {}
	const tstring& Msg() const { return msg_; }
private:
	tstring msg_;
};

//---------------------------------------------------------------------------------
// base class for the family of functors responcible for main application functions
//---------------------------------------------------------------------------------

class Task
{
public:
	virtual ~Task() {}
public:
	virtual void operator() (TaskData &data) = 0;
public:
	static TaskData task_data_;
};

//--------------------------------------------------
// creates a default heightmap and a default texture
//--------------------------------------------------

class CreateDefaultFilesTask : public Task, public ErrorHandler
{
public:
	CreateDefaultFilesTask(const HWND &error_hwnd);
	void operator() (TaskData &data);
};

//----------------------------------------------
// notifies windows that the project has changed
//----------------------------------------------

class ChangeProjectTask : public Task
{
public:
	ChangeProjectTask(InfoWnd &info_wnd, PreviewWnd &preview_wnd, bool read_only = false);
	void operator() (TaskData &data);
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
	CreateResourceTask(uint id, const HWND &error_hwnd);
	void operator() (TaskData &data);
private:
	uint id_;
};

//----------------------------------------
// converts the given script file into XML
//----------------------------------------

class ImportScriptTask : public Task
{
public:
	ImportScriptTask(LPCTSTR script_path, LPCTSTR xml_path);
	void operator() (TaskData &data);
private:
	tstring script_;
	tstring xml_;
};

//-----------------
// installs a Shrub
//-----------------

class InstallMapTask : public Task, public ErrorHandler
{
// construction/destruction
public:
	InstallMapTask(
		const HWND &hwnd,
		LPCTSTR     install_path,
		uint        version,
		bool        custom_zero_layer,
		bool        rename_to_unregistered);
// Task interface
public:
	void operator() (TaskData &data);
// internal functioni
private:
	void AppendBTDB(
		LPCTSTR      path,
		LPCTSTR      folder_name,
		ProjectState project_state,
		tstring      map_name);
	void AppendWorldsPrm(LPCTSTR path, LPCTSTR folder_name, uint version);
	void SaveMission(
		TaskCommon::MapInfo map_info,
		LPCTSTR             path,
		LPCTSTR             folder_name,
		bool                survival);
	void SaveMission2(
		TaskCommon::MapInfo map_info,
		LPCTSTR             path,
		LPCTSTR             folder_name,
		bool                survival);
// data
private:
	HWND          hwnd_;
	const tstring install_path_;
	const uint    version_;
	const bool    custom_zero_layer_;
	const bool    rename_to_unregistered_;
};

//----------------------------------------
// notifies that a project has been opened
//----------------------------------------
class NotifyProjectOpenTask : public Task
{
public:
	NotifyProjectOpenTask(HWND main_hwnd);
	void operator() (TaskData &data);
private:
	HWND main_hwnd_;
};

//------------------------------------------
// notifies that a resource has been created
//------------------------------------------
class NotifyResourceCreatedTask : public Task
{
public:
	NotifyResourceCreatedTask(Resource id, HWND main_hwnd);
	void operator() (TaskData &data);
private:
	Resource id_;
	HWND     main_hwnd_;
};

//------------------------------------------
// notifies that a project has been unpacked
//------------------------------------------
class NotifyProjectUnpackedTask : public Task
{
public:
	NotifyProjectUnpackedTask(HWND main_hwnd);
	void operator() (TaskData &data);
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
		bool        custom_hardness,
		bool        custom_sky,
		bool        custom_surface,
		bool        custom_zero_layer,
		bool        use_registration,
		const HWND &error_hwnd);
	void operator() (TaskData &data);
private:
	bool custom_hardness_;
	bool custom_sky_;
	bool custom_surface_;
	bool custom_zero_layer_;
	bool use_registration_;
};

//-----------------------------------
// saves a thumbnail image of the map
//-----------------------------------

class SaveThumbTask : public Task, public ErrorHandler
{
public:
	SaveThumbTask(const HWND &error_hwnd);
	void operator() (TaskData &data);
};

//---------------------------------------
// enable or disable the resource manager
//---------------------------------------

class SetResourceManagerEnabledTask : public Task
{
public:
	SetResourceManagerEnabledTask(bool enable);
	void operator() (TaskData &data);
private:
	bool enable_;
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
		tstring      file_names[resource_count],
		const TaskCommon::MapInfo &map_info);
	void operator() (TaskData &data);
private:
	bool         display_hardness_;
	bool         display_texture_;
	bool         display_zero_layer_;
	bool         enable_lighting_;
	bool         fast_quantization_;
	float        mesh_threshold_;
	ProjectState project_state_;
	SIZE         map_size_;
	TaskCommon::MapInfo map_info_;
	tstring      file_names_[resource_count];
	tstring      map_name_;
	tstring      project_folder_;
};

//----------------------------------------------
// an abstract base for the panel-updating tasks
//----------------------------------------------

class UpdatePanelTask : public Task, public ErrorHandler
{
// nested types
protected:
	typedef void (ProjectManager::*update_t)(IdsType ids);
	typedef PanelWindow::on_show_t::connection_t connection_t;
	typedef PanelWindow::on_show_t::delegate_t   delegate_t;
protected:
	// calls the project manager to update the corresponding window
	struct OnPanelVisible
	{
	public:
		OnPanelVisible();
		void Set(
			IdsType         ids,
			ProjectManager *project_manager,
			update_t        updade,
			connection_t    connection);
		void operator() ();
	private:
		IdsType         ids_;
		ProjectManager *project_manager_;
		update_t        update_;
		connection_t    connection_;
	};
// construction
public:
	UpdatePanelTask(
		IdsType         ids,
		PanelWindow    &wnd,
		ProjectManager &project_manager,
		update_t        update,
		const HWND     &error_hwnd);
// Task interface
public:
	void operator() (TaskData &data);
// interface
protected:
	virtual void UpdatePanel(
		IdsType      ids,
		PanelWindow &wnd,
		TaskData    &data) = 0;
	virtual OnPanelVisible &GetOnPanelVisible() = 0;
// implementation
private:
	void Enqueue();
// data
private:
	update_t        update_;
	IdsType         ids_;
	PanelWindow    &wnd_;
	ProjectManager &project_manager_;
};

//------------------------------------------------
// update the info panel to reflect data changes
//------------------------------------------------

class UpdateInfoWndTask : public UpdatePanelTask
{
// construction
public:
	UpdateInfoWndTask(
		IdsType         ids,
		InfoWnd        &stat_wnd,
		ProjectManager &project_manager,
		const HWND     &error_hwnd);
// implementation
protected:
	void UpdatePanel(
		IdsType      ids,
		PanelWindow &wnd,
		TaskData    &data);
	OnPanelVisible& GetOnPanelVisible();
// data
private:
	static OnPanelVisible on_panel_visible_;
};

//-------------------------------------------------
// update the preview panel to reflect data changes
//-------------------------------------------------

class UpdatePreviewWndTask : public UpdatePanelTask
{
// construction
public:
	UpdatePreviewWndTask(
		IdsType         ids,
		PreviewWnd     &stat_wnd,
		ProjectManager &project_manager,
		const HWND     &error_hwnd);
// implementation
protected:
	void UpdatePanel(
		IdsType      ids,
		PanelWindow &wnd,
		TaskData    &data);
	OnPanelVisible& GetOnPanelVisible();
// data
private:
	static OnPanelVisible on_panel_visible_;
};

//------------------------------------------------
// update the status panel to reflect data changes
//------------------------------------------------

class UpdateStatWndTask : public UpdatePanelTask
{
// construction
public:
	UpdateStatWndTask(
		IdsType         ids,
		StatWnd        &stat_wnd,
		ProjectManager &project_manager,
		const HWND     &error_hwnd);
// implementation
protected:
	void UpdatePanel(
		IdsType      ids,
		PanelWindow &wnd,
		TaskData    &data);
	OnPanelVisible& GetOnPanelVisible();
// data
private:
	static OnPanelVisible on_panel_visible_;
};

//----------------
// unpacks a Shrub
//----------------

class UnpackShrubTask : public Task, public ErrorHandler
{
// construction/destruction
public:
	UnpackShrubTask(
		TiXmlDocument *document,
		BYTE          *buffer,
		BYTE          *initial_offset,
		const HWND    &error_hwnd);
	~UnpackShrubTask();
// Task interface
public:
	void operator() (TaskData &data);
// data
private:
	TiXmlDocument  *document_;
	BYTE           *buffer_;
	BYTE           *initial_offset_;
};
