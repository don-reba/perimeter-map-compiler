#include "StdAfx.h"
#include ".\preference data.h"

CPreferenceData::CPreferenceData()
{
	InitializeCriticalSection(&critical_section);
}

CPreferenceData::~CPreferenceData()
{
	DeleteCriticalSection(&critical_section);
}

void CPreferenceData::SignIn()
{	
	LeaveCriticalSection(&critical_section);
}

void CPreferenceData::SignOut()
{	
	EnterCriticalSection(&critical_section);
}