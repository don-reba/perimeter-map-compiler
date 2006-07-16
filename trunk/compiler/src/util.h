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
