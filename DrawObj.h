//############################################################
//
// DrawObj.h
//
// Kari Pulli
// Thu Jan 21 18:14:19 CET 1999
//
// A plugin object for adding OpenGL renderings into your
// refresh routine.
//
//############################################################

#ifndef _DRAWOBJ_H_
#define _DRAWOBJ_H_

#include <vector>
#include <GL/gl.h>

class DrawObj {
private:
  bool enabled;
public:
  DrawObj(void) : enabled(1) {}
  ~DrawObj(void) {}

  void enable(void)  { enabled = true; }
  void disable(void) { enabled = false; }
  void draw(void)
    {
      if (!enabled) return;
      glPushAttrib(GL_ALL_ATTRIB_BITS);
      drawthis();
      glPopAttrib();
    }
  virtual void drawthis(void) = 0;
};


// make an instance of the following object just
// in one place and use it as external variable
// to add/remove DrawObj's
class DrawObjects {

private:
  vector<DrawObj *> list;
  typedef vector<DrawObj*>::iterator itT;

public:
  DrawObjects(void) {}
  ~DrawObjects(void)
    {
      // magi -- this seems like a bad idea, since we don't know who added
      // stuff here or why, so we can't assume ownership.  Plus it's not
      // that useful, since this class is only instantiated once, as a
      // static object, so the only time this destructor will be called is
      // at exit time... who really cares about freeing memory.  But
      // crashes on exit look bad, and purify showed this actually getting
      // called for stuff that had already been freed... not sure how, but
      // easiest to disable it.

      //for (itT it = list.begin(); it != list.end(); it++) {
      //delete *it;
      //}
    }

  void operator()(void)
    {
      for (itT it = list.begin(); it != list.end(); it++) {
	(*it)->draw();
      }
    }

  bool add(DrawObj *o)
    {
      if (find(list.begin(), list.end(), o) == list.end()) {
	list.push_back(o);
	return true;
      } else {
	return false;
      }
    }

  bool remove(DrawObj *o)
    {
      itT it = find(list.begin(), list.end(), o);
      if (it == list.end()) {
	return false;
      } else {
	list.erase(it);
	return true;
      }
    }
};

#endif
