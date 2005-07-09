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

#include "pmc wnd.h"

class MapManager : public PMCWindow
{
// interface
public:
	INT_PTR DoModal(HWND parent_wnd);
// message handlers
private:
	// window
	void OnCommand      (Msg<WM_COMMAND>       &msg);
	void OnGetMinMaxInfo(Msg<WM_GETMINMAXINFO> &msg);
	void OnInitDialog   (Msg<WM_INITDIALOG>    &msg);
	void OnSize         (Msg<WM_SIZE>          &msg);
	// command
	void OnDeleteMap(Msg<WM_COMMAND> &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
private:
	static void FilesInDir(LPCTSTR dir, vector<tstring> &files, size_t depth = 0);
	static void ScreenToClient(HWND hwnd, RECT *rect);
private:
	void    GetFilesList(LPCTSTR map_name, vector<tstring> &files, bool recurse = true);
	bool    GetInstallPath(string &install_path);
	tstring GetMapFilesSize(LPCTSTR map_name);
// data
	tstring install_path_;
	RECT    map_list_border_;
	SIZE    min_size_;
	LONG    delete_btn_offset_;
};
