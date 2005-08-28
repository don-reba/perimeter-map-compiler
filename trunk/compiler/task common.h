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

#include <utility>

struct TextureAllocation;
struct SimpleVertex;

//------------------------------------------------------------
// some functions used by the Tasks, but also useful to others
//------------------------------------------------------------

namespace TaskCommon
{
	//------------------------------------------------------------------------
	// types used by the rest of the namespace, as well as possibly other code
	//------------------------------------------------------------------------

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

	//--------------------------------------------------------------
	// 8-bit bitmap storing lightness of the map
	// is to be incremented by 0x80 and multiplyied with the texture
	//--------------------------------------------------------------

	struct Lightmap : public ErrorHandler
	{
	public:
		Lightmap(SIZE size, HWND &error_hwnd);
		~Lightmap();
		bool Create(const Heightmap &heightmap);
	private:
		bool Create8 (const Heightmap &heightmap);
		bool Create16(const Heightmap &heightmap);
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

	//---------
	// defaults
	//---------

	void DefaultTexture(BYTE *buffer, COLORREF palette[256], SIZE size);

	//--------
	// file IO
	//--------

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
		const ZeroLayer *zero_layer,
		LPCTSTR          path,
		ErrorHandler    &error_handler);
	void  SaveVMP(
		const Heightmap8 &heightmap,
		const Texture    &texture,
		const ZeroLayer  *zero_layer,
		LPCTSTR           path,
		ErrorHandler     &error_handler);
	void  SaveVMP(
		const Heightmap16 &heightmap,
		const Texture     &texture,
		const ZeroLayer   *zero_layer,
		LPCTSTR            path,
		ErrorHandler      &error_handler);

	//-----------------------------------
	// "incredible math" (Lithium Flower)
	//-----------------------------------

	COLORREF AverageColour(const Texture &texture, const Heightmap &heightmap);
	float    AverageHeight(const Heightmap   &heightmap);
	float    AverageHeight(const Heightmap8  &heightmap);
//	float    AverageHeight(const Heightmap16 &heightmap);
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
		const Heightmap8 &heightmap,
		vector<SimpleVertex> &vertices,
		float mesh_threshold);
	
	//------
	// other
	//------

	bool GetInstallPath(tstring &install_path, ErrorHandler &error_handler);
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

	// fast 5x5 Gaussian blur by Frederick M. Waltz and John W. V. Miller
	// http://www-personal.engin.umd.umich.edu/~jwvm/ece581/21_GBlur.pdf
	// does not blur the 3-pixel border around the image
	// TODO: calculate how large the state variables get
	// TODO: write a metaprogram to create blurs with arbitrary matrices
	template <typename T, typename StateT = T>
	class GaussianBlurType
	{
	private:
		struct SC {
			SC() : sc0(), sc1(), sc2(), sc3() {}
			StateT sc0, sc1, sc2, sc3;
		};
	public:
		static void Apply(T *pix, SIZE size)
		{
			StateT sr0, sr1, sr2, sr3; // row state
			vector<SC> sc(size.cx);    // column state
			const LONG write_offset(-size.cx * 2 - 2);
			for (LONG y = 0; y != 4; ++y)
			{
				sr3 = sr2 = sr1 = sr0 = 0;
				for (LONG x = 0; x != size.cx; ++x)
				{
					StateT &sc0(sc[x].sc0), &sc1(sc[x].sc1), &sc2(sc[x].sc2), &sc3(sc[x].sc3);
					StateT tmp1 = *pix;
					StateT tmp2 = sr0 + tmp1;
					sr0 = tmp1;
					tmp1 = sr1 + tmp2;
					sr1 = tmp2;
					tmp2 = sr2 + tmp1;
					sr2 = tmp1;
					tmp1 = sr3 + tmp2;
					sr3 = tmp2;
					tmp2 = sc0 + tmp1;
					sc0 = tmp1;
					tmp1 = sc1 + tmp2;
					sc1 = tmp2;
					tmp2 = sc2 + tmp1;
					sc2 = tmp1;
					sc3 = tmp2;
					++pix;
				}
			}
			for (LONG y = 4; y != size.cy; ++y)
			{
				sr3 = sr2 = sr1 = sr0 = 0;
				for (LONG x = 0; x != 4; ++x)
				{
					StateT &sc0(sc[x].sc0), &sc1(sc[x].sc1), &sc2(sc[x].sc2), &sc3(sc[x].sc3);
					StateT tmp1 = *pix;
					StateT tmp2 = sr0 + tmp1;
					sr0 = tmp1;
					tmp1 = sr1 + tmp2;
					sr1 = tmp2;
					tmp2 = sr2 + tmp1;
					sr2 = tmp1;
					tmp1 = sr3 + tmp2;
					sr3 = tmp2;
					tmp2 = sc0 + tmp1;
					sc0 = tmp1;
					tmp1 = sc1 + tmp2;
					sc1 = tmp2;
					tmp2 = sc2 + tmp1;
					sc2 = tmp1;
					sc3 = tmp2;
					++pix;
				}
				for (LONG x = 4; x != size.cx; ++x)
				{
					StateT &sc0(sc[x].sc0), &sc1(sc[x].sc1), &sc2(sc[x].sc2), &sc3(sc[x].sc3);
					StateT tmp1 = *pix;
					StateT tmp2 = sr0 + tmp1;
					sr0 = tmp1;
					tmp1 = sr1 + tmp2;
					sr1 = tmp2;
					tmp2 = sr2 + tmp1;
					sr2 = tmp1;
					tmp1 = sr3 + tmp2;
					sr3 = tmp2;
					tmp2 = sc0 + tmp1;
					sc0 = tmp1;
					tmp1 = sc1 + tmp2;
					sc1 = tmp2;
					tmp2 = sc2 + tmp1;
					sc2 = tmp1;
					pix[write_offset] = static_cast<T>((static_cast<StateT>(0x80) + sc3 + tmp2) / static_cast<StateT>(0x100));
					sc3 = tmp2;
					++pix;
				}
			}
		}
	};
	template <typename T>
	void GaussianBlur(T *pix, SIZE size)
	{
		GaussianBlurType<T>::Apply(pix, size);
	}
	template <typename T, typename StateT>
	void GaussianBlur(T *pix, SIZE size)
	{
		GaussianBlurType<T, StateT>::Apply(pix, size);
	}
}
