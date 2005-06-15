//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// � Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// � Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// � Neither the name of Don Reba nor the names of its contributors may be used
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

#include "error handler.h"
#include "preview wnd.h"
#include "project data.h"
#include "resource.h"
#include "task common.h"

#include <fstream>
#include <Wininet.h>

namespace TaskCommon
{
	//----------------------------------------
	// Heightmap implementation
	//----------------------------------------

	Heightmap::Heightmap(SIZE size, HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,data_(NULL)
	{
		size_.cx = size.cx + 1;
		size_.cy = size.cy + 1;
	}

	Heightmap::~Heightmap()
	{
		delete [] data_;
	}

	bool Heightmap::Load(LPCTSTR path)
	{
		const SIZE map_size = { size_.cx - 1, size_.cy - 1 };
		fipImage image;
		// load the heightmap image
		{
			const DWORD delay(256);
			const uint  try_count(32);
			uint        try_num(0);
			for (; try_num != try_count; ++try_num)
			{
				image.load(path);
				if (TRUE == image.isValid())
					break;
				else
					Sleep(delay);
			}
			if (try_count == try_num)
			{
				MacroDisplayError(_T("Heightmap could not be loaded."));
				return false;
			}
		}
		// make sure that dimensions are correct
		if (image.getWidth() != map_size.cx || image.getHeight() != map_size.cy)
		{
			MacroDisplayError(_T("Heightmap dimensions do not correspond to project settigns."));
			return false;
		}
		// turn the image to grayscale if necessary
		if (8 != image.getBitsPerPixel())
		{
			if (FALSE == image.convertToGrayscale())
			{
				MacroDisplayError(_T("Heightmap could not be converted to grayscale."));
				return false;
			}
		}
		// flip the image vertically
		if (FALSE == image.flipVertical())
			MacroDisplayError(_T("Heightmap bitmap could not be flipped."));
		// allocate new memory for the heightmap
		const size_t data_size(size_.cx * size_.cy);
		_ASSERTE(NULL == data_);
		data_ = new BYTE[data_size];
		if (NULL == data_)
		{
			MacroDisplayError(_T("Memory for loading the heightmap could not be allocated."));
			return false;
		}
		// read in data
		{
			const BYTE *image_iter(image.accessPixels());
			BYTE *heightmap_iter(data_);
			for (LONG r(0); r != map_size.cy; ++r)
			{
				CopyMemory(heightmap_iter, image_iter, map_size.cx);
				heightmap_iter += map_size.cx;
				image_iter += map_size.cx;
				// pad horizontally
				*heightmap_iter = *(image_iter - 1);
				++heightmap_iter;
			}
			// pad vertically
			CopyMemory(heightmap_iter, heightmap_iter - (map_size.cx + 1), map_size.cx + 1);
		}
		return true;
	}

	int Heightmap::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask)
	{
		SIZE map_size = { size_.cx - 1, size_.cy - 1 };
		mask.reserve(map_size.cx * map_size.cy);
		// pack heightmap itself
		int encoded_size;
		{
			// create the image data matrix
			jas_matrix_t *data(jas_matrix_create(map_size.cy, map_size.cx)); // rows then columns
			{
				size_t index(0);
				for (LONG r(0); r != map_size.cy; ++r)
				{
					for (LONG c(0); c != map_size.cx; ++c)
					{
						jas_matrix_set(data, r, c, data_[index]);
						mask.push_back(0 == data_[index]);
						++index;
					}
					++index; // skip padding
				}
			}
			// initialize the image component structure
			jas_image_cmptparm_t component;
			component.width  = map_size.cx;
			component.height = map_size.cy;
			component.tlx    = 0;
			component.tly    = 0;
			component.hstep  = 1;
			component.vstep  = 1;
			component.prec   = 8;
			component.sgnd   = false;
			// create the image
			jas_image_t *image(jas_image_create(1, &component, JAS_CLRSPC_SGRAY));
			if (0 == image)
				MacroDisplayError(_T("jas_image_create failed"));
			// fill the component of the image
			jas_image_setcmpttype(image, 0, 0);
			if (0 != jas_image_writecmpt(image, 0, 0, 0, map_size.cx, map_size.cy, data))
				MacroDisplayError(_T("jas_imagewritecmpt failed"));
			// create an output stream
			size_t buffer_size(map_size.cx * map_size.cy);
			jas_stream_t *stream = jas_stream_memopen(ri_cast<char*>(buffer), buffer_size);
			if (0 == stream)
				MacroDisplayError(_T("jas_stream_open failure"));
			// encode and write the image into the stream
			int format = jas_image_strtofmt("jpc");
			if (-1 == format)
				MacroDisplayError(_T("jas_image_strtofmt failed"));
			if (0 != jas_image_encode(image, stream, format, "mode=real rate=0.1"))
				MacroDisplayError(_T("jas_image_encode failed"));
			encoded_size = stream->rwcnt_;
			// clean up
			jas_stream_close(stream);
			jas_image_destroy(image);
			jas_matrix_destroy(data);
		}
		// pack the mask
		_ASSERTE(mask.size() % 8 == 0);
		const int mask_size(mask.size() / 8);
		{
			BYTE *mask_buffer(new BYTE[mask_size]);
			ZeroMemory(mask_buffer, mask_size);
			int bit_index(0);
			for (int i(0); i != mask_size; ++i)
				for (int b(0); b != 8; ++b)
					if (mask[bit_index++])
						mask_buffer[i] |= 1 << b;
			CopyMemory(buffer + encoded_size, mask_buffer, mask_size);
			delete [] mask_buffer;
		}
		// record xml metadata
		{
			char str[16];
			// compression format
			node.InsertEndChild(TiXmlElement("compression"))->InsertEndChild(TiXmlText("JPC"));
			// offset of compressed heightmap data
			_itot(buffer - initial_offset, str, 10);
			node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
			// size of compressed heightmap data
			_itot(encoded_size, str, 10);
			node.InsertEndChild(TiXmlElement("size"))->InsertEndChild(TiXmlText(str));
			// offset of the null point mask
			_itot(buffer - initial_offset + encoded_size, str, 10);
			node.InsertEndChild(TiXmlElement("mask_offset"))->InsertEndChild(TiXmlText(str));
		}
		return encoded_size + mask_size;
	}

	void Heightmap::Unpack(TiXmlNode *node, BYTE *buffer)
	{
		SIZE map_size = { size_.cx - 1, size_.cy - 1 };
		// allocate memory for the heightmap
		const size_t data_size(size_.cx * size_.cy);
		_ASSERTE(NULL == data_);
		data_ = new BYTE[data_size];
		// read in XML metadata
		size_t compressed_heightmap_size;
		BYTE *compressed_heightmap;
		BYTE *mask_buffer;
		{
			// find data
			TiXmlHandle node_handle(node);
			TiXmlText *compression_node(node_handle.FirstChildElement("compression").FirstChild().Text());
			TiXmlText *offset_node     (node_handle.FirstChildElement("offset"     ).FirstChild().Text());
			TiXmlText *size_node       (node_handle.FirstChildElement("size"       ).FirstChild().Text());
			TiXmlText *mask_offset_node(node_handle.FirstChildElement("mask_offset").FirstChild().Text());
			if (
				NULL == compression_node ||
				NULL == offset_node      ||
				NULL == size_node        ||
				NULL == mask_offset_node)
			{
				_RPT0(_CRT_WARN, "loading default heightmap\n");
				DefaultHeightmap(data_, size_);
				return;
			}
			// make sure the compression format matches
			if (0 != strcmp(compression_node->Value(), "JPC"))
			{
				_RPT0(_CRT_WARN, "loading default heightmap\n");
				DefaultHeightmap(data_, size_);
				return;
			}
			// finally parse the data
			compressed_heightmap      = buffer + atoi(offset_node->Value());
			compressed_heightmap_size = atoi(size_node->Value()); // WARN: possible buffer overflow
			mask_buffer               = buffer + atoi(mask_offset_node->Value());
		}
		// unpack the mask
		vector<bool> mask;
		{
			int mask_size(map_size.cx * map_size.cy);
			_ASSERTE(mask_size % 8 == 0);
			mask.reserve(mask_size);
			mask_size /= 8;
			for (int i(0); i != mask_size; ++i)
				for (int b(0); b != 8; ++b)
					mask.push_back(0 != (mask_buffer[i] & 1 << b));
		}
		// unpack the heightmap itself
		{
			// create an input stream
			jas_stream_t *stream(jas_stream_memopen(
				ri_cast<char*>(compressed_heightmap),
				compressed_heightmap_size));
			if (0 == stream)
			{
				MacroDisplayError(_T("jas_stream open failure"));
				DefaultHeightmap(data_, size_);
				return;
			}
			// decode the image
			int format = jas_image_strtofmt("jpc");
			if (-1 == format)
			{
				MacroDisplayError(_T("jas_image_strtofmt failed"));
				DefaultHeightmap(data_, size_);
				return;
			}
			jas_image_t *image(jas_image_decode(stream, format, ""));
			if (0 == image)
			{
				MacroDisplayError(_T("jas_image_decode failed"));
				DefaultHeightmap(data_, size_);
				return;
			}
			// extract image data
			{
				// get the data matrix
				jas_matrix_t *data_matrix(jas_matrix_create(map_size.cy, map_size.cx));
				if (0 == data_matrix)
					MacroDisplayError(_T("jas_matrix_create failed"));
				if (0 > jas_image_readcmpt(image, 0, 0, 0, map_size.cx, map_size.cy, data_matrix))
					MacroDisplayError(_T("jas_image_readcmpt failed"));
				// extract image data, with padding, and 0 where mask is true
				BYTE *data_ptr(data_);
				int mask_index(0);
				for (LONG r(0); r != map_size.cy; ++r)
				{
					for (LONG c(0); c != map_size.cx; ++c)
						*data_ptr++ = mask[mask_index++] ? 0 : static_cast<BYTE>(jas_matrix_get(data_matrix, r, c));
					*data_ptr++ = *(data_ptr - 1); // pad horizontally
				}
				// pad vertically
				for (int c(0); c != map_size.cx + 1; ++c)
					*data_ptr++ = *(data_ptr - map_size.cx - 1);
			}
		}
	}

	//---------------------------------------
	// Lightmap implementation
	//---------------------------------------

	Lightmap::Lightmap(SIZE size, HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,data_(NULL)
		,size_(size)
	{}

	Lightmap::~Lightmap()
	{
		delete [] data_;
	}

	bool Lightmap::Create(const Heightmap &heightmap)
	{
		_ASSERTE(size_.cx + 1 == heightmap.size_.cx && size_.cy + 1 == heightmap.size_.cy);
		// allocate memory for the lightmap
		const size_t data_size(size_.cx * size_.cy);
		_ASSERTE(NULL == data_);
		data_ = new BYTE[data_size];
		if (NULL == data_)
		{
			MacroDisplayError(_T("Memory for loading the heightmap could not be allocated."));
			return false;
		}
		// calculate lighting
		const BYTE *hi(heightmap.data_); // heightmap iterator
		BYTE *li(data_); // lightmap iterator
		for (LONG r(0); r != size_.cy; ++r)
		{
			const BYTE * row_i(hi + size_.cx);
			int          high_point_z(*row_i);
			const BYTE * high_point_x(row_i);
			li += size_.cx;
			while (row_i != hi)
			{
				--li;
				--row_i;
				const ptrdiff_t point_x(high_point_x - row_i);
				const int       point_z(*row_i);
				if (high_point_z - point_z < point_x)
				// if the point is unshadowed
				{
					const float dx(1.0f);
					const float dy(static_cast<float>(row_i[0] - row_i[1]));
					float length(sqrt(2 * (dx * dx + dy * dy)));
					float dot((dx + dy) / length);
					*li = (BYTE)(255 * dot);
					high_point_z = point_z;
					high_point_x = row_i;
				}
				else
					*li = 0x00;
			}
			li += size_.cx;
			hi += size_.cx + 1;
		}
		// blur the lightmap (fake soft shadows :) )
		// TODO: profile to see if the lightmap should have been integer all along
		{
			int *int_lightmap(new int[data_size]);
			for (size_t i(0); i != data_size; ++i)
				int_lightmap[i] = data_[i];
			StackBlur(int_lightmap, size_.cx, size_.cy, 4);
			for (size_t i(0); i != data_size; ++i)
				data_[i] = static_cast<BYTE>(int_lightmap[i]);
			delete [] int_lightmap;
		}
		return true;
	}

	//-----------------------
	// MapInfo implementation
	//-----------------------

	void MapInfo::Load()
	{
		map_name_   = MacroProjectData(ID_MAP_NAME);
		power_x_    = MacroProjectData(ID_POWER_X);
		power_y_    = MacroProjectData(ID_POWER_Y);
		zero_level_ = MacroProjectData(ID_ZERO_LEVEL);
		fog_start_  = MacroProjectData(ID_FOG_START);
		fog_end_    = MacroProjectData(ID_FOG_END);
		fog_colour_ = MacroProjectData(ID_FOG_COLOUR);
		sps_[0]     = MacroProjectData(ID_SP_0);
		sps_[1]     = MacroProjectData(ID_SP_1);
		sps_[2]     = MacroProjectData(ID_SP_2);
		sps_[3]     = MacroProjectData(ID_SP_3);
		sps_[4]     = MacroProjectData(ID_SP_4);
	}

	void MapInfo::ReplaceSubstring(tstring &str, const tstring &target, const tstring &replacement)
	{
		typedef tstring::size_type SizeType;
		SizeType i(str.find(target));
		str = str.replace(i, target.size(), replacement);
	}

	tstring MapInfo::GenerateWorldIni()
	{
		tstring world_ini;
		// load the template resource
		{
			HRSRC   resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_WORLD_INI), "Text"));
			HGLOBAL resource(LoadResource(NULL, resource_info));
			LPCTSTR text(ri_cast<LPCTSTR>(LockResource(resource)));
			world_ini = text;
		}
		TCHAR str[256];
		// replace map_power_x
		_itot(power_x_, str, 10);
		ReplaceSubstring(world_ini, "%map_power_x%", str);
		// replace map_power_y
		_itot(power_y_, str, 10);
		ReplaceSubstring(world_ini, "%map_power_y%", str);
		// replace zero_plast
		_itot(zero_level_, str, 10);
		ReplaceSubstring(world_ini, "%zero_plast%", str);
		// replace fog_start
		_itot(fog_start_, str, 10);
		ReplaceSubstring(world_ini, "%fog_start%", str);
		// replace fog_end
		_itot(fog_end_, str, 10);
		ReplaceSubstring(world_ini, "%fog_end%", str);
		// replace fog_colour
		_stprintf(
			str,
			_T("%u %u %u"),
			(fog_colour_ & 0x000000FF) >> 16,
			(fog_colour_ & 0x0000FF00) >> 8,
			(fog_colour_ & 0x00FF0000) >> 0);
		ReplaceSubstring(world_ini, "%fog_colour%", str);
		return world_ini;
	}

	void MapInfo::GetRawData(BYTE **data, size_t *size)
	{
		size_t sizes[8] =
		{
			map_name_.size() * sizeof(tstring::value_type),
			sizeof(power_x_),
			sizeof(power_y_),
			sizeof(zero_level_),
			sizeof(fog_start_),
			sizeof(fog_end_),
			sizeof(fog_colour_),
			sizeof(sps_)
		};
		*size = 0;
		for (int i(0); i != 8; ++i)
			*size += sizes[i];
		*data = new BYTE[*size];
		CopyMemory(*data, map_name_.c_str(), sizes[0]);
		CopyMemory(*data, &power_x_,         sizes[1]);
		CopyMemory(*data, &power_y_,         sizes[2]);
		CopyMemory(*data, &zero_level_,      sizes[3]);
		CopyMemory(*data, &fog_start_,       sizes[4]);
		CopyMemory(*data, &fog_end_,         sizes[5]);
		CopyMemory(*data, &fog_colour_,      sizes[6]);
		CopyMemory(*data, &sps_,             sizes[7]);
	}

	void MapInfo::Pack(TiXmlNode &node)
	{
		const uint max_str_length(16);
		TCHAR str[max_str_length];
		// map_name
		node.InsertEndChild(TiXmlElement("map_name"))->InsertEndChild(TiXmlText(map_name_));
		// map_power_x_
		itoa(power_x_, str, 10);
		node.InsertEndChild(TiXmlElement("map_power_x"))->InsertEndChild(TiXmlText(str));
		// map_power_y_
		itoa(power_y_, str, 10);
		node.InsertEndChild(TiXmlElement("map_power_y"))->InsertEndChild(TiXmlText(str));
		// zero_level_
		itoa(zero_level_, str, 10);
		node.InsertEndChild(TiXmlElement("zero_plast"))->InsertEndChild(TiXmlText(str));
		// fog_start_
		itoa(fog_start_, str, 10);
		node.InsertEndChild(TiXmlElement("fog_start"))->InsertEndChild(TiXmlText(str));
		// fog_end_
		itoa(fog_end_, str, 10);
		node.InsertEndChild(TiXmlElement("fog_end"))->InsertEndChild(TiXmlText(str));
		// fog_colour_
		ltoa(fog_colour_, str, 10);
		node.InsertEndChild(TiXmlElement("fog_colour"))->InsertEndChild(TiXmlText(str));
		// start_pos
		for (size_t i(0); i != 5; ++i)
		{
			// sps_[i].x
			itoa(sps_[i].x, str, 10);
			node.InsertEndChild(TiXmlElement(GenerateStartPosName(i, true)))->InsertEndChild(TiXmlText(str));
			// sps_[i].y
			itoa(sps_[i].y, str, 10);
			node.InsertEndChild(TiXmlElement(GenerateStartPosName(i, false)))->InsertEndChild(TiXmlText(str));
		}
	}

	string MapInfo::GenerateStartPosName(int index, bool x) const
	{
		tstring name(_T("starting_position_"));
		TCHAR index_string[16];
		_itot(index, index_string, 10);
		name.append(index_string);
		name.append(x ? _T("_x") : _T("_y"));
		return name;
	}

	//-----------------------
	// Texture implementation
	//-----------------------

	Texture::Texture(SIZE size, HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,indices_    (NULL)
		,size_       (size)
	{}

	Texture::~Texture()
	{
		delete [] indices_;
	}

	bool Texture::Load(LPCTSTR path, bool fast_quantization)
	{
		fipImage image;
		// load the texture image
		{
			const DWORD delay(256);
			const uint  try_count(32);
			uint        try_num(0);
			for (; try_num != try_count; ++try_num)
			{
				image.load(path);
				if (TRUE == image.isValid())
					break;
				else
					Sleep(delay);
			}
			if (try_count == try_num)
			{
				MacroDisplayError(_T("Texture could not be loaded."));
				return false;
			}
		}
		// make sure the dimensions are correct
		if (image.getWidth() != size_.cx || image.getHeight() != size_.cy)
		{
			MacroDisplayError(_T("Texture dimensions do not correspond to project settigns."));
			return false;
		}
		// quantize the image if necessary
		if (FIC_PALETTE != image.getColorType())
		{
			FREE_IMAGE_QUANTIZE mode(fast_quantization ? FIQ_WUQUANT : FIQ_NNQUANT);
			if (FALSE == image.colorQuantize(mode))
			{
				MacroDisplayError(_T("Texture could not be quantized."));
				return false;
			}
		}
		// flip the image vertically
		if (FALSE == image.flipVertical())
			MacroDisplayError(_T("Texture bitmap could not be flipped."));
		// allocate new memory for the texture
		const size_t indices_size(size_.cx * size_.cy);
		_ASSERTE(NULL == indices_);
		indices_ = new BYTE[indices_size];
		if (NULL == indices_)
		{
			MacroDisplayError(_T("Memory for loading the texture could not be allocated."));
			return false;
		}
		// extract image data
		CopyMemory(indices_, image.accessPixels(), indices_size);
		const size_t palette_size(__min(256, image.getPaletteSize() / 4));
		RGBQUAD *palette(image.getPalette());
		for (size_t i(0); i != palette_size; ++i)
			palette_[i] = RGB(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
		return true;
	}

	int Texture::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask) const
	{
		// pack the texture and the palette
		const int texture_size(size_.cx * size_.cy);
		const int palette_size(256 * sizeof(COLORREF));
		for (int i(0); i != texture_size; ++i)
			if (mask[i])
				indices_[i] = 0;
		CopyMemory(buffer, indices_, texture_size);
		CopyMemory(buffer + texture_size, palette_, palette_size);
		// write XML metadata
		{
			char str[16];
			// offset of compressed texture data
			_itot(buffer - initial_offset, str, 10);
			node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
			// offset of the null point mask
			_itot(buffer - initial_offset + texture_size, str, 10);
			node.InsertEndChild(TiXmlElement("palette_offset"))->InsertEndChild(TiXmlText(str));
		}
		return texture_size + palette_size;
	}

	void Texture::Unpack(TiXmlNode *node, BYTE *buffer)
	{
		// allocate memory for the texture
		const size_t indices_size(size_.cx * size_.cy);
		const size_t palette_size(256 * sizeof(COLORREF));
		_ASSERTE(NULL == indices_);
		indices_ = new BYTE[indices_size];
		// read in XML metadata
		BYTE *texture_buffer;
		BYTE *palette_buffer;
		{
			// find data
			TiXmlHandle node_handle(node);
			TiXmlText *offset_node        (node_handle.FirstChildElement("offset"        ).FirstChild().Text());
			TiXmlText *palette_offset_node(node_handle.FirstChildElement("palette_offset").FirstChild().Text());
			if (
				NULL == offset_node ||
				NULL == palette_offset_node)
			{
				_RPT0(_CRT_WARN, "loading default texture\n");
				DefaultTexture(indices_, palette_, size_);
				return;
			}
			// parse the data
			texture_buffer = buffer + atoi(offset_node->Value());
			palette_buffer = buffer + atoi(palette_offset_node->Value());
		}
		CopyMemory(palette_, palette_buffer, palette_size);
		CopyMemory(indices_, texture_buffer, indices_size); // WARN: possible buffer overflow
		return;
	}

	//---------
	// defaults
	//---------

	void DefaultHeightmap(BYTE *buffer, SIZE size)
	{
		ZeroMemory(buffer, size.cx * size.cy);
	}

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

	void SaveHeightmap(LPCTSTR path, const BYTE *buffer, SIZE size, ErrorHandler &error_handler)
	{
		// initialize the heightmap image
		fipImage image(
			FIT_BITMAP,
			static_cast<WORD>(size.cx),
			static_cast<WORD>(size.cy),
			8);
		// fill the image
		if (NULL != buffer)
			CopyMemory(image.accessPixels(), buffer, size.cx * size.cy);
		else
			ZeroMemory(image.accessPixels(), size.cx * size.cy);
		// save the image
		if (FALSE == image.save(path, BMP_DEFAULT))
			error_handler.MacroDisplayError(_T("Heightmap could not be saved."));
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

	void SaveSPG(
		MapInfo      &map_info,
		LPCTSTR       path,
		LPCTSTR       folder_name,
		const bool    survival,
		ErrorHandler &error_handler)
	{
		const size_t spg_alloc(8388608); // 8 MB should be enough
		size_t spg_size(spg_alloc);
		char *spg(new char[spg_size]);
		char *spg_iter1(spg), *spg_iter2;
		// make sure the allocation was successful
		if (NULL == spg)
		{
			error_handler.MacroDisplayError("Not enough memory.");
			return;
		}
		// load the spg template
		{
			BYTE *compressed_buffer;
			// load the compressed template
			HRSRC resource_info(FindResource(
				NULL,
				MAKEINTRESOURCE(survival ? IDR_SURVIVAL_SPG : IDR_SPG),
				"BZ2"));
			HGLOBAL resource(LoadResource(NULL, resource_info));
			compressed_buffer = ri_cast<BYTE*>(LockResource(resource));
			// uncompress
			if (BZ_OK != BZ2_bzBuffToBuffDecompress(
				spg,
				&spg_size,
				ri_cast<char*>(compressed_buffer),
				SizeofResource(NULL, resource_info),
				0,
				0))
			{
				error_handler.MacroDisplayError(_T("BZ2_bzBuffToBuffDecompress failed"));
				delete [] spg;
				return;
			}
		}
		// create an output stream
		std::ofstream spg_out(path, std::ios_base::binary | std::ios_base::out);
		// set map name
		{
			// get map name
			char target[] = "%folder_name%";
			spg_iter2 = strstr(spg, target);
			if (NULL == spg_iter2)
			{
				error_handler.MacroDisplayError(_T("Perimeter Map Compiler seems to have been corrupted. Please reinstall."));
				delete [] spg;
				return;
			}
			spg_out.write(spg_iter1, spg_iter2 - spg_iter1);
			spg_out << folder_name;
			spg_iter2 += sizeof(target) - 1;
		}
		// set Frame positions
		const unsigned int pos_range_start(survival ? 0 : 1);
		const unsigned int pos_range_end  (survival ? 1 : 5);
		for (size_t i(pos_range_start); i != pos_range_end; ++i)
		{
			char target[] = "%frame_position_X%";
			itoa(i, target + sizeof(target) - 3, 10);
			spg_iter1 = spg_iter2;
			spg_iter2 = strstr(spg_iter2, target);
			if (NULL == spg_iter2)
			{
				error_handler.MacroDisplayError(_T("Program seems to have been corrupted. Please reinstall."));
				delete [] spg;
				return;
			}
			spg_out.write(spg_iter1, spg_iter2 - spg_iter1);
			if (survival)
				spg_out << map_info.sps_[0].x << " " << map_info.sps_[0].y << " " << 256;
			else
				spg_out << map_info.sps_[i].x << " " << map_info.sps_[i].y << " " << 256;
			spg_iter2 += sizeof(target) - 1;
		}
		// set camera positions
		for (size_t i(pos_range_start); i != pos_range_end; ++i)
		{
			char target[] = "%camera_position_X%";
			itoa(i, target + sizeof(target) - 3, 10);
			// replace both occurences
			spg_iter1 = spg_iter2;
			spg_iter2 = strstr(spg_iter2, target);
			if (NULL == spg_iter2)
			{
				error_handler.MacroDisplayError(_T("Program seems to have been corrupted. Please reinstall."));
				delete [] spg;
				return;
			}
			spg_out.write(spg_iter1, spg_iter2 - spg_iter1);
			if (survival)
				spg_out << map_info.sps_[0].x << " " << map_info.sps_[0].y;
			else
				spg_out << map_info.sps_[i].x << " " << map_info.sps_[i].y;
			spg_iter2 += sizeof(target) - 1;
		}
		// output the rest of the data
		spg_out.write(spg_iter2, spg_size - (spg_iter2 - spg) - 1); // minus one for the terminating zero
		// clean up
		delete [] spg;
	}

	void SaveSPH(
		LPCTSTR path,
		LPCTSTR folder_name,
		const bool survival,
		ErrorHandler &error_handler)
	{
		const size_t sph_alloc(4096); // 4 KB should be enough
		size_t sph_size(sph_alloc);
		char *sph(new char[sph_size]);
		char *sph_iter1(sph), *sph_iter2(sph);
		// make sure the allocation was successful
		if (NULL == sph)
		{
			error_handler.MacroDisplayError("Not enough memory.");
			return;
		}
		// load the sph template
		{
			BYTE *compressed_buffer;
			// load the compressed template
			HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(IDR_SPH), "BZ2"));
			HGLOBAL resource(LoadResource(NULL, resource_info));
			compressed_buffer = ri_cast<BYTE*>(LockResource(resource));
			// uncompress
			if (BZ_OK != BZ2_bzBuffToBuffDecompress(
				sph,
				&sph_size,
				ri_cast<char*>(compressed_buffer),
				SizeofResource(NULL, resource_info),
				0,
				0))
			{
				error_handler.MacroDisplayError(_T("BZ2_bzBuffToBuffDecompress failed"));
				delete [] sph;
				return;
			}
		}
		// create an output stream
		std::ofstream sph_out(path, std::ios_base::binary | std::ios_base::out);
		// set map name
		{
			char target[] = "%folder_name%";
			sph_iter1 = sph_iter2;
			sph_iter2 = strstr(sph_iter2, target);
			if (NULL == sph_iter2)
			{
				error_handler.MacroDisplayError(_T("Program seems to have been corrupted. Please reinstall."));
				delete [] sph;
				return;
			}
			sph_out.write(sph_iter1, sph_iter2 - sph_iter1);
			sph_out << folder_name;
			sph_iter2 += sizeof(target) - 1;
		}
		// set the number of players
		{
			char target[] = "%player_count%";
			sph_iter1 = sph_iter2;
			sph_iter2 = strstr(sph_iter2, target);
			if (NULL == sph_iter2)
			{
				error_handler.MacroDisplayError(_T("Program seems to have been corrupted. Please reinstall."));
				delete [] sph;
				return;
			}
			sph_out.write(sph_iter1, sph_iter2 - sph_iter1);
			char *player_count(survival ? "1" : "4");
			sph_out << player_count;
			sph_iter2 += sizeof(target) - 1;
		}
		// set map path
		{
			char target[] = "%folder_name%";
			sph_iter1 = sph_iter2;
			sph_iter2 = strstr(sph_iter2, target);
			if (NULL == sph_iter2)
			{
				error_handler.MacroDisplayError(_T("Program seems to have been corrupted. Please reinstall."));
				delete [] sph;
				return;
			}
			sph_out.write(sph_iter1, sph_iter2 - sph_iter1);
			string map_name;
			if (survival)
				map_name = "survival\\\\";
			map_name += folder_name;
			sph_out << map_name;
			sph_iter2 += sizeof(target) - 1;
		}
		// output the rest of the data
		sph_out.write(sph_iter2, sph_size - (sph_iter2 - sph) - 1); // minus one for the terminating zero
		// clean up
		delete [] sph;
	}

	void SaveTexture(
		LPCTSTR path,
		const BYTE *buffer,
		const COLORREF palette[256],
		SIZE size,
		ErrorHandler &error_handler)
	{
		// initialize the texture image
		fipImage image(
			FIT_BITMAP,
			static_cast<WORD>(size.cx),
			static_cast<WORD>(size.cy),
			8);
		// fill the image
		if (NULL != buffer)
		{
			CopyMemory(image.accessPixels(), buffer, size.cx * size.cy);
			CopyMemory(image.getPalette(), palette, 256 * 4); // ASSUME sizeof(COLORREF) == 4
		}
		else
		{
			ZeroMemory(image.accessPixels(), size.cx * size.cy);
			FillMemory(image.getPalette(), 256 * 4, 0xFF);
		}
		// convert the image to 24 bits (for easier editing)
		if (FALSE == image.convertTo24Bits())
			error_handler.MacroDisplayError(_T("Could not convert the texture to 24-bit."));
		// save the image
		if (FALSE == image.save(path, BMP_SAVE_RLE))
			error_handler.MacroDisplayError(_T("Texture could not be saved"));
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
		SIZE img_size(texture.size_);
		// initialize the map preview image
		fipImage image(FIT_BITMAP, static_cast<WORD>(img_size.cx), static_cast<WORD>(img_size.cy), 24);
		// fill the image with data from the texture and from the lightmap
		{
			const BYTE *lightmap_i(lightmap.data_);
			const BYTE *texture_i(texture.indices_);
			const BYTE * const texture_end(texture_i + texture.size_.cx * texture.size_.cy);
			BYTE *image_i(image.accessPixels());
			while (texture_i != texture_end)
			{
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
		{
			const BYTE *heightmap_ptr(heightmap.data_);
			BYTE *image_data(image.accessPixels());
			for (int c(0); c != img_size.cx; ++c)
			{
				for (LONG r(0); r != img_size.cy; ++r)
				{
					if (0 == *heightmap_ptr++)
						image_data[0] = image_data[1] = image_data[2] = 0;
					image_data +=3;
				}
				++heightmap_ptr;
			}
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
	void SaveVMP(const Heightmap &heightmap, const Texture &texture, LPCTSTR path, ErrorHandler &error_handler)
	{
		// get dimensions of the map
		SIZE map_size(texture.size_);
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
		// fill the first layer with zeros
		{
			ZeroMemory(vmp_iter, map_data_size);
			vmp_iter += map_data_size;
		}
		// extract necessary heightmap data
		int *int_heightmap(NULL);
		bool *null_pixels(NULL);
		{
			// allocate memory
			const size_t heightmap_data_size(heightmap.size_.cx * heightmap.size_.cy);
			int_heightmap = new int [heightmap_data_size];
			null_pixels   = new bool[heightmap_data_size];
			// set iterators
					int  *       int_heightmap_iter(int_heightmap);
					bool *       null_pixels_iter(null_pixels);
					const BYTE *       heightmap_iter(heightmap.data_);
			const BYTE * const heightmap_end(heightmap_iter + heightmap_data_size);
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
		StackBlur(int_heightmap, map_size.cx + 1, map_size.cy + 1, 4);
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

	//-----------------------------------
	// "incredible math" (Lithium Flower)
	//-----------------------------------

	COLORREF AverageColour(const Texture &texture, const Heightmap &heightmap)
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

	uint AverageHeight(const Heightmap &heightmap)
	{
		// ASSUME the size is no larger than 2^28x2^28
		unsigned __int64 sum(0);
		int n(0);
		const BYTE* i(heightmap.data_);
		const BYTE* end(i + heightmap.size_.cx * heightmap.size_.cy);
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
		return static_cast<uint>(sum);
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
		// TODO: can be optimized - profile to see if it is necessary
		for (uint texture_y(0); texture_y != allocation.y_count_; ++texture_y)
		{
			for (uint texture_x(0); texture_x != allocation.x_count_; ++texture_x)
			{
				BYTE *texture_iterator(ri_cast<BYTE*>(allocation.bitmaps_[texture_index]));
				// copy texture data
				int offset(initial_offset);
				for (uint y(0); y != allocation.height_; ++y)
				{
					for (uint x(0); x != allocation.width_; ++x)
					{
						const COLORREF colour(texture.palette_[texture.indices_[offset]]);
						if (!enable_lighting)
						{
							texture_iterator[3] = 0xFF; // alpha
							texture_iterator[2] = GetRValue(colour);
							texture_iterator[1] = GetGValue(colour);
							texture_iterator[0] = GetBValue(colour);
						}
						else
						{
							float f_light(static_cast<float>(lightmap.data_[offset]));
							f_light = (f_light + 128.0f) / 255.0f;
							texture_iterator[3] = 0xFF; // alpha
							texture_iterator[2] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetRValue(colour) * f_light)));
							texture_iterator[1] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetGValue(colour) * f_light)));
							texture_iterator[0] = static_cast<BYTE>(__max(0.0f, __min(255.0f, GetBValue(colour) * f_light)));
						}
						++offset;
						texture_iterator += 4;
					}
					offset += texture.size_.cx - allocation.width_;
				}
				// prepare for the next cycle
				initial_offset += allocation.width_;
				++texture_index;
			}
			initial_offset += texture.size_.cx * (allocation.height_ - 1);
		}
	}

	// based on code by Mario Klingemann <mario@quasimondo.com>
	// blurs a 2d integer array with not regard for components
	void StackBlur(int *pix, int w, int h, int radius)
	{
		int wm(w - 1);
		int hm(h - 1);
		int wh(w * h);
		int div(radius + radius + 1);

		int *c(new int[wh]);
		int sum, x, y, i, p, yp, yi, yw;
		int *vmin(new int[__max(w,h)]);

		int divsum((div + 1) >> 1);
		divsum *= divsum;
		int *dv(new int[8192 * divsum]);
		for (i = 0; i < 8192 * divsum; ++i)
			dv[i] = (i / divsum);

		yw = yi = 0;

		int *stack(new int[div]);
		int stackpointer;
		int stackstart;
		int *sir;
		int rbs;
		int r1(radius + 1);
		int outsum;
		int insum;

		for (y = 0; y < h; ++y)
		{
			insum = outsum = sum = 0;
			// fill the stack and calculuate sums
			for(i = -radius; i <= radius; ++i)
			{
				p = pix[yi + __min(wm, __max(i, 0))];
				stack[i + radius] = p;
				rbs = r1 - abs(i);
				sum += p * rbs;
				if (i > 0)
					insum += p;
				else
					outsum += p;
			}
			stackpointer = radius;

			for (x = 0; x < w; ++x)
			{
				c[yi] = dv[sum];
				sum -= outsum;
				stackstart = stackpointer - radius + div;
				sir = stack + stackstart % div;
				outsum -= *sir;
				if(y == 0)
					vmin[x] = __min(x + radius + 1, wm);
				*sir = pix[yw + vmin[x]];
				insum += *sir;
				sum += insum;
				stackpointer = (stackpointer + 1) % div;
				sir = stack + stackpointer % div;
				outsum += *sir;
				insum -= *sir;
				++yi;
			}
			yw += w;
		}
		for (x = 0; x < w; x++)
		{
			insum = outsum = sum = 0;
			yp =- radius * w;
			for(i = -radius; i <= radius; ++i)
			{
				yi = __max(0,yp) + x;
				sir = stack + i + radius;
				*sir = c[yi];
				rbs=r1-abs(i);
				sum += c[yi] * rbs;
				if (i > 0)
					insum += *sir;
				else
					outsum += *sir;
				if (i < hm)
					yp += w;
			}
			yi = x;
			stackpointer = radius;
			for (y = 0; y < h; ++y)
			{
				pix[yi] = dv[sum];
				sum -= outsum;
				stackstart = stackpointer - radius + div;
				sir = stack + stackstart % div;
				outsum -= *sir;
				if(x==0)
					vmin[y] = __min(y + r1,hm) * w;
				p = x + vmin[y];
				*sir = c[p];
				insum += *sir;
				sum += insum;
				stackpointer=(stackpointer+1)%div;
				sir = stack + stackpointer;
				outsum += *sir;
				insum -= *sir;
				yi += w;
			}
		}
		delete [] stack;
		delete [] dv;
		delete [] vmin;
		delete [] c;
	}

	// triangulation helper function - flatness heuristic
	// determines flatness from the center point and the middles of the four sides of a quad
	// the result is the distance between the projections of the middle point on the horizontal and the vertical
	float Flatness(int h1, int h2, int v1, int v2, int c, int r)
	{
		float dx;
		float dy;
		float c_dy;
		float length;
		float cnst;
		// project c onto h2 - h1
		dx   = static_cast<float>(r + r);
		dy   = static_cast<float>(h2 - h1);
		c_dy = static_cast<float>(c - h1);
		length = sqrt(dx * dx + dy * dy);
		dx /= length;
		dy /= length;
		cnst = (r * dx + c_dy * dy) / (dx * dx + dy * dy);
		float h_proj_x = r    - cnst * dx;
		float h_proj_y = c_dy - cnst * dy;
		// project c onto v2 - v1
		dx   = static_cast<float>(r  + r);
		dy   = static_cast<float>(v2 - v1);
		c_dy = static_cast<float>(c  - v1);
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

	// convert a heightmap to a mesh in O(n*ln(n))
	// the algorithm takes two passes over the heightmap:
	//  at first pass it checks progressively coarser grids, determining which areas are "flat enough"
	//  at second pass it checks progressively finer grids, converting flat areas to quads, and aligning edges
	void Triangulate(Heightmap &heightmap, vector<SimpleVertex> &vertices, float mesh_threshold)
	{
		int n; // map power
		SIZE map_size = { heightmap.size_.cx - 1, heightmap.size_.cy - 1 };
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
					int offset = xi * (map_size.cx + 1) * 2 + (map_size.cx + 1) + yi * 2 + 1;
					// stop if any of the underlying points are zero
					for (int x(-1); x != 2; ++x)
						for (int y(-1); y != 2; ++y)
							if (heightmap.data_[offset + y * (map_size.cx + 1) + x] == 0)
								goto end;
					// check if the region is flat enough
					if (Flatness(
						(int)heightmap.data_[offset - 1],
						(int)heightmap.data_[offset + 1],
						(int)heightmap.data_[offset - map_size.cx - 1],
						(int)heightmap.data_[offset + map_size.cx + 1],
						(int)heightmap.data_[offset],
						1) < threshold)
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
							(int)heightmap.data_[offset - radius],
							(int)heightmap.data_[offset + radius],
							(int)heightmap.data_[offset - (map_size.cx + 1) * radius],
							(int)heightmap.data_[offset + (map_size.cx + 1) * radius],
							(int)heightmap.data_[offset],
							radius) < threshold)
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
							BYTE *offset(heightmap.data_ + side * ((map_size.cx + 1) * yi + xi));
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
					BYTE *offset(heightmap.data_ + 2 * ((map_size.cx + 1) * yi + xi));
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
		}
		delete [] regions;
	}

	//------
	// other
	//------

	// get Perimeter installation path from the registry
	bool GetInstallPath(tstring &install_path, ErrorHandler &error_handler)
	{
		// check the registry for necessary information
		HKEY perimeter_key;
		// open Perimeter's registry key
		if (ERROR_SUCCESS != RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			_T("SOFTWARE\\Codemasters\\Perimeter"),
			0,
			KEY_READ,
			&perimeter_key))
		{
			error_handler.MacroDisplayError(_T("Please make sure you have Perimeter installed."));
			return false;
		}
		// verify Perimeter version
		{
			DWORD version;
			DWORD version_length;
			DWORD version_type;
			bool russian(true);
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Version"),
				NULL,
				&version_type,
				NULL,
				&version_length))
			{
				russian = false;
			}
			if (russian)
			{
				if (version_length != 4 || version_type != REG_DWORD)
				{
					RegCloseKey(perimeter_key);
					error_handler.MacroDisplayError(_T("Wrong Version type."));
					return false;
				}
				if (ERROR_SUCCESS != RegQueryValueEx(
					perimeter_key,
					_T("Version"),
					NULL,
					NULL,
					ri_cast<BYTE*>(&version),
					&version_length))
				{
					RegCloseKey(perimeter_key);
					error_handler.MacroDisplayError(_T("RegQueryValueEx failed"));
					return false;
				}
				if (version != 101)
				{
					RegCloseKey(perimeter_key);
					error_handler.MacroDisplayError(_T("Incompatible Perimeter version. Please make sure you have version 1.01."));
					return false;
				}
			}
			else
			{
				HKEY patch_key;
				if (ERROR_SUCCESS != RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					_T("SOFTWARE\\Codemasters\\Perimeter Patch 1.01"),
					0,
					KEY_READ,
					&patch_key))
				{
					RegCloseKey(perimeter_key);
					error_handler.MacroDisplayError(_T("Incompatible Perimeter version. Please make sure you have version 1.01."));
					return false;
				}
				RegCloseKey(patch_key);
			}
		}
		// get path to Perimeter's installation folder
		{
			DWORD install_path_type;
			DWORD install_path_length;
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("INSTALL_PATH"),
				NULL,
				&install_path_type,
				NULL,
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				error_handler.MacroDisplayError(_T("RegQueryValueEx failed"));
				return false;
			}
			if (REG_SZ != install_path_type)
			{
				RegCloseKey(perimeter_key);
				error_handler.MacroDisplayError(_T("Wrong Install_Path type."));
				return false;
			}
			TCHAR install_path_temp[MAX_PATH];
			if (ERROR_SUCCESS != RegQueryValueEx(
				perimeter_key,
				_T("Install_Path"),
				NULL,
				NULL,
				ri_cast<BYTE*>(install_path_temp),
				&install_path_length))
			{
				RegCloseKey(perimeter_key);
				error_handler.MacroDisplayError(_T("RegQueryValueEx failed"));
				return false;
			}
			install_path = install_path_temp;
		}
		RegCloseKey(perimeter_key);
		return true;
	}

	bool RegisterMap(LPCTSTR map_name, DWORD checksum, ErrorHandler &error_handler)
	{
		// check connection status
		if (FALSE == InternetCheckConnection(
			_T("http://www.rul-clan.ru/map_registration/map_register.php"),
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
					tstring unsafe_url(_T("http://www.rul-clan.ru/map_registration/map_register.php?map_name="));
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
//			const TCHAR * const error_name(_T("Registration Error"));
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