//############################################################
// TbObj.cc
// Kari Pulli
// Tue Jun 30 11:10:13 PDT 1998
//
// A base class for objects that are moved using the trackball
//############################################################

#include <stdlib.h>
#include <time.h>
#include "FileNameUtils.h"
#include "TbObj.h"
#include "Trackball.h"
#include "plvGlobals.h"

vector<TbObj::TbRedoInfo> TbObj::undo_stack;
int TbObj::real_size = 0;


TbObj::TbObj(void)
{

}

const int UNDO_SIZE = 400;

TbObj::~TbObj(void)
{
  // remove instances of myself from the undo_stack
  clear_undo (this);
}


Pnt3
TbObj::worldCenter(void)
{
  Pnt3 wctr = rot_ctr;
  xf(wctr);
  return wctr;
}


Bbox
TbObj::worldBbox(void)
{
  return bbox.worldBox(xf);
}


float
TbObj::radius(void)
{
  return bbox.max_dist(rot_ctr);
}


#if 1
#define __DUMP
#else
#define __DUMP  { \
  cout << "dump start" << endl; \
  for (int i = 0; i < real_size; i++) { \
    cout << undo_stack[i].self << ": " << undo_stack[i].xf << endl; \
  } \
  cout << "-- break -- " << endl; \
  for (; i < undo_stack.size(); i++) { \
    cout << undo_stack[i].self << ": " << undo_stack[i].xf << endl; \
  } \
  cout << "dump end" << endl << endl; \
}
#endif


void
TbObj::save_for_undo(void)
{
  // CAREFUL: 'this' pointer could be NULL, indicating trackball itself!

  // no more redo's
  if (real_size != undo_stack.size()) {
// STL Update
    undo_stack.erase(undo_stack.begin() + real_size, undo_stack.end());
  }

  assert(real_size == undo_stack.size());

  if (real_size >= UNDO_SIZE) {
    // forget the 100 first xforms
// STL Update
    undo_stack.erase(undo_stack.begin(), undo_stack.begin() + 100);
  }

  // create the entry
  TbRedoInfo a; a.self = this;
  a.xf = this ? this->xf : tbView->getUndoXform();

  // save the entry
  undo_stack.push_back(a);
  real_size = undo_stack.size();

  __DUMP;
}


void
TbObj::undo(void)
{
  if (real_size == 0) return;
  assert(real_size > 0);

#if 0
  if (real_size == undo_stack.size()) {
    // if the first undo, save the current situation first
    undo_stack.back().self->save_for_undo();
    real_size--;
  }
#endif
  real_size--;

  TbRedoInfo &info = undo_stack[real_size];

  // careful, info.self could be NULL
  Xform<float> tmp;
  if (info.self) {
    tmp = info.self->xf;
    info.self->xf = info.xf;
  } else {
    tmp = tbView->getUndoXform();
    tbView->setUndoXform (info.xf);
  }
  info.xf = tmp;

  __DUMP;
}


void
TbObj::redo(void)
{
  if (real_size < undo_stack.size()) {
    TbRedoInfo &info = undo_stack[real_size];
    real_size++;

    // careful, info.self could be NULL
    Xform<float> tmp;
    if (info.self) {
      tmp = info.self->xf;
      info.self->xf = info.xf;
    } else {
      tmp = tbView->getUndoXform();
      tbView->setUndoXform (info.xf);
    }
    info.xf = tmp;
  }

  __DUMP;
}


void
TbObj::clear_undo (TbObj* objToRemove)
{
  if (objToRemove) {
    int top = 0;
    for (int i=0; i<real_size; i++) {
      if (undo_stack[i].self != objToRemove) {
	undo_stack[top++] = undo_stack[i];
      }
    }
    real_size = top;
// STL Update
    undo_stack.erase(undo_stack.begin() + real_size, undo_stack.end());
  } else {
    // clear all
    undo_stack.clear();
    real_size = 0;
  }
}


void
TbObj::rotate(float q0, float q1, float q2, float q3,
	      bool undoable)
{
  if (undoable) save_for_undo();
  Pnt3 wc = worldCenter();

  xf.translate(-wc[0], -wc[1], -wc[2]);
  xf.rotQ(q0, q1, q2, q3);
  xf.translate(wc);
}


void
TbObj::translate(float t0, float t1, float t2,
		 bool undoable)
{
  if (undoable) save_for_undo();
  xf.translate(t0,t1,t2);
}


void
TbObj::new_rotation_center(const Pnt3 &wc)
{
  // have to apply inverse transformation because
  // the rotation center is in local coordinates!
  xf.apply_inv(wc, rot_ctr);
}


Xform<float>
TbObj::getXform(void) const
{
  return xf;
}


void
TbObj::setXform(const Xform<float> &x, bool undoable)
{
  if (undoable) save_for_undo();
  xf = x;
}


bool
TbObj::readXform(istream& in)
{
  save_for_undo();
  in >> xf;
  return (!in.fail());
}


bool
TbObj::writeXform(ostream& out)
{
  out << xf;
  return !out.fail();
}


void
TbObj::resetXform(void)
{
  save_for_undo();
  xf.identity();
}


bool
TbObj::readXform(const crope& baseFile)
{
  crope name = (baseFile + ".xf");
// C++ Update
//  ifstream istr (name.c_str(), ios::in | ios::nocreate);
  ifstream istr (name.c_str(), ios::in );     // ios::nocreate no longer exists, and ios::in should never create files anyway.

  bool success = readXform(istr);
  if (success)
    cout << "Read xform " << name << endl;
  //save_for_undo(); // already done by the other reader
  return success;
}


bool
TbObj::writeXform (const crope& baseFile)
{
  crope name = (baseFile + ".xf");

  // find out whether the file exists already
  // and is readable
  if (access(name.c_str(), R_OK) == 0) {
    // append to the backup file, change also access mode
    crope buname = name + ".bu";

    chmod (buname.c_str(), 0664);

    crope tmpfile = buname + ".tmp";
    unlink(tmpfile.c_str());
    FILE* to = fopen (tmpfile.c_str(), "w");

    FILE* from = fopen (buname.c_str(), "r");

    bool problem = false;

    if (from) {
      if (to) {
	int ch;
	while ((ch = getc(from)) != EOF) {
	  putc(ch, to);
	}
      } else {
	problem |= true;
      }
      fclose(from);
    }

    time_t t;
    fprintf(to, "%s", asctime(localtime (&t)));

    from = fopen(name.c_str(), "r");
    if (from) {
      char buf[2000];
      int r = fread (buf, 1, 2000, from);
      problem |= (r != fwrite (buf, 1, r, to));

      fclose(from);
      fclose(to);
    }
    unlink(buname.c_str());

    if (0 == rename(tmpfile.c_str(), buname.c_str())) {
      chmod (buname.c_str(), 0664);
    } else {
      problem |= true;
    }

    if (problem) {
      cerr << "Problem updating backup " << buname << endl;
    } else {
      cerr << "Appended " << name << " to " << buname << endl;
    }
  } else {
    cerr << "No previous xform file " << name << endl;
  }

  cerr << "Writing xform to " << name << " ... " << flush;
  // delete the file first so the owner changes
  unlink(name.c_str());
  ofstream ostr (name.c_str());
  bool success = TbObj::writeXform(ostr);
  cout << (success ? "ok" : "fail") << "." << endl;
  if (success) {
    // change the permissions...
    chmod (name.c_str(), 0664);
  }
  return success;
}
