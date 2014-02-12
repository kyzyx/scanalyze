//############################################################
//
// MeshTransport.cc
//
// Matt Ginzton
// Tue Feb  9 16:38:33 CET 1999
//
// Wrapper class for sending geometry information, as one or
// more triangle meshes, from point a to b, managing memory
// issues along the way.
//
//############################################################


#include "MeshTransport.h"


#ifdef SCZ_NORMAL_FORCEFLOAT
const GLenum MeshTransport::normal_type = GL_FLOAT;
#else
const GLenum MeshTransport::normal_type = GL_SHORT;
#endif


MeshTransport::MeshTransport()
{
}


void
MeshTransport::setVtx (const vector<Pnt3>* in,
		       TransportMode mode,
		       const Xform<float>& xfBy)
{
  switch (mode) {
  case copy:
    vtx.push_back (new vector<Pnt3> (*in));
    freeVtx.push_back (true);
    break;

  case share:
    vtx.push_back (in);
    freeVtx.push_back (false);
    break;

  case steal:
    vtx.push_back (in);
    freeVtx.push_back (true);
  }

  xf.push_back (xfBy);
  bbox.push_back(Bbox());
}


void
MeshTransport::setBbox (const Bbox& in)
{
  // depends on being called after setVtx has already pushed a blank bbox
  // on the end of the bbox vector
  assert (bbox.size() > 0);
  assert (!bbox.back().valid());

  bbox.back() = in;
}


void
MeshTransport::setNrm (const vector<short>* in,
		       TransportMode mode)
{
#ifndef SCZ_NORMAL_FORCEFLOAT
  switch (mode) {
  case copy:
    nrm.push_back (new vector<short> (*in));
    freeNrm.push_back (true);
    break;

  case share:
    nrm.push_back (in);
    freeNrm.push_back (false);
    break;

  case steal:
    nrm.push_back (in);
    freeNrm.push_back (true);
  }
#else
#pragma message ( "Normals will be floats! ")
  // convert normals to float, always copy
  // this is a slow hack workaround for the fact that some graphics cards,
  // including many PC implementations, can't render normals-as-shorts right
  vector<float>* out = new vector<float> (in->size());
  float* pOut;
  const short* pIn;
  for (pIn = in->begin(), pOut = out->begin();
       pIn != in->end();
       pIn++, pOut++) {
    *pOut = *pIn / 32767.0;
  }
  nrm.push_back (out);
  freeNrm.push_back (true);
#endif
}


#ifdef SCZ_NORMAL_FORCEFLOAT
void
MeshTransport::setNrm (const vector<float>* in, TransportMode mode)
{
  // to be called only by appendMT
  switch (mode) {
  case share:
    nrm.push_back (in);
    freeNrm.push_back (false);
    break;

  case steal:
    nrm.push_back (in);
    freeNrm.push_back (true);
  }
}
#endif

void
MeshTransport::setTris (const vector<int>* in,
			TransportMode mode)
{
  switch (mode) {
  case copy:
    tri_inds.push_back (new vector<int> (*in));
    freeTris.push_back (true);
    break;

  case share:
    tri_inds.push_back (in);
    freeTris.push_back (false);
    break;

  case steal:
    tri_inds.push_back (in);
    freeTris.push_back (true);
  }
}


void
MeshTransport::setColor (const vector<uchar>* in,
			 TransportMode mode)
{
  switch (mode) {
  case copy:
    color.push_back (new vector<uchar> (*in));
    freeColor.push_back (true);
    break;

  case share:
    color.push_back (in);
    freeColor.push_back (false);
    break;

  case steal:
    color.push_back (in);
    freeColor.push_back (true);
  }
}


void
MeshTransport::appendMT (MeshTransport* in,
			 const Xform<float>& xfBy)

{
  for (int i = 0; i < in->vtx.size(); i++) {
    setVtx (in->vtx[i], in->freeVtx[i] ? steal : share, xfBy * in->xf[i]);
    in->freeVtx[i] = false;

    setBbox (in->bbox[i]);

    setNrm (in->nrm[i], in->freeNrm[i] ? steal : share);
    in->freeNrm[i] = false;

    setTris (in->tri_inds[i], in->freeTris[i] ? steal : share);
    in->freeTris[i] = false;

    if (i < in->color.size()) {
      setColor (in->color[i], in->freeColor[i] ? steal : share);
      in->freeColor[i] = false;
    }
  }
}


MeshTransport::~MeshTransport()
{
  int i;
  for (i = 0; i < vtx.size(); i++)
    if (freeVtx[i])
      delete vtx[i];
  for (i = 0; i < nrm.size(); i++)
    if (freeNrm[i])
      delete nrm[i];
  for (i = 0; i < tri_inds.size(); i++)
    if (freeTris[i])
      delete tri_inds[i];
  for (i = 0; i < color.size(); i++)
    if (freeColor[i])
      delete color[i];
}
