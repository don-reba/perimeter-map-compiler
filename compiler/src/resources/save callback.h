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

#include "resource ids.h"

namespace TaskCommon
{
	//----------------------------------------
	// saving callback base class
	// inherited by all the resource that save
	//----------------------------------------

	class SaveCallback
	{
	public:
		struct SaveHandler {
			virtual void OnSaveBegin(Resource id) = 0;
			virtual void OnSaveEnd  (Resource id) = 0;
		};
	public:
		SaveCallback(Resource id) : save_handler_(NULL), id_(id) {}
		void SetOnSave(SaveHandler *handler)
		{
			save_handler_ = handler;
		}
	protected:
		void SaveBegin() const
		{
			if (NULL != save_handler_)
				save_handler_->OnSaveBegin(id_);
		}
		void SaveEnd() const
		{
			if (NULL != save_handler_)
				save_handler_->OnSaveEnd(id_);
		}
	private:
		const Resource id_;
		SaveHandler *save_handler_;
	};
} // TaskCommon
