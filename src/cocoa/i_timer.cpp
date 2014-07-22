
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>

#include <SDL.h>

#include "basictypes.h"
#include "basicinlines.h"
#include "doomdef.h"
#include "i_system.h"
#include "templates.h"


unsigned int I_MSTime()
{
	return SDL_GetTicks();
}

unsigned int I_FPSTime()
{
	return SDL_GetTicks();
}


bool g_isTicFrozen;


namespace
{

timespec GetNextTickTime()
{
	timeval tv;
	gettimeofday( &tv, NULL );

	static const long MILLISECONDS_IN_SECOND = 1000;
	static const long MICROSECONDS_IN_SECOND = 1000 * MILLISECONDS_IN_SECOND;
	static const long NANOSECONDS_IN_SECOND  = 1000 * MICROSECONDS_IN_SECOND;

	timespec ts;
	ts.tv_sec  = tv.tv_sec;
	ts.tv_nsec = (tv.tv_usec + MICROSECONDS_IN_SECOND / TICRATE) * MILLISECONDS_IN_SECOND;

	if (ts.tv_nsec >= NANOSECONDS_IN_SECOND)
	{
		ts.tv_sec++;
		ts.tv_nsec -= NANOSECONDS_IN_SECOND;
	}

	return ts;
}


pthread_cond_t  s_timerEvent;
pthread_mutex_t s_timerMutex;
pthread_t       s_timerThread;

bool s_timerExitRequested;

uint32_t s_ticStart;
uint32_t s_ticNext;

uint32_t s_timerStart;
uint32_t s_timerNext;

int  s_tics;


void* TimerThreadFunc( void* )
{
	while ( true )
	{
		if (s_timerExitRequested)
		{
			break;
		}

		const timespec timeToNextTick = GetNextTickTime();

		pthread_mutex_lock(&s_timerMutex );
		pthread_cond_timedwait(&s_timerEvent, &s_timerMutex, &timeToNextTick);

		if ( !g_isTicFrozen )
		{
			__sync_add_and_fetch(&s_tics, 1);
		}

		s_timerStart = SDL_GetTicks();
		s_timerNext  = Scale(Scale(s_timerStart, TICRATE, 1000) + 1, 1000, TICRATE);

		pthread_cond_signal (&s_timerEvent);
		pthread_mutex_unlock(&s_timerMutex);
	}

	return NULL;
}

int GetTimeThreaded(bool saveMS)
{
	if (saveMS)
	{
		s_ticStart = s_timerStart;
		s_ticNext  = s_timerNext;
	}

	return s_tics;
}

int WaitForTicThreaded(int prevTic)
{
	assert(!g_isTicFrozen);

	while (s_tics <= prevTic)
	{
		pthread_mutex_lock(&s_timerMutex);

		const timespec timeToNextTick = GetNextTickTime();

		pthread_cond_timedwait(&s_timerEvent, &s_timerMutex, &timeToNextTick);
		pthread_mutex_unlock  (&s_timerMutex);
	}

	return s_tics;
}

void FreezeTimeThreaded(bool frozen)
{
	g_isTicFrozen = frozen;
}

} // unnamed namespace


fixed_t I_GetTimeFrac(uint32* ms)
{
	const uint32_t now = SDL_GetTicks();

	if (NULL != ms)
	{
		*ms = s_ticNext;
	}

	const uint32_t step = s_ticNext - s_ticStart;

	return 0 == step
		? FRACUNIT
		: clamp<fixed_t>( (now - s_ticStart) * FRACUNIT / step, 0, FRACUNIT);
}


void I_InitTimer ()
{
	pthread_cond_init (&s_timerEvent,  NULL);
	pthread_mutex_init(&s_timerMutex,  NULL);

	pthread_create(&s_timerThread, NULL, TimerThreadFunc, NULL);

	I_GetTime = GetTimeThreaded;
	I_WaitForTic = WaitForTicThreaded;
	I_FreezeTime = FreezeTimeThreaded;
}

void I_ShutdownTimer ()
{
	s_timerExitRequested = true;

	pthread_join(s_timerThread, NULL);

	pthread_mutex_destroy(&s_timerMutex);
	pthread_cond_destroy (&s_timerEvent);
}
