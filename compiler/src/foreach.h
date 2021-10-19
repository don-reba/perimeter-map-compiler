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

//--------------------------------------------
// a powerful foreach construction
// taken from http://rsdn.ru/Forum/?mid=988467
//--------------------------------------------

namespace foreach_detail
{
    struct wrapper_holder{};

    template <typename T>
    struct wrapper
        :wrapper_holder
    {
        mutable T wrapped;
        wrapper(const T& t)
            :wrapped(t) 
        {}
    };

    template <typename T> 
    wrapper<T> wrap(const T& t)
    { 
        return t; 
    }

    template <typename T>
    T& unwrap(const wrapper_holder& b)
    { 
        return static_cast<const wrapper<T>&>(b).wrapped; 
    }

    template <typename T>
    T& unwrap(const wrapper_holder& b, const T&)
    { 
        return unwrap<T>(b); 
    }

    template <typename T>
    bool is_wrapped_equal(const wrapper_holder& lhs, const wrapper_holder& rhs, const T&)
    { 
        return unwrap<T>(lhs)==unwrap<T>(rhs);
    }
}

#define foreach_(Decl, First, Last)\
if(bool _fe_break = false){}else \
    for(const foreach_detail::wrapper_holder\
             &_fe_cur=foreach_detail::wrap(First)\
            ,&_fe_end=foreach_detail::wrap(Last)\
        ;!foreach_detail::is_wrapped_equal(_fe_cur, _fe_end, Last)&&!_fe_break\
        ;++foreach_detail::unwrap(_fe_cur, Last)\
        )\
        if(!(_fe_break=true,_fe_break)){}else\
        for(Decl=*foreach_detail::unwrap(_fe_cur, Last);_fe_break;_fe_break=false)\
// end macro

#define foreach(Decl, Cont)    foreach_(Decl, (Cont).begin(), (Cont).end())