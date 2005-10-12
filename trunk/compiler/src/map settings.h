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
	size_t GetBinaryBlock(BYTE *&buffer) const;
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