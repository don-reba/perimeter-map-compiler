#pragma once

class CBTDB
{
protected:
	// one entry of a btdb file
	struct CBtdbEntry
	{
	// functions
		CBtdbEntry();
		bool operator == (const char *title) const;
		bool operator == (const CBTDB::CBtdbEntry &entry) const;
		void Delete();
	//data
		char *title;
		char *content;
		char *auxillary;
		DWORD title_size;
		DWORD content_size;
		DWORD auxillary_size;
	};
public:
// construction/destruction
	CBTDB(const TCHAR * const btdb_file);
	~CBTDB();
// interface
	void AddMapEntry(const string &title, const string &content);
	void RemoveMapEntry(const string &title);
	void Flush();
protected:
// utilities
	void AddEntry(const CBtdbEntry &entry);
	void RemoveEntry(const char *title);
protected:
// data
	vector<CBtdbEntry> entries;
	string             btdb_file;
	bool               is_open;
};
