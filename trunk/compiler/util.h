//----------------------------------------------
// holds a critical section within its life span
//----------------------------------------------
class CAutoCriticalSection
{
public:
	CAutoCriticalSection(CRITICAL_SECTION *section) : section(section)
	{
		EnterCriticalSection(section);
	}
	~CAutoCriticalSection()
	{
		LeaveCriticalSection(section);
	}
protected:
	CRITICAL_SECTION *section;
};