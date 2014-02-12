//############################################################
// TbObj.h
// Kari Pulli
// Tue Jun 30 11:10:13 PDT 1998
//
// A base class for objects that are moved using the trackball
//############################################################

#ifndef _TBOBJ_H_
#define _TBOBJ_H_

#ifdef WIN32
# include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include <fstream.h>
#include <iostream.h>
#include <rope.h>

#include "Xform.h"
#include "Pnt3.h"
#include "Bbox.h"


class TbObj {
private:
  struct TbRedoInfo {
    TbObj        *self;
    Xform<float>  xf;
    //Pnt3         rot_ctr;
  };
  static vector<TbRedoInfo> undo_stack;
  static int                real_size;

  Xform<float> xf;        // registration/modeling transformation

protected:
  Pnt3         rot_ctr;   // rotation center (in local coords)
  Bbox         bbox;      // bounding box (in local coords)

public:
  TbObj(void);
  ~TbObj(void);

  void         save_for_undo(void);
  static void  undo(void);
  static void  redo(void);
  static void  clear_undo(TbObj* objToRemove = NULL);

  const Pnt3  &localCenter(void)      { return rot_ctr; }
  Pnt3         worldCenter(void);

  const Bbox  &localBbox(void)        { return bbox; }
  Bbox         worldBbox(void);

  float   radius(void);

  void    rotate (float q0, float q1, float q2, float q3,
		  bool undoable = true);
  void    translate (float t0, float t1, float t2,
		     bool undoable = true);

  void    new_rotation_center (const Pnt3 &wc);

  Xform<float> getXform(void) const;
  void         setXform(const Xform<float> &x,
			bool undoable = true);

private:
  bool    readXform  (istream& in);
  bool    writeXform (ostream& out);
public:
  void    resetXform (void);
  void    xformPnt   (Pnt3 &p)  { xf(p); }
  void    xformInvPnt(Pnt3 &p)  { xf.apply_inv(p,p); }
  void    gl_xform   (void)     { glMultMatrixf(xf); }

  // These are virtual only so that we can do a dynamic
  // cast to TbObj
  // The input arg can be a string, the conversion is automatic
  virtual bool    readXform  (const crope& baseFile);
  virtual bool    writeXform (const crope& baseFile);
};

#endif
