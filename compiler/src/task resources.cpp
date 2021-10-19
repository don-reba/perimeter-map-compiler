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
#include "project data.h"
#include "resource management.h"
#include "task resources.h"
#include "xml creator.h"

#include "gaussian blur.ipp"

#include <algorithm>
#include <sstream>

#include <loki/ScopeGuard.h>

using namespace RsrcMgmt;

namespace TaskCommon
{
	
	//------------------------
	// Hardness implementation
	//------------------------

	Hardness::info_t::info_t()
		:path_()
		,size_()
	{}

	Hardness::Hardness(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,SaveCallback(RS_HARDNESS)
		,data_(NULL)
	{}

	Hardness::~Hardness()
	{
		if (NULL != data_)
			delete [] data_;
		data_ = NULL;
	}

	bool Hardness::Load()
	{
		_ASSERTE(NULL == data_);
		// load the image
		fipImage image;
		image.load(info_.path_.c_str());
		if (FALSE == image.isValid())
		{
			Sleep(512);
			image.load(info_.path_.c_str());
			if (FALSE == image.isValid())
			{
				MakeDefault();
				return false;
			}
		}
		// make sure that dimensions are correct
		if (image.getWidth() != info_.size_.cx || image.getHeight() != info_.size_.cy)
		{
			MacroDisplayError(_T("Hardness map dimensions do not correspond to project settings."));
			return false;
		}
		// convert to grayscale if necessary
		if (8 != image.getBitsPerPixel())
		{
			if (FALSE == image.convertToGrayscale())
			{
				MacroDisplayError(_T("Hardness map could not be converted to grayscale."));
				return false;
			}
		}
		// flip vertically
		if (FALSE == image.flipVertical())
			MacroDisplayError(_T("Hardness map bitmap could not be flipped."));
		// allocate memory
		const size_t data_size(info_.size_.cx * info_.size_.cy);
		_ASSERTE(NULL == data_);
		data_ = new BYTE[data_size];
		// read in data_
		CopyMemory(data_, image.accessPixels(), data_size);
		return true;
	}

	void Hardness::Unload()
	{
		//Save();
		delete [] data_;
		data_ = NULL;
	}

	int Hardness::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask)
	{
		// pack hardness map
		int encoded_size;
		{
			// create the image data matrix
			jas_matrix_t *data(jas_matrix_create(info_.size_.cy, info_.size_.cx)); // rows then columns
			{
				size_t index(0);
				for (LONG r(0); r != info_.size_.cy; ++r)
				{
					for (LONG c(0); c != info_.size_.cx; ++c)
					{
						jas_matrix_set(data, r, c, mask[index] ? 0 : data_[index]);
						++index;
					}
				}
			}
			// initialize the image component structure
			jas_image_cmptparm_t component;
			component.width  = info_.size_.cx;
			component.height = info_.size_.cy;
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
			if (0 != jas_image_writecmpt(image, 0, 0, 0, info_.size_.cx, info_.size_.cy, data))
				MacroDisplayError(_T("jas_imagewritecmpt failed"));
			// create an output stream
			size_t buffer_size(info_.size_.cx * info_.size_.cy);
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
		// record xml metadata
		{
			char str[16];
			// compression format
			node.InsertEndChild(TiXmlElement("compression"))->InsertEndChild(TiXmlText("JPC"));
			// offset of the compressed data
			_itot(buffer - initial_offset, str, 10);
			node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
			// size of the compressed data
			_itot(encoded_size, str, 10);
			node.InsertEndChild(TiXmlElement("size"))->InsertEndChild(TiXmlText(str));
		}
		return encoded_size;
	}

	void Hardness::Unpack(TiXmlNode *node, BYTE *buffer, const vector<bool> &mask)
	{
		if (NULL == node)
		{
			_RPT0(_CRT_WARN, "loading default hardness map\n");
			MakeDefault();
			return;
		}
		// allocate memory for the hardness
		const size_t data_size(info_.size_.cx * info_.size_.cy);
		_ASSERTE(NULL == data_);
		data_ = new BYTE[data_size];
		// read in XML metadata
		size_t compressed_hardness_size;
		BYTE *compressed_hardness;
		{
			// find data
			TiXmlHandle node_handle(node);
			TiXmlText *compression_node(node_handle.FirstChildElement("compression").FirstChild().Text());
			TiXmlText *offset_node     (node_handle.FirstChildElement("offset"     ).FirstChild().Text());
			TiXmlText *size_node       (node_handle.FirstChildElement("size"       ).FirstChild().Text());
			if (
				NULL == compression_node ||
				NULL == offset_node      ||
				NULL == size_node)
			{
				_RPT0(_CRT_WARN, "loading default hardness map\n");
				MakeDefault();
				return;
			}
			// make sure the compression format matches
			if (0 != strcmp(compression_node->Value(), "JPC"))
			{
				_RPT0(_CRT_WARN, "loading default hardness map\n");
				MakeDefault();
				return;
			}
			// finally parse the data
			compressed_hardness      = buffer + atoi(offset_node->Value());
			compressed_hardness_size = atoi(size_node->Value()); // WARN: possible buffer overflow
		}
		// unpack the hardness itself
		{
			// create an input stream
			jas_stream_t *stream(jas_stream_memopen(
				ri_cast<char*>(compressed_hardness),
				compressed_hardness_size));
			if (0 == stream)
			{
				MacroDisplayError(_T("jas_stream open failure"));
				MakeDefault();
				return;
			}
			// decode the image
			int format = jas_image_strtofmt("jpc");
			if (-1 == format)
			{
				MacroDisplayError(_T("jas_image_strtofmt failed"));
				MakeDefault();
				return;
			}
			jas_image_t *image(jas_image_decode(stream, format, ""));
			if (0 == image)
			{
				MacroDisplayError(_T("jas_image_decode failed"));
				MakeDefault();
				return;
			}
			// extract image data
			{
				// get the data matrix
				jas_matrix_t *data_matrix(jas_matrix_create(info_.size_.cy, info_.size_.cx));
				if (0 == data_matrix)
					MacroDisplayError(_T("jas_matrix_create failed"));
				if (0 > jas_image_readcmpt(image, 0, 0, 0, info_.size_.cx, info_.size_.cy, data_matrix))
					MacroDisplayError(_T("jas_image_readcmpt failed"));
				// extract image data,  0 where mask is true
				BYTE *data_ptr(data_);
				int mask_index(0);
				for (LONG r(0); r != info_.size_.cy; ++r)
					for (LONG c(0); c != info_.size_.cx; ++c)
						*data_ptr++ = mask[mask_index++] ? 0 : static_cast<BYTE>(jas_matrix_get(data_matrix, r, c));
			}
		}
	}

	void Hardness::MakeDefault()
	{
		_ASSERTE(NULL == data_);
		const size_t data_size(info_.size_.cx * info_.size_.cy);
		data_ = new BYTE[data_size];
		FillMemory(data_, data_size, 0xFF);
	}

	void Hardness::Save()
	{
		using namespace Loki;
		SaveBegin();
		LOKI_ON_BLOCK_EXIT_OBJ(*this, &Hardness::SaveEnd);
		SaveAs(info_.path_.c_str());
	}

	void Hardness::SaveAs(LPCTSTR path)
	{
		_ASSERTE(NULL != data_);
		// initialize the hardness image
		fipImage image(
			FIT_BITMAP,
			static_cast<WORD>(info_.size_.cx),
			static_cast<WORD>(info_.size_.cy),
			8);
		// fill the image
		CopyMemory(image.accessPixels(), data_, info_.size_.cx * info_.size_.cy);
		image.threshold(1);
		// save the image
		if (FALSE == image.save(path, BMP_DEFAULT))
			MacroDisplayError(_T("Hardness map could not be saved."));
	}

	//-------------------------
	// Heightmap implementation
	//-------------------------

	Heightmap::info_t::info_t()
		:path_      ()
		,size_      ()
		,zero_level_(0)
		,zero_layer_(NULL)
	{}

	Heightmap::Heightmap(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,SaveCallback(RS_HEIGHTMAP)
		,bpp_   (0)
		,data8_ (NULL)
		,data16_(NULL)
	{}

	Heightmap::~Heightmap()
	{
		if (NULL != data8_)
			delete [] data8_;
		if (NULL != data16_)
			delete [] data16_;
		data8_  = NULL;
		data16_ = NULL;
	}

	bool Heightmap::Load()
	{
		fipImage image;
		// load the image
		{
			const DWORD delay(256);
			const uint  try_count(32);
			uint        try_num(0);
			for (; try_num != try_count; ++try_num)
			{
				image.load(info_.path_.c_str());
				if (TRUE == image.isValid())
					break;
				else
					Sleep(delay);
			}
			if (try_count == try_num)
			{
				MacroDisplayError(_T("Heightmap could not be loaded."));
				return NULL;
			}
		}
		// make sure that dimensions are correct
		if (image.getWidth() != info_.size_.cx || image.getHeight() != info_.size_.cy)
		{
			MacroDisplayError(_T("Heightmap dimensions do not correspond to project settings."));
			return NULL;
		}
		size_.cx = info_.size_.cx + 1;
		size_.cy = info_.size_.cy + 1;
		// flip the image vertically
		if (FALSE == image.flipVertical())
			MacroDisplayError(_T("Heightmap bitmap could not be flipped."));
		// turn the image to grayscale, if necessary, and initialize the bitmap
		bpp_ = image.getBitsPerPixel();
		if (8 != bpp_ && 16 != bpp_)
		{
			if (FALSE == image.convertToGrayscale())
			{
				MacroDisplayError(_T("Heightmap could not be converted to grayscale."));
				return NULL;
			}
			bpp_ = 8;
		}
		switch (bpp_)
		{
		case 8:  return Load8 (image);
		case 16: return Load16(image);
		}
		return false;
	}

	void Heightmap::Unload()
	{
		//Save();
		switch (bpp_)
		{
		case 8:
			delete [] data8_;
			data8_ = NULL;
			break;
		case 16:
			delete [] data16_;
			data16_ = NULL;
			break;
		}
	}

	void Heightmap::Unpack(TiXmlNode *node, BYTE *buffer, vector<bool> &mask)
	{
		bpp_ = 8;
		size_.cx = info_.size_.cx + 1;
		size_.cy = info_.size_.cy + 1;
		TiXmlHandle node_handle(node);
		TiXmlText *bpp_node(node_handle.FirstChildElement("bpp").FirstChild().Text());
		if (NULL != bpp_node)
		{
			if (0 == strcmp(bpp_node->Value(), "16"))
				bpp_ = 16;
		}
		switch (bpp_)
		{
		case 8:
			Unpack8(node, buffer, mask);
			break;
		case 16:
			Unpack16(node, buffer, mask);
			break;
		}
	}

	int Heightmap::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask)
	{
		switch (bpp_)
		{
		case 8:  return Pack8 (node, buffer, initial_offset, mask);
		case 16: return Pack16(node, buffer, initial_offset, mask);
		}
		_ASSERT(false);
		return -1;
	}

	void Heightmap::MakeDefault()
	{
		switch (bpp_)
		{
		case 8:  MakeDefault8 (); break;
		case 16: MakeDefault16(); break;
		}
	}

	void Heightmap::Save()
	{
		using namespace Loki;
		SaveBegin();
		LOKI_ON_BLOCK_EXIT_OBJ(*this, &Heightmap::SaveEnd);
		SaveAs(info_.path_.c_str());
	}

	void Heightmap::SaveAs(LPCTSTR path)
	{
		// initialize the heightmap image
		fipImage image(
			FIT_BITMAP,
			static_cast<WORD>(info_.size_.cx),
			static_cast<WORD>(info_.size_.cy),
			static_cast<WORD>(bpp_));
		// fill the image
		switch (bpp_)
		{
		case 8:
			CopyMemory(image.accessPixels(), data8_, info_.size_.cx * info_.size_.cy);
			break;
		case 16:
			CopyMemory(image.accessPixels(), data16_, info_.size_.cx * info_.size_.cy * 2);
			break;
		default:
			MacroDisplayError("Invalid heightmap BPP.");
			return;
		}
		// save the image
		if (FALSE == image.save(path, 0))
			MacroDisplayError(_T("Heightmap could not be saved."));
	}

	bool Heightmap::Load8(fipImage &image)
	{
		_ASSERTE(8 == image.getBitsPerPixel());
		// set memory for the heightmap
		{
			// allocate
			const size_t data_size(size_.cx * size_.cy);
			_ASSERTE(NULL == data8_);
			data8_ = new BYTE[data_size];
			// copy data
			{
				const BYTE *image_iter(image.accessPixels());
				BYTE *heightmap_iter(data8_);
				for (LONG r(0); r != size_.cy - 1; ++r)
				{
					CopyMemory(heightmap_iter, image_iter, size_.cx - 1);
					heightmap_iter += size_.cx - 1;
					image_iter += size_.cx - 1;
					// pad horizontally
					*heightmap_iter = *(image_iter - 1);
					++heightmap_iter;
				}
				// pad vertically
				CopyMemory(heightmap_iter, heightmap_iter - size_.cx, size_.cx);
			}
		}
		// scale the heightmap vertically where zero layer is levelled, and nearby
		if (NULL != info_.zero_layer_)
		{
			const ZeroLayer &zero_layer(*info_.zero_layer_);
			// convert the zero layer into a float array, and blur the array
			const size_t size(zero_layer.data_.size());
			vector<ushort> blurred_zero_layer(size);
			for (size_t i(0); i != size; ++i)
				blurred_zero_layer[i] = zero_layer.data_[i] ? 0xFFFF : 0x0000;
			GaussianBlur<ushort, int>(&*blurred_zero_layer.begin(), size_);
			GaussianBlur<ushort, int>(&*blurred_zero_layer.begin(), size_);
			GaussianBlur<ushort, int>(&*blurred_zero_layer.begin(), size_);
			// scale the heightmap
			vector<ushort>::const_iterator bzl_iter (blurred_zero_layer.begin());
			BYTE *                        data_iter(data8_);
			for (LONG y(0); y != size_.cy - 1; ++y)
			{
				for (LONG x(0); x != size_.cx - 1; ++x)
				{
					// make sure the blur only expands the black areas
					uint v(*bzl_iter);
					v = (0 == (v & 0x8000)) ? 0 : 0xFF & v >> 7;
					// scale the pixel (optimized to avoid fp ops)
					uint scale(23);
					uint ratio((v << scale) / 0xFF);
					*data_iter = static_cast<BYTE>((*data_iter * ratio + info_.zero_level_ * ((1 << scale) - ratio)) >> scale);
					++data_iter;
					++bzl_iter;
				}
				++data_iter;
			}
		}
		return true;
	}

	bool Heightmap::Load16(fipImage &image)
	{
		_ASSERTE(16 == image.getBitsPerPixel());
		// set memory for the heightmap
		{
			// allocate
			const size_t data_size(size_.cx * size_.cy);
			_ASSERTE(NULL == data16_);
			data16_ = new WORD[data_size];
			// copy data
			{
				const WORD *image_iter(ri_cast<WORD*>(image.accessPixels()));
				WORD *heightmap_iter(data16_);
				for (LONG r(0); r != size_.cy - 1; ++r)
				{
					for (LONG c(0); c != size_.cx - 1; ++c)
						*heightmap_iter++ = *image_iter++ >> 3; // we really need only 13-bit maps
					// pad horizontally
					*heightmap_iter = *(image_iter - 1);
					++heightmap_iter;
				}
				// pad vertically
				CopyMemory(heightmap_iter, heightmap_iter - size_.cx, size_.cx * 2);
			}
		}
		// scale the heightmap vertically where zero layer is levelled, and nearby
		if (NULL != info_.zero_layer_)
		{
			const ZeroLayer &zero_layer(*info_.zero_layer_);
			// convert the zero layer into a float array, and blur the array
			const size_t size(zero_layer.data_.size());
			vector<ushort> blurred_zero_layer(size);
			for (size_t i(0); i != size; ++i)
				blurred_zero_layer[i] = zero_layer.data_[i] ? 0xFFFF : 0x0000;
			GaussianBlur<ushort, int>(&*blurred_zero_layer.begin(), size_);
			GaussianBlur<ushort, int>(&*blurred_zero_layer.begin(), size_);
			GaussianBlur<ushort, int>(&*blurred_zero_layer.begin(), size_);
			// adjust info_.zero_level_ to the higher bpp
			info_.zero_level_ *= 32;
			// scale the heightmap
			vector<ushort>::const_iterator  bzl_iter (blurred_zero_layer.begin());
			WORD                           *data_iter(data16_);
			for (LONG y(0); y != size_.cy - 1; ++y)
			{
				for (LONG x(0); x != size_.cx - 1; ++x)
				{
					// make sure the blur only expands the black areas
					uint v(*bzl_iter);
					v = (0 == (v & 0x8000)) ? 0 : 0xFF & v >> 7;
					// scale the pixel (optimized to avoid fp ops)
					unsigned __int64 scale(23);
					unsigned __int64 ratio((v << scale) / 0xFF);
					*data_iter = static_cast<WORD>((*data_iter * ratio + info_.zero_level_ * ((1 << scale) - ratio)) >> scale);
					++data_iter;
					++bzl_iter;
				}
				++data_iter;
			}
		}
		return true;
	}

	void Heightmap::MakeDefault8()
	{
		const size_t data_size(info_.size_.cx * info_.size_.cy);
		if (NULL == data8_)
			data8_ = new BYTE[data_size];
		ZeroMemory(data8_, data_size);
	}

	void Heightmap::MakeDefault16()
	{
		const size_t data_size(info_.size_.cx * info_.size_.cy);
		if (NULL == data16_)
			data16_ = new WORD[data_size];
		ZeroMemory(data16_, data_size * 2);
	}

	int Heightmap::Pack8(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask)
	{
		SIZE map_size = { info_.size_.cx - 1, info_.size_.cy - 1 };
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
						jas_matrix_set(data, r, c, data8_[index]);
						mask.push_back(0 == data8_[index]);
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
			mask_buffer = NULL;
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

	int Heightmap::Pack16(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask)
	{
		SIZE map_size = { info_.size_.cx - 1, info_.size_.cy - 1 };
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
						WORD value(data16_[index]);
						jas_matrix_set(data, r, c, value);
						mask.push_back(0 == data16_[index]);
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
			component.prec   = 13;
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
			mask_buffer = NULL;
		}
		// record xml metadata
		{
			char str[16];
			// 16-bit marker
			node.InsertEndChild(TiXmlElement("bpp"))->InsertEndChild(TiXmlText("16"));
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

	void Heightmap::Unpack8(TiXmlNode *node, BYTE *buffer, vector<bool> &mask)
	{
		SIZE map_size = { info_.size_.cx, info_.size_.cy };
		// allocate memory for the heightmap
		const size_t data_size(size_.cx * size_.cy);
		_ASSERTE(NULL == data8_);
		data8_ = new BYTE[data_size];
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
				MakeDefault();
				return;
			}
			// make sure the compression format matches
			if (0 != strcmp(compression_node->Value(), "JPC"))
			{
				_RPT0(_CRT_WARN, "loading default heightmap\n");
				MakeDefault();
				return;
			}
			// finally parse the data
			compressed_heightmap      = buffer + atoi(offset_node->Value());
			compressed_heightmap_size = atoi(size_node->Value()); // WARN: possible buffer overflow
			mask_buffer               = buffer + atoi(mask_offset_node->Value());
		}
		// unpack the mask
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
				MakeDefault();
				return;
			}
			// decode the image
			int format = jas_image_strtofmt("jpc");
			if (-1 == format)
			{
				MacroDisplayError(_T("jas_image_strtofmt failed"));
				MakeDefault();
				return;
			}
			jas_image_t *image(jas_image_decode(stream, format, ""));
			if (0 == image)
			{
				MacroDisplayError(_T("jas_image_decode failed"));
				MakeDefault();
				return;
			}
			_ASSERTE(8 == image->cmpts_[0]->prec_);
			// extract image data
			{
				// get the data matrix
				jas_matrix_t *data_matrix(jas_matrix_create(map_size.cy, map_size.cx));
				if (0 == data_matrix)
					MacroDisplayError(_T("jas_matrix_create failed"));
				if (0 > jas_image_readcmpt(image, 0, 0, 0, map_size.cx, map_size.cy, data_matrix))
					MacroDisplayError(_T("jas_image_readcmpt failed"));
				// extract image data, with padding, and 0 where mask is true
				BYTE *data_ptr(data8_);
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

	void Heightmap::Unpack16(TiXmlNode *node, BYTE *buffer, vector<bool> &mask)
	{
		SIZE map_size = { info_.size_.cx, info_.size_.cy };
		// allocate memory for the heightmap
		const size_t data_size(size_.cx * size_.cy);
		_ASSERTE(NULL == data16_);
		data16_ = new WORD[data_size];
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
				MakeDefault();
				return;
			}
			// make sure the compression format matches
			if (0 != strcmp(compression_node->Value(), "JPC"))
			{
				_RPT0(_CRT_WARN, "loading default heightmap\n");
				MakeDefault();
				return;
			}
			// finally parse the data
			compressed_heightmap      = buffer + atoi(offset_node->Value());
			compressed_heightmap_size = atoi(size_node->Value()); // WARN: possible buffer overflow
			mask_buffer               = buffer + atoi(mask_offset_node->Value());
		}
		// unpack the mask
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
				MakeDefault();
				return;
			}
			// decode the image
			int format = jas_image_strtofmt("jpc");
			if (-1 == format)
			{
				MacroDisplayError(_T("jas_image_strtofmt failed"));
				MakeDefault();
				return;
			}
			jas_image_t *image(jas_image_decode(stream, format, ""));
			if (0 == image)
			{
				MacroDisplayError(_T("jas_image_decode failed"));
				MakeDefault();
				return;
			}
			_ASSERTE(8 < image->cmpts_[0]->prec_);
			// extract image data
			{
				// get the data matrix
				jas_matrix_t *data_matrix(jas_matrix_create(map_size.cy, map_size.cx));
				if (0 == data_matrix)
					MacroDisplayError(_T("jas_matrix_create failed"));
				if (0 > jas_image_readcmpt(image, 0, 0, 0, map_size.cx, map_size.cy, data_matrix))
					MacroDisplayError(_T("jas_image_readcmpt failed"));
				// extract image data, with padding, and 0 where mask is true
				WORD *data_ptr(data16_);
				int mask_index(0);
				fipImage image(FIT_UINT16, static_cast<WORD>(map_size.cx), static_cast<WORD>(map_size.cy), 16);
				WORD *image_data(ri_cast<WORD*>(image.accessPixels()));
				for (LONG r(0); r != map_size.cy; ++r)
				{
					for (LONG c(0); c != map_size.cx; ++c)
						if (0 != mask[mask_index++])
							*data_ptr++ = 0;
						else
						{
							WORD value(static_cast<WORD>(jas_matrix_get(data_matrix, r, c)));
							*image_data++ = value;
							*data_ptr++ = value;
						}
					*data_ptr++ = *(data_ptr - 1); // pad horizontally
				}
				// pad vertically
				for (int c(0); c != map_size.cx + 1; ++c)
					*data_ptr++ = *(data_ptr - map_size.cx - 1);
			}
		}
	}

	//------------------------
	// Lightmap implementation
	//------------------------

	Lightmap::Lightmap(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,data_(NULL)
	{}

	Lightmap::~Lightmap()
	{
		if (NULL != data_)
			delete [] data_;
		data_ = NULL;
	}

	
	bool Lightmap::Create(const Heightmap &heightmap)
	{
		size_ = heightmap.info_.size_;
		switch (heightmap.bpp_)
		{
		case 8:  return Create8 (heightmap);
		case 16: return Create16(heightmap);
		}
		return false;
	}

	bool Lightmap::Create8(const Heightmap &heightmap)
	{
		// precalculate surface-sun dot products
		BYTE surface_sun_dot[0x100];
		{
			float normal_dy(1.0f);
			float light_dx (1.0f / static_cast<float>(sqrt(2.0f)));
			float light_dy (1.0f / static_cast<float>(sqrt(2.0f)));
			for (int i(0); i != 0x100; ++i)
			{
				float normal_dx(static_cast<float>(i));
				// calculate and store the cosine of the angle between the vectors
				float normal_l(sqrt(normal_dx * normal_dx + normal_dy * normal_dy));
				float cos_a((normal_dx * light_dx + normal_dy * light_dy) / normal_l);
				surface_sun_dot[i] = static_cast<BYTE>(0xFF * abs(cos_a));
			}
		}
		// allocate memory for the lightmap
		const size_t data_size(size_.cx * size_.cy);
		_ASSERTE(NULL == data_);
		data_ = new BYTE[data_size];
		// calculate lighting
		const BYTE *hi(heightmap.data8_); // heightmap iterator
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
				{
					int dy(__max(0, point_z - row_i[1]));
					*li = surface_sun_dot[dy];
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
		GaussianBlur<BYTE, ushort>(data_, size_);
		GaussianBlur<BYTE, ushort>(data_, size_);
		GaussianBlur<BYTE, ushort>(data_, size_);
		return true;
	}

	bool Lightmap::Create16(const Heightmap &heightmap)
	{
		// precalculate surface-sun dot products
		BYTE surface_sun_dot[0x400];
		{
			float normal_dy(4.0f);
			float light_dx (1.0f / static_cast<float>(sqrt(2.0f)));
			float light_dy (1.0f / static_cast<float>(sqrt(2.0f)));
			for (int i(0); i != 0x400; ++i)
			{
				float normal_dx(static_cast<float>(i));
				// calculate and store the cosine of the angle between the vectors
				float normal_l(sqrt(normal_dx * normal_dx + normal_dy * normal_dy));
				float cos_a((normal_dx * light_dx + normal_dy * light_dy) / normal_l);
				surface_sun_dot[i] = static_cast<BYTE>(0xFF * abs(cos_a));
			}
		}
		// allocate memory for the lightmap
		const size_t data_size(size_.cx * size_.cy);
		_ASSERTE(NULL == data_);
		data_ = new BYTE[data_size];
		// calculate lighting
		const WORD * hi(heightmap.data16_); // heightmap iterator
		BYTE       * li(data_); // lightmap iterator
		for (LONG r(0); r != size_.cy; ++r)
		{
			const WORD * row_i(hi + size_.cx);
			int          high_point_z(*row_i);
			const WORD * high_point_x(row_i);
			li += size_.cx;
			while (row_i != hi)
			{
				--li;
				--row_i;
				const ptrdiff_t point_x(high_point_x - row_i);
				const int       point_z(*row_i);
				if ((high_point_z - point_z) / 0x20 < point_x)
				{
					int dy((point_z - row_i[1]) >> 3);
					dy = __max(0, dy);
					*li = surface_sun_dot[dy];
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
		GaussianBlur<BYTE, ushort>(data_, size_);
		return true;
	}

	//-----------------------
	// MapInfo implementation
	//-----------------------

	// for use from the interface thread only
	MapInfo MapInfo::LoadFromGlobal()
	{
		MapInfo map_info;
		map_info.map_name_          = MacroProjectData(ID_MAP_NAME);
		map_info.power_x_           = MacroProjectData(ID_POWER_X);
		map_info.power_y_           = MacroProjectData(ID_POWER_Y);
		map_info.zero_level_        = MacroProjectData(ID_ZERO_LEVEL);
		map_info.fog_start_         = MacroProjectData(ID_FOG_START);
		map_info.fog_end_           = MacroProjectData(ID_FOG_END);
		map_info.fog_colour_        = MacroProjectData(ID_FOG_COLOUR);
		map_info.sps_[0]            = MacroProjectData(ID_SP_0);
		map_info.sps_[1]            = MacroProjectData(ID_SP_1);
		map_info.sps_[2]            = MacroProjectData(ID_SP_2);
		map_info.sps_[3]            = MacroProjectData(ID_SP_3);
		map_info.sps_[4]            = MacroProjectData(ID_SP_4);
		map_info.custom_hardness_   = MacroProjectData(ID_CUSTOM_HARDNESS);
		map_info.custom_sky_        = MacroProjectData(ID_CUSTOM_SKY);
		map_info.custom_surface_    = MacroProjectData(ID_CUSTOM_SURFACE);
		map_info.custom_zero_layer_ = MacroProjectData(ID_CUSTOM_ZERO_LAYER);
		return map_info;
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
			(fog_colour_ & 0x000000FF) >> 0,
			(fog_colour_ & 0x0000FF00) >> 8,
			(fog_colour_ & 0x00FF0000) >> 16);
		ReplaceSubstring(world_ini, "%fog_colour%", str);
		return world_ini;
	}

	void MapInfo::GetRawData(BYTE **data, size_t *size)
	{
		size_t sizes[8] =
		{
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
		CopyMemory(*data, &power_x_,    sizes[0]);
		CopyMemory(*data, &power_y_,    sizes[1]);
		CopyMemory(*data, &zero_level_, sizes[2]);
		CopyMemory(*data, &fog_start_,  sizes[3]);
		CopyMemory(*data, &fog_end_,    sizes[4]);
		CopyMemory(*data, &fog_colour_, sizes[5]);
		CopyMemory(*data, &sps_,        sizes[6]);
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

	void MapInfo::Unpack(TiXmlNode &node)
	{
		// read data from XML
		TiXmlNode *text_node;
		TiXmlHandle node_handle(&node);
		// map_name
		text_node = node_handle.FirstChildElement("map_name").FirstChild().Text();
		if (NULL != text_node)
			map_name_ = text_node->Value();
		// power_x
		text_node = node_handle.FirstChildElement("map_power_x").FirstChild().Text();
		if (NULL != text_node)
			power_x_ = atoi(text_node->Value());
		// power_y
		text_node = node_handle.FirstChildElement("map_power_y").FirstChild().Text();
		if (NULL != text_node)
			power_y_ = atoi(text_node->Value());
		// zero_plast
		text_node = node_handle.FirstChildElement("zero_plast").FirstChild().Text();
		if (NULL != text_node)
			zero_level_ = atoi(text_node->Value());
		// fog_start
		text_node = node_handle.FirstChildElement("fog_start").FirstChild().Text();
		if (NULL != text_node)
			fog_start_ = atoi(text_node->Value());
		// fog_end
		text_node = node_handle.FirstChildElement("fog_end").FirstChild().Text();
		if (NULL != text_node)
			fog_end_ = atoi(text_node->Value());
		// fog_colour
		text_node = node_handle.FirstChildElement("fog_colour").FirstChild().Text();
		if (NULL != text_node)
			fog_colour_ = atol(text_node->Value());
		// start_pos
		for (size_t i(0); i != 5; ++i)
		{
			// start_pos[i].x
			text_node = node_handle.FirstChildElement(GenerateStartPosName(i, true)).FirstChild().Text();
			if (NULL != text_node)
				sps_[i].x = atoi(text_node->Value());
			// start_pos[i].y
			text_node = node_handle.FirstChildElement(GenerateStartPosName(i, false)).FirstChild().Text();
			if (NULL != text_node)
				sps_[i].y = atoi(text_node->Value());
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

	//----------------------
	// Script implementation
	//----------------------

	Script::Script(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,SaveCallback(RS_SCRIPT)
	{}

	bool Script::Load()
	{
		if (false == doc_.LoadFile(info_.path_))
			return false;
		return true;
	}

	void Script::Unload()
	{
		Save();
		doc_.Clear();
	}

	void Script::Pack(TiXmlNode &node) const
	{
		node.InsertEndChild(*doc_.RootElement());
	}

	bool Script::Unpack(TiXmlNode &node)
	{
		doc_.Clear();
		TiXmlElement *script(node.FirstChildElement());
		if (NULL == script)
			return false;
		else
			doc_.InsertEndChild(*script);
		return true;
	}

	void Script::Save() const
	{
		using namespace Loki;
		SaveBegin();
		LOKI_ON_BLOCK_EXIT_OBJ(*this, &Script::SaveEnd);
		SaveAs(info_.path_.c_str());
	}

	void Script::SaveAs(LPCTSTR path) const
	{
		doc_.SaveFile(info_.path_);
	}

	//-------------------
	// Sky implementation
	//-------------------

	const SIZE Sky::size_ = { 0x200, 0x200 };

	Sky::info_t::info_t()
		:path_()
	{}

	Sky::Sky(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,SaveCallback(RS_SKY)
		,pixels_(NULL)
	{}

	Sky::~Sky()
	{
		if (NULL != pixels_)
			delete [] pixels_;
		pixels_ = NULL;
	}

	bool Sky::Load()
	{
		fipImage image;
		// load the image
		image.load(info_.path_.c_str());
		if (FALSE == image.isValid())
		{
			Sleep(512);
			image.load(info_.path_.c_str());
			if (FALSE == image.isValid())
			{
				MakeDefault();
				return true;
			}
		}
		// make sure the dimensions are correct
		if (image.getWidth() != size_.cx || image.getHeight() != size_.cy)
		{
			MacroDisplayError(_T("The sky texture dimensions are incorrect.\nThe correct dimensions are 512x512."));
			return false;
		}
		// quantize the image if necessary
		if (24 != image.getBitsPerPixel())
		{
			if (FALSE == image.convertTo24Bits())
			{
				MacroDisplayError(_T("Sky could not be converted to 24-bit."));
				return false;
			}
		}
		// flip the image vertically
		if (FALSE == image.flipVertical())
			MacroDisplayError(_T("Sky bitmap could not be flipped."));
		// allocate new memory for the Sky
		const size_t pixels_size(size_.cx * size_.cy);
		_ASSERTE(NULL == pixels_);
		pixels_ = new COLORREF[pixels_size];
		// extract image data
		{
			const BYTE *img_iter(image.accessPixels());
			COLORREF *pixels_iter(pixels_);
			const COLORREF * const pixels_end(pixels_iter + pixels_size);
			while (pixels_iter != pixels_end)
				*pixels_iter++ = RGB(img_iter[0], img_iter[1], img_iter[2]), img_iter += 3;
		}
		return true;
	}

	void Sky::Unload()
	{
		_ASSERTE(NULL != pixels_);
		//Save();
		delete [] pixels_;
		pixels_ = NULL;
	}

	int Sky::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset) const
	{
		// pack the sky texture and the palette
		const int pixels_size(size_.cx * size_.cy * sizeof(COLORREF));
		CopyMemory(buffer, pixels_, pixels_size);
		// write XML metadata
		{
			char str[16];
			// offset of compressed Sky data
			_itot(buffer - initial_offset, str, 10);
			node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
		}
		return pixels_size;
	}

	void Sky::Unpack(TiXmlNode *node, BYTE *buffer)
	{
		if (NULL == node)
		{
			_RPT0(_CRT_WARN, "loading default sky texture\n");
			MakeDefault();
			return;
		}
		// allocate memory for the Sky
		const size_t pixels_size(size_.cx * size_.cy);
		_ASSERTE(NULL == pixels_);
		pixels_ = new COLORREF[pixels_size];
		// read in XML metadata
		BYTE *pixels_buffer;
		{
			// find data
			TiXmlHandle node_handle(node);
			TiXmlText *offset_node (node_handle.FirstChildElement("offset").FirstChild().Text());
			if (
				NULL == offset_node)
			{
				_RPT0(_CRT_WARN, "loading default sky texture\n");
				MakeDefault();
				return;
			}
			// parse the data
			pixels_buffer = buffer + atoi(offset_node->Value());
		}
		CopyMemory(pixels_, pixels_buffer, pixels_size * sizeof(COLORREF));
		return;
	}

	void Sky::MakeDefault()
	{
		_ASSERTE(NULL == pixels_);
		const size_t pixels_size(size_.cx * size_.cy);
		const size_t buffer_size (pixels_size * sizeof(COLORREF));
		pixels_ = new COLORREF[pixels_size];
		UncompressResource(IDR_SKY_TX, ri_cast<BYTE*>(pixels_), buffer_size);
	}

	void Sky::Save()
	{
		using namespace Loki;
		SaveBegin();
		LOKI_ON_BLOCK_EXIT_OBJ(*this, &Sky::SaveEnd);
		SaveAs(info_.path_.c_str());
	}

	void Sky::SaveAs(LPCTSTR path)
	{
		if (NULL == pixels_)
			MakeDefault();
		const size_t pixels_size(size_.cx * size_.cy);
		fipImage img(FIT_BITMAP, static_cast<WORD>(size_.cx), static_cast<WORD>(size_.cy), 24);
		BYTE *img_iter(img.accessPixels());
		const COLORREF *pixels_iter(pixels_);
		const COLORREF * const pixels_end(pixels_iter + pixels_size);
		while (pixels_iter != pixels_end)
		{
			*img_iter++ = GetRValue(*pixels_iter);
			*img_iter++ = GetGValue(*pixels_iter);
			*img_iter++ = GetBValue(*pixels_iter);
			++pixels_iter;
		}
		img.save(path);
	}

	//-----------------------
	// Surface implementation
	//-----------------------

	const SIZE Surface::size_ = { 0x100, 0x100 };

	Surface::info_t::info_t()
		:path_()
	{}

	Surface::Surface(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,SaveCallback(RS_SURFACE)
		,indices_(NULL)
	{}

	Surface::~Surface()
	{
		if (NULL != indices_)
			delete [] indices_;
		indices_ = NULL;
	}

	bool Surface::Load()
	{
		fipImage image;
		// load the image
		image.load(info_.path_.c_str());
		if (FALSE == image.isValid())
		{
			Sleep(512);
			image.load(info_.path_.c_str());
			if (FALSE == image.isValid())
			{
				MakeDefault();
				return true;
			}
		}
		// make sure the dimensions are correct
		if (image.getWidth() != size_.cx || image.getHeight() != size_.cy)
		{
			MacroDisplayError(_T("The surface texture dimensions are incorrect.\nThe correct dimensions are 512x512."));
			return false;
		}
		// quantize the image if necessary
		if (FIC_PALETTE != image.getColorType())
		{
			FREE_IMAGE_QUANTIZE mode(FIQ_NNQUANT);
			if (FALSE == image.colorQuantize(mode))
			{
				MacroDisplayError(_T("Surface could not be quantized."));
				return false;
			}
		}
		// flip the image vertically
		if (FALSE == image.flipVertical())
			MacroDisplayError(_T("Surface bitmap could not be flipped."));
		// allocate new memory for the Surface
		const size_t indices_size(size_.cx * size_.cy);
		_ASSERTE(NULL == indices_);
		indices_ = new BYTE[indices_size];
		// extract image data
		CopyMemory(indices_, image.accessPixels(), indices_size);
		const size_t palette_size(__min(256, image.getPaletteSize() / 4));
		RGBQUAD *palette(image.getPalette());
		for (size_t i(0); i != palette_size; ++i)
			palette_[i] = RGB(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
		return true;
	}

	void Surface::Unload()
	{
		_ASSERTE(NULL != indices_);
		//Save();
		delete [] indices_;
		indices_ = NULL;
	}

	int Surface::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset) const
	{
		// pack the surface texture and the palette
		const int surface_size(size_.cx * size_.cy);
		const int palette_size(256 * sizeof(COLORREF));
		CopyMemory(buffer, indices_, surface_size);
		CopyMemory(buffer + surface_size, palette_, palette_size);
		// write XML metadata
		{
			char str[16];
			// offset of compressed Surface data
			_itot(buffer - initial_offset, str, 10);
			node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
		}
		return surface_size + palette_size;
	}

	void Surface::Unpack(TiXmlNode *node, BYTE *buffer)
	{
		if (NULL == node)
		{
			_RPT0(_CRT_WARN, "loading default surface texture\n");
			MakeDefault();
			return;
		}
		// allocate memory for the Surface
		const size_t indices_size(size_.cx * size_.cy);
		const size_t palette_size(256 * sizeof(COLORREF));
		_ASSERTE(NULL == indices_);
		indices_ = new BYTE[indices_size];
		// read in XML metadata
		BYTE *surface_buffer;
		BYTE *palette_buffer;
		{
			// find data
			TiXmlHandle node_handle(node);
			TiXmlText *offset_node (node_handle.FirstChildElement("offset").FirstChild().Text());
			if (
				NULL == offset_node)
			{
				_RPT0(_CRT_WARN, "loading default Surface\n");
				MakeDefault();
				return;
			}
			// parse the data
			surface_buffer = buffer + atoi(offset_node->Value());
			palette_buffer = surface_buffer + indices_size;
		}
		CopyMemory(palette_, palette_buffer, palette_size);
		CopyMemory(indices_, surface_buffer, indices_size); // WARN: possible buffer overflow
		return;
	}

	void Surface::MakeDefault()
	{
		_ASSERTE(NULL == indices_);
		const size_t indices_size(size_.cx * size_.cy);
		const size_t palette_size(0x100 * sizeof(COLORREF));
		const size_t buffer_size (indices_size + palette_size);
		vector<BYTE> buffer(buffer_size);
		UncompressResource(IDR_SURFACE_TX, &buffer[0], buffer_size);
		indices_ = new BYTE[indices_size];
		CopyMemory(indices_, &buffer[0], indices_size);
		CopyMemory(palette_, &buffer[indices_size], palette_size);
	}

	void Surface::Save()
	{
		using namespace Loki;
		SaveBegin();
		LOKI_ON_BLOCK_EXIT_OBJ(*this, &Surface::SaveEnd);
		SaveAs(info_.path_.c_str());
	}

	void Surface::SaveAs(LPCTSTR path)
	{
		if (NULL == indices_)
			MakeDefault();
		const size_t indices_size(size_.cx * size_.cy);
		const size_t palette_size(0x100 * sizeof(COLORREF));
		fipImage img(FIT_BITMAP, static_cast<WORD>(size_.cx), static_cast<WORD>(size_.cy), 8);
		CopyMemory(img.accessPixels(), indices_, indices_size);
		CopyMemory(img.getPalette(),   palette_, palette_size);
		img.save(path);
	}

	//-----------------------
	// Texture implementation
	//-----------------------

	Texture::info_t::info_t()
		:fast_quantization_(true)
		,path_             ()
		,size_             ()
	{}

	Texture::Texture(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,SaveCallback(RS_TEXTURE)
		,indices_(NULL)
	{}

	Texture::~Texture()
	{
		if (NULL != indices_)
			delete [] indices_;
		indices_ = NULL;
	}

	bool Texture::Load()
	{
		fipImage image;
		// load the texture image
		{
			const DWORD delay(256);
			const uint  try_count(32);
			uint        try_num(0);
			for (; try_num != try_count; ++try_num)
			{
				image.load(info_.path_.c_str());
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
		if (image.getWidth() != info_.size_.cx || image.getHeight() != info_.size_.cy)
		{
			MacroDisplayError(_T("Texture dimensions do not correspond to project settings."));
			return false;
		}
		// quantize the image if necessary
		if (FIC_PALETTE != image.getColorType())
		{
			FREE_IMAGE_QUANTIZE mode(info_.fast_quantization_ ? FIQ_WUQUANT : FIQ_NNQUANT);
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
		const size_t indices_size(info_.size_.cx * info_.size_.cy);
		_ASSERTE(NULL == indices_);
		indices_ = new BYTE[indices_size];
		// extract image data
		CopyMemory(indices_, image.accessPixels(), indices_size);
		const size_t palette_size(__min(256, image.getPaletteSize() / 4));
		RGBQUAD *palette(image.getPalette());
		for (size_t i(0); i != palette_size; ++i)
			palette_[i] = RGB(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
		return true;
	}

	void Texture::Unload()
	{
		_ASSERTE(NULL != indices_);
		//Save();
		delete [] indices_;
		indices_ = NULL;
	}

	int Texture::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask) const
	{
		// pack the texture and the palette
		const int texture_size(info_.size_.cx * info_.size_.cy);
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
		const size_t indices_size(info_.size_.cx * info_.size_.cy);
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
				MakeDefault();
				return;
			}
			// parse the data
			texture_buffer = buffer + atoi(offset_node->Value());
			palette_buffer = buffer + atoi(palette_offset_node->Value());
		}
		CopyMemory(palette_, palette_buffer, palette_size);
		CopyMemory(indices_, texture_buffer, indices_size);
		return;
	}

	void Texture::MakeDefault()
	{
		ZeroMemory(indices_, info_.size_.cx * info_.size_.cy);
		FillMemory(palette_, 0x100, 0xFF);
	}

	void Texture::Save()
	{
		using namespace Loki;
		SaveBegin();
		LOKI_ON_BLOCK_EXIT_OBJ(*this, &Texture::SaveEnd);
		SaveAs(info_.path_.c_str());
	}

	void Texture::SaveAs(LPCTSTR path)
	{
		// initialize the texture image
		fipImage image(
			FIT_BITMAP,
			static_cast<WORD>(info_.size_.cx),
			static_cast<WORD>(info_.size_.cy),
			8);
		// fill the image
		if (NULL != indices_)
		{
			CopyMemory(image.accessPixels(), indices_, info_.size_.cx * info_.size_.cy);
			CopyMemory(image.getPalette(), palette_, 256 * 4); // ASSUME sizeof(COLORREF) == 4
		}
		else
		{
			ZeroMemory(image.accessPixels(), info_.size_.cx * info_.size_.cy);
			FillMemory(image.getPalette(), 256 * 4, 0xFF);
		}
		// convert the image to 24 bits (for easier editing)
		if (FALSE == image.convertTo24Bits())
			MacroDisplayError(_T("Could not convert the texture to 24-bit."));
		// save the image
		if (FALSE == image.save(path, PNG_DEFAULT))
			MacroDisplayError(_T("Texture could not be saved"));
	}

	//-------------------------
	// ZeroLayer implementation
	//-------------------------

	ZeroLayer::info_t::info_t()
		:path_()
		,size_()
	{}

	ZeroLayer::ZeroLayer(const HWND &error_hwnd)
		:ErrorHandler(error_hwnd)
		,SaveCallback(RS_ZERO_LAYER)
	{}

	bool ZeroLayer::Load()
	{
		fipImage image;
		// load the image
		image.load(info_.path_.c_str());
		if (FALSE == image.isValid())
		{
			Sleep(512);
			image.load(info_.path_.c_str());
			if (FALSE == image.isValid())
			{
				MakeDefault();
				return true;
			}
		}
		// make sure that dimensions are correct
		if (image.getWidth() != info_.size_.cx || image.getHeight() != info_.size_.cy)
		{
			MacroDisplayError(_T("Hardness map dimensions do not correspond to project settings."));
			return false;
		}
		// convert to grayscale if necessary
		if (8 != image.getBitsPerPixel())
		{
			if (FALSE == image.convertToGrayscale())
			{
				MacroDisplayError(_T("Hardness map could not be converted to grayscale."));
				return false;
			}
		}
		// flip vertically
		if (FALSE == image.flipVertical())
			MacroDisplayError(_T("Hardness map bitmap could not be flipped."));
		// allocate memory
		const size_t data_size(info_.size_.cx * info_.size_.cy);
		data_.resize(data_size);
		// fill data_
		{
			BYTE *img_iter(image.accessPixels());
			for (size_t i(0); i != data_size; ++i)
				data_[i] = *img_iter++ != 0;
		}
		return true;
	}

	void ZeroLayer::Unload()
	{
		//Save();
		vector<bool> temp;
		data_.swap(temp);
	}

	int ZeroLayer::Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset)
	{
		// pack the data
		_ASSERTE(data_.size() % 8 == 0);
		const int byte_count(data_.size() / 8);
		{
			size_t bit_index(0);
			const BYTE * const buffer_end(buffer + byte_count);
			for (BYTE *buffer_iter(buffer); buffer_iter != buffer_end; ++buffer_iter)
				for (int b(0); b != 8; ++b)
					if (data_[bit_index++])
						*buffer_iter |= 1 << b;
		}
		// record xml metadata
		{
			TCHAR str[16];
			// offset of the compressed data
			_itot(buffer - initial_offset, str, 10);
			node.InsertEndChild(TiXmlElement("offset"))->InsertEndChild(TiXmlText(str));
		}
		return byte_count;
	}

	void ZeroLayer::Unpack(TiXmlNode *node, BYTE *buffer)
	{
		if (NULL == node)
		{
			_RPT0(_CRT_WARN, "loading default zero level\n");
			MakeDefault();
			return;
		}
		// read in XML metadata
		BYTE *offset;
		{
			TiXmlHandle node_handle(node);
			TiXmlText *offset_node(node_handle.FirstChildElement("offset").FirstChild().Text());
			if (NULL == offset_node)
			{
				_RPT0(_CRT_WARN, "loading default zero level\n");
				MakeDefault();
				return;
			}
			offset = buffer + _ttoi(offset_node->Value());
		}
		// unpack
		{
			const size_t data_size = info_.size_.cx * info_.size_.cy;
			_ASSERTE(data_size % 8 == 0);	
			_ASSERTE(data_.empty());
			data_.reserve(data_size);
			const BYTE * const buffer_end(offset + data_size / 8);
			for (BYTE *buffer_iter(offset); buffer_iter != buffer_end; ++buffer_iter)
				for (int b(0); b != 8; ++b)
					data_.push_back(0 != (*buffer_iter & 1 << b));
		}
	}

	void ZeroLayer::MakeDefault()
	{
		_ASSERTE(data_.empty());
		const size_t data_size(info_.size_.cx * info_.size_.cy);
		data_.resize(data_size);
		for (size_t i(0); i != data_size; ++i)
			data_[i] = true;
	}

	void ZeroLayer::Save()
	{
		using namespace Loki;
		SaveBegin();
		LOKI_ON_BLOCK_EXIT_OBJ(*this, &ZeroLayer::SaveEnd);
		SaveAs(info_.path_.c_str());
	}

	void ZeroLayer::SaveAs(LPCTSTR path)
	{
		_ASSERTE(!data_.empty());
		// initialize the image
		fipImage image(
			FIT_BITMAP,
			static_cast<WORD>(info_.size_.cx),
			static_cast<WORD>(info_.size_.cy),
			8);
		// fill the image
		BYTE *image_iter(image.accessPixels());
		for (size_t i(0); i != data_.size(); ++i)
			*image_iter++ = data_[i] ? 0xFF : 0x00;
		image.threshold(1);
		// save the image
		if (FALSE == image.save(info_.path_.c_str(), BMP_DEFAULT))
			MacroDisplayError(_T("Hardness map could not be saved."));
	}
}