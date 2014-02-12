//############################################################
//
// MeshTransport.h
//
// Matt Ginzton
// Tue Feb  9 16:38:33 CET 1999
//
// Wrapper class for sending geometry information, as one or
// more triangle meshes, from point a to b, managing memory
// issues along the way.
//
//############################################################


#ifndef _MESHTRANSPORT_H_
#define _MESHTRANSPORT_H_

#include <vector.h>
#ifdef WIN32
#       include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include "Pnt3.h"
#include "Xform.h"
#include "Bbox.h"
#include "defines.h"


class MeshTransport {
 public:
  MeshTransport();
  ~MeshTransport();

  vector<const vector<Pnt3>*>  vtx;
  vector< Xform<float> >       xf;
  vector<Bbox>                 bbox;
  vector<const vector<int>*>   tri_inds;
  vector<const vector<uchar>*> color;

  static const GLenum normal_type;
#ifdef SCZ_NORMAL_FORCEFLOAT
  vector<const vector<float>*> nrm;
#else
  vector<const vector<short>*> nrm;
#endif


  enum TransportMode { copy, share, steal };
  void setVtx (const vector<Pnt3>* in, TransportMode mode,
	       const Xform<float>& xfBy = Xform<float>());
  void setBbox (const Bbox& in);
  void setNrm (const vector<short>* in, TransportMode mode);
  void setTris (const vector<int>* in, TransportMode mode);
  void setColor (const vector<uchar>* in, TransportMode mode);

  void appendMT (MeshTransport* in,
		 const Xform<float>& xfBy = Xform<float>());

#ifdef SCZ_NORMAL_FORCEFLOAT
 private:
  void setNrm (const vector<float>* in, TransportMode mode);
#endif

 private:
  vector<bool> freeVtx;
  vector<bool> freeNrm;
  vector<bool> freeTris;
  vector<bool> freeColor;
};


#endif // _MESHTRANSPORT_H_
