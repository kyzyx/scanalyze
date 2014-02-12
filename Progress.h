//############################################################
//
// Progress.h
//
// Matt Ginzton
// Mon Aug  3 13:17:07 PDT 1998
//
// C++ interface to tcl progressbar code.
//
//############################################################


#ifndef _PROGRESS_H_
#define _PROGRESS_H_


#include <tcl.h>


class Progress
{
 public:
  Progress (int end, const char* name = NULL, bool cancellable = false);
  Progress (int end, const char* operation, const char* name,
	    bool cancellable = false);

  ~Progress();

  bool update    (int current);
  bool updateInc (int increment = 1);

 private:

  void _init (int end, const char* name);

  int value;
  int maximum;
  int lastUpdateTime;
  int lastUpdateValue;
  int nUpdates;
  int iId;
  int baseY;
  char name[300];
  double mProjMatrix[16];

  class BailDetector* pBailDetector;

  static int nProgressBarsActive;
};


int PlvProgressCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[]);


#endif // _PROGRESS_H_
