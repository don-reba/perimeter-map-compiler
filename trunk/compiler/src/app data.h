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
		ID_DISPLAY_HARDNESS,
		ID_DISPLAY_TEXTURE,
		ID_DISPLAY_ZERO_LAYER,
		ID_ENABLE_SWAP,
		ID_FAST_TEXTURE_QUANTIZATION,
		ID_ENABLE_LIGHTING,
		ID_PERIMETER_PATH,
		ID_PERIMETER_VERSION,
		ID_RENAME_TO_UNREGISTERED,
		ID_USE_REGISTRATION,
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
			_T("display heightmap"),
			_T("display texture"),
			_T("display zero layer"),
			_T("enable swap"),
			_T("fast texture quantization"),
			_T("enable lighting"),
			_T("perimeter path"),
			_T("perimeter version"),
			_T("rename to unregistered"),
			_T("use registration")
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
			_T("Map Preview Settings"),
			_T("Map Preview Settings"),
			_T("Map Preview Settings"),
			_T("Project Settings"),
			_T("Project Settings"),
			_T("Miscelleneous"),
			_T("Miscelleneous"),
			_T("Miscelleneous"),
			_T("Miscelleneous")
		};	
		return sections_table[i];
	}
	// data
	MacroSSVar2(ID_RESOLUTION, SIZE, 1024, 768);
	MacroSSVar4(ID_STAT_WND_RECT, RECT, 245, 25, 383, 105);
	MacroSSVar (ID_STAT_WND_VISIBLE, bool, true);
	MacroSSVar4(ID_PREVIEW_WND_RECT, RECT, 207, 110, 1002, 671);
	MacroSSVar (ID_PREVIEW_WND_VISIBLE, bool, true);
	MacroSSVar4(ID_INFO_WND_RECT, RECT, 13, 111, 201, 412);
	MacroSSVar (ID_INFO_WND_VISIBLE, bool, true);
	MacroSSVar4(ID_MAIN_WND_RECT, RECT, 13, 17, 239, 105);
	MacroSSVar (ID_THRESHOLD, float, 3.0f);
	MacroSSVar (ID_ZERO_LAYER_COLOUR, COLORREF, 0L);
	MacroSSVar (ID_ZERO_LAYER_OPACITY, uint, 40u);
	MacroSSVar (ID_OPACITY, int, 40);
	MacroSSVar (ID_ENABLE_SWAP, bool, true);
	MacroSSVar (ID_DISPLAY_HARDNESS,   bool, false);
	MacroSSVar (ID_DISPLAY_TEXTURE,    bool, true);
	MacroSSVar (ID_DISPLAY_ZERO_LAYER, bool, false);
	MacroSSVar (ID_FAST_TEXTURE_QUANTIZATION, bool, true);
	MacroSSVar (ID_ENABLE_LIGHTING, bool, true);
	MacroSSVar (ID_PERIMETER_PATH, tstring, _T(""));
	MacroSSVar (ID_PERIMETER_VERSION, int, 0);
	MacroSSVar (ID_RENAME_TO_UNREGISTERED, bool, true);
	MacroSSVar (ID_USE_REGISTRATION,       bool, true);
};

typedef StaticSerializer<ApplicationData> SSAppData;
#define MacroAppData(id) SSAppData::Get<SSAppData::id>()