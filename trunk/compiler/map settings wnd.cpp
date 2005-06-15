#include "StdAfx.h"














/*
#include "map info.h"
#include "resource.h"
#include <map>
using std::map;

//----------
// constants
//----------

const TCHAR * const section_name("world ini");

//------------------------
// CMapInfo implementation
//------------------------

CMapInfo::CMapInfo()
{
}

CMapInfo::~CMapInfo()
{
}

void CMapInfo::Default()
{
	// ASSUME critical data is initialized
	int map_width(exp2(map_power_x));
	int map_height(exp2(map_power_y));
	zero_plast = 127;
	fog_start = 8191;
	fog_end = 8191;
	fog_colour = 0L;
	start_pos[0].x = map_width / 2;
	start_pos[0].y = map_height / 2;
	start_pos[1].x = map_width / 3;
	start_pos[1].y = map_height / 3;
	start_pos[2].x = map_width / 3 * 2;
	start_pos[2].y = map_height / 3;
	start_pos[3].x = map_width / 3 * 2;
	start_pos[3].y = map_height / 3 * 2;
	start_pos[4].x = map_width / 3;
	start_pos[4].y = map_height / 3 * 2;
}

void CMapInfo::Save(const string &file_name) const
{
	const int max_string_length(64);
	TCHAR str[max_string_length];
	// save the critical information
	SaveCriticalInfo(file_name);
	// save zero_plast
	_itot(zero_plast, str, 10);
	WritePrivateProfileString(section_name, _T("hZeroPlast"), str, file_name.c_str());
	// save fog_start
	_itot(fog_start, str, 10);
	WritePrivateProfileString(section_name, _T("FogStart"), str, file_name.c_str());
	// save fog_end
	_itot(fog_end, str, 10);
	WritePrivateProfileString(section_name, _T("FogEnd"), str, file_name.c_str());
	// save fog_colour
	_stprintf(str, "%X", fog_colour);
	WritePrivateProfileString(section_name, _T("FogColor"), str, file_name.c_str());
	// save starting positions
	for (int i(0); i != 5; ++i)
	{
		_stprintf(str, "(%i, %i)", start_pos[i].x, start_pos[i].y);
		WritePrivateProfileString(section_name, GenerateStartPosName(i).c_str(), str, file_name.c_str());
	}
}

void CMapInfo::SaveCriticalInfo(const string &file_name) const
{
	const int max_string_length(32);
	TCHAR str[max_string_length];
	// save map_name
	WritePrivateProfileString(section_name, _T("Map Name"), map_name.c_str(), file_name.c_str());
	// save map_power_x
	_itot(map_power_x, str, 10);
	WritePrivateProfileString(section_name, _T("Map Power X"), str, file_name.c_str());
	// save map_power_y
	_itot(map_power_y, str, 10);
	WritePrivateProfileString(section_name, _T("Map Power Y"), str, file_name.c_str());
}

void CMapInfo::Load(string file_name)
{
	const int max_string_length(64);
	TCHAR str[max_string_length];
	TCHAR default_str[max_string_length];
	// load map name
	GetPrivateProfileString(section_name, _T("Map Name"), _T("CUSTOM MAP"), str, max_string_length, file_name.c_str());
	map_name = str;
	// load map_power_x
	GetPrivateProfileString(section_name, _T("Map Power X"), default_str, str, max_string_length, file_name.c_str());
	map_power_x = _ttoi(str);
	// load map_power_y
	GetPrivateProfileString(section_name, _T("Map Power Y"), default_str, str, max_string_length, file_name.c_str());
	map_power_y = _ttoi(str);
	// set defaults (after loading critical info)
	Default();
	// load zero_plast
	_itot(zero_plast, default_str, 10);
	GetPrivateProfileString(section_name, _T("hZeroPlast"), default_str, str, max_string_length, file_name.c_str());
	zero_plast = _ttoi(str);
	// load fog_start
	_itot(fog_start, default_str, 10);
	GetPrivateProfileString(section_name, _T("FogStart"), default_str, str, max_string_length, file_name.c_str());
	fog_start = _ttoi(str);
	// load fog_end
	_itot(fog_end, default_str, 10);
	GetPrivateProfileString(section_name, _T("FogEnd"), default_str, str, max_string_length, file_name.c_str());
	fog_end = _ttoi(str);
	// load fog_colour
	_stprintf(default_str, "%X", fog_colour);
	GetPrivateProfileString(section_name, _T("FogColor"),default_str, str, max_string_length, file_name.c_str());
	_stscanf(str, "%X", &fog_colour);
	// load starting positions
	for (int i(0); i != 5; ++i)
	{
		_ltot(start_pos[0].x, default_str, 10);
		_stprintf(default_str, "(%i, %i)", start_pos[i].x, start_pos[i].y);
		GetPrivateProfileString(
			section_name,
			GenerateStartPosName(i).c_str(),
			default_str,
			str,
			max_string_length,
			file_name.c_str());
		_stscanf(str, "(%i, %i)", &start_pos[i].x, &start_pos[i].y);
	}
}

string CMapInfo::GenerateWorldIni() const
{
	string world_ini;
	// load the template resource
	{
		HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_WORLD_INI), "Text"));
		HGLOBAL resource(LoadResource(NULL, resource_info));
		char *text(ri_cast<char*>(LockResource(resource)));
		world_ini = text;
	}
	char str[256];
	// replace map_power_x
	_itot(map_power_x, str, 10);
	ReplaceSubstring(world_ini, "%map_power_x%", str);
	// replace map_power_y
	_itot(map_power_y, str, 10);
	ReplaceSubstring(world_ini, "%map_power_y%", str);
	// replace zero_plast
	_itot(zero_plast, str, 10);
	ReplaceSubstring(world_ini, "%zero_plast%", str);
	// replace fog_start
	_itot(fog_start, str, 10);
	ReplaceSubstring(world_ini, "%fog_start%", str);
	// replace fog_end
	_itot(fog_end, str, 10);
	ReplaceSubstring(world_ini, "%fog_end%", str);
	// replace fog_colour
	_stprintf(
		str,
		_T("%u %u %u"),
		(fog_colour & 0x000000FF) >> 0,
		(fog_colour & 0x0000FF00) >> 8,
		(fog_colour & 0x00FF0000) >> 16);
	ReplaceSubstring(world_ini, "%fog_colour%", str);
	return world_ini;
}

// for check sum computation
size_t CMapInfo::GetBinaryBlock(BYTE *&buffer) const
{
	// define data sizes
	map<string, size_t> size_map;
	size_map["map_name size"] = sizeof(size_t);
	size_map["map_name"]      = sizeof(TCHAR) * map_name.size();
	size_map["map_power_x"]   = sizeof(unsigned int);
	size_map["map_power_y"]   = sizeof(unsigned int);
	size_map["zero_plast"]    = sizeof(unsigned int);
	size_map["fog_start"]     = sizeof(unsigned int);
	size_map["fog_end"]       = sizeof(unsigned int);
	size_map["fog_colour"]    = sizeof(COLORREF);
	size_map["start_pos"]     = 5 * sizeof(CPos);
	// calculate total size
	size_t buffer_size(0);
	{
		      map<string, size_t>::const_iterator i(size_map.begin());
		const map<string, size_t>::const_iterator end(size_map.end());
		for (; i != end; ++i)
			buffer_size += i->second;
		size_t remainder(buffer_size % 4);
		if (remainder != 0)
			buffer_size += 4 - remainder;
	}
	// allocate memory
	buffer = new BYTE[buffer_size]; // start_pos
	ZeroMemory(buffer, buffer_size);
	BYTE *buffer_i(buffer);
	// map_name size
	{
		size_t map_name_size(map_name.size());
		CopyMemory(buffer_i, &map_name_size, size_map["map_name size"]);
		buffer_i += size_map["map_name_size"];
	}
	// map_name
	CopyMemory(buffer_i, map_name.c_str(), size_map["map_name"]);
	buffer_i += size_map["map_name"];
	// map_power_x
	CopyMemory(buffer_i, &map_power_x, size_map["map_power_x"]);
	buffer_i += size_map["map_power_x"];
	// map_power_y
	CopyMemory(buffer_i, &map_power_y, size_map["map_power_y"]);
	buffer_i += size_map["map_power_y"];
	// zero_plast
	CopyMemory(buffer_i, &zero_plast, size_map["zero_plast"]);
	buffer_i += size_map["zero_plast"];
	// fog_start
	CopyMemory(buffer_i, &fog_start, size_map["fog_start"]);
	buffer_i += size_map["fog_start"];
	// fog_end
	CopyMemory(buffer_i, &fog_end, size_map["fog_end"]);
	buffer_i += size_map["fog_end"];
	// fog_colour
	CopyMemory(buffer_i, &fog_colour, size_map["fog_colour"]);
	buffer_i += size_map["fog_colour"];
	// fog_colour
	CopyMemory(buffer_i, start_pos, size_map["start_pos"]);
	buffer_i += size_map["start_pos"];
	return buffer_size;
}

void CMapInfo::SaveToXml(TiXmlNode &node) const
{
	const int max_str_length(16);
	TCHAR str[max_str_length];
	// map_name
	node.InsertEndChild(TiXmlElement("map_name"))->InsertEndChild(TiXmlText(map_name));
	// map_power_x
	itoa(map_power_x, str, 10);
	node.InsertEndChild(TiXmlElement("map_power_x"))->InsertEndChild(TiXmlText(str));
	// map_power_y
	itoa(map_power_y, str, 10);
	node.InsertEndChild(TiXmlElement("map_power_y"))->InsertEndChild(TiXmlText(str));
	// zero_plast
	itoa(zero_plast, str, 10);
	node.InsertEndChild(TiXmlElement("zero_plast"))->InsertEndChild(TiXmlText(str));
	// fog_start
	itoa(fog_start, str, 10);
	node.InsertEndChild(TiXmlElement("fog_start"))->InsertEndChild(TiXmlText(str));
	// fog_end
	itoa(fog_end, str, 10);
	node.InsertEndChild(TiXmlElement("fog_end"))->InsertEndChild(TiXmlText(str));
	// fog_colour
	ltoa(fog_colour, str, 10);
	node.InsertEndChild(TiXmlElement("fog_colour"))->InsertEndChild(TiXmlText(str));
	// start_pos
	for (size_t i(0); i != 5; ++i)
	{
		// start_pos[i].x
		itoa(start_pos[i].x, str, 10);
		node.InsertEndChild(TiXmlElement(GenerateStartPosName(i, true)))->InsertEndChild(TiXmlText(str));
		// start_pos[i].y
		itoa(start_pos[i].y, str, 10);
		node.InsertEndChild(TiXmlElement(GenerateStartPosName(i, false)))->InsertEndChild(TiXmlText(str));
	}
}

void CMapInfo::LoadFromXml(TiXmlNode *node)
{
	// set all values to default
	Default();
	// read data from XML
	TiXmlNode *text_node;
	TiXmlHandle node_handle(node);
	// zero_plast
	text_node = node_handle.FirstChildElement("zero_plast").FirstChild().Text();
	if (NULL != text_node)
		zero_plast = atoi(text_node->Value());
	// fog_start
	text_node = node_handle.FirstChildElement("fog_start").FirstChild().Text();
	if (NULL != text_node)
		fog_start = atoi(text_node->Value());
	// fog_end
	text_node = node_handle.FirstChildElement("fog_end").FirstChild().Text();
	if (NULL != text_node)
		fog_end = atoi(text_node->Value());
	// fog_colour
	text_node = node_handle.FirstChildElement("fog_colour").FirstChild().Text();
	if (NULL != text_node)
		fog_colour = atol(text_node->Value());
	// start_pos
	for (size_t i(0); i != 5; ++i)
	{
		// start_pos[i].x
		text_node = node_handle.FirstChildElement(GenerateStartPosName(i, true)).FirstChild().Text();
		if (NULL != text_node)
			start_pos[i].x = atoi(text_node->Value());
		// start_pos[i].y
		text_node = node_handle.FirstChildElement(GenerateStartPosName(i, false)).FirstChild().Text();
		if (NULL != text_node)
			start_pos[i].y = atoi(text_node->Value());
	}
}

string CMapInfo::GenerateStartPosName(int index, bool x) const
{
	string name(_T("starting_position_"));
	TCHAR index_string[16];
	_itot(index, index_string, 10);
	name.append(index_string);
	name.append(x ? _T("_x") : _T("_y"));
	return name;
}

string CMapInfo::GenerateStartPosName(int index) const
{
	string name(_T("starting_position_"));
	TCHAR index_string[16];
	_itot(index, index_string, 10);
	name.append(index_string);
	return name;
}

void CMapInfo::ReplaceSubstring(string &str, const string &target, const string &replacement) const
{
	typedef string::size_type size_t;
	size_t i(str.find(target));
	str = str.replace(i, target.size(), replacement);
}

inline int CMapInfo::exp2(unsigned int n) const
{
	int e(1);
	while (0 != n)
	{
		e <<= 1;
		--n;
	}
	return e;
}
*/