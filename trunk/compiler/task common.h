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

#include <utility>

class ErrorHandler;
struct TextureAllocation;
struct SimpleVertex;

//----------------------------------------------------------------------------------
// some functions used by the Tasks, but also useful to others
//----------------------------------------------------------------------------------

namespace TaskCommon
{
// types
	struct Hardness;
	struct Heightmap;
	struct MapInfo;
	struct ZeroLayer;

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
		SIZE size_;
	};
	struct Heightmap : public ErrorHandler
	{
		Heightmap(SIZE size, HWND &error_hwnd);
		~Heightmap();
		bool Load(LPCTSTR path, const ZeroLayer *zero_layer, uint zero_level);
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, vector<bool> &mask);
		void Unpack(TiXmlNode *node, BYTE *buffer, vector<bool> &mask);
		BYTE *data_;
		SIZE size_;
	};
	struct Lightmap : public ErrorHandler
	{
		Lightmap(SIZE size, HWND &error_hwnd);
		~Lightmap();
		bool Create(const Heightmap &heightmap);
		BYTE *data_;
		SIZE size_;
	};
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
	/*struct Surface : public ErrorHandler
	{
		Surface(HWND &error_hwnd);
		~Surface();
		bool Load(LPCTSTR path);
		void MakeDefault();
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset) const;
		void Unpack(TiXmlNode *node, BYTE *buffer);
		BYTE     *indices_;
		COLORREF  palette_[256];
		static const SIZE size_;
	};*/
	struct Texture : public ErrorHandler
	{
		Texture(SIZE size, HWND &error_hwnd);
		~Texture();
		bool Load(LPCTSTR path, bool fast_quantization);
		int  Pack(TiXmlNode &node, BYTE *buffer, const BYTE *initial_offset, const vector<bool> &mask) const;
		void Unpack(TiXmlNode *node, BYTE *buffer);
		BYTE     *indices_;
		COLORREF  palette_[256];
		SIZE      size_;
	};
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
	// triangulation element
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
// defaults
	void DefaultHeightmap(BYTE *buffer, SIZE size);
	void DefaultTexture(BYTE *buffer, COLORREF palette[256], SIZE size);
// file IO
	DWORD LoadFile(const LPCTSTR name, BYTE *&pBuffer, ErrorHandler &error_handler);
	void  SaveHardness(
		LPCTSTR       path,
		const BYTE   *bufffer,
		SIZE          size,
		ErrorHandler &error_handler);
	void  SaveHeightmap(
		LPCTSTR path,
		const BYTE *buffer,
		SIZE size,
		ErrorHandler &error_handler);
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
	void  SaveTexture(
		LPCTSTR path,
		const BYTE *buffer,
		const COLORREF palette[256],
		SIZE size,
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
		const ZeroLayer &zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler);
// "incredible math" (Lithium Flower)
	COLORREF AverageColour(const Texture &texture, const Heightmap &heightmap);
	uint     AverageHeight(const Heightmap &heightmap);
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
	float    Flatness(int h1, int h2, int v1, int v2, int c, int r);
	void     ReplaceSubstringSeq(
		LPCTSTR                                     str,
		size_t                                      str_length,
		const vector<std::pair<tstring, tstring> > &seq,
		tstring                                    &result);
	void     Triangulate(Heightmap &heightmap, vector<SimpleVertex> &vertices, float mesh_threshold);
// other
	bool GetInstallPath(tstring &install_path, ErrorHandler &error_handler);
	bool RegisterMap(LPCTSTR map_name, DWORD checksum, ErrorHandler &error_handler);
}

//-------------------------
// template implementations
//-------------------------

namespace TaskCommon
{
	// fast 5x5 Gaussian blur by Frederick M. Waltz and John W. V. Miller
	// http://www-personal.engin.umd.umich.edu/~jwvm/ece581/21_GBlur.pdf
	// does not blur the 3-pixel border around the image
	// TODO: calculate how large the state variables get
	// TODO: write a metaprogram to create blurs with arbitrary matrices
	template <typename T>
	void GaussianBlur(T *pix, SIZE size)
	{
		T sr0, sr1, sr2, sr3;
		vector<T> sc0(size.cx);
		vector<T> sc1(size.cx);
		vector<T> sc2(size.cx);
		vector<T> sc3(size.cx);
		for (LONG i = 0; i != size.cx; ++i)
			sc3[i] = sc2[i] = sc1[i] = sc0[i] = 0;
		for (LONG y = 0; y != size.cy; ++y)
		{
			sr3 = sr2 = sr1 = sr0 = 0;
			for (LONG x = 0; x != size.cx; ++x)
			{
				T tmp1 = *pix;
				T tmp2 = sr0 + tmp1;
				sr0 = tmp1;
				tmp1 = sr1 + tmp2;
				sr1 = tmp2;
				tmp2 = sr2 + tmp1;
				sr2 = tmp1;
				tmp1 = sr3 + tmp2;
				sr3 = tmp2;
				tmp2 = sc0[x] + tmp1;
				sc0[x] = tmp1;
				tmp1 = sc1[x] + tmp2;
				sc1[x] = tmp2;
				tmp2 = sc2[x] + tmp1;
				sc2[x] = tmp1;
				if (x > 3 && y > 3)
					pix[-size.cx * 2 - 2] = (128 + sc3[x] + tmp2) / 256;
				sc3[x] = tmp2;
				++pix;
			}
		}
	}
}
