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

#include "interop.h"

#include <loki/Factory.h>

#include <list>

class UnitManager
{
// nested types
public:
	enum Category
	{
		// the first prm_player_count ids are devoted to player buildings
		Neutral = prm_player_count,
		Environment,
		Filth,
		Category_count
	};
	struct Unit
	{
		Unit(interop::SaveUnitData * data, Category category) : data_(data), category_(category) {}
		interop::SaveUnitData * data_;
		Category                category_;
	};
	typedef fd::FastDelegate<void(bool)> on_enabled_change_t;
	typedef std::vector<Unit>            units_t;
private:
	typedef Loki::Factory<interop::SaveUnitData, std::string> factory_t;
	typedef std::list<on_enabled_change_t>                    listeners_t;
// creation
public:
	UnitManager();
// interface
public:
	// unit management
	void CreateUnit(const char * name, Category category);
	void DestroyUnit(units_t::iterator unit);
	void Clear();
	// state management
	void AddEnabledChangeListener(on_enabled_change_t listener);
	void SetEnabled(bool enabled);
	bool IsEnabled() const;
	// container operations
	units_t::iterator begin();
	units_t::iterator end();
	Unit &operator [] (int i);
	Unit &at(int i);
	// data access
	units_t &GetUnits();
// implementation
private:
	void NotifyListeners(bool enabled);
// data
private:
	int         is_enabled_;
	listeners_t listeners_;
	units_t     units_;
	factory_t   unit_factory_;
};

