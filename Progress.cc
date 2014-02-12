//############################################################
//
// Progress.cc
//
// Matt Ginzton
// Mon Aug  3 13:17:07 PDT 1998
//
// C++ interface to tcl progressbar code.
//
//############################################################


#include <tcl.h>
#include <string.h>
#include <stdlib.h>
#include <vector.h>
#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/glu.h>
#include "togl.h"
#include "Progress.h"
#include "BailDetector.h"
#include "plvGlobals.h"
#include "Timer.h"
#include "ToglText.h"


int Progress::nProgressBarsActive = 0;

// performance tweaks -- higher values will seem less responsive,
// but waste less time on progress bars.
// can be changed through "plv_progress" public Tcl interface.

// minimum amount of time to elapse between successive updates
static int kUpdateFrequency = 300; // in ms

// min. operation length to deserve a status bar
static int kTransience = 4;        // in multiples of update frequency


Progress::Progress (int end, const char* name, bool cancellable)
{
  _init (end, name);
}


Progress::Progress (int end, const char* operation, const char* name,
		    bool cancellable)
{
  char nameBuf[600];
  sprintf (nameBuf, operation, name);

  _init (end, nameBuf);
}


void Progress::_init (int end, const char* _name)
{
  value = 0;
  maximum = end;
  pBailDetector = new BailDetector;
  lastUpdateTime = 0;
  lastUpdateValue = 0;
  nUpdates = 0;

  iId = nProgressBarsActive++;
  baseY = 5 + iId * 28;

  if (strlen (_name) >= sizeof (name)) {
    strncpy (name, _name, sizeof(name));
    name[sizeof(name)-1] = 0;
  } else {
    strcpy (name, _name);
  }
}


static void rect (int x1, int y1, int x2, int y2)
{
  glVertex2i (x1, y1);
  glVertex2i (x2, y1);
  glVertex2i (x2, y2);
  glVertex2i (x1, y2);
}

Progress::~Progress()
{
  delete pBailDetector;

  nProgressBarsActive--;
}


bool Progress::update (int current)
{
  if (g_bNoUI)
    return true;

  value = current;
  unsigned int timestamp = Timer::get_system_tick_count();
  if (value == lastUpdateValue ||
      timestamp < (lastUpdateTime + kUpdateFrequency))
    return true;

  // make sure enough updates have happened to warrant a redraw
  if (nUpdates++ % kTransience != 0)
    return true;

  glDrawBuffer (GL_FRONT);

  // save all the bits just so I don't turn off something
  // that someone else expects to be on
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glDisable (GL_DEPTH_TEST);
  glDisable (GL_LIGHTING);

  // clear the modelview matrix - save previous
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // clear the projection matrix - save previous
  glMatrixMode (GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  // set orthagraphic to encompass the entire screen
  gluOrtho2D (0, theWidth, 0, theHeight);

  glBegin (GL_QUADS);
  glColor3f (.7, 0, 0); rect (0, baseY, theWidth - 1, baseY + 19);
  glColor3f (.2, 0, 0); rect (1, baseY + 1, theWidth, baseY + 20);
  glEnd();

  // render new portion of progress bar
  // subtract one so the bar "
  int width = value / (double)maximum * theWidth;
  if (g_verbose) cerr << "width = " << width << endl;

  glBegin (GL_QUADS);
  glColor3f (1, 0, 0); rect (0, baseY + 2, width, baseY + 18);
  glEnd();

  // setup XOR mode
  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);

  // draw text in red for past-end-of-bar
  glColor3f (1, 0, 0);
  glRasterPos2i (15, baseY + 6);
  DrawText (toglCurrent, name);

  glLogicOp (GL_COPY);
  glDisable (GL_COLOR_LOGIC_OP);

  //pop the matrices
  glMatrixMode (GL_PROJECTION);
  glPopMatrix();
  glMatrixMode (GL_MODELVIEW);
  glPopMatrix();

  glFinish();
  glPopAttrib(); // restore the previous attributes

  lastUpdateTime = timestamp;
  lastUpdateValue = value;

  return !(*pBailDetector)();
}


bool Progress::updateInc (int increment)
{
  return update (value + increment);
}


static vector<Progress*> inprogress;

int
PlvProgressCmd(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "missing args to plv_progress";
    return TCL_ERROR;
  }

  bool result = true;

  if (!strcmp (argv[1], "start")) {

    Progress* prog = new Progress (atoi (argv[2]), argv[3]);
    inprogress.push_back (prog);

  } else if (!strcmp (argv[1], "update")) {

    assert (inprogress.size() > 0);
    result = inprogress.back()->update (atoi (argv[2]));

  } else if (!strcmp (argv[1], "updateinc")) {

    assert (inprogress.size() > 0);
    if (argc == 2)
      result = inprogress.back()->updateInc();
    else
      result = inprogress.back()->updateInc (atoi (argv[2]));

  } else if (!strcmp (argv[1], "done")) {

    assert (inprogress.size() > 0);
    Progress* prog = inprogress.back();
    inprogress.pop_back();
    delete prog;

  } else if (!strcmp (argv[1], "transience")) {

    kTransience = atoi (argv[2]);

  } else if (!strcmp (argv[1], "frequency")) {

    kUpdateFrequency = atoi (argv[2]);

  } else {

    interp->result = "Bad arg to plv_progress";
    return TCL_ERROR;

  }

  interp->result = result ? "1" : "0";
  return TCL_OK;
}


