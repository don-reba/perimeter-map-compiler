// All the data types needed for data-viewer architecture


#pragma once

// includes
#include <vector>
#include "viewer.h"
#include "map info.h"

using std::vector;


// a fixed-size array allocated at runtime
template<typename data_type>
struct CStaticArray
{
	CStaticArray () : ptr(NULL), length(0){}
	data_type *ptr;
	size_t length;
};

// 256-colour paletted texture structure
struct CPalettedTexture : public CStaticArray<BYTE>
{
	COLORREF palette[256];
};

// adds viewer binding capability to arbitrary data
template<typename data_type>
class CData
{
public:
	CData(void) : currently_updated(false), is_valid(false)
	{
		InitializeCriticalSection(&critical_section);
	}
public:
	// update each viewer
	void Update(int caller)
	{
		_ASSERTE(is_valid);
		currently_updated = true;
		vector<CViewer*>::const_iterator i = viewers.begin();
		const vector<CViewer*>::const_iterator end = viewers.end();
		for (; i != end; ++i)
			(*i)->Update(caller);
		currently_updated = false;
	}
	// remember another viewer
	void AddViewer(CViewer *viewer)
	{
		viewers.push_back(viewer);
	}
	// check if the data is currently updated
	inline bool IsCurrentlyUpdated() const
	{
		return currently_updated;
	}
	// get access to the data
	data_type &SignOut()
	{
		EnterCriticalSection(&critical_section);
		InterlockedExchange(&is_valid, false);
		is_valid = false;
		return data;
	}
	const data_type &SignOutConst()
	{
		while (!is_valid)
			Sleep(128);
		EnterCriticalSection(&critical_section);
		return data;
	}
	// attempt to get access to the data; returns NULL if not possible
	data_type *TrySignOut()
	{
		if (TRUE == TryEnterCriticalSection(&critical_section))
			return &data;
		return NULL;
	}
	// release acces
	void SignIn()
	{
		InterlockedExchange(&is_valid, true);
		LeaveCriticalSection(&critical_section);
	}
	// check validity
	bool IsValid() const
	{
		return is_valid != false;
	}
protected:
	vector<CViewer*> viewers;
	bool currently_updated;
	LONG is_valid;
	data_type data;
	CRITICAL_SECTION critical_section;
};
