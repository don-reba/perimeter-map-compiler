#pragma once

class CPreferenceData
{
public:
// construction/destruction
	CPreferenceData();
	virtual ~CPreferenceData();
public:
// interface
	virtual void Save(string file_name) = 0;
	virtual void Load(string file_name) = 0;
	virtual void Update() = 0;
	virtual void SignOut();
	virtual void SignIn();
protected:
// data
		CRITICAL_SECTION critical_section;
};
