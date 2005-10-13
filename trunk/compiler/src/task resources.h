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

#include "error handler.h"

namespace TaskCommon
{
	//--------------------
	// forward definitions
	//--------------------

	struct Hardness;
	class  Heightmap;
	class  Heightmap8;
	class  Heightmap16;
	struct Lightmap;
	struct MapInfo;
	struct ZeroLayer;

	//--------------------------------------------------------------------------
	// 1-bit bitmap specifying the areas that are not accessible to terraforming
	//--------------------------------------------------------------------------

	struct Hardness : public ErrorHandler
	{
		Hardness(SIZE size, HWND &error_hwnd);
		~Hardness();
		bool Load(LPCTSTR path);
		void MakeDefault();
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask);
		void Save(LPCTSTR path);
		void Unpack(TiXmlNode *node, BYTE *buffer, const vector<bool> &mask);
		BYTE *data_;
		WORD *data16_;
		SIZE size_;
	};

	//---------------------------------------------------------------
	// 8-bit or 16-bit grayscale bitmap defining the shape of the map
	//---------------------------------------------------------------

	class Heightmap : public ErrorHandler
	{
	public:
		Heightmap(WORD bpp, ErrorHandler &error_handler);
		virtual ~Heightmap();
	public:
		virtual void MakeDefault() = 0;
		virtual int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask) = 0;
	public:
		WORD GetBpp() const;
	private:
		const WORD bpp_;
	public:
		SIZE  size_;
	};

	class Heightmap8 : public Heightmap
	{
	public:
		Heightmap8(SIZE size, ErrorHandler &error_handler);
		~Heightmap8();
	public:
		void MakeDefault();
		bool Load(fipImage &image, const ZeroLayer *zero_layer, uint zero_level);
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask);
		void Unpack(TiXmlNode *node, BYTE *buffer, vector<bool> &mask);
	public:
		BYTE *data_;
	};

	class Heightmap16 : public Heightmap
	{
	public:
		Heightmap16(SIZE size, ErrorHandler &error_handler);
		~Heightmap16();
	public:
		void MakeDefault();
		bool Load(fipImage &image, const ZeroLayer *zero_layer, uint zero_level);
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask);
		void Unpack(TiXmlNode *node, BYTE *buffer, vector<bool> &mask);
	public:
		operator Heightmap8*();
	public:
		WORD *data_;
	};

	Heightmap* LoadHeightmap(
		SIZE             size,
		ErrorHandler    &error_handler,
		LPCTSTR          path,
		const ZeroLayer *zero_layer,
		uint             zero_level);

	Heightmap* UnpackHeightmap(
		SIZE size,
		ErrorHandler &error_handler,
		TiXmlNode *node,
		BYTE *buffer,
		vector<bool> &mask);

	//-------------------------------------------------------------
	// 8-bit bitmap storing lightness of the map
	// is to be incremented by 0x80 and multiplied with the texture
	//-------------------------------------------------------------

	struct Lightmap : public ErrorHandler
	{
	public:
		Lightmap(SIZE size, HWND &error_hwnd);
		~Lightmap();
		bool Create(const Heightmap &heightmap);
	private:
		bool Create(const Heightmap8  &heightmap);
		bool Create(const Heightmap16 &heightmap);
	public:
		BYTE *data_;
		SIZE size_;
	};

	//---------------------------------------------
	// additional information associated with a map
	//---------------------------------------------

	struct MapInfo
	{
	public:
		static MapInfo LoadFromGlobal(); // for use from the interface thread only
	public:
		tstring GenerateWorldIni();
		void    GetRawData(BYTE **data, size_t *size);
		void    Pack(TiXmlNode &node);
		void    Unpack(TiXmlNode &node);
	private:
		void    ReplaceSubstring(tstring &str, const tstring &target, const tstring &replacement);
		tstring GenerateStartPosName(int index, bool x) const;
	public:
		tstring  map_name_;
		uint     power_x_;
		uint     power_y_;
		uint     zero_level_;
		uint     fog_start_;
		uint     fog_end_;
		COLORREF fog_colour_;
		POINT    sps_[5];
		bool     custom_hardness_;
		bool     custom_sky_;
		bool     custom_surface_;
		bool     custom_zero_layer_;
	};

	//---------------------------------
	// custom 24-bit colour sky texture
	//---------------------------------

	struct Sky : public ErrorHandler
	{
		Sky(HWND &error_hwnd);
		~Sky();
		bool Load(LPCTSTR path);
		void MakeDefault();
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset) const;
		void Save(LPCTSTR path);
		void Unpack(TiXmlNode *node, BYTE *buffer);
		COLORREF *pixels_;
		static const SIZE size_;
	};

	//--------------------------------------
	// custom paletted 8-bit surface texture
	//--------------------------------------

	struct Surface : public ErrorHandler
	{
		Surface(HWND &error_hwnd);
		~Surface();
		bool Load(LPCTSTR path);
		void MakeDefault();
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset) const;
		void Save(LPCTSTR path);
		void Unpack(TiXmlNode *node, BYTE *buffer);
		BYTE     *indices_;
		COLORREF  palette_[0x100];
		static const SIZE size_;
	};

	//--------------------------------------------------
	// paletted 8-bit texture defining colour of the map
	//--------------------------------------------------

	struct Texture : public ErrorHandler
	{
		Texture(SIZE size, HWND &error_hwnd);
		~Texture();
		bool Load(LPCTSTR path, bool fast_quantization);
		void MakeDefault();
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask) const;
		void Unpack(TiXmlNode *node, BYTE *buffer);
		BYTE     *indices_;
		COLORREF  palette_[0x100];
		SIZE      size_;
	};

	//--------------------------------------------------------------------
	// 1-bit bitmap defining the areas appearing terraformed at game start
	//--------------------------------------------------------------------

	struct ZeroLayer : public ErrorHandler
	{
		ZeroLayer(SIZE size, HWND &error_hwnd);
		bool Load(LPCTSTR path);
		void MakeDefault();
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset);
		void Save(LPCTSTR path);
		void Unpack(TiXmlNode *node, BYTE *buffer);
		vector<bool> data_;
		SIZE size_;
	};

	//-------------------------------------------------
	// a triangulation element for use by Triangulate()
	//-------------------------------------------------

	struct TE
	{
		struct Constraint
		{
			inline float ConstrainedVal(int position) const
			{
				_ASSERTE(position > start);
				_ASSERTE(position < start + length);
				_ASSERTE(position != start);
				_ASSERTE(position != start + length);
				return s_val + (f_val - s_val) * (position - start) / static_cast<float>(length);
			}
			short start, length; // direction is implied
			float s_val, f_val;
		};
		TE() : l_constrained(false), r_constrained(false), b_constrained(false), t_constrained(false) {}
		bool l_constrained;
		bool r_constrained;
		bool b_constrained;
		bool t_constrained;
		Constraint l_constraint;
		Constraint r_constraint;
		Constraint b_constraint;
		Constraint t_constraint;
		int index;
	};
}