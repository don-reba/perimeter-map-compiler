#pragma once

class Btdb
{
private:
	// one entry of a btdb file
	struct BtdbEntry
	{
	// functions
		BtdbEntry();
		bool operator == (const char *title) const;
		bool operator == (const Btdb::BtdbEntry &entry) const;
		void Delete();
	//data
		char *auxillary_;
		DWORD auxillary_size_;
		char *content_;
		DWORD content_size_;
		char *title_;
		DWORD title_size_;
	};
public:
// construction/destruction
	Btdb(LPCTSTR btdb_file);
	~Btdb();
// interface
	void AddMapEntry(const string &title, const string &content);
	void RemoveMapEntry(const string &title);
	void Flush();
private:
// utilities
	void AddEntry(const BtdbEntry &entry);
	void RemoveEntry(const char *title);
private:
// data
	vector<BtdbEntry> entries_;
	string            btdb_file_;
	bool              is_open_;
};
