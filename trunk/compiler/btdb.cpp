#include "StdAfx.h"
#include "btdb.h"
#include <algorithm>
#include <fstream>
using std::ifstream;
using std::ofstream;
using std::find;

//---------------------
// CBTDB implementation
//---------------------
CBTDB::CBTDB(const TCHAR * const btdb_file)
{
	DWORD size(0);
	CBtdbEntry entry;
	// save the path
	this->btdb_file = btdb_file;
	// read in the contents of btdb into an array
	ifstream btdb_in(btdb_file, std::ios::ios_base::binary);
	is_open = btdb_in.is_open();
	if (!is_open)
		return;
	btdb_in.read(ri_cast<char*>(&size), 4);
	for (DWORD i(0); i != size; ++i)
	{
		// read in title
		btdb_in.read(ri_cast<char*>(&entry.title_size), 4);
		entry.title = new char[entry.title_size];
		btdb_in.read(entry.title, entry.title_size);
		// read in content
		btdb_in.read(ri_cast<char*>(&entry.content_size), 4);
		entry.content = new char[entry.content_size];
		btdb_in.read(entry.content, entry.content_size);
		// read in auxillary
		btdb_in.read(ri_cast<char*>(&entry.auxillary_size), 4);
		entry.auxillary = new char[entry.auxillary_size];
		btdb_in.read(entry.auxillary, entry.auxillary_size);
		// add the entry to the arrray
		entries.push_back(entry);
	}
	btdb_in.close();
}

CBTDB::~CBTDB()
{
	Flush();
	vector<CBtdbEntry>::iterator i(entries.begin());
	const vector<CBtdbEntry>::const_iterator end(entries.end());
	for (; i != end; ++i)
		i->Delete();
}

void CBTDB::AddMapEntry(const string &title, const string &content)
{
	CBtdbEntry entry;
	// create title
	const char * const title_prefix("MapNames.");
	const size_t title_prefix_size(strlen(title_prefix));
	entry.title_size = title_prefix_size + title.size() + 1;
	entry.title = new char[entry.title_size];
	strcpy(entry.title, title_prefix);
	strcpy(entry.title + title_prefix_size, title.c_str());
	// create content
	entry.content_size = content.size() + 1;
	entry.content = new char[entry.content_size];
	strcpy(entry.content, content.c_str());
	// create auxillary
	entry.auxillary_size = 1;
	entry.auxillary = new char[entry.auxillary_size];
	*entry.auxillary = '\0';
	// add the entry
	AddEntry(entry);
}

void CBTDB::RemoveMapEntry(const string &title)
{
	const char * const title_prefix("MapNames.");
	const size_t title_prefix_size(strlen(title_prefix));
	char *char_title(new char[title_prefix_size + title.size() + 1]);
	strcpy(char_title, title_prefix);
	strcpy(char_title + title_prefix_size, title.c_str());
	RemoveEntry(char_title);
}

void CBTDB::AddEntry(const CBtdbEntry &entry)
{
	vector<CBtdbEntry>::iterator entry_i(find(entries.begin(), entries.end(), entry));
	if (entries.end() != entry_i)
	{
		entry_i->Delete();
		*entry_i = entry;
	}
	else
		entries.push_back(entry);
}

void CBTDB::RemoveEntry(const char *title)
{
	const vector<CBtdbEntry>::iterator entry_i(find(entries.begin(), entries.end(), title));
	if (entries.end() != entry_i)
	{
		entry_i->Delete();
		entries.erase(entry_i);
	}
}

void CBTDB::Flush()
{
	// create a new btdb
	ofstream btdb_out(btdb_file.c_str(), std::ios::ios_base::binary);
	// write number of entries
	{
		DWORD size = entries.size();
		btdb_out.write(ri_cast<char*>(&size), 4);
	}
	vector<CBtdbEntry>::const_iterator i(entries.begin());
	const vector<CBtdbEntry>::const_iterator end(entries.end());
	for (; i != end; ++i)
	{
		// write title
		btdb_out.write(ri_cast<const char*>(&i->title_size), 4);
		btdb_out.write(i->title, i->title_size);
		// write conent
		btdb_out.write(ri_cast<const char*>(&i->content_size), 4);
		btdb_out.write(i->content, i->content_size);
		// write auxillary
		btdb_out.write(ri_cast<const char*>(&i->auxillary_size), 4);
		btdb_out.write(i->auxillary, i->auxillary_size);
	}
	btdb_out.close();
}

//---------------------------------
// CBTDB::CBtdbEntry implementation
//---------------------------------
CBTDB::CBtdbEntry::CBtdbEntry()
	:title    (NULL)
	,content  (NULL)
	,auxillary(NULL)
{}

bool CBTDB::CBtdbEntry::operator == (const char *title) const
{
	return 0 == strcmp(title, this->title);
}

bool CBTDB::CBtdbEntry::operator == (const CBTDB::CBtdbEntry &entry) const
{
	// compare by title only
	return
		entry.title_size == title_size &&
		0 == memcmp(entry.title, title, title_size);
}

void CBTDB::CBtdbEntry::Delete()
{
	delete [] title;
	delete [] content;
	delete [] auxillary;
}