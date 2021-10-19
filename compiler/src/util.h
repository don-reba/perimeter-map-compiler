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


inline int exp2(unsigned int n)
{
	int e(1);
	while (0 != n)
	{
		e <<= 1;
		--n;
	}
	return e;
}

inline int log2(unsigned int n)
{
	int l(-1);
	while (0 != n)
	{
		n >>= 1;
		++l;
	}
	return l;
}

inline void ScreenToClient(HWND hwnd, RECT *rect)
{
	POINT corner;
	RECT &rect_ref(*rect);
	corner.x = rect_ref.left;
	corner.y = rect_ref.top;
	ScreenToClient(hwnd, &corner);
	rect_ref.left = corner.x;
	rect_ref.top  = corner.y;
	corner.x = rect_ref.right;
	corner.y = rect_ref.bottom;
	ScreenToClient(hwnd, &corner);
	rect_ref.right  = corner.x;
	rect_ref.bottom = corner.y;
}

inline void ClientToScreen(HWND hwnd, RECT *rect)
{
	POINT corner;
	RECT &rect_ref(*rect);
	corner.x = rect_ref.left;
	corner.y = rect_ref.top;
	ClientToScreen(hwnd, &corner);
	rect_ref.left = corner.x;
	rect_ref.top  = corner.y;
	corner.x = rect_ref.right;
	corner.y = rect_ref.bottom;
	ClientToScreen(hwnd, &corner);
	rect_ref.right  = corner.x;
	rect_ref.bottom = corner.y;
}

// binary search on cstrings
inline const char * const * binary_search(
	const char * const * begin,
	const char * const * end,
	const char *         val)
{
	const char * const *l = begin;
	const char * const *r = end;
	while (l <= r)
	{
		const char * const * m = l + (r - l) / 2;
		int result(strcmp(*m, val));
		if (result < 0)
			l = m + 1;
		else if (result > 0)
			r = m - 1;
		else
			return m;
	}
	return end;
}

// find the index of a cstring in a sorted array
inline size_t string_index(
	const char * const name_array[],
	size_t             array_size,
	const char *       name)
{
	const char * const * const begin  = name_array;
	const char * const * const end    = name_array + array_size;
	const char * const * const result = binary_search(begin, end, name);
	if (result != end)
		return static_cast<size_t>(end - result);
	_RPT1(_CRT_ERROR, "Unknown enumeration value: %s.", name);
	return static_cast<size_t>(-1);
}

// determine the size of a static array as a compile-time constant
// the trick is to call sizeof on a function that returns a reference
//  to a char array with the same number of elements as in the argument
template <typename T, size_t N>
char (&equal_char_array(T (&)[N]))[N] { }
#define  MacroArraySize(array) \
	sizeof(equal_char_array(array))
