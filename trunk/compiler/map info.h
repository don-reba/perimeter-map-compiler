#pragma once

#include <tinyxml/tinyxml.h>

// structure containing auxillary map data, mainly for creation of world.ini
class CMapInfo
{
protected:
	struct CPos
	{
		unsigned int x;
		unsigned int y;
	};
public:
	// lifetime management
	CMapInfo();
	~CMapInfo();
public:
	// serialization
	void   Default();
	void   Save(const string &file_name) const;
	void   SaveCriticalInfo(const string &file_name) const;
	void   Load(string file_name);
	string GenerateWorldIni() const;
	size_t GetBinaryBlock(BYTE *buffer) const;
	void   SaveToXml(TiXmlNode &node) const;
	void   LoadFromXml(TiXmlNode *node);
protected:
	// utility functions
	string GenerateStartPosName(int index, bool x) const;
	string GenerateStartPosName(int index) const;
	void   ReplaceSubstring(string &str, const string &target, const string &replacement) const;
	// math
	inline int exp2(unsigned int n) const;
public:
	// data
	string       map_name;
	string       folder_path;
	unsigned int map_power_x;
	unsigned int map_power_y;
	unsigned int zero_plast;
	unsigned int fog_start;
	unsigned int fog_end;
	COLORREF     fog_colour;
	CPos         start_pos[5];
};