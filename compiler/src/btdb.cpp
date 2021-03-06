//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// ? Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// ? Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// ? Neither the name of Don Reba nor the names of his contributors may be used
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


#include "StdAfx.h"

#include "btdb.h"

#include <algorithm>
#include <fstream>

using std::ifstream;
using std::ofstream;
using std::find;

//--------------------
// Btdb implementation
//--------------------
Btdb::Btdb(LPCTSTR btdb_file)
{
	DWORD size(0);
	BtdbEntry entry;
	// save the path
	btdb_file_ = btdb_file;
	// read in the contents of btdb into an array
	ifstream btdb_in(btdb_file, std::ios::ios_base::binary);
	is_open_ = btdb_in.is_open();
	if (!is_open_)
		return;
	btdb_in.read(ri_cast<char*>(&size), 4);
	for (DWORD i(0); i != size; ++i)
	{
		// read in title_
		btdb_in.read(ri_cast<char*>(&entry.title_size_), 4);
		entry.title_ = new char[entry.title_size_];
		btdb_in.read(entry.title_, entry.title_size_);
		// read in content_
		btdb_in.read(ri_cast<char*>(&entry.content_size_), 4);
		entry.content_ = new char[entry.content_size_];
		btdb_in.read(entry.content_, entry.content_size_);
		// read in auxillary_
		btdb_in.read(ri_cast<char*>(&entry.auxillary_size_), 4);
		entry.auxillary_ = new char[entry.auxillary_size_];
		btdb_in.read(entry.auxillary_, entry.auxillary_size_);
		// add the entry to the arrray
		entries_.push_back(entry);
	}
	btdb_in.close();
}

Btdb::~Btdb()
{
	Flush();
	vector<BtdbEntry>::iterator i(entries_.begin());
	const vector<BtdbEntry>::const_iterator end(entries_.end());
	for (; i != end; ++i)
		i->Delete();
}

void Btdb::AddMapEntry(const string &title, const string &content)
{
	BtdbEntry entry;
	// create title_
	const char * const title_prefix("MapNames.");
	const size_t title_prefix_size(strlen(title_prefix));
	entry.title_size_ = title_prefix_size + title.size() + 1;
	entry.title_ = new char[entry.title_size_];
	strcpy(entry.title_, title_prefix);
	strcpy(entry.title_ + title_prefix_size, title.c_str());
	// create content_
	entry.content_size_ = content.size() + 1;
	entry.content_ = new char[entry.content_size_];
	strcpy(entry.content_, content.c_str());
	// create auxillary_
	entry.auxillary_size_ = 1;
	entry.auxillary_ = new char[entry.auxillary_size_];
	*entry.auxillary_ = '\0';
	// add the entry
	AddEntry(entry);
}

void Btdb::RemoveMapEntry(const char *title)
{
	const char * const title_prefix("MapNames.");
	const size_t title_prefix_size(strlen(title_prefix));
	const size_t title_size       (strlen(title));
	char *full_title(new char[title_prefix_size + title_size + 1]);
	strcpy(full_title, title_prefix);
	strcpy(full_title + title_prefix_size, title);
	RemoveEntry(full_title);
	delete [] full_title;
}

void Btdb::AddEntry(const BtdbEntry &entry)
{
	vector<BtdbEntry>::iterator entry_i(find(entries_.begin(), entries_.end(), entry));
	if (entries_.end() != entry_i)
	{
		entry_i->Delete();
		*entry_i = entry;
	}
	else
		entries_.push_back(entry);
}

void Btdb::RemoveEntry(const char *title)
{
	const vector<BtdbEntry>::iterator entry_i(find(entries_.begin(), entries_.end(), title));
	if (entries_.end() != entry_i)
	{
		entry_i->Delete();
		entries_.erase(entry_i);
	}
}

void Btdb::Flush()
{
	// create a new btdb
	ofstream btdb_out(btdb_file_.c_str(), std::ios::ios_base::binary);
	// write number of entries_
	{
		DWORD size = entries_.size();
		btdb_out.write(ri_cast<char*>(&size), 4);
	}
	vector<BtdbEntry>::const_iterator i(entries_.begin());
	const vector<BtdbEntry>::const_iterator end(entries_.end());
	for (; i != end; ++i)
	{
		// write title_
		btdb_out.write(ri_cast<const char*>(&i->title_size_), 4);
		btdb_out.write(i->title_, i->title_size_);
		// write conent
		btdb_out.write(ri_cast<const char*>(&i->content_size_), 4);
		btdb_out.write(i->content_, i->content_size_);
		// write auxillary_
		btdb_out.write(ri_cast<const char*>(&i->auxillary_size_), 4);
		btdb_out.write(i->auxillary_, i->auxillary_size_);
	}
	btdb_out.close();
}

//-------------------------------
// BTDB::BtdbEntry implementation
//-------------------------------
Btdb::BtdbEntry::BtdbEntry()
	:title_    (NULL)
	,content_  (NULL)
	,auxillary_(NULL)
{}

bool Btdb::BtdbEntry::operator == (const char *title) const
{
	const size_t title_size(strlen(title) + 1);
	return
		title_size == title_size_ && 
		0 == memcmp(title, title_, title_size);
}

bool Btdb::BtdbEntry::operator == (const Btdb::BtdbEntry &entry) const
{
	// compare by title_ only
	return
		entry.title_size_ == title_size_ &&
		0 == memcmp(entry.title_, title_, title_size_);
}

void Btdb::BtdbEntry::Delete()
{
	delete [] title_;
	delete [] content_;
	delete [] auxillary_;
}
