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


#include "stdafx.h"

#include "resource.h"
#include "preview wnd.h"
#include "project data.h"
#include "resource management.h"
#include "task common.h"
#include "task resources.h"

#include "gaussian blur.ipp"

#include <fstream>
#include <sstream>
#include <Wininet.h>

using namespace RsrcMgmt;

namespace TaskCommon
{
	//---------
	// defaults
	//---------

	void DefaultTexture(BYTE *buffer, COLORREF palette[256], SIZE size)
	{
		ZeroMemory(buffer, size.cx * size.cy);
		FillMemory(palette, 256, 0xFF);
	}

	//-------
	// saving
	//-------

	DWORD LoadFile(const LPCTSTR name, BYTE *&pBuffer, ErrorHandler &error_handler)
	{
		DWORD dwDesiredAccess       = FILE_READ_DATA;
		DWORD dwShareMode           = FILE_SHARE_READ;
		DWORD dwCreationDisposition = OPEN_EXISTING;
		DWORD dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
		// open the file
		HANDLE hFile = CreateFile(
			name,
			dwDesiredAccess,
			dwShareMode,
			NULL,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return 0;
		// querry file size
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		if (INVALID_FILE_SIZE == dwFileSize)
		{
			CloseHandle(hFile);
			error_handler.MacroDisplayError("GetFileSize failed");
			return 0;
		}
		// load file into memory
		pBuffer = new BYTE[dwFileSize];
		DWORD NumberOfBytesRead;
		if (0 == ReadFile(
			hFile,
			pBuffer,
			dwFileSize,
			&NumberOfBytesRead,
			NULL) || NumberOfBytesRead != dwFileSize)
		{
			CloseHandle(hFile);
			error_handler.MacroDisplayError("ReadFile failed");
			return 0;
		}
		CloseHandle(hFile);
		return dwFileSize;
	}
	
	bool SaveMemToFile(LPCTSTR path, const BYTE *buffer, DWORD size, ErrorHandler &error_handler)
	{
		// create the file
		DWORD dwDesiredAccess       = GENERIC_WRITE;
		DWORD dwShareMode           = 0;
		DWORD dwCreationDisposition = CREATE_ALWAYS;
		DWORD dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL;
		// open the file
		HANDLE hFile = CreateFile(
			path,
			dwDesiredAccess,
			dwShareMode,
			NULL,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			error_handler.MacroDisplayError(_T("CreateFile failed"));
			return false;
		}
		DWORD num_bytes_written;
		if (FALSE == WriteFile(
			hFile,
			buffer,
			size,
			&num_bytes_written,
			NULL))
		{
			CloseHandle(hFile);
			error_handler.MacroDisplayError(_T("WriteFile failed"));
			return false;
		}
		CloseHandle(hFile);
		return true;
	}
	
	// save texture palette
	// TODO: is copying even necessary?
	void SavePalette(Texture &texture, LPCTSTR path, ErrorHandler &error_handler)
	{
		// allocate memory
		const size_t palette_size(768);
		BYTE *image_palette(new BYTE[palette_size]);
		BYTE *palette_iter(image_palette);
		// fill the palette
		const COLORREF *texture_palette_iter(texture.palette_);
		for (int i(0); i != 256; ++i)
		{
			*palette_iter++ = GetRValue(*texture_palette_iter);
			*palette_iter++ = GetGValue(*texture_palette_iter);
			*palette_iter++ = GetBValue(*texture_palette_iter);
			++texture_palette_iter;
		}
		// save the palette to a file
		SaveMemToFile(path, image_palette, palette_size, error_handler);
		delete [] image_palette;
	}

	// v 1.01
	void SaveSPG(
		MapInfo      &map_info,
		LPCTSTR       path,
		LPCTSTR       folder_name,
		const bool    survival,
		ErrorHandler &error_handler)
	{
		// create a replacement sequence
		vector<std::pair<tstring, tstring> > seq;
		{
			const size_t pos_range_start(survival ? 0 : 1);
			const size_t pos_range_end  (survival ? 1 : 5);
			// folder name
			seq.push_back(std::make_pair(_T("%folder_name%"), folder_name));
			// frame and squad positions
			for (size_t i(pos_range_start); i != pos_range_end; ++i)
			{
				{
					tostringstream stream1, stream2;
					stream1 << _T("%frame_position_") << i << _T("%");
					stream2 << map_info.sps_[i].x << _T(" ") << map_info.sps_[i].y << " " << 256;
					seq.push_back(std::make_pair(stream1.str(), stream2.str()));
					seq.push_back(std::make_pair(stream1.str(), stream2.str()));
				}
				{
					tostringstream stream1, stream2;
					stream1 << _T("%squad_position_") << i << _T("%");
					stream2 << map_info.sps_[i].x << _T(" ") << map_info.sps_[i].y;
					seq.push_back(std::make_pair(stream1.str(), stream2.str()));
				}
			}
			// camera positions
			for (size_t i(pos_range_start); i != pos_range_end; ++i)
			{
				tostringstream stream1, stream2;
				stream1 << _T("%camera_position_") << i << _T("%");
				stream2  << map_info.sps_[i].x << " " << map_info.sps_[i].y;
				seq.push_back(std::make_pair(stream1.str(), stream2.str()));
			}
		}
		vector<TCHAR> spg_string;
		const size_t spg_alloc(8388608); // 8 MB should be enough
		spg_string.resize(spg_alloc);
		// get the spg string from resources
		if (!UncompressResource(survival ? IDR_SURVIVAL_SPG : IDR_SPG, ri_cast<BYTE*>(&spg_string[0]), spg_alloc))
		{
			error_handler.MacroDisplayError(_T("SPG resource could not be loaded."));
			return;
		}
		// create an output stream
		std::ofstream spg_out(path, std::ios_base::binary | std::ios_base::out);
		tstring result;
		ReplaceSubstringSeq(&spg_string[0], spg_string.size(), seq, result);
		spg_out << result;
	}

	// v 2.00
	void SaveSPG2(
		MapInfo      &map_info,
		LPCTSTR       path,
		LPCTSTR       folder_name,
		const bool    survival,
		ErrorHandler &error_handler)
	{
		// create a replacement sequence
		vector<std::pair<tstring, tstring> > seq;
		{
			const size_t pos_range_start(survival ? 0 : 1);
			const size_t pos_range_end  (survival ? 1 : 5);
			// folder name
			seq.push_back(std::make_pair(_T("%folder_name%"), folder_name));
			seq.push_back(std::make_pair(_T("%folder_name%"), folder_name));
			// frame and squad positions
			for (size_t i(pos_range_start); i != pos_range_end; ++i)
			{
				{
					tostringstream stream1, stream2;
					stream1 << _T("%frame_position_") << i << _T("%");
					stream2 << map_info.sps_[i].x << _T(" ") << map_info.sps_[i].y << " " << 256;
					seq.push_back(std::make_pair(stream1.str(), stream2.str()));
					seq.push_back(std::make_pair(stream1.str(), stream2.str()));
				}
				{
					tostringstream stream1, stream2;
					stream1 << _T("%squad_position_") << i << _T("%");
					stream2 << map_info.sps_[i].x << _T(" ") << map_info.sps_[i].y;
					seq.push_back(std::make_pair(stream1.str(), stream2.str()));
				}
			}
			// camera positions
			for (size_t i(pos_range_start); i != pos_range_end; ++i)
			{
				tostringstream stream1, stream2;
				stream1 << _T("%camera_position_") << i << _T("%");
				stream2  << map_info.sps_[i].x << " " << map_info.sps_[i].y;
				seq.push_back(std::make_pair(stream1.str(), stream2.str()));
			}
		}
		vector<TCHAR> spg_string;
		const size_t spg_alloc(8388608); // 8 MB should be enough
		spg_string.resize(spg_alloc);
		// get the spg string from resources
		if (!UncompressResource(survival ? IDR_SURVIVAL_SPG : IDR_SPG2, ri_cast<BYTE*>(&spg_string[0]), spg_alloc))
		{
			error_handler.MacroDisplayError(_T("SPG resource could not be loaded."));
			return;
		}
		// create an output stream
		std::ofstream spg_out(path, std::ios_base::binary | std::ios_base::out);
		tstring result;
		ReplaceSubstringSeq(&spg_string[0], spg_string.size(), seq, result);
		spg_out << result;
	}

	void SaveSPH(
		LPCTSTR path,
		LPCTSTR folder_name,
		const bool survival,
		ErrorHandler &error_handler)
	{
		// create a replacement sequence
		vector<std::pair<tstring, tstring> > seq;
		{
			// folder name
			seq.push_back(std::make_pair(_T("%folder_name%"), folder_name));
			// player count
			seq.push_back(std::make_pair(_T("%player_count%"), survival ? _T("1") : _T("4")));
			// map path
			if (survival)
				seq.push_back(std::make_pair(_T("%folder_name%"), tstring(_T("survival\\\\")) + folder_name));
			else
				seq.push_back(std::make_pair(_T("%folder_name%"), folder_name));
		}
		vector<TCHAR> sph_string;
		const size_t sph_alloc(8192); // 8 KB should be enough
		sph_string.resize(sph_alloc);
		// get the spg string from resources
		if (!UncompressResource(IDR_SPH, ri_cast<BYTE*>(&sph_string[0]), sph_alloc))
		{
			error_handler.MacroDisplayError(_T("SPH resource could not be loaded."));
			return;
		}
		// create an output stream
		std::ofstream sph_out(path, std::ios_base::binary | std::ios_base::out);
		tstring result;
		ReplaceSubstringSeq(&sph_string[0], sph_string.size(), seq, result);
		sph_out << result;
	}
	
	// create a thumbnail version of the map texture and save it
	bool SaveThumb(
		const Heightmap &heightmap,
		const Lightmap  &lightmap,
		const Texture   &texture,
		LPCTSTR          path,
		SIZE             size,
		ErrorHandler    &error_handler)
	{
		SIZE img_size(texture.info_.size_);
		// initialize the map preview image
		fipImage image(FIT_BITMAP, static_cast<WORD>(img_size.cx), static_cast<WORD>(img_size.cy), 24);
		// fill the image with data from the texture and from the lightmap
		{
			const BYTE *lightmap_i(lightmap.data_);
			const BYTE *texture_i(texture.indices_);
			const BYTE * const texture_end(texture_i + img_size.cx * img_size.cy);
			BYTE *image_i(image.accessPixels());
			while (texture_i != texture_end)
			{
				// TODO: get rid of the fp ops
				float f_light(static_cast<float>(*lightmap_i));
				f_light = (f_light + 128.0f) / 255.0f;
				COLORREF colour(texture.palette_[*texture_i]);
				image_i[0] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetBValue(colour) * f_light)));
				image_i[1] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetGValue(colour) * f_light)));
				image_i[2] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetRValue(colour) * f_light)));
				image_i += 3;
				++texture_i;
				++lightmap_i;
			}
		}
		// paint all pixels underneath which the heightmap is zero black
		// TODO: merge the two loops
		switch (heightmap.bpp_)
		{
		case 8:
			{
				const BYTE *heightmap_ptr(heightmap.data8_);
				BYTE *image_data(image.accessPixels());
				for (LONG r(0); r != img_size.cy; ++r)
				{
					for (int c(0); c != img_size.cx; ++c)
					{
						if (0 == *heightmap_ptr++)
							image_data[0] = image_data[1] = image_data[2] = 0;
						image_data +=3;
					}
					++heightmap_ptr;
				}
			} break;
		case 16:
			{
				const WORD *heightmap_ptr(heightmap.data16_);
				BYTE *image_data(image.accessPixels());
				for (LONG r(0); r != img_size.cy; ++r)
				{
					for (int c(0); c != img_size.cx; ++c)
					{
						if (0 == *heightmap_ptr++)
							image_data[0] = image_data[1] = image_data[2] = 0;
						image_data +=3;
					}
					++heightmap_ptr;
				}
			} break;
		}
		// resize the image
		if (FALSE == image.rescale(
			static_cast<WORD>(size.cx),
			static_cast<WORD>(size.cy),
			FILTER_BILINEAR))
		{
			error_handler.MacroDisplayError(_T("fipImage::rescale failed."));
			return false;
		}
		// flip the image vertically
		if (FALSE == image.flipVertical())
			error_handler.MacroDisplayError(_T("fipImage::flipVertical failed."));
		// save the image
		if (FALSE == image.save(path))
		{
			error_handler.MacroDisplayError(_T("fipImage::save failed."));
			return false;
		}
		return true;
	}

	// create the VMP file and save it
	void SaveVMP(
		const Heightmap &heightmap,
		const Texture   &texture,
		const ZeroLayer *zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler)
	{
		switch (heightmap.bpp_)
		{
		case 8:
			SaveVMP(
				heightmap,
				texture,
				zero_layer,
				path,
				error_handler);
			break;
		case 16:
			SaveVMP(
				heightmap,
				texture,
				zero_layer,
				path,
				error_handler);
			break;
		}
	}

	void SaveVMP8(
		const Heightmap &heightmap,
		const Texture   &texture,
		const ZeroLayer *zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler)
	{
		_ASSERTE(NULL != heightmap.data8_);
		_ASSERTE(NULL != texture.indices_);
		// get dimensions of the map
		SIZE map_size(texture.info_.size_);
		// allocate memory
		const size_t map_data_size(map_size.cx * map_size.cy);
		const size_t vmp_size(map_data_size * 4 + 20);
		BYTE *vmp(new BYTE[vmp_size]);
		BYTE *vmp_iter(vmp);
		// write the magic number
		{
			BYTE magic_number[4] = { 0x53, 0x32, 0x54, 0x30 };
			CopyMemory(vmp_iter, magic_number, 4);
			vmp_iter += 4;
		}
		// write map size
		{
			_ASSERTE(sizeof(SIZE) == 8);
			CopyMemory(vmp_iter, &map_size, 8);
			vmp_iter += 8;
		}
		// write static data
		{
			BYTE unused[8] = { 0x08, 0x82, 0x40, 0x00, 0x01, 0x00, 0x00, 0x00 };
			CopyMemory(vmp_iter, unused, 8);
			vmp_iter += 8;
		}
		// fill the geo layer with zeros
		{
			ZeroMemory(vmp_iter, map_data_size);
			vmp_iter += map_data_size;
		}
		// extract necessary heightmap data
		int *int_heightmap(NULL);
		bool *null_pixels(NULL);
		{
			// allocate memory
			const size_t heightmap_data_size(heightmap.info_.size_.cx * heightmap.info_.size_.cy);
			int_heightmap = new int [heightmap_data_size];
			null_pixels   = new bool[heightmap_data_size];
			// set iterators
					int  *       int_heightmap_iter(int_heightmap);
					bool *       null_pixels_iter  (null_pixels);
			const BYTE *       heightmap_iter    (heightmap.data8_);
			const BYTE * const heightmap_end     (heightmap_iter + heightmap_data_size);
			// main loop
			while (heightmap_iter != heightmap_end)
			{
				*null_pixels_iter   = *heightmap_iter == 0;
				*int_heightmap_iter = *heightmap_iter << 5;
				++null_pixels_iter;
				++heightmap_iter;
				++int_heightmap_iter;
			}
		}
		// interpolate heightmap
		GaussianBlur(int_heightmap, heightmap.info_.size_);
		GaussianBlur(int_heightmap, heightmap.info_.size_);
		{
			fipImage img(FIT_UINT16, (WORD)map_size.cy, (WORD)map_size.cx, 16);
			WORD *img_iter((WORD*)img.accessPixels());
			int *ihm_iter(int_heightmap);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				for (LONG c(0); c != map_size.cx; ++c)
					*img_iter++ = static_cast<WORD>(*ihm_iter++ << 3);
				++ihm_iter;
			}
			img.save("pre_blur.tiff", TIFF_NONE);
		}
		// fill the second layer with the heightmap
		{
			const int  *int_heightmap_iter(int_heightmap);
			const bool *null_pixels_iter(null_pixels);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				for (LONG c(0); c != map_size.cx; ++c)
				{
					*vmp_iter = *null_pixels_iter ? 0 : static_cast<BYTE>(*int_heightmap_iter >> 5);
					++null_pixels_iter;
					++vmp_iter;
					++int_heightmap_iter;
				}
				++int_heightmap_iter;
				++null_pixels_iter;
			}
		}
		delete [] null_pixels;
		// fill the third layer with the least significant bits of the heightmap extrapolation
		if (NULL != zero_layer)
		{
			const ZeroLayer &zero_layer_ref(*zero_layer);
			int *int_heightmap_iter(int_heightmap);
			size_t index(0);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				for (LONG c(0); c != map_size.cx; ++c)
				{
					*vmp_iter = zero_layer_ref.data_[index] ? (BYTE)(*int_heightmap_iter & 0x1F) : 0x80;
					++vmp_iter;
					++int_heightmap_iter;
					++index;
				}
				++int_heightmap_iter;
			}
		}
		else
		{
			int *int_heightmap_iter(int_heightmap);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				for (LONG c(0); c != map_size.cx; ++c)
					*vmp_iter++ = (BYTE)(*int_heightmap_iter++ & 0x1F);
				++int_heightmap_iter;
			}
		}
		delete [] int_heightmap;
		// record the texture
		{
			CopyMemory(vmp_iter, texture.indices_, map_data_size);
			vmp_iter += map_data_size;	
		}
		// create the file
		SaveMemToFile(path, vmp, vmp_size, error_handler);
		// clean up
		delete [] vmp;
	}

	void SaveVMP16(
		const Heightmap &heightmap,
		const Texture   &texture,
		const ZeroLayer *zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler)
	{
		_ASSERTE(NULL != heightmap.data16_);
		_ASSERTE(NULL != texture.indices_);
		// get dimensions of the map
		SIZE map_size(texture.info_.size_);
		// allocate memory
		const size_t map_data_size(map_size.cx * map_size.cy);
		const size_t vmp_size(map_data_size * 4 + 20);
		BYTE *vmp(new BYTE[vmp_size]);
		BYTE *vmp_iter(vmp);
		// write the magic number
		{
			BYTE magic_number[4] = { 0x53, 0x32, 0x54, 0x30 };
			CopyMemory(vmp_iter, magic_number, 4);
			vmp_iter += 4;
		}
		// write map size
		{
			_ASSERTE(sizeof(SIZE) == 8);
			CopyMemory(vmp_iter, &map_size, 8);
			vmp_iter += 8;
		}
		// write static data
		{
			BYTE unused[8] = { 0x08, 0x82, 0x40, 0x00, 0x01, 0x00, 0x00, 0x00 };
			CopyMemory(vmp_iter, unused, 8);
			vmp_iter += 8;
		}
		// fill the geo layer with zeros
		{
			ZeroMemory(vmp_iter, map_data_size);
			vmp_iter += map_data_size;
		}
		// fill the second layer with the 8 most significant bits of the heightmap
		// and the third with the 5 least significant
		// ASSUME the heightmap is 13-bit
		{
			const WORD *heightmap_iter(heightmap.data16_);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				for (LONG c(0); c != map_size.cx; ++c)
					*vmp_iter++ = static_cast<BYTE>(*heightmap_iter++ >> 5);
				++heightmap_iter;
			}
			heightmap_iter = heightmap.data16_;
			if (NULL != zero_layer)
			{
				const ZeroLayer &zero_layer_ref(*zero_layer);
				size_t index(0);
				for (LONG r(0); r != map_size.cy; ++r)
				{
					for (LONG c(0); c != map_size.cx; ++c)
					{
						*vmp_iter =
							zero_layer_ref.data_[index]
								?static_cast<BYTE>(*heightmap_iter & 0x1F)
								:0x80;
						++vmp_iter;
						++index;
						++heightmap_iter;
					}
					++heightmap_iter;
				}
			}
			else
				for (LONG r(0); r != map_size.cy; ++r)
				{
					for (LONG c(0); c != map_size.cx; ++c)
						*vmp_iter++ = static_cast<BYTE>(*heightmap_iter++ & 0x1F);
					++heightmap_iter;
				}
		}
		// record the texture
		{
			CopyMemory(vmp_iter, texture.indices_, map_data_size);
			vmp_iter += map_data_size;	
		}
		// create the file
		SaveMemToFile(path, vmp, vmp_size, error_handler);
		// clean up
		delete [] vmp;
	}

	//-----------------------------------
	// "incredible math" (Lithium Flower)
	//-----------------------------------

	COLORREF AverageColour(const Texture &texture, const Heightmap &heightmap)
	{
		switch (heightmap.bpp_)
		{
		case 8:  return AverageColour8 (texture, heightmap);
		case 16: return AverageColour16(texture, heightmap);
		}
		return 0;
	}

	COLORREF AverageColour8(const Texture &texture, const Heightmap &heightmap)
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
		heightmap_iter = heightmap.data8_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
				if (0 != *heightmap_iter++)
					++half_size;
			++heightmap_iter;
		}
		half_size /= 2;
		// count the number of occurences of each value of blue
		ZeroMemory(color_count, num_colors * sizeof(int));
		heightmap_iter = heightmap.data8_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
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
		heightmap_iter = heightmap.data8_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
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
		heightmap_iter = heightmap.data8_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
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

	COLORREF AverageColour16(const Texture &texture, const Heightmap &heightmap)
	{
		const BYTE *texture_iter;
		const WORD *heightmap_iter;
		const size_t num_colors(256);
		int color_count[num_colors];
		const int *color_iter;
		uint half_size(0);
		uint sum;
		LONG r, c; // row, column
		// count the number of non-null pixels of the heightmap
		heightmap_iter = heightmap.data16_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
				if (0 != *heightmap_iter++)
					++half_size;
			++heightmap_iter;
		}
		half_size /= 2;
		// count the number of occurences of each value of blue
		ZeroMemory(color_count, num_colors * sizeof(int));
		heightmap_iter = heightmap.data16_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
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
		heightmap_iter = heightmap.data16_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
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
		heightmap_iter = heightmap.data16_;
		texture_iter = texture.indices_;
		for (r = 0; r != texture.info_.size_.cy; ++r)
		{
			for (c = 0; c != texture.info_.size_.cx; ++c)
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

	float AverageHeight(const Heightmap &heightmap)
	{
		switch(heightmap.bpp_)
		{
		case 8:  return AverageHeight8 (heightmap);
		case 16: return AverageHeight16(heightmap);
		}
		return 0.0f;
	}

	float AverageHeight8(const Heightmap &heightmap)
	{
		double sum(0);
		int n(0);
		const BYTE* i(heightmap.data8_);
		const BYTE* end(i + heightmap.info_.size_.cx * heightmap.info_.size_.cy);
		for (; i != end; ++i)
			if (*i != 0)
			{
				sum += *i;
				++n;
			}
		if (0 == n)
			sum = 0;
		else
			sum /= n;
		_ASSERTE(sum < 256);
		return static_cast<float>(sum);
	}

	float AverageHeight16(const Heightmap &heightmap)
	{
		double sum(0);
		int n(0);
		const WORD* i(heightmap.data16_);
		const WORD* end(i + heightmap.info_.size_.cx * heightmap.info_.size_.cy);
		for (; i != end; ++i)
			if (*i != 0)
			{
				sum += *i;
				++n;
			}
		if (0 == n)
			sum = 0;
		else
			sum /= n * 32;
		_ASSERTE(sum < 256);
		return static_cast<float>(sum);
	}

	// calculate the CRC32 checksum for the data
	// if Perimeter was at all popular, MD5 might have been worth consideration
	DWORD CalculateChecksum(BYTE *data, size_t size, DWORD seed)
	{
		// generate table
		const uint table_size(256);
		DWORD crctab[table_size];
		{
			const DWORD quotent(0x04c11db7);
			DWORD crc;
			for (uint i(0); i != table_size; ++i)
			{
				crc = i << 24;
				for (size_t j(0); j != 8; ++j)
				{
					if (crc & 0x80000000)
						crc = (crc << 1) ^ quotent;
					else
						crc = crc << 1;
				}
				crctab[i] = crc;
			}
		}
		// calulate checksum
		for (size_t i(0); i != size; ++i)
			seed = (seed << 8 | *data++) ^ crctab[seed >> 24];
		return ~seed;
	}

	void CreateTextures(
		const Texture     &texture,
		TextureAllocation &allocation,
		const Lightmap    &lightmap,
		bool               enable_lighting)
	{
		int texture_index(0);
		int initial_offset(0);
		// main loop
		for (uint texture_y(0); texture_y != allocation.y_count_; ++texture_y)
		{
			for (uint texture_x(0); texture_x != allocation.x_count_; ++texture_x)
			{
				BYTE *texture_iterator(ri_cast<BYTE*>(allocation.bitmaps_[texture_index]));
				// copy texture data
				int offset(initial_offset);
				if (!enable_lighting)
				{
					for (uint y(0); y != allocation.height_; ++y)
					{
						for (uint x(0); x != allocation.width_; ++x)
						{
							const COLORREF colour(texture.palette_[texture.indices_[offset++]]);
							*texture_iterator++ = GetBValue(colour);
							*texture_iterator++ = GetGValue(colour);
							*texture_iterator++ = GetRValue(colour);
							*texture_iterator++ = 0xFF; // alpha
						}
						offset += texture.info_.size_.cx - allocation.width_;
					}
				}
				else
				{
					for (uint y(0); y != allocation.height_; ++y)
					{
						for (uint x(0); x != allocation.width_; ++x)
						{
							// NOTE: optimized to avoid fp ops (the compiler insists on calling _ftol)
							const COLORREF colour(texture.palette_[texture.indices_[offset]]);
							const uint factor(0x101); // precision (odd to avoid bit shifts)
							uint i_light(lightmap.data_[offset++] * factor);
							i_light = (i_light + (0x80 * factor)) / 0xFF;
							*texture_iterator++ = static_cast<BYTE>(__min(0xFF, (GetBValue(colour) * i_light) / factor));
							*texture_iterator++ = static_cast<BYTE>(__min(0xFF, (GetGValue(colour) * i_light) / factor));
							*texture_iterator++ = static_cast<BYTE>(__min(0xFF, (GetRValue(colour) * i_light) / factor));
							*texture_iterator++ = 0xFF; // alpha
						}
						offset += texture.info_.size_.cx - allocation.width_;
					}
				}
				// prepare for the next cycle
				initial_offset += allocation.width_;
				++texture_index;
			}
			initial_offset += texture.info_.size_.cx * (allocation.height_ - 1);
		}
	}

	void CreateTextures(
		const Hardness    &hardness,
		TextureAllocation &allocation,
		const Lightmap    &lightmap,
		bool               enable_lighting)
	{
		int texture_index(0);
		int initial_offset(0);
		// main loop
		for (uint texture_y(0); texture_y != allocation.y_count_; ++texture_y)
		{
			for (uint texture_x(0); texture_x != allocation.x_count_; ++texture_x)
			{
				BYTE *texture_iterator(ri_cast<BYTE*>(allocation.bitmaps_[texture_index]));
				// copy texture data
				int offset(initial_offset);
				if (!enable_lighting)
				{
					for (uint y(0); y != allocation.height_; ++y)
					{
						for (uint x(0); x != allocation.width_; ++x)
						{
							BYTE val(hardness.data_[offset++]);
							*texture_iterator++ = val;  // B
							*texture_iterator++ = val;  // G
							*texture_iterator++ = val;  // R
							*texture_iterator++ = 0xFF; // A
						}
						offset += hardness.info_.size_.cx - allocation.width_;
					}
				}
				else
				{
					for (uint y(0); y != allocation.height_; ++y)
					{
						for (uint x(0); x != allocation.width_; ++x)
						{
							float f_light(static_cast<float>(lightmap.data_[offset]));
							f_light = (f_light + 128.0f) / 255.0f;
							BYTE val(hardness.data_[offset]);
							val = static_cast<BYTE>(__max(0.0f, __min(255.0f, val * f_light)));
							++offset;
							*texture_iterator++ = val;  // B
							*texture_iterator++ = val;  // G
							*texture_iterator++ = val;  // R
							*texture_iterator++ = 0xFF; // A
						}
						offset += hardness.info_.size_.cx - allocation.width_;
					}
				}
				// prepare for the next cycle
				initial_offset += allocation.width_;
				++texture_index;
			}
			initial_offset += hardness.info_.size_.cx * (allocation.height_ - 1);
		}
	}

	void CreateTextures(
		const ZeroLayer   &zero_layer,
		TextureAllocation &allocation,
		const Lightmap    &lightmap,
		bool               enable_lighting)
	{
		int texture_index(0);
		int initial_offset(0);
		// main loop
		for (uint texture_y(0); texture_y != allocation.y_count_; ++texture_y)
		{
			for (uint texture_x(0); texture_x != allocation.x_count_; ++texture_x)
			{
				BYTE *texture_iterator(ri_cast<BYTE*>(allocation.bitmaps_[texture_index]));
				// copy texture data
				int offset(initial_offset);
				if (!enable_lighting)
				{
					for (uint y(0); y != allocation.height_; ++y)
					{
						for (uint x(0); x != allocation.width_; ++x)
						{
							BYTE val(zero_layer.data_[offset++] ? 0xFF : 0x00);
							*texture_iterator++ = val;  // B
							*texture_iterator++ = val;  // G
							*texture_iterator++ = val;  // R
							*texture_iterator++ = 0xFF; // A
						}
						offset += zero_layer.info_.size_.cx - allocation.width_;
					}
				}
				else
				{
					for (uint y(0); y != allocation.height_; ++y)
					{
						for (uint x(0); x != allocation.width_; ++x)
						{
							float f_light(static_cast<float>(lightmap.data_[offset]));
							f_light = (f_light + 128.0f) / 255.0f;
							BYTE val(zero_layer.data_[offset++] ? 0xFF : 0x00);
							 val = static_cast<BYTE>(__max(0.0f, __min(255.0f, val * f_light)));
							*texture_iterator++ = val;  // B
							*texture_iterator++ = val;  // G
							*texture_iterator++ = val;  // R
							*texture_iterator++ = 0xFF; // A
						}
						offset += zero_layer.info_.size_.cx - allocation.width_;
					}
				}
				// prepare for the next cycle
				initial_offset += allocation.width_;
				++texture_index;
			}
			initial_offset += zero_layer.info_.size_.cx * (allocation.height_ - 1);
		}
	}

	// triangulation helper function - flatness heuristic
	// determines flatness from the center point and the middles of the four sides of a quad
	// the result is the distance between the projections of the middle point on the horizontal and the vertical
	float Flatness(float h1, float h2, float v1, float v2, float c, float r)
	{
		float dx;
		float dy;
		float c_dy;
		float length;
		float cnst;
		// project c onto h2 - h1
		dx   = r + r;
		dy   = h2 - h1;
		c_dy = c - h1;
		length = sqrt(dx * dx + dy * dy);
		dx /= length;
		dy /= length;
		cnst = (r * dx + c_dy * dy) / (dx * dx + dy * dy);
		float h_proj_x = r    - cnst * dx;
		float h_proj_y = c_dy - cnst * dy;
		// project c onto v2 - v1
		dx   = r  + r;
		dy   = v2 - v1;
		c_dy = c  - v1;
		length = sqrt(dx * dx + dy * dy);
		dx /= length;
		dy /= length;
		cnst = (r * dx + c_dy * dy) / (dx * dx + dy * dy);
		float v_proj_x = r - cnst * dx;
		float v_proj_y = c_dy - cnst * dy;
		// compute the length of the distance between the projections
		float proj_dy = v_proj_y + h_proj_y;
		float distance = h_proj_x * h_proj_x + v_proj_x * v_proj_x + proj_dy * proj_dy;
		return distance;
	}

	// replace a sequence of tokens occuring in order in the given string
	// str is not supposed to be user-supplied
	void ReplaceSubstringSeq(
		LPCTSTR                                     str,
		size_t                                      str_length,
		const vector<std::pair<tstring, tstring> > &seq,
		tstring                                    &result)
	{
		typedef std::pair<tstring, tstring> SeqType;
		result.clear();
		// reserve enough space for the new string
		{
			int total_diff(0);
			foreach (const SeqType &pair, seq)
				total_diff += static_cast<int>(pair.second.size()) - pair.first.size();
			_ASSERTE(str_length + total_diff >= 0);
			result.reserve(str_length + total_diff);
		}
		// make the replacements
		LPCTSTR offset(str);
		foreach (const SeqType &pair, seq)
		{
			LPCTSTR new_offset(_tcsstr(offset, pair.first.c_str()));
			_ASSERTE(NULL != new_offset); // debug version only check
			result.append(offset, new_offset - offset);
			result.append(pair.second);
			offset = new_offset + pair.first.size();
		}
		// append the rest of the data
		result.append(offset);
	}

	// convert a heightmap to a mesh in O(n*ln(n))
	// the algorithm takes two passes over the heightmap:
	//  at first pass it checks progressively coarser grids, determining which areas are "flat enough"
	//  at second pass it checks progressively finer grids, converting flat areas to quads, and aligning edges
	void Triangulate(const Heightmap &heightmap, vector<SimpleVertex> &vertices, float mesh_threshold)
	{
		int n; // map power
		SIZE map_size = { heightmap.info_.size_.cx, heightmap.info_.size_.cy };
		// check preconditions and calculate n
		{
			int power_x(log2(map_size.cx));
			int power_y(log2(map_size.cy));
			n = __min(power_x, power_y);
			_ASSERTE(n >= 2);             // algorithm requirement
			_ASSERTE(power_x < 14);       // sanity check
			_ASSERTE(power_y < 14);       // sanity check
			_ASSERTE(power_x == power_y); // TODO: add the capability of handling non-square maps
		}
		// get the source data
		vector<BYTE> src_data_v;
		const BYTE *src_data(NULL);
		switch (heightmap.bpp_)
		{
		case 8:
			{
				src_data = heightmap.data8_;
			} break;
		case 16:
			{
				const int size(heightmap.size_.cx * heightmap.size_.cy);
				src_data_v.reserve(size);
				for (int i(0); i != size; ++i)
					src_data_v.push_back((BYTE)((heightmap.data16_[i] >> 5) & 0xFF));
				src_data = &src_data_v[0];
			} break;
		default:
			return;
		}
		// allocate and initialize storage for the algorithm
		vector<bool> *regions(new vector<bool>[n - 1]);
		{
			int num(2);
			for (int i(n - 2); i != -1; --i)
			{
				regions[i].resize(num * num);
				num *= 2;
			}
		}
		// determine flatness of map regions of increasing size
		{
			const float threshold(mesh_threshold * mesh_threshold); // preferences really use sqrt
			int index(0);
			// base case (k = 1)
			for (int xi(0); xi != map_size.cx / 2; ++xi)
			{
				for (int yi(0); yi != map_size.cy / 2; ++yi)
				{
					int offset(xi * (map_size.cx + 1) * 2 + (map_size.cx + 1) + yi * 2 + 1);
					// stop if any of the underlying points are zero
					for (int y(-1); y != 2; ++y)
						for (int x(-1); x != 2; ++x)
							if (src_data[offset + y * (map_size.cx + 1) + x] == 0)
								goto end;
					// check if the region is flat enough
					if (Flatness(
						(float)src_data[offset - 1],
						(float)src_data[offset + 1],
						(float)src_data[offset - map_size.cx - 1],
						(float)src_data[offset + map_size.cx + 1],
						(float)src_data[offset],
						1.0f) < threshold)
						regions[0][index] = true;
				end:
					++index;
				}
			}
			// general case
			for (int k(2); k != n; ++k)
			{
				index = 0;
				int num(exp2(n - k));
				for (int xi(0); xi != num; ++xi)
				{
					for (int yi(0); yi != num; ++yi, ++index)
					{
						// stop if any of the underlying regions are false
						int prev_offset(2 * (2 * num * xi + yi));
						if (
							!regions[k - 2][prev_offset]           ||
							!regions[k - 2][prev_offset + 1]       ||
							!regions[k - 2][prev_offset + num * 2] ||
							!regions[k - 2][prev_offset + num * 2 + 1])
							continue;
						// check if the region is flat enough
						int radius(exp2(k - 1)); // horizontal distance from center to the edge
						int side(radius + radius); // length of a side of the region
						int offset((map_size.cx + 1) * (side * xi + radius) + side * yi + radius);
						if (Flatness(
							(float)src_data[offset - radius],
							(float)src_data[offset + radius],
							(float)src_data[offset - (map_size.cx + 1) * radius],
							(float)src_data[offset + (map_size.cx + 1) * radius],
							(float)src_data[offset],
							(float)radius) < threshold)
						{
							regions[k - 1][index] = true;
							// set the underlying regions to false
							regions[k - 2][prev_offset]               = false;
							regions[k - 2][prev_offset + 1]           = false;
							regions[k - 2][prev_offset + num * 2]     = false;
							regions[k - 2][prev_offset + num * 2 + 1] = false;
						}
					}
				}
			}
		}
		// build the mesh
		{
			// we only need information for two levels at a time,
			//  so a recursive approach would be wasteful
			vector<TE>  queue1;
			vector<TE>  queue2;
			vector<TE> *current(&queue1);
			vector<TE> *queued (&queue2);
			vector<TE> *temp_array;
			// add the first elements to the current queue
			for (int i(0); i != 4; ++i)
			{
				TE te;
				te.index = i;
				current->push_back(te);
			}
			short num; // number of regions per row or column of the heightmap
			// heightmap values underlying the region
			float tl_val, tr_val;
			float bl_val, br_val;
			// corresponding heightmap values
			SimpleVertex tl_vert, tr_vert;
			SimpleVertex bl_vert, br_vert;
			// cartesian components of an index
			short xi, yi;
			// iterate through the hierarchy
			for (int k(n - 2); k != 0; --k)
			{
				num = static_cast<short>(exp2(n - k - 1));
				short side(static_cast<short>(exp2(k + 1))); // length of the region's side
				vector<TE>::const_iterator te_i(current->begin());
				const vector<TE>::const_iterator te_end(current->end());
				for (; te_i != te_end; ++te_i)
				{
					const TE te(*te_i);
					// split the index into cartesian coordinates
					xi = static_cast<short>(te.index % num);
					yi = static_cast<short>(te.index / num);
					// calculate sides of the region
					short l_coord(static_cast<short>(xi * side));
					short r_coord(static_cast<short>(l_coord + side));
					short b_coord(static_cast<short>(yi * side));
					short t_coord(static_cast<short>(b_coord + side));
					// calculate heightmap values at the corners of the region
					{
						const BYTE *offset(src_data + side * ((map_size.cx + 1) * yi + xi));
						bl_val = *offset;
						br_val = *(offset + side);
						offset += side * (map_size.cx + 1);
						tl_val = *offset;
						tr_val = *(offset + side);
					}
					// constrain the heightmap values, if necessary
					// bottom left
					if (te.l_constrained && te.l_constraint.start != b_coord)
						bl_val = te.l_constraint.ConstrainedVal(b_coord);
					if (te.b_constrained && te.b_constraint.start != l_coord)
						bl_val = te.b_constraint.ConstrainedVal(l_coord);
					// bottom right
					if (te.r_constrained && te.r_constraint.start != b_coord)
						br_val = te.r_constraint.ConstrainedVal(b_coord);
					if (te.b_constrained && (te.b_constraint.start + te.b_constraint.length) != r_coord)
						br_val = te.b_constraint.ConstrainedVal(r_coord);
					// top left
					if (te.l_constrained && (te.l_constraint.start + te.l_constraint.length) != t_coord)
						tl_val = te.l_constraint.ConstrainedVal(t_coord);
					if (te.t_constrained && te.t_constraint.start != l_coord)
						tl_val = te.t_constraint.ConstrainedVal(l_coord);
					// top right
					if (te.r_constrained && (te.r_constraint.start + te.r_constraint.length) != t_coord)
						tr_val = te.r_constraint.ConstrainedVal(t_coord);
					if (te.t_constrained && (te.t_constraint.start + te.t_constraint.length) != r_coord)
						tr_val = te.t_constraint.ConstrainedVal(r_coord);
					// if the region is flat, add it to the triangulation and continue
					if (regions[k][te.index])
					{
						// bottom left vertex
						bl_vert.x = l_coord;
						bl_vert.y = b_coord;
						bl_vert.z = bl_val;
						// bottom right vertex
						br_vert.x = r_coord;
						br_vert.y = b_coord;
						br_vert.z = br_val;
						// top left vertex
						tl_vert.x = l_coord;
						tl_vert.y = t_coord;
						tl_vert.z = tl_val;
						// top right vertex
						tr_vert.x = r_coord;
						tr_vert.y = t_coord;
						tr_vert.z = tr_val;
						// add the vertices to the triangulation (counter-clockwise)
						// TODO: try using indexes
						vertices.push_back(bl_vert);
						vertices.push_back(br_vert);
						vertices.push_back(tr_vert);
						vertices.push_back(bl_vert);
						vertices.push_back(tr_vert);
						vertices.push_back(tl_vert);
						continue;
					}
					// check if each of the region's neighbours exists (relies on shortcircuiting)
					bool l_constrained((xi != 0)       && regions[k][te.index - 1]);
					bool r_constrained((xi != num - 1) && regions[k][te.index + 1]);
					bool b_constrained((yi != 0)       && regions[k][te.index - num]);
					bool t_constrained((yi != num - 1) && regions[k][te.index + num]);
					// locate subregions
					TE bl_te, br_te, tl_te, tr_te;
					{
						int offset(4 * te.index - 2 * xi);
						bl_te.index = offset;
						br_te.index = offset + 1;
						tl_te.index = offset + num * 2;
						tr_te.index = offset + num * 2 + 1;
					}
					// copy constraints
					if (te.l_constrained) // left
					{
						_ASSERTE(!l_constrained);
						bl_te.l_constrained = tl_te.l_constrained = true;
						bl_te.l_constraint  = tl_te.l_constraint  = te.l_constraint;
					}
					if (te.r_constrained) // right
					{
						_ASSERTE(!r_constrained);
						br_te.r_constrained = tr_te.r_constrained = true;
						br_te.r_constraint  = tr_te.r_constraint  = te.r_constraint;
					}
					if (te.b_constrained) // bottom
					{
						_ASSERTE(!b_constrained);
						bl_te.b_constrained = br_te.b_constrained = true;
						bl_te.b_constraint  = br_te.b_constraint  = te.b_constraint;
					}
					if (te.t_constrained) // top
					{
						_ASSERTE(!t_constrained);
						tl_te.t_constrained = tr_te.t_constrained = true;
						tl_te.t_constraint  = tr_te.t_constraint  = te.t_constraint;
					}
					// add the constraints
					if (l_constrained) // left
					{
						// constrain the bottom left region and the top left region
						_ASSERTE(bl_te.l_constrained == false);
						_ASSERTE(tl_te.l_constrained == false);
						bl_te.l_constrained       = tl_te.l_constrained       = true;
						bl_te.l_constraint.start  = tl_te.l_constraint.start  = b_coord;
						bl_te.l_constraint.length = tl_te.l_constraint.length = side;
						bl_te.l_constraint.s_val  = tl_te.l_constraint.s_val  = bl_val;
						bl_te.l_constraint.f_val  = tl_te.l_constraint.f_val  = tl_val;
					}
					if (r_constrained) // right
					{
						// constrain the bottom right region and the top right region
						_ASSERTE(br_te.r_constrained == false);
						_ASSERTE(tr_te.r_constrained == false);
						br_te.r_constrained       = tr_te.r_constrained       = true;
						br_te.r_constraint.start  = tr_te.r_constraint.start  = b_coord;
						br_te.r_constraint.length = tr_te.r_constraint.length = side;
						br_te.r_constraint.s_val  = tr_te.r_constraint.s_val  = br_val;
						br_te.r_constraint.f_val  = tr_te.r_constraint.f_val  = tr_val;
					}
					if (b_constrained) // bottom
					{
						// constrain the bottom left region and the bottom right region
						_ASSERTE(bl_te.b_constrained == false);
						_ASSERTE(br_te.b_constrained == false);
						bl_te.b_constrained       = br_te.b_constrained       = true;
						bl_te.b_constraint.start  = br_te.b_constraint.start  = l_coord;
						bl_te.b_constraint.length = br_te.b_constraint.length = side;
						bl_te.b_constraint.s_val  = br_te.b_constraint.s_val  = bl_val;
						bl_te.b_constraint.f_val  = br_te.b_constraint.f_val  = br_val;
					}
					if (t_constrained) // top
					{
						// constrain the top left region and the top right region
						_ASSERTE(tl_te.t_constrained == false);
						_ASSERTE(tr_te.t_constrained == false);
						tl_te.t_constrained       = tr_te.t_constrained       = true;
						tl_te.t_constraint.start  = tr_te.t_constraint.start  = l_coord;
						tl_te.t_constraint.length = tr_te.t_constraint.length = side;
						tl_te.t_constraint.s_val  = tr_te.t_constraint.s_val  = tl_val;
						tl_te.t_constraint.f_val  = tr_te.t_constraint.f_val  = tr_val;
					}
					// enqueue the subregions
					queued->push_back(bl_te);
					queued->push_back(br_te);
					queued->push_back(tl_te);
					queued->push_back(tr_te);
				}
				// switch queues
				current->clear();
				temp_array = current;
				current = queued;
				queued = temp_array;
			}
			// handle the special case of k = 0
			num = static_cast<short>(map_size.cx / 2);
			vector<TE>::const_iterator te_i(current->begin());
			const vector<TE>::const_iterator te_end(current->end());
			for (; te_i != te_end; ++te_i)
			{
				const TE te(*te_i);
				// split the index into cartesian coordinates
				xi = static_cast<short>(te.index % num);
				yi = static_cast<short>(te.index / num);
				// calculate sides of the region
				short l_coord(xi + xi);
				short r_coord(l_coord + 2);
				short b_coord(yi + yi);
				short t_coord(b_coord + 2);
				// calculate heightmap values at the corners of the region
				const BYTE *offset(src_data + 2 * ((map_size.cx + 1) * yi + xi));
				bl_val = *offset;
				br_val = *(offset + 2);
				offset += 2 * (map_size.cx + 1);
				tl_val = *offset;
				tr_val = *(offset + 2);
				// constrain the heightmap values, if necessary
				// bottom left
				if (te.l_constrained && te.l_constraint.start != b_coord)
					bl_val = te.l_constraint.ConstrainedVal(b_coord);
				if (te.b_constrained && te.b_constraint.start != l_coord)
					bl_val = te.b_constraint.ConstrainedVal(l_coord);
				// bottom right
				if (te.r_constrained && te.r_constraint.start != b_coord)
					br_val = te.r_constraint.ConstrainedVal(b_coord);
				if (te.b_constrained && (te.b_constraint.start + te.b_constraint.length) != r_coord)
					br_val = te.b_constraint.ConstrainedVal(r_coord);
				// top left
				if (te.l_constrained && (te.l_constraint.start + te.l_constraint.length) != t_coord)
					tl_val = te.l_constraint.ConstrainedVal(t_coord);
				if (te.t_constrained && te.t_constraint.start != l_coord)
					tl_val = te.t_constraint.ConstrainedVal(l_coord);
				// top right
				if (te.r_constrained && (te.r_constraint.start + te.r_constraint.length) != t_coord)
					tr_val = te.r_constraint.ConstrainedVal(t_coord);
				if (te.t_constrained && (te.t_constraint.start + te.t_constraint.length) != r_coord)
					tr_val = te.t_constraint.ConstrainedVal(r_coord);
				// if the region is flat, add it to the triangulation and continue
				if (regions[k][te.index])
				{
					// bottom left vertex
					bl_vert.x = l_coord;
					bl_vert.y = b_coord;
					bl_vert.z = bl_val;
					// bottom right vertex
					br_vert.x = r_coord;
					br_vert.y = b_coord;
					br_vert.z = br_val;
					// top left vertex
					tl_vert.x = l_coord;
					tl_vert.y = t_coord;
					tl_vert.z = tl_val;
					// top right vertex
					tr_vert.x = r_coord;
					tr_vert.y = t_coord;
					tr_vert.z = tr_val;
					// add the vertices to the triangulation (counter-clockwise)
					// TODO: try using indexes
					vertices.push_back(bl_vert);
					vertices.push_back(br_vert);
					vertices.push_back(tr_vert);
					vertices.push_back(bl_vert);
					vertices.push_back(tr_vert);
					vertices.push_back(tl_vert);
					continue;
				}
				// shift "offset" to the region's center
				offset -= map_size.cx;
				// find the middle point
				float center(*offset);
				// if any point under the region is zero, do not triangulate it
				//  this is justified because inner regions will not have zero subregions
				//  and boundary regions are not as important
				if (
					*offset                     == 0 ||
					*(offset + 1)               == 0 ||
					*(offset - 1)               == 0 ||
					*(offset + map_size.cx)     == 0 ||
					*(offset + map_size.cx + 2) == 0 ||
					*(offset - map_size.cx - 2) == 0 ||
					*(offset - map_size.cx)     == 0)
					continue;
				// bottom left subregion
				// bottom left vertex
				bl_vert.x = l_coord;
				bl_vert.y = b_coord;
				bl_vert.z = bl_val;
				// bottom right vertex
				br_vert.x = l_coord + 1.0f;
				br_vert.y = b_coord;
				br_vert.z = (bl_val + br_val) / 2.0f;
				// top left vertex
				tl_vert.x = l_coord;
				tl_vert.y = b_coord + 1.0f;
				tl_vert.z = (bl_val + tl_val) / 2.0f;
				// top right vertex
				tr_vert.x = l_coord + 1.0f;
				tr_vert.y = b_coord + 1.0f;
				tr_vert.z = center;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
				// bottom right subregion
				// bottom left vertex
				bl_vert.x = l_coord + 1.0f;
				bl_vert.y = b_coord;
				bl_vert.z = (bl_val + br_val) / 2.0f;
				// bottom right vertex
				br_vert.x = r_coord;
				br_vert.y = b_coord;
				br_vert.z = br_val;
				// top left vertex
				tl_vert.x = l_coord + 1.0f;
				tl_vert.y = b_coord + 1.0f;
				tl_vert.z = center;
				// top right vertex
				tr_vert.x = r_coord;
				tr_vert.y = b_coord + 1.0f;
				tr_vert.z = (br_val + tr_val) / 2.0f;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
				// top left subregion
				// bottom left vertex
				bl_vert.x = l_coord;
				bl_vert.y = b_coord + 1.0f;
				bl_vert.z = (bl_val + tl_val) / 2.0f;
				// bottom right vertex
				br_vert.x = l_coord + 1.0f;
				br_vert.y = b_coord + 1.0f;
				br_vert.z = center;
				// top left vertex
				tl_vert.x = l_coord;
				tl_vert.y = t_coord;
				tl_vert.z = tl_val;
				// top right vertex
				tr_vert.x = l_coord + 1.0f;
				tr_vert.y = t_coord;
				tr_vert.z = (tl_val + tr_val) / 2.0f;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
				// top right subregion
				// bottom left vertex
				bl_vert.x = l_coord + 1.0f;
				bl_vert.y = b_coord + 1.0f;
				bl_vert.z = center;
				// bottom right vertex
				br_vert.x = r_coord;
				br_vert.y = b_coord + 1.0f;
				br_vert.z = (br_val + tr_val) / 2.0f;
				// top left vertex
				tl_vert.x = l_coord + 1.0f;
				tl_vert.y = t_coord;
				tl_vert.z = (tl_val + tr_val) / 2.0f;
				// top right vertex
				tr_vert.x = r_coord;
				tr_vert.y = t_coord;
				tr_vert.z = tr_val;
				// add the vertices to the triangulation (counter-clockwise)
				// TODO: try using indexes
				vertices.push_back(bl_vert);
				vertices.push_back(br_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(bl_vert);
				vertices.push_back(tr_vert);
				vertices.push_back(tl_vert);
			}
		}
		delete [] regions;
	}

	//------
	// other
	//------

	bool RegisterMap(LPCTSTR map_name, DWORD checksum, ErrorHandler &error_handler)
	{
		// check connection status
		if (FALSE == InternetCheckConnection(
#ifdef PRE_RELEASE
			_T("http://www.rul-clan.ru/map_registration/pre_release/map_register.php"),
#else
			_T("http://www.rul-clan.ru/map_registration/map_register.php"),
#endif
			FLAG_ICC_FORCE_CONNECTION,
			0))
		{
			error_handler.MacroDisplayError(_T("Connection to the server could not be established."));
			return false;
		}
		// register map
		const size_t buffer_size(1048576);
		TCHAR *buffer(new TCHAR[buffer_size]);
		{
			// establish connection
			HINTERNET wi_handle(InternetOpen(
				_T("Perimeter Map Compiler"),
				INTERNET_OPEN_TYPE_PRECONFIG,
				NULL,
				NULL,
				0));
			if (NULL == wi_handle)
			{
				error_handler.MacroDisplayError(_T("InternetOpen failed"));
				delete [] buffer;
				return false;
			}
			// open URL
			HINTERNET url_handle;
			{
				// create the URL
				TCHAR url[MAX_PATH];
				{
					// convert the checksum into a string
					TCHAR cs_string[16];
					ZeroMemory(cs_string, 16);
					_ltot(checksum, cs_string, 16);
					// create an unsafe url
#ifdef PRE_RELEASE
					tstring unsafe_url(_T("http://www.rul-clan.ru/map_registration/pre_release/map_register.php?map_name="));
#else
					tstring unsafe_url(_T("http://www.rul-clan.ru/map_registration/map_register.php?map_name="));
#endif
					unsafe_url += map_name;
					unsafe_url += _T("&map_checksum=");
					unsafe_url += cs_string;
					// convert the unsafe url to canonical form
					DWORD num_chars(MAX_PATH);
					if (S_OK != UrlEscape(
						unsafe_url.c_str(),
						url,
						&num_chars,
						URL_ESCAPE_UNSAFE))
					{
						error_handler.MacroDisplayError(_T("UrlEscape failed"));
						InternetCloseHandle(wi_handle);
						delete [] buffer;
						return false;
					}
					url[num_chars] = _T('\0');
				}
				// open the URL
				url_handle = InternetOpenUrl(wi_handle, url, NULL, 0L, 0L, 0L);
				if (NULL == url_handle)
				{
					error_handler.MacroDisplayError(_T("InternetOpenUrl failed"));
					InternetCloseHandle(wi_handle);
					delete [] buffer;
					return false;
				}
			}
			// get data
			DWORD bytes_read;
			if (FALSE == InternetReadFile(
				url_handle,
				buffer,
				buffer_size,
				&bytes_read))
			{
				error_handler.MacroDisplayError(_T("InternetReadFile failed"));
				InternetCloseHandle(url_handle);
				InternetCloseHandle(wi_handle);
				delete [] buffer;
				return false;
			}
			buffer[bytes_read] = _T('\0');
			// wrap up
			InternetCloseHandle(url_handle);
			InternetCloseHandle(wi_handle);
		}
		// process result
		if (0 != _tcscmp(buffer, _T("success")))
		{
			if (0 == _tcscmp(buffer, _T("taken")))
				error_handler.MacroDisplayError(_T("The name of this map is already in use."));
			else if (0 == _tcscmp(buffer, _T("invalid")))
				error_handler.MacroDisplayError(_T("Invalid map name."));
			else if (0 == _tcscmp(buffer, _T("reg_limit_exceeded")))
				error_handler.MacroDisplayError(_T("Registration limit for today exceeded."));
			else
				error_handler.MacroDisplayError(_T("Map could not be registered."));
			delete [] buffer;
			return false;
		}
		delete [] buffer;
		return true;
	}
}
