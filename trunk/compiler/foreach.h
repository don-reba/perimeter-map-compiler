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