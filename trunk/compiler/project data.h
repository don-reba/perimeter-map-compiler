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

#include "static serializer.h"

//-----------------------------------------------------
// application data for user with the static serializer
//-----------------------------------------------------

class ProjectData
{
public:
	// array for referencing the variables
	enum
	{
		ID_MAP_NAME,
		ID_POWER_X,
		ID_POWER_Y,
		ID_ZERO_LEVEL,
		ID_FOG_START,
		ID_FOG_END,
		ID_FOG_COLOUR,
		ID_SP_0,
		ID_SP_1,
		ID_SP_2,
		ID_SP_3,
		ID_SP_4,
		ID_CUSTOM_HARDNESS,
		ID_CUSTOM_ZERO_LAYER,
		ID_CUSTOM_SURFACE,
		ID_CUSTOM_SKY,
		count_
	};
protected:
	template <uint id>
	struct Datum {};

	// names of the variables to be used in serialization
	static const char * keys(uint i)
	{
		static const char * const keys_table[count_] =
		{
			"Map Name",
			"Map Power X",
			"Map Power Y",
			"hZeroPlast",
			"FogStart",
			"FogEnd",
			"FogColor",
			"starting_position_0",
			"starting_position_1",
			"starting_position_2",
			"starting_position_3",
			"starting_position_4",
			"custom hardness map",
			"custom zero layer",
			"custom surface texture",
			"custom sky texture"
		};
		return keys_table[i];
	}

	// sections for the variables to be used in serialization
	static const char * sections(uint i)
	{
		static const char * const sections_table[count_] =
		{
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("world ini"),
			_T("custom resources"),
			_T("custom resources"),
			_T("custom resources"),
			_T("custom resources")
		};	
		return sections_table[i];
	}

	// data
	MacroSSVar (ID_MAP_NAME,          tstring,  _T(""));
	MacroSSVar (ID_POWER_X,           uint,     0);
	MacroSSVar (ID_POWER_Y,           uint,     0);
	MacroSSVar (ID_ZERO_LEVEL,        uint,     0);
	MacroSSVar (ID_FOG_START,         uint,     8192);
	MacroSSVar (ID_FOG_END,           uint,     8192);
	MacroSSVar (ID_FOG_COLOUR,        COLORREF, 0L);
	MacroSSVar2(ID_SP_0,              POINT,    0, 0);
	MacroSSVar2(ID_SP_1,              POINT,    0, 0);
	MacroSSVar2(ID_SP_2,              POINT,    0, 0);
	MacroSSVar2(ID_SP_3,              POINT,    0, 0);
	MacroSSVar2(ID_SP_4,              POINT,    0, 0);
	MacroSSVar (ID_CUSTOM_HARDNESS,   bool, false);
	MacroSSVar (ID_CUSTOM_ZERO_LAYER, bool, false);
	MacroSSVar (ID_CUSTOM_SURFACE,    bool, false);
	MacroSSVar (ID_CUSTOM_SKY,        bool, false);
};

typedef StaticSerializer<ProjectData> SSProjectData;
#define MacroProjectData(id) SSProjectData::Get<SSProjectData::id>()
