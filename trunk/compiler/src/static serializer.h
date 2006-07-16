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

#include <limits>

//-----------------------------------------------------
// static serialization class definition
//
// To use it, create a new data class,
//  as in the example at the end of this file,
//  and instantiate the StaticSerializer class with it.
// StaticSerializer is an empty class;
//  it is to be used through its static functions.
//-----------------------------------------------------

// generators of the Datum structures, which hold the data
// TODO: remove macros
#define MacroSSVar(id, type, val)\
	template <>\
	struct Datum<id>\
	{\
		typedef type type_;\
		static type& data_()\
		{\
			static type data(val);\
			return data;\
		}\
	}
#define MacroSSVar2(id, type, val1, val2)\
	template <>\
	struct Datum<id>\
	{\
		typedef type type_;\
		static type& data_()\
		{\
			static type data = { val1, val2 };\
			return data;\
		}\
	}
#define MacroSSVar3(id, type, val1, val2, val3)\
	template <>\
	struct Datum<id>\
	{\
		typedef type type_;\
		static type& data_()\
		{\
			static type data = { val1, val2, val3 };\
			return data;\
		}\
	}
#define MacroSSVar4(id, type, val1, val2, val3, val4)\
	template <>\
	struct Datum<id>\
	{\
		typedef type type_;\
		static type& data_()\
		{\
			static type data = { val1, val2, val3, val4 };\
			return data;\
		}\
	}

// main class
template <class Data>	
class StaticSerializer : public Data
{
// construction/destruction
private:
	StaticSerializer() {}
	~StaticSerializer() {}
// interface
public:
	// get the data corresponding to given id
	template <uint id>
	static typename Data::template Datum<id>::type_& Get()	
	{
		return Datum<id>::data_();
	}
	// output the data (for debugging purposes)
	static void Output()
	{
		TAct(&POutput, NULL);
	}
	// save the data to a file
	static void Save(LPCTSTR file_name)
	{
		TAct(&PSave, file_name);
	}
	static void Load(LPCTSTR file_name)
	{
		TLoad(file_name);
	}
// internal function
private:
	// the drawback is in having a long list of converter definitions
	// TODO: fix organize definition inheritance
	// parse data appropriately for each type
	template <typename T>
	static tstring ToString(T)
	{
		// return nothing - it's a trap!
	}
	template <>
	static tstring ToString<tstring>(tstring data)
	{
		return data;
	}
	template <>
	static tstring ToString<int>(int data)
	{
		TCHAR str[16];
		_stprintf(str, "%i", data);
		return str;
	}
	template <>
	static tstring ToString<uint>(uint data)
	{
		TCHAR str[16];
		_stprintf(str, "%u", data);
		return str;
	}
	template <>
	static tstring ToString<float>(float data)
	{
		TCHAR str[32];
		_stprintf(str, "%f", data);
		return str;
	}
	template <>
	static tstring ToString<bool>(bool data)
	{
		return data ? "true" : "false";
	}
	template <>
	static tstring ToString<RECT>(RECT data)
	{
		TCHAR str[64];
		_stprintf(str, "%li %li %li %li", data.left, data.top, data.right, data.bottom);
		return str;
	}
	template <>
	static tstring ToString<SIZE>(SIZE data)
	{
		TCHAR str[64];
		_stprintf(str, "%li %li", data.cx, data.cy);
		return str;
	}
	template <>
	static tstring ToString<POINT>(POINT data)
	{
		TCHAR str[64];
		_stprintf(str, "%li %li", data.x, data.y);
		return str;
	}
	template <>
	static tstring ToString<COLORREF>(COLORREF data)
	{
		TCHAR str[64];
		_stprintf(str, "%X %X %X", GetRValue(data), GetGValue(data), GetBValue(data));
		return str;
	}
	// convert each type of data from string appropriately for each type
	template <typename T>
	static T FromString(LPCTSTR)
	{
		// return nothing - it's a trap!
	}
	template <>
	static tstring FromString<tstring>(LPCTSTR str)
	{
		return str;
	}
	template <>
	static int FromString<int>(LPCTSTR str)
	{
		int i(0);
		_stscanf(str, _T("%i"), &i);
		return i;
	}
	template <>
	static uint FromString<uint>(LPCTSTR str)
	{
		uint i(0);
		_stscanf(str, _T("%u"), &i);
		return i;
	}
	template <>
	static float FromString<float>(LPCTSTR str)
	{
		float f(0);
		_stscanf(str, _T("%f"), &f);
		return f;
	}
	template <>
	static bool FromString<bool>(LPCTSTR str)
	{
		return (0 == _tcscmp(str, "true"));
	}
	template <>
	static RECT FromString<RECT>(LPCTSTR str)
	{
		RECT rect = { 0, 0, 0, 0 };
		_stscanf(str, _T("%li %li %li %li"), &rect.left, &rect.top, &rect.right, &rect.bottom);
		return rect;
	}
	template <>
	static SIZE FromString<SIZE>(LPCTSTR str)
	{
		const LONG max_val(std::numeric_limits<LONG>::max()); // reserved value
		SIZE size = { max_val, max_val };
		_stscanf(str, _T("%li %li"), &size.cx, &size.cy);
		if (max_val != size.cx && max_val != size.cy)
			return size;
		_stscanf(str, _T("(%li, %li)"), &size.cx, &size.cy);
		return size;
	}
	template <>
	static POINT FromString<POINT>(LPCTSTR str)
	{
		const LONG max_val(std::numeric_limits<LONG>::max()); // reserved value
		POINT point = { max_val, max_val };
		_stscanf(str, _T("%li %li"), &point.x, &point.y);
		if (max_val != point.x && max_val != point.y)
			return point;
		_stscanf(str, _T("(%li, %li)"), &point.x, &point.y);
		return point;
	}
	template <>
	static COLORREF FromString<COLORREF>(LPCTSTR str)
	{
		const UINT max_val(std::numeric_limits<UINT>::max());
		UINT r(max_val), g(max_val), b(max_val);
		_stscanf(str, _T("%X %X %X"), &r, &g, &b);
		if (max_val != r && max_val != g && max_val != b)
			return RGB(r, g, b);
		COLORREF colour;
		_stscanf(str, _T("%X%"), &colour);
		return colour;
	}
	// some actions to be performed on the data
	static void POutput(LPCTSTR section, LPCTSTR key, LPCTSTR value, LPCTSTR)
	{
		_RPT3(_CRT_WARN, "%s %s %s\n", section, key, value);
	}
	static void PSave(LPCTSTR section, LPCTSTR key, LPCTSTR value, LPCTSTR file_name)
	{
		if (_T('\0') != *value)
			WritePrivateProfileString(section, key, value, file_name);
	}
	// recursively work on the data
	// TODO: add arbitrary predicate/binding support
	typedef void (*PFn)(LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR);
	template <uint id>
	static void TAct(PFn fn, LPCSTR str)
	{
		tstring value(ToString(Get<id>()));
		(*fn)(sections(id), keys(id), value.c_str(), str);
		TAct<id + 1>(fn, str);
	}
	template <>
	static void TAct<Data::count_>(PFn, LPCTSTR)
	{}
	static void TAct(PFn fn, LPCTSTR str)
	{
		TAct<0>(fn, str);
	}
	// recurse through the data, loading them from the given file
	template <uint id>
	static void TLoad(LPCTSTR file_name)
	{
		tstring value(ToString(Get<id>()));
		const unsigned int buffer_size(256);
		TCHAR buffer[buffer_size];
		GetPrivateProfileString(sections(id), keys(id), value.c_str(), buffer, buffer_size, file_name);
		Get<id>() = FromString<typename Data::template Datum<id>::type_>(buffer);
		TLoad<id + 1>(file_name);
	}
	template <>
	static void TLoad<Data::count_>(LPCTSTR)
	{}
	static void TLoad(LPCTSTR file_name)
	{
		TLoad<0>(file_name);
	}
};

//------------------
// sample data class
//------------------

//class MyData
//{
//public:
//	// array for referencing the variables
//	enum
//	{
//		ID_INT = 0,
//		ID_BOOL,
//		ID_RECT,
//		ID_SIZE,
//		ID_COLOUR,
//		count_ // necessary
//	};
//protected:
//	template <uint id>
//	struct Datum {};
//
//	// names of the variables to be used in serialization
//	static const char * keys(uint i)
//	{
//		static const char * const keys_table[count_] =
//		{
//			_T("integer"),
//			_T("boolean"),
//			_T("rectangle"),
//			_T("size"),
//			_T("colour")
//		};
//		return keys_table[i];
//	}
//
//	// sections for the vriables to be used in serialization
//	static const char * sections(uint i)
//	{
//		static const char * const sections_table[count_] =
//		{
//			_T("POD"),
//			_T("POD"),
//			_T("structure"),
//			_T("structure"),
//			_T("compound")
//		};
//		return sections_table[i];
//	}
//
//	// data
//	MacroSSVar (ID_INT,    int,  5);
//	MacroSSVar (ID_BOOL,   bool, true);
//	MacroSSVar4(ID_RECT,   RECT, 1, 2, 3, 4);
//	MacroSSVar2(ID_SIZE,   SIZE, 1, 2);
//	MacroSSVar (ID_COLOUR, COLORREF, 0x01020304);
//};