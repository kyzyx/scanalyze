//###############################################################
// Random.h
// Kari Pulli
// 02/12/1996
//###############################################################

#ifndef _Random_h
#define _Random_h

#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

#ifdef WIN32
#define srand48 srand
static double drand48 (void)
{
	return rand() / (float)RAND_MAX;
}
#endif


class Random {
private:
  static long TimeSeed(void)
    { return (long) time(NULL); }

public:
  Random(unsigned long initSeed = TimeSeed())
    { srand48(initSeed); }
  ~Random(void) {}

  void setSeed(unsigned long newSeed = TimeSeed())
    { srand48(newSeed); }

  double operator()()
    // return a random number [0.0,1.0)
    { return drand48(); }

  double operator()(double fact)
    // return a random number [0.0,fact)
    { return fact*drand48(); }
};

#endif
