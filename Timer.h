////////////////////////////////////////////////////////////
// Timer.h
// Kari Pulli
//
////////////////////////////////////////////////////////////

#ifndef _TIMER_H_
#define _TIMER_H_

#include <iostream.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <sys/time.h>		// for gettimeofday()
#  include <sys/times.h>		// for times()
#endif
#include <time.h>		// for gettimeofday()
#include <assert.h>

const int NTICKS=100;
#define REALTIME

class Timer {
private:
  const char* iname;
  int started;
  int tmspu, tmsps;	// process{user,system}
  double ireal;

public:
  Timer(const char* pname=0) : iname(pname) { zero(); }
  void setname(const char* pname) { iname = pname; }
  void zero()                 { started =tmspu =tmsps =ireal = 0; }
  const char* name() const    { return iname; }
  float real() const          { return ireal; }
  float user() const          { return float(tmspu+tmsps)/NTICKS; }

  void start()
    {
      started = 1;
      //assert(!started++);
#ifdef WIN32
      tmspu -= GetTickCount();
#else
      struct tms tm;
      (void)times(&tm);
      tmspu-=(int)tm.tms_utime; tmsps-=(int)tm.tms_stime;
#     ifdef REALTIME
      struct timeval ti; struct timezone tz;
      gettimeofday(&ti,&tz);
      ireal-=double(ti.tv_sec)+ti.tv_usec/1e6;
#     endif
#endif
    }

  void stop()
    {
      if (started) {
	started = 0;
	//assert(!--started);
#ifdef WIN32
	tmspu += GetTickCount();
#else
	struct tms tm;
	(void)times(&tm);
	tmspu+=(int)tm.tms_utime; tmsps+=(int)tm.tms_stime;
#       ifdef REALTIME
	struct timeval ti; struct timezone tz;
	gettimeofday(&ti,&tz);
	ireal+=double(ti.tv_sec)+ti.tv_usec/1e6;
#       endif
#endif
      }
    }

  ~Timer()                    { stop(); cerr << *this; }
  friend inline ostream& operator<<(ostream& s, const Timer& t);

  static unsigned int get_system_tick_count() // in milliseconds
    {
#ifdef WIN32
      return GetTickCount();
#else
#     ifdef REALTIME
      struct timeval ti; struct timezone tz;
      gettimeofday(&ti,&tz);
      return ti.tv_sec * 1000 + ti.tv_usec / 1000;
#     else
      struct tms tm;
      (void)times(&tm);
      return (int)tm.tms_utime + (int)tm.tms_stime;
#     endif
#endif
    }
};

#define TIMERN(id) Timer_##id
#define TIMERC(id) Timer TIMERN(id)(#id)
#define TIMER(id)  TIMERC(id); TIMERN(id).start()
#define TIMER_START(id)  TIMERN(id).start()
#define TIMER_STOP(id)   TIMERN(id).stop()


inline
ostream& operator<<(ostream& s, const Timer& t)
{
  s << "Timer " << (t.iname ? t.iname : "?") << ": ";
  if (t.started) return s << "running\n";
  return s << "real=" << t.ireal
	   << " u=" << float(t.tmspu)/NTICKS
	   << " s=" << float(t.tmsps)/NTICKS << endl;
}

#endif
