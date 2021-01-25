#pragma once

#ifdef _WIN32
	#include<windows.h>
#else
	#include<pthread.h>
#endif

class CLock
{
public:
	CLock();
	~CLock();

	void Lock();
	void UnLock();

private:
#ifdef _WIN32
	CRITICAL_SECTION m_Section;
#else
	pthread_mutex_t m_mutex;
#endif
};


class AutoLock
{
public:
	AutoLock();
	AutoLock(CLock& lock);
	~AutoLock();
private:
	CLock* m_pLock;
};