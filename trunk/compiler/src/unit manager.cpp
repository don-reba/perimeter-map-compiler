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

#include "stdafx.h"

#include "unit manager.h"

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/array/size.hpp>
#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/stringize.hpp>

//-------------------------
// UnitManager construction
//-------------------------

UnitManager::UnitManager()
	:is_enabled_(true)
{
	// register the unit classes in the factory
	#define MacroRegisterClass(z, n, data)                                            \
	   {                                                                              \
	      struct MacroRegisterClassCreator                                            \
	      {                                                                           \
	         static interop::SaveUnitData *create()                                   \
	         {                                                                        \
	            return new interop::BOOST_PP_ARRAY_ELEM(n, INTEROP_UNIT_TYPE_LIST)(); \
	         }                                                                        \
	      };                                                                          \
	      unit_factory_.Register(                                                     \
	         BOOST_PP_STRINGIZE(BOOST_PP_ARRAY_ELEM(n, INTEROP_UNIT_TYPE_LIST)),      \
	         &MacroRegisterClassCreator::create);                                     \
	   }
	BOOST_PP_REPEAT(BOOST_PP_ARRAY_SIZE(INTEROP_UNIT_TYPE_LIST), MacroRegisterClass, ~);
	#undef MacroRegisterClass
}

//----------------------
// UnitManager interface
// unit management
//----------------------

void UnitManager::CreateUnit(const char * name, Category category)
{
	units_.push_back(Unit(unit_factory_.CreateObject(name), category));
}

void UnitManager::DestroyUnit(units_t::iterator unit)
{
	delete unit->data_;
	units_.erase(unit);
}

void UnitManager::Clear()
{
	foreach (Unit &unit, units_)
		delete unit.data_;
	units_.clear();
}

//----------------------
// UnitManager interface
// state management
//----------------------

void UnitManager::AddEnabledChangeListener(on_enabled_change_t listener)
{
	listeners_.push_back(listener);
}

void UnitManager::SetEnabled(bool enabled)
{
	is_enabled_ += enabled ? 1 : -1;
	if (0 == is_enabled_)
		NotifyListeners(true);
	else if (-1 == is_enabled_)
		NotifyListeners(false);
}

bool UnitManager::IsEnabled() const
{
	return is_enabled_ >= 0;
}

//----------------------
// UnitManager interface
// container operations
//----------------------
UnitManager::units_t::iterator UnitManager::begin()
{
	return units_.begin();
}

UnitManager::units_t::iterator UnitManager::end()
{
	return units_.end();
}

UnitManager::Unit &UnitManager::operator [] (int i)
{
	return units_[i];
}

UnitManager::Unit &UnitManager::at(int i)
{
	return units_.at(i);
}

//----------------------
// UnitManager interface
// data access
//----------------------

UnitManager::units_t &UnitManager::GetUnits()
{
	return units_;
}

//---------------------------
// UnitManager implementation
//---------------------------

void UnitManager::NotifyListeners(bool enabled)
{
	foreach (const on_enabled_change_t &delegate, listeners_)
		on_enabled_change_t(enabled);
}
