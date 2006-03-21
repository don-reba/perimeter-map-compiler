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

//--------------------------------------------------
// detects information about Perimeter installations
//--------------------------------------------------

class VersionDetector
{
// nested types
public:
	// version information
	struct VersionInfo
	{
		VersionInfo() : version_(0) {}
		tstring description_;
		tstring path_;
		uint    version_;
	};
private:
	// dialog to have user choose a version
	class Chooser : public PMCWindow
	{
	// nested types
		typedef std::vector<VersionInfo> entries_t;
	// interface
	public:
		// creation
		Chooser(const entries_t &entries);
		// operations
		INT_PTR DoModal(HWND parent_wnd);
		// access
		uint GetChoice()  const;
		bool MustRemember() const;
	// message handlers
	private:
		// window
		void OnCommand   (Msg<WM_COMMAND>    &msg);
		void OnInitDialog(Msg<WM_INITDIALOG> &msg);
		// command
	// internal implementation
	protected:
		void ProcessMessage(WndMsg &msg);
	private:
		void ExchangeData();
	// data
	private:
		bool             must_remember_;
		const entries_t &entries_;
		uint             choice_;
	};
// public interface
public:
	// operations
	bool ShowSelectDialog(HWND parent_wnd);
	// access
	tstring GetPath()    const;
	uint    GetVersion() const;
// internal implementation
private:
	void FindVersions   (std::vector<VersionInfo> &entries);
	bool TestPrimeter101(VersionInfo &entry);
	bool TestPrimeter102(VersionInfo &entry);
	bool TestPrimeterET (VersionInfo &entry);
// data
private:
	VersionInfo version_info_;
};