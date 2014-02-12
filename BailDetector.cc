// BailDetector.cc    interface for peeking at events,
//                    and bailing out of long operations
// created 2/5/99     magi@cs


#include <tk.h>
#include <iostream>
#include "BailDetector.h"
#include "Timer.h"



// static member var declarations
int  BailDetector::s_iBailDepth = 0;
int  BailDetector::s_iLastChecked = 0;
bool BailDetector::s_bBail = false;
bool BailDetector::s_bBailNoticed = false;


BailDetector::BailDetector()
{
  // if we're the first one,
  if (!s_iBailDepth++) {
    s_bBail = false;       // initialize bail flag
    s_bBailNoticed = false;
    s_iLastChecked = 0;
  }
}


BailDetector::~BailDetector()
{
  // and uncount us from bail stack
  if (--s_iBailDepth == 0) {
    if (s_bBail) {
      if (s_bBailNoticed)
	cerr << "command cancelled." << endl;
      else
	cerr << "but command completed anyway." << endl;
    }
  }
}



Tk_RestrictAction
BailDetector::filterproc (ClientData clientData, XEvent* eventPtr)
{
  if (!s_bBail) {
    if (eventPtr->type == KeyPress) {
      //cerr << (((XKeyPressedEvent*)eventPtr)->keycode) << endl;
      // escape, as empirically evinced by the previous line, is 16:
      if ((((XKeyPressedEvent*)eventPtr)->keycode & 0xFF) == 16) {
      /* could also use:
	 || (eventPtr->type == MotionNotify
	 && ((XMotionEvent*)eventPtr)->state & (Button1Mask | Button2Mask)))

	 to bail out of a render that's already unwanted because you've
	 moved the trackball since it started.
	 but we'd want to be more careful than that, maybe look for a few in
	 a row, otherwise it's too sensitive.
      */

	s_bBail = true;
	cerr << "Cancel request noted... " << flush;
	return TK_DISCARD_EVENT;
      }
    }
  }

  // no matter what, don't process this event right now
  return TK_DEFER_EVENT;
}


bool
BailDetector::bail (void)
{
  // quick out if flag is already set
  unsigned int iTimeStamp = Timer::get_system_tick_count();
  if (!s_bBail && (iTimeStamp - s_iLastChecked  > 100)) {
    // ok, pump events
    s_iLastChecked = iTimeStamp;

    // set up message filter, so we can be notified if messages arrive
    Tk_RestrictProc* old_filter_proc;
    ClientData old_filter_data;
    old_filter_proc = Tk_RestrictEvents (filterproc, NULL, &old_filter_data);

    // both file and x events are necessary to hear keystrokes... they first
    // occur as a file event, which when processed becomes an X event.
    // careful not to specify idle events, or TK_ALL_EVENTS, because
    // that will interfere with the redraw itself and break it.
    if (Tk_DoOneEvent (TK_WINDOW_EVENTS | TK_FILE_EVENTS | TK_DONT_WAIT))
      // call again, because the first event could have been a file event
      // which queued the window event we actually care about
      Tk_DoOneEvent (TK_WINDOW_EVENTS | TK_DONT_WAIT);

    // remove message filter
    Tk_RestrictEvents (old_filter_proc, old_filter_data, &old_filter_data);
  }

  // then, need to check flag again since filter proc might have set it
  if (s_bBail)
    s_bBailNoticed = true;

  return s_bBail;
}


int PlvBailDetectCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  BailDetector bail;

  int result = Tcl_Eval (interp, argv[1]);
  if (argc > 2) {
    Tcl_SetVar (interp, argv[2], interp->result, 0);
  }

  interp->result = (bail() ? "1" : "0");

  return result;
}
