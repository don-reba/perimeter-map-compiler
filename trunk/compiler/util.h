//----------------------------------------------
// holds a critical section within its life span
//----------------------------------------------
class AutoCriticalSection
{
public:
	AutoCriticalSection(CRITICAL_SECTION *section) : section(section)
	{
		in_section = true;
		if (NULL != section)
			EnterCriticalSection(section);
	}
	~AutoCriticalSection()
	{
		if (NULL != section && in_section)
			LeaveCriticalSection(section);
		in_section = false;
	}
	void Enter()
	{
		if (!in_section)
		{
			in_section = true;
			if (NULL != section)
				EnterCriticalSection(section);
		}
	}
	void Leave()
	{
		if (NULL != section)
			LeaveCriticalSection(section);
		in_section = false;
	}
protected:
	CRITICAL_SECTION *section;
	bool in_section;
};

inline int exp2(unsigned int n)
{
	int e(1);
	while (0 != n)
	{
		e <<= 1;
		--n;
	}
	return e;
}

inline int log2(unsigned int n)
{
	int l(-1);
	while (0 != n)
	{
		n >>= 1;
		++l;
	}
	return l;
}