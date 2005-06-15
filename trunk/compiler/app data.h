#pragma once

#include "static serializer.h"

//-----------------------------------------------------
// application data for user with the static serializer
//-----------------------------------------------------

class ApplicationData
{
public:
	// array for referencing the variables
	enum
	{
		ID_RESOLUTION = 0,
		ID_STAT_WND_RECT,
		ID_STAT_WND_VISIBLE,
		ID_PREVIEW_WND_RECT,
		ID_PREVIEW_WND_VISIBLE,
		ID_INFO_WND_RECT,
		ID_INFO_WND_VISIBLE,
		ID_MAIN_WND_RECT,
		ID_THRESHOLD,
		ID_ZERO_LAYER_COLOUR,
		ID_ZERO_LAYER_OPACITY,
		ID_OPACITY,
		ID_ENABLE_SWAP,
		ID_FAST_TEXTURE_QUANTIZATION,
		ID_ENABLE_LIGHTING,
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
			_T("resolution"),
			_T("stat wnd rect"),
			_T("stat wnd visible"),
			_T("preview wnd rect"),
			_T("preview wnd visible"),
			_T("info wnd rect"),
			_T("info wnd visible"),
			_T("main wnd rect"),
			_T("threshold"),
			_T("colour"),
			_T("zero layer opacity"),
			_T("opacity"),
			_T("enable swap"),
			_T("fast texture quantization"),
			_T("enable lighting")
		};
		return keys_table[i];
	}

	// sections for the variables to be used in serialization
	static const char * sections(uint i)
	{
		static const char * const sections_table[count_] =
		{
			_T("Windows"),
			_T("Windows"),
			_T("Windows"),
			_T("Windows"),
			_T("Windows"),
			_T("Windows"),
			_T("Windows"),
			_T("Windows"),
			_T("Map Preview Settings"),
			_T("Map Preview Settings"),
			_T("Map Preview Settings"),
			_T("Map Preview Settings"),
			_T("Map Preview Settings"),
			_T("Project Settings"),
			_T("Project Settings")
		};	
		return sections_table[i];
	}

	// data
	MacroSSVar2(ID_RESOLUTION, SIZE, 1024, 768);
	MacroSSVar4(ID_STAT_WND_RECT, RECT, 51, 220, 189, 300);
	MacroSSVar (ID_STAT_WND_VISIBLE, bool, true);
	MacroSSVar4(ID_PREVIEW_WND_RECT, RECT, 210, 21, 1016, 747);
	MacroSSVar (ID_PREVIEW_WND_VISIBLE, bool, true);
	MacroSSVar4(ID_INFO_WND_RECT, RECT, 10, 368, 198, 669);
	MacroSSVar (ID_INFO_WND_VISIBLE, bool, true);
	MacroSSVar4(ID_MAIN_WND_RECT, RECT, 60, 21, 186, 197);
	MacroSSVar (ID_THRESHOLD, float, 0.3f);
	MacroSSVar (ID_ZERO_LAYER_COLOUR, COLORREF, 0L);
	MacroSSVar (ID_ZERO_LAYER_OPACITY, uint, 40u);
	MacroSSVar (ID_OPACITY, int, 40);
	MacroSSVar (ID_ENABLE_SWAP, bool, true);
	MacroSSVar (ID_FAST_TEXTURE_QUANTIZATION, bool, true);
	MacroSSVar (ID_ENABLE_LIGHTING, bool, true);
};

typedef StaticSerializer<ApplicationData> SSAppData;
#define MacroAppData(id) SSAppData::Get<SSAppData::id>()