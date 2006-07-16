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
// • Neither the name of Don Reba nor the names of his contributors may be used
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

#include "error handler.h"
#include "task resources.h"

#include <utility>

struct TextureAllocation;
struct SimpleVertex;

//------------------------------------------------------------
// some functions used by the Tasks, but also useful to others
//------------------------------------------------------------

namespace TaskCommon
{
	//--------
	// file IO
	//--------

	DWORD LoadFile(const LPCTSTR name, BYTE *&pBuffer, ErrorHandler &error_handler);
	bool  SaveMemToFile(LPCTSTR path, const BYTE *buffer, DWORD size, ErrorHandler &error_handler);
	void  SavePalette(Texture &texture, LPCTSTR path, ErrorHandler &error_handler);
	void  SaveSPG(
		MapInfo      &map_info,
		LPCTSTR       path,
		LPCTSTR       folder_name,
		const bool    survival,
		ErrorHandler &error_handler);
	void  SaveSPG2(
		MapInfo      &map_info,
		LPCTSTR       path,
		LPCTSTR       folder_name,
		const bool    survival,
		ErrorHandler &error_handler);
	void  SaveSPH(
		LPCTSTR path,
		LPCTSTR folder_name,
		const bool survival,
		ErrorHandler &error_handler);
	bool  SaveThumb(
		const Heightmap &heightmap,
		const Lightmap  &lightmap,
		const Texture   &texture,
		LPCTSTR          path,
		SIZE             size,
		ErrorHandler    &error_handler);
	void  SaveVMP(
		const Heightmap &heightmap,
		const Texture   &texture,
		const ZeroLayer *zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler);
	void  SaveVMP8(
		const Heightmap &heightmap,
		const Texture   &texture,
		const ZeroLayer *zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler);
	void  SaveVMP16(
		const Heightmap &heightmap,
		const Texture   &texture,
		const ZeroLayer *zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler);

	//-----------------------------------
	// "incredible math" (Lithium Flower)
	//-----------------------------------

	COLORREF AverageColour  (const Texture &texture, const Heightmap &heightmap);
	COLORREF AverageColour8 (const Texture &texture, const Heightmap &heightmap);
	COLORREF AverageColour16(const Texture &texture, const Heightmap &heightmap);
	float    AverageHeight  (const Heightmap &heightmap);
	float    AverageHeight8 (const Heightmap &heightmap);
	float    AverageHeight16(const Heightmap &heightmap);
	DWORD    CalculateChecksum(BYTE *data, size_t size, DWORD seed);
	void     CreateTextures(
		const Texture     &texture,
		TextureAllocation &allocation,
		const Lightmap    &lightmap,
		bool               enable_lighting);
	void     CreateTextures(
		const Hardness    &hardness,
		TextureAllocation &allocation,
		const Lightmap    &lightmap,
		bool               enable_lighting);
	void     CreateTextures(
		const ZeroLayer   &zero_layer,
		TextureAllocation &allocation,
		const Lightmap    &lightmap,
		bool               enable_lighting);
	float    Flatness(float h1, float h2, float v1, float v2, float c, float r);
	void     ReplaceSubstringSeq(
		LPCTSTR                                     str,
		size_t                                      str_length,
		const vector<std::pair<tstring, tstring> > &seq,
		tstring                                    &result);
	void     Triangulate(
		const Heightmap &heightmap,
		vector<SimpleVertex> &vertices,
		float mesh_threshold);
	
	//------
	// other
	//------

	bool RegisterMap(LPCTSTR map_name, DWORD checksum, ErrorHandler &error_handler);
}

//-------------------------
// template implementations
//-------------------------

namespace TaskCommon
{
	template<typename HMType>
	COLORREF AverageColour(const Texture &texture, const HMType &heightmap)
	{
		const BYTE *texture_iter;
		const BYTE *heightmap_iter;
		const size_t num_colors(256);
		int color_count[num_colors];
		const int *color_iter;
		uint half_size(0);
		uint sum;
		LONG r, c; // row, column
		// count the number of non-null pixels of the heightmap
		heightmap_iter = heightmap.data_;
		for (r = 0; r != texture.size_.cy; ++r)
		{
			for (c = 0; c != texture.size_.cx; ++c)
				if (0 != *heightmap_iter++)
					++half_size;
			++heightmap_iter;
		}
		half_size /= 2;
		// count the number of occurences of each value of blue
		ZeroMemory(color_count, num_colors * sizeof(int));
		heightmap_iter = heightmap.data_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.size_.cy; ++r)
		{
			for (c = 0; c != texture.size_.cx; ++c)
			{
				if (0 != *heightmap_iter)
					++color_count[GetBValue(texture.palette_[*texture_iter])];
				++heightmap_iter;
				++texture_iter;
			}
			++heightmap_iter;
		}
		// find the median blue
		BYTE median_b;
		sum = 0;
		color_iter = color_count;
		do
		{
			_ASSERTE(color_iter != color_count + num_colors);
			sum += *color_iter;
			++color_iter;
		} while (sum < half_size);
		--color_iter;
		median_b = static_cast<BYTE>(color_iter - color_count);
		// count the number of occurences of each value of green
		ZeroMemory(color_count, num_colors * sizeof(int));
		heightmap_iter = heightmap.data_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.size_.cy; ++r)
		{
			for (c = 0; c != texture.size_.cx; ++c)
			{
				if (0 != *heightmap_iter)
					++color_count[GetGValue(texture.palette_[*texture_iter])];
				++heightmap_iter;
				++texture_iter;
			}
			++heightmap_iter;
		}
		// find the median green
		BYTE median_g;
		sum = 0;
		color_iter = color_count;
		do
		{
			_ASSERTE(color_iter != color_count + num_colors);
			sum += *color_iter;
			++color_iter;
		} while (sum < half_size);
		--color_iter;
		median_g = static_cast<BYTE>(color_iter - color_count);
		// count the number of occurences of each value of green
		ZeroMemory(color_count, num_colors * sizeof(int));
		heightmap_iter = heightmap.data_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.size_.cy; ++r)
		{
			for (c = 0; c != texture.size_.cx; ++c)
			{
				if (0 != *heightmap_iter)
					++color_count[GetRValue(texture.palette_[*texture_iter])];
				++heightmap_iter;
				++texture_iter;
			}
			++heightmap_iter;
		}
		// find the median red
		BYTE median_r;
		sum = 0;
		color_iter = color_count;
		do
		{
			_ASSERTE(color_iter != color_count + num_colors);
			sum += *color_iter;
			++color_iter;
		} while (sum < half_size);
		--color_iter;
		median_r = static_cast<BYTE>(color_iter - color_count);
		return RGB(median_r, median_g, median_b);
	}
}
