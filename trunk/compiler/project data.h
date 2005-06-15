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
			"starting_position_4"
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
			_T("world ini")
		};	
		return sections_table[i];
	}

	// data
	MacroSSVar (ID_MAP_NAME,   tstring,  _T(""));
	MacroSSVar (ID_POWER_X,    uint,     0);
	MacroSSVar (ID_POWER_Y,    uint,     0);
	MacroSSVar (ID_ZERO_LEVEL, uint,     0);
	MacroSSVar (ID_FOG_START,  uint,     8192);
	MacroSSVar (ID_FOG_END,    uint,     8192);
	MacroSSVar (ID_FOG_COLOUR, COLORREF, 0L);
	MacroSSVar2(ID_SP_0,       POINT,    0, 0);
	MacroSSVar2(ID_SP_1,       POINT,    0, 0);
	MacroSSVar2(ID_SP_2,       POINT,    0, 0);
	MacroSSVar2(ID_SP_3,       POINT,    0, 0);
	MacroSSVar2(ID_SP_4,       POINT,    0, 0);
};

typedef StaticSerializer<ProjectData> SSProjectData;
#define MacroProjectData(id) SSProjectData::Get<SSProjectData::id>()