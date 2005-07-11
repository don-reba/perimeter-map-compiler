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


#include "StdAfx.h"

#include "error handler.h"
#include "resource.h"
#include "resource management.h"
#include "task common.h"

using namespace TaskCommon;

namespace RsrcMgmt
{
	//bool SaveImageFromResource(uint id, LPCTSTR path, ErrorHandler &error_handler)
	//{
	//	// load image from the resourcesBYTE *compressed_buffer
	//	BYTE *raw_image;
	//	DWORD raw_image_size;
	//	{
	//		HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(id), _T("IMG")));
	//		HGLOBAL resource(LoadResource(NULL, resource_info));
	//		raw_image = ri_cast<BYTE*>(LockResource(resource));
	//		if (NULL == raw_image)
	//		{
	//			error_handler.MacroDisplayError(_T("Image resource could not be locked."));
	//			return false;
	//		}
	//		raw_image_size = SizeofResource(NULL, resource_info);
	//	}
	//	// save the image
	//	SaveMemToFile(path, raw_image, raw_image_size, error_handler);
	//	return true;
	//}

	bool UncompressResource(uint id, BYTE *result, size_t alloc)
	{
		// load the compressed template
		BYTE *compressed_buffer;
		DWORD compressed_buffer_size;
		{
			HRSRC resource_info(FindResource(NULL, MAKEINTRESOURCE(id), _T("BZ2")));
			HGLOBAL resource(LoadResource(NULL, resource_info));
			compressed_buffer = ri_cast<BYTE*>(LockResource(resource));
			if (NULL == compressed_buffer)
				return false;
			compressed_buffer_size = SizeofResource(NULL, resource_info);
		}
		// uncompress
		if (BZ_OK != BZ2_bzBuffToBuffDecompress(
			ri_cast<char*>(result),
			&alloc,
			ri_cast<char*>(compressed_buffer),
			compressed_buffer_size,
			0,
			0))
		{
			return false;
		}
		return true;
	}
}
