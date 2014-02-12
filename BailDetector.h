// BailDetector.cc    interface for peeking at events,
//                    and bailing out of long operations
// created 2/5/99     magi@cs


#ifndef _BAILDETECTOR_H_
#define _BAILDETECTOR_H_

#include <tk.h>


class BailDetector
{
 public:
  BailDetector();
  ~BailDetector();

  static bool bail (void);
  bool operator() (void) { return bail(); };

 private:

  static bool s_bBail;      // shared between bail detectors on stack
  static int  s_iBailDepth; // keep track of when contstructor is master
                            // in chain of several nested bail detectors
  static bool s_bBailNoticed; // check if bail was acted on
  static int  s_iLastChecked; // time events last pumped

  static Tk_RestrictAction
    filterproc (ClientData clientData, XEvent* eventPtr);
};


int PlvBailDetectCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);


#endif // _BAILDETECTOR_H_
