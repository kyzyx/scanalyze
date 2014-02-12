//############################################################
// DisplayMesh.cc
// Matt Ginzton / Brian Curless
// Wed Jul  1 15:08:21 PDT 1998
//
// Rendering code for mesh data
//############################################################


#include <vector>
#include <algorithm>
#include "RigidScan.h"
#include "Mesh.h"
#include "DisplayMesh.h"
#include "plvMeshCmds.h"
#include "plvScene.h"
#include "plvDraw.h"
#include "plvGlobals.h"
#include "togl.h"
#include "GlobalReg.h"
#include "BailDetector.h"
#include "VertexFilter.h"
#include "Bbox.h"
#include "MeshTransport.h"
#include "plvViewerCmds.h"
#include "OrganizingScan.h"


// we use some rendering features (glPolygonOffset, vertex arrays) that
// are only supported under OpenGL 1.1.  IRIX versions prior to 6.5 only
// have OpenGL 1.0, but have EXT versions of the same functionality that
// work almost as well.
// g_glVersion >= 1.1 enables usage of OpenGL 1.1 features.



// Core functionality: shared by both real meshes and organizing groups.
// Lots of other stuff is "supported" by both (in the public interface)
// but is stubbed out by one or the other and doesn't share implementation.
DisplayableMesh::DisplayableMesh()
{
  meshData = NULL;
  displayName = NULL;
}

DisplayableMesh::~DisplayableMesh()
{
  delete[] displayName;
}


bool
DisplayableMesh::getVisible (void)
{
  return bVisible
    && meshData->localBbox().valid();
}


void
DisplayableMesh::setVisible (bool bVis)
{
  bVisible = bVis;
}


RigidScan*
DisplayableMesh::getMeshData (void) const
{
  return meshData;
}


void
DisplayableMesh::resetMeshData (RigidScan* _meshData)
{
  invalidateCachedData();
  meshData = _meshData;
}


RigidScan*
MeshData (DisplayableMesh* disp)
{
  return disp ? disp->getMeshData() : NULL;
}


const char*
DisplayableMesh::getName (void) const
{
  return displayName;
}


void
DisplayableMesh::setName (const char* name)
{
  displayName = new char [strlen (name) + 1];
  strcpy (displayName, name);
}


const vec3uc&
DisplayableMesh::getFalseColor (void) const
{
  return colorFalse;
}


void
DisplayableMesh::setFalseColor (const vec3uc& color)
{
  colorFalse[0] = color[0];
  colorFalse[1] = color[1];
  colorFalse[2] = color[2];
}



//////////////////////////////////////////////////////////////////////
// a real DisplayableMesh::
//////////////////////////////////////////////////////////////////////


DisplayableRealMesh::DisplayableRealMesh (RigidScan* _meshData, char* nameBase)
{
  meshData = _meshData;
  setHome();
  if (nameBase == NULL)
    setName((char*)meshData->get_basename().c_str());
  else
    setName (nameBase);

  theScene->meshColors.chooseNewColor(colorFalse);
  myTexture = NULL;
  bBlend = false;
  alpha = 1.0;

  bUseDisplayList = false;
  iDisplayList = 0;

  bVisible = true;

  mBounds = NULL;
  cache[0].mesh = cache[1].mesh = NULL;
  invalidateCachedData();
}


DisplayableRealMesh::~DisplayableRealMesh (void)
{
  invalidateCachedData();
}


bool
DisplayableRealMesh::useDisplayList (void) const
{
  return bUseDisplayList;
}


void
DisplayableRealMesh::useDisplayList (bool bUseNow)
{
  if (bUseDisplayList && !bUseNow) {
    invalidateDisplayList();
  }

  bUseDisplayList = bUseNow;
}


void
DisplayableRealMesh::invalidateCachedData (void)
{
  for (int iCache = 0; iCache < 2; iCache++) {
    delete cache[iCache].mesh;
    cache[iCache].mesh = NULL;
    cache[iCache].bPerVertex = 0;
    cache[iCache].bStrips = 0;
    cache[iCache].color = RigidScan::colorNone;
    cache[iCache].cbColor = 0;
  }

  invalidateDisplayList();
}


void
DisplayableRealMesh::invalidateDisplayList (void)
{
  if (iDisplayList != 0) {
    cout << "Warning: nuking display list for "
	 << displayName << endl;
	    glDeleteLists(iDisplayList, 1);
	    iDisplayList = 0;
  }
}


void
DisplayableRealMesh::setHome (void)
{
  homePos.setXform (meshData->getXform());
  homePos.new_rotation_center (meshData->worldCenter());
}


void
DisplayableRealMesh::goHome (void)
{
  meshData->setXform (homePos.getXform());
  meshData->new_rotation_center (homePos.worldCenter());
  theScene->computeBBox();
}


void
DisplayableRealMesh::setBlend (bool newBlend, float newAlpha)
{
  bBlend = newBlend;
  alpha = newAlpha;
}


bool
DisplayableRealMesh::transparent (void)
{
  return (theRenderParams->blend && bBlend)
    || (theRenderParams->colorMode == registrationColor);
}


void
DisplayableRealMesh::setTexture(Ref<TextureObj> newtexture)
{
  myTexture = newtexture;  // Ain't reference counting great!
}


void
DisplayableRealMesh::drawSelf (bool bAllowList)
{
  if (!getVisible())
    return;

  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  meshData->gl_xform();
  glMatrixMode (GL_TEXTURE);
  glPushMatrix();
  meshData->gl_xform();

  // get clipping filter, if we're using bbox acceleration
  if (theRenderParams->accelerateWithBbox)
    mBounds = new ScreenBox (NULL,
			     0, Togl_Width(toglCurrent) - 1,
			     0, Togl_Height(toglCurrent) - 1);
  // but if the whole thing is onscreen, testing the fragment bboxes
  // is a waste of time
  if (mBounds && mBounds->acceptFully (meshData->localBbox())) {
    //cout << displayName << ": whole scan onscreen; no clip test necessary"
    // << endl;
    delete mBounds;
    mBounds = NULL;
  }

  if (!mBounds || mBounds->accept (meshData->localBbox())) {

    bManipulating = isManipulatingRender();
    if (transparent()) {
      glEnable (GL_BLEND);
      glDepthMask (GL_FALSE);
      glBlendFunc (GL_SRC_ALPHA, GL_DST_ALPHA);
    }

    if (bAllowList)
      drawList();
    else
      drawImmediate();

    if (theScene->wantMeshBBox (this))
      drawBoundingBox();

    if (transparent()) {
      glDisable(GL_BLEND);
      glDepthMask (GL_TRUE);
    }

  } else {
    cerr << displayName << ": skipping entire mesh" << endl;
  }

  glMatrixMode (GL_TEXTURE);
  glPopMatrix();
  glMatrixMode (GL_MODELVIEW);
  glPopMatrix();

  delete mBounds;
  mBounds = NULL;
}


void
DisplayableRealMesh::drawList (void)
{
  if (bUseDisplayList
      && !(bManipulating && theRenderParams->bRenderManipsSkipDlist)) {
    if (iDisplayList > 0) {
      glCallList (iDisplayList);
    } else {
      cout << "Warning: building new display list for mesh "
	   << meshData->get_name() << endl;

      iDisplayList = glGenLists (1);
      glNewList(iDisplayList, GL_COMPILE);
      drawImmediate();
      glEndList();
      glCallList (iDisplayList);
    }
  } else {
    drawImmediate();
  }
}


// hidden line rendering -- works by rendering black polygons shifted
// just slightly back in z, to fill the z buffer with the necessary values
// for depth testing, then rendering visible lines -- which only win the
// depth test if they should be visible (in case of a tie, they should win
// due to the offset given the polygons).

// This is carried out with glPolygonOffset, but it's a bit of a pain
// because I can't get the OGL-1.1 specified call to work on SGI's --
// whereas an older EXT flavor works fine.  On WIN32, at least with the
// AccelGraphics card, glPolygonOffset with the same parameters works fine.

// here, for nobody's benefit, but since it took me a while to figure this
// out, is a table of what works where:
// glPolygonOffset: (supp'd) (works); glPolygonOffsetEXT: (supp'd) (works)
// Lambert: (RE2, OGL1.0)  no,no                           yes,yes
// Aegean: (O2, OGL1.0)    yes,no                          yes,yes
// Radiance: (IR, OGL1.0)  yes,no                          yes,no
// Maglio: (IR, OGL1.1)    yes,???                         yes,yes
// Wavelet: (RE2, OGL1.1)  yes,yes                         yes,yes
// Shape: (GLINT, OGL1.1)  yes,yes                         yes*,?
//             * glGetString says it's there, but no way to link to it
// This applies to the display, not the computational host -- so if you
// run plyview on radiance using lambert's display, it works ok.

// The problem appears to be that lambert, radiance, and aegean are using
// pre-IRIX 6.5 OS's that implement OGL 1.0.  On IRIX 6.3 and 6.4,
// glPolygonOffset is supported as a no-op; on IRIX 6.2 it fails and exits
// the program.  Hopefully once everyone is running IRIX 6.5 this can be
// revisited.



void
DisplayableRealMesh::drawImmediate (void)
{
  if (theRenderParams->hiddenLine && !bManipulating) {
    //valid for lines and points... don't use this for polys
    GLenum realMode = theRenderParams->polyMode;
    GLenum ofsMode;
    if (g_glVersion >= 1.1) {
      glEnable (ofsMode = GL_POLYGON_OFFSET_FILL);
      glPolygonOffset (1.0, 1.0);
    } else {
#ifdef sgi
      glEnable (ofsMode = GL_POLYGON_OFFSET_EXT);
      glPolygonOffsetEXT (1.0, 1.0e-6);
#endif
    }

    //first render polys to fill depth buffer
    theRenderParams->polyMode = GL_FILL;
    drawImmediateOnce();

    //then render the visible lines/points
    theRenderParams->polyMode = realMode;
    glDisable (ofsMode);
  }

  drawImmediateOnce();
}


void
DisplayableRealMesh::drawImmediateOnce (void)
{
  if (meshData->num_resolutions() == 0)
    return;         //nothing to do

  int cbColor;
  RigidScan::ColorSource color;
  setMaterials (cbColor, color);

  bool perVertex = theRenderParams->shadeModel != realPerFace;
  bool strips = theRenderParams->useTstrips && perVertex;

  getMeshTransport (perVertex, strips, color, cbColor);
  renderMeshTransport();

  if (!theRenderParams->shadows) {
    // A bit of cleanup...
    glDisable(GL_TEXTURE_2D);
  }
}


void
DisplayableRealMesh::getMeshTransport (bool perVertex, bool strips,
				   ColorSource color, int cbColor)
{
  bool bLores = bManipulating && theRenderParams->bRenderManipsLores;
  DrawData& cache = bLores ? this->cache[1] : this->cache[0];

  if (perVertex != cache.bPerVertex
      || strips != cache.bStrips
      || color != cache.color
      || cbColor != cache.cbColor) {
    delete cache.mesh;
    cache.mesh = NULL;
  }

  if (!cache.mesh) {
    int iOldRes = 0;
    if (bLores) {
      iOldRes = meshData->current_resolution().abs_resolution;
      meshData->select_coarsest();
    }
    // RigidScan* meshData
    cache.mesh = meshData->mesh (perVertex, strips, color, cbColor);
    if (bLores)
      meshData->select_by_count (iOldRes);

    cache.bPerVertex = perVertex;
    cache.bStrips = strips;
    cache.color = color;
    cache.cbColor = cbColor;
    buildStripInds (cache);
  }
}



void
DisplayableRealMesh::renderMeshTransport (void)
{
  bool bLores = bManipulating && theRenderParams->bRenderManipsLores;
  DrawData& cache = bLores ? this->cache[1] : this->cache[0];

  if (!cache.mesh) {
    if (!meshData->render_self (cache.color))
      cerr << displayName << ": cannot render!" << endl;
    return;
  }

  if (cache.bPerVertex) {
    renderMeshArrays();
  } else {
    renderMeshSingle();
  }
}


void
DisplayableRealMesh::renderMeshArrays (void)
{
  BailDetector bail;

  bool bPointsOnly = (theRenderParams->polyMode == GL_POINT);
  bool bGeometryOnly = false;
  bool bLores = bManipulating && theRenderParams->bRenderManipsLores;
  DrawData& cache = bLores ? this->cache[1] : this->cache[0];

  if (!bUseDisplayList || theRenderParams->bRenderManipsSkipDlist) {
    // don't stick the points-only view in a dlist!
    if (bManipulating) {
      if (theRenderParams->bRenderManipsPoints) {
	// draw points-only view for speed
	bPointsOnly = true;

	if (theRenderParams->bRenderManipsTinyPoints) {
	  glPointSize (1.0);
	} else {
	  glPointSize (2.0);
	}
      }

      if (theRenderParams->bRenderManipsUnlit) {
	glDisable (GL_LIGHTING);
	bGeometryOnly = true;
      }
    }
  }

  // hidden-line back pass doesn't need anything but geometry
  if (theRenderParams->hiddenLine &&
      (theRenderParams->polyMode == GL_FILL))
    bGeometryOnly = true;

  bool bWantNormals = theRenderParams->light && !bGeometryOnly;
  bool bWantColor = (cache.mesh->color.size() == cache.mesh->vtx.size()
		     && !bGeometryOnly);

  if (bWantColor)
    glEnable (GL_COLOR_MATERIAL);

  // set up client vertex-pointer state with relevant pointers
  if (g_glVersion >= 1.1) {
    // vertex arrays -- only supported under OpenGL 1.1 (Irix 6.5)
    glEnableClientState (GL_VERTEX_ARRAY);
    if (bWantNormals)
      glEnableClientState (GL_NORMAL_ARRAY);

    for (int imesh = 0; imesh < cache.mesh->vtx.size(); imesh++) {
      if (cache.mesh->bbox[imesh].valid()) {
	Bbox bbox = cache.mesh->bbox[imesh].worldBox (cache.mesh->xf[imesh]);
	if (mBounds != NULL && !mBounds->accept (bbox)) {
	  //cerr << displayName << ": skipping fragment " << imesh << endl;
	  continue;
	}
      }

// STL Update
      glVertexPointer (3, GL_FLOAT, 0, &*(cache.mesh->vtx[imesh]->begin()));

      glMatrixMode (GL_MODELVIEW);
      glPushMatrix();
      glMultMatrixf (cache.mesh->xf[imesh]);
      // we should do this but it overflows the texture matrix stack...
      // TODO: make our own internalPushMatrix function
      //glMatrixMode (GL_TEXTURE);
      //glPushMatrix();
      //glMultMatrixf (cache.mesh->xf[imesh]);

      if (bWantNormals)
// STL Update
	glNormalPointer (MeshTransport::normal_type, 0,
			 &*(cache.mesh->nrm[imesh]->begin()));

      // the second test below avoids using color arrays of size 1, even
      // if there is only 1 vertex -- this tends to hang GL on maglio.
      // The "color the whole fragment that color without using
      // glColorPointer" case takes care of it anyway, without hanging.
      bool bThisWantColor = bWantColor
	&& (cache.mesh->color[imesh]->size() > 4)
	&& (cache.mesh->color[imesh]->size()
	    == 4 * cache.mesh->vtx[imesh]->size());

      if (bThisWantColor) {
	// only enable vertex-array color if array is expected size.
	glEnableClientState (GL_COLOR_ARRAY);
// STL Update
	glColorPointer (4, GL_UNSIGNED_BYTE, 0,
			&*(cache.mesh->color[imesh]->begin()));
      } else {
	// don't have a full color array...
	glDisableClientState (GL_COLOR_ARRAY);
	// but we might have a per-fragment color for all these vertices.
	if (bWantColor && cache.mesh->color[imesh]->size() == 4) {
// STL Update
	  glColor4ubv (&*(cache.mesh->color[imesh]->begin()));
	}
      }

      if (bPointsOnly) {
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
#if 1
	// this is obviously slower than hell for small count.  But maglio
	// appears to have a bug such that the second form times out and
	// hangs for a huge vtx list, if lighting is enabled.  Turn off
	// lighting or glDisableClientState (GL_NORMAL_ARRAY) and it's ok.
	// And if we render the same data 1 at a time, no prob.
	// In fact, it works for count = 1M, but not 1.5M. ???

	// one more note on this: a dejanews search for "GL:Wait Rdata"
	// turns up exactly one pair of hits, in which someone else
	// complains of a similar situation on an Onyx2 IR irix 6.5.1, and
	// an sgi rep acknowledges it sounds like an sgi bug.

	// with any significant value for count, this shouldn't be any
	// slower anyway so I'll leave it enabled.

	// 1M vertices per pass works fine for non-colored renderings
	// but crashes for color sometimes, so
	// we'll use 400K.

	// While we're documenting the follies of the IR, when color is
	// enabled, maglio consistently hits the same GL:Wait Rdata timeout
	// for vertex arrays of size 1.

	int total = cache.mesh->vtx[imesh]->size();
	int count = 400000;
	if (bThisWantColor ) {
	  while ((total % count) == 1)  // to avoid yet another hang
	    --count;
	}
	glDrawArrays (GL_POINTS, 0, total % count);
	for (int i = total%count; i < total; i += count) {
	  glDrawArrays (GL_POINTS, i, count);
	}
#else
	glDrawArrays (GL_POINTS, 0, cache.mesh->vtx[imesh]->size());
#endif
      } else if (cache.bStrips) {
// STL Update
	vector<int>::iterator lenEnd = cache.StripInds[imesh].end();
	vector<int>::const_iterator start = cache.mesh->tri_inds[imesh]->begin();
	for (vector<int>::iterator len = cache.StripInds[imesh].begin();
	     len < lenEnd; len++) {
	  glDrawElements (GL_TRIANGLE_STRIP, *len,
			  GL_UNSIGNED_INT, &*start);
        start += *len + 1;
	}
      } else {
#if 1
	int total = cache.mesh->tri_inds[imesh]->size();
	int count = 600000; // must be divisible by 3
	glDrawElements (GL_TRIANGLES, total%count,
			GL_UNSIGNED_INT, &*(cache.mesh->tri_inds[imesh]->begin()));
	for (int i = total%count; i < total; i += count)
// STL Update
	  glDrawElements (GL_TRIANGLES, count,
			  GL_UNSIGNED_INT,
			  &*(cache.mesh->tri_inds[imesh]->begin() + i));
#else
	glDrawElements (GL_TRIANGLES, cache.mesh->tri_inds[imesh]->size(),
			GL_UNSIGNED_INT, &*(cache.mesh->tri_inds[imesh]->begin()));
#endif
      }

      //glMatrixMode (GL_TEXTURE);
      //glPopMatrix();
      glMatrixMode (GL_MODELVIEW);
      glPopMatrix();

      if (bail())
	break;
    }

    glDisableClientState (GL_VERTEX_ARRAY);
    glDisableClientState (GL_NORMAL_ARRAY);
    glDisableClientState (GL_COLOR_ARRAY);
  } else {
    // OpenGL 1.0 -- no standard vertex arrays
    // but can use gldrawarraysext, glvertexpointerext, etc. on older IRIX
    // if anyone needs this, they need to rewrite it
    cerr << "OpenGL 1.0 not currently supported.  "
	 << "Have fun doing anything but rendering." << endl;
  }

  glDisable (GL_COLOR_MATERIAL);
  glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
}


void
DisplayableRealMesh::renderMeshSingle (void)
{
  // kberg 10 July 2001 - actually draws while manipulating now
  //DrawData& cache = bManipulating ? this->cache[1] : this->cache[0];
  bool bLores = bManipulating && theRenderParams->bRenderManipsLores;
  DrawData& cache = bLores ? this->cache[1] : this->cache[0];

  bool bPointsOnly = (theRenderParams->polyMode == GL_POINT);

  if (!bUseDisplayList || theRenderParams->bRenderManipsSkipDlist) {
    // don't stick the points-only view in a dlist!
    if (bManipulating) {
      if (theRenderParams->bRenderManipsPoints) {
	// draw points-only view for speed
	bPointsOnly = true;

	if (theRenderParams->bRenderManipsTinyPoints) {
	  glPointSize (1.0);
	} else {
	  glPointSize (2.0);
	}
      }

      /* TODO: implement bGeometryOnly in renderMeshSingle*/
      if (theRenderParams->bRenderManipsUnlit) {
	glDisable (GL_LIGHTING);
	//bGeometryOnly = true;
      }
    }
  }

  int frags = cache.mesh->vtx.size();
  const vector<Pnt3>* vtx = NULL;
  const vector<int>* tri = NULL;
  const vector<short>* nrm = NULL;
  const vector<uchar>* color = NULL;

  for (int ifrag = 0; ifrag < frags; ifrag++) {
    vtx = cache.mesh->vtx[ifrag];
    tri = cache.mesh->tri_inds[ifrag];
    int nTris = tri->size() / 3;
#ifndef SCZ_NORMAL_FORCEFLOAT
    if (ifrag < cache.mesh->nrm.size())
      nrm = cache.mesh->nrm[ifrag];
    else
#else
	cerr << "Flat normals not supported in hack-normals-back-to-floats mode" << endl;
#endif
      nrm = NULL;
    if (nrm && nrm->size() != nTris * 3)
      nrm = NULL;
    if (ifrag < cache.mesh->color.size())
      color = cache.mesh->color[ifrag];
    else
      color = NULL;
    if (color && color->size() != nTris * 4)
      color = NULL;

    if (color)
      glEnable (GL_COLOR_MATERIAL);

    if (bPointsOnly)
      glBegin (GL_POINTS);
    else
      glBegin(GL_TRIANGLES);

    for (int it = 0; it < nTris; it++) {
      int it3 = 3*it;
      if (color)
	glColor4ubv (&(*color)[4*it]);
      if (nrm)
	glNormal3sv (&(*nrm)[it3]);

      glVertex3fv (&((*vtx)[(*tri)[it3]])[0]);
      glVertex3fv (&((*vtx)[(*tri)[it3+1]])[0]);
      glVertex3fv (&((*vtx)[(*tri)[it3+2]])[0]);
    }
    glEnd();
    glDisable (GL_COLOR_MATERIAL);
  }
}


void
DisplayableRealMesh::buildStripInds (DrawData& cache)
{
  cache.StripInds.clear();
  if (cache.bStrips && cache.mesh) {
    // store indices of ends of strips

    for (int imesh = 0; imesh < cache.mesh->tri_inds.size(); imesh++) {
      cache.StripInds.push_back (vector<int>());
      vector<int>& si = cache.StripInds.back();
// STL Update
      const vector<int>::const_iterator last = cache.mesh->tri_inds[imesh]->begin() - 1;
      const vector<int>::const_iterator triEnd = cache.mesh->tri_inds[imesh]->end();
      for (const vector<int>::const_iterator i = cache.mesh->tri_inds[imesh]->begin();
	   i < triEnd; i++) {
	if (*i == -1) { // end of strip
	  si.push_back (i - last - 1);
	  last = i;
	}
      }
    }
  }
}


#if 0
#define OFFSETOF(Struct,Field) \
      ((char*)((Struct).Field) - (char*)&(Struct))


void
DisplayableRealMesh::cache_smooth_confidence (const vector<Pnt3>& vtx,
					 const vector<Pnt3>& nrm,
					 const vector<int>& tri_inds,
					 const vector<uchar>& colors,
					 bool stripped)
{
  uchar conf;
  uchar _alpha = alpha * 255;

  const int* triEnd = tri_inds.end();
  freeVtxArray();
  buildStripInds (stripped, tri_inds);

  // optimization for InfiniteReality: (man glVertexPointerEXT)
  // need to store: GLubyte c[4]; GLshort n[3]; GLfloat v[3];
  struct {
    GLubyte c[4];
    NORMAL  n[3];
    GLfloat v[3];
  } vertex;
  cachedVertSize = sizeof (vertex);
  cachedVertCount = vtx.size();
  cache = new char [cachedVertSize * cachedVertCount];
  cachedClrOfs = OFFSETOF (vertex, c);
  cachedNrmOfs = OFFSETOF (vertex, n);
  cachedVtxOfs = OFFSETOF (vertex, v);

  for (int vert = 0; vert < cachedVertCount; vert++) {
    char* basePtr = cache + (vert * cachedVertSize);
    GLubyte* colorPtr = (GLubyte*)(basePtr + cachedClrOfs);
    NORMAL* normPtr =   (NORMAL*) (basePtr + cachedNrmOfs);
    GLfloat* vertPtr =  (GLfloat*)(basePtr + cachedVtxOfs);

    conf = colors[vert] * theRenderParams->confScale;
    conf = MIN(conf, 255) * 0.7;
    colorPtr[0] = 178;
    colorPtr[1] = conf;
    colorPtr[2] = conf;
    colorPtr[3] = _alpha;
    normPtr[0] = nrm[vert][0] * NRM_SCALE;
    normPtr[1] = nrm[vert][1] * NRM_SCALE;
    normPtr[2] = nrm[vert][2] * NRM_SCALE;
    memcpy (vertPtr, &vtx[vert][0], 3 * sizeof(vertPtr[0]));
  }

  cachedType = smooth_conf;
}


void
DisplayableRealMesh::cache_smooth_color (const vector<Pnt3>& vtx,
				    const vector<Pnt3>& nrm,
				    const vector<int>& tri_inds,
				    const vector<uchar>& colors,
				    int colorsize,
				    bool stripped)
{
  uchar _alpha = 255 * alpha;

  freeVtxArray();
  buildStripInds (stripped, tri_inds);

  // need to store: GLubyte c[4]; GLshort n[3]; GLfloat v[3];
  struct {
    GLubyte c[4];
    NORMAL  n[3];
    GLfloat v[3];
  } vertex;
  cachedVertSize = sizeof (vertex);
  cachedVertCount = vtx.size();
  cache = new char [cachedVertSize * cachedVertCount];
  cachedClrOfs = OFFSETOF (vertex, c);
  cachedNrmOfs = OFFSETOF (vertex, n);
  cachedVtxOfs = OFFSETOF (vertex, v);

  for (int vert = 0; vert < cachedVertCount; vert++) {
    char* basePtr = cache + (vert * cachedVertSize);
    GLubyte* colorPtr = (GLubyte*)(basePtr + cachedClrOfs);
    NORMAL* normPtr =   (NORMAL*) (basePtr + cachedNrmOfs);
    GLfloat* vertPtr =  (GLfloat*)(basePtr + cachedVtxOfs);

    if (colorsize == 3) {
      colorPtr[0] = colors[3*vert + 0];
      colorPtr[1] = colors[3*vert + 1];
      colorPtr[2] = colors[3*vert + 2];
    } else {
      colorPtr[0] = colorPtr[1] = colorPtr[2] = colors[vert];
    }
    colorPtr[3] = _alpha;

    normPtr[0] = nrm[vert][0] * NRM_SCALE;
    normPtr[1] = nrm[vert][1] * NRM_SCALE;
    normPtr[2] = nrm[vert][2] * NRM_SCALE;
    memcpy (vertPtr, &vtx[vert][0], 3 * sizeof (vertPtr[0]));
  }

  cachedType = smooth_color;
}


void
DisplayableRealMesh::cache_smooth_solid (const vector<Pnt3>& vtx,
				    const vector<Pnt3>& nrm,
				    const vector<int>& tri_inds,
				    bool stripped)
{
  freeVtxArray();
  buildStripInds (stripped, tri_inds);

  // need to store: GLshort n[3]; GLfloat v[3];
  struct {
    NORMAL  n[3];
    GLfloat v[3];
  } vertex;
  cachedVertSize = sizeof (vertex);
  cachedVertCount = vtx.size();
  cache = new char [cachedVertSize * cachedVertCount];
  cachedNrmOfs = OFFSETOF (vertex, n);
  cachedVtxOfs = OFFSETOF (vertex, v);
  cachedClrOfs = -1;

  for (int vert = 0; vert < cachedVertCount; vert++) {
    char* basePtr = cache + (vert * cachedVertSize);
    NORMAL* normPtr =   (NORMAL*) (basePtr + cachedNrmOfs);
    GLfloat* vertPtr =  (GLfloat*)(basePtr + cachedVtxOfs);

    normPtr[0] = nrm[vert][0] * NRM_SCALE;
    normPtr[1] = nrm[vert][1] * NRM_SCALE;
    normPtr[2] = nrm[vert][2] * NRM_SCALE;
    memcpy (vertPtr, &vtx[vert][0], 3 * sizeof (vertPtr[0]));
  }

  cachedType = smooth_solid;
}


void
DisplayableRealMesh::draw_flat_confidence (const vector<Pnt3>& vtx,
				       const vector<Pnt3>& nrm,
				       const vector<int>& tri_inds,
				       const vector<uchar>& colors)
{
  uchar conf;
  uchar confColor[4];
  confColor[0] = 178;
  confColor[3] = alpha * 255;

  glEnable (GL_COLOR_MATERIAL);
  glBegin (GL_TRIANGLES);

  const int* triEnd = tri_inds.end();
  int iTri = 0;
  for (const int* tri = &tri_inds[0];
       tri < triEnd; tri += 3, iTri++) {

    conf = theRenderParams->confScale*colors[*tri];
    conf = MIN(conf, 255);
    conf *= 0.7;
    confColor[1] = conf;
    confColor[2] = conf;
    glColor4ubv(confColor);

    glNormal3fv(nrm[iTri]);

    glVertex3fv (vtx[tri[0]]);
    glVertex3fv (vtx[tri[1]]);
    glVertex3fv (vtx[tri[2]]);
  }

  glEnd();
  glDisable (GL_COLOR_MATERIAL);
  cachedType = flat_conf;
}


void
DisplayableRealMesh::draw_flat_color (const vector<Pnt3>& vtx,
				  const vector<Pnt3>& nrm,
				  const vector<int>& tri_inds,
				  const vector<uchar>& colors,
				  int colorsize)
{
  glEnable (GL_COLOR_MATERIAL);
  glBegin (GL_TRIANGLES);

  const int* triEnd = tri_inds.end();
  int iTri = 0;
  for (const int* tri = &tri_inds[0];
       tri < triEnd; tri += 3, iTri++) {

    if (colorsize == 3)
      glColor3ubv(&colors[3*iTri]);
    else
      glColor3ub (colors[iTri], colors[iTri], colors[iTri]);

    glNormal3fv(nrm[iTri]);

    glVertex3fv(vtx[tri[0]]);
    glVertex3fv(vtx[tri[1]]);
    glVertex3fv(vtx[tri[2]]);
  }

  glEnd();
  glDisable (GL_COLOR_MATERIAL);
  cachedType = flat_color;
}


void
DisplayableRealMesh::draw_flat_solid (const vector<Pnt3>& vtx,
				  const vector<Pnt3>& nrm,
				  const vector<int>& tri_inds)
{
  glBegin (GL_TRIANGLES);
  const int* triEnd = tri_inds.end();
  int iTri = 0;
  for (const int* tri = &tri_inds[0];
       tri < triEnd; tri += 3, iTri++) {

    glNormal3fv(nrm[iTri]);

    glVertex3fv(vtx[tri[0]]);
    glVertex3fv(vtx[tri[1]]);
    glVertex3fv(vtx[tri[2]]);
  }

  glEnd();
  cachedType = flat_solid;
}
#endif


void glMaterialubv (GLenum face, GLenum pname, const uchar* bp)
{
  // glMaterialubv doesn't exist, this wraps glMaterialfv
  float fp[4] = { bp[0] / 255., bp[1] / 255., bp[2] / 255., bp[3] / 255. };
  glMaterialfv (face, pname, fp);
}


void
DisplayableRealMesh::setMaterials (int& colorSize,
			       RigidScan::ColorSource& colorSource)
{
  static float nocolor[4] =  { 0, 0, 0, 1 };
  static float regThisColor[4] = { 0, 0, 1, .5 };
  static float regYesColor[4] = { 0, 1, 0, .5 };
  static float regNoColor[4] = { 1, 0, 0, .5 };
  static float white[4] = { 1, 1, 1, 1 };
  float diff[4];
  bool bDontReset = false;

  // Reset materials
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, nocolor);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, nocolor);
  glMaterialubv(GL_FRONT, GL_SPECULAR, theRenderParams->specular);
  glMaterialfv(GL_BACK, GL_SPECULAR, nocolor);
  glMaterialf (GL_FRONT, GL_SHININESS,
	       theRenderParams->shininess);
  if (!theRenderParams->shadows) {
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
  }

  // not needed - two sided lighting takes care of back emmisive
  //if (theRenderParams->backFaceEmissive)
  //  glMaterialubv(GL_BACK, GL_EMISSION, theRenderParams->background);

  if (theRenderParams->twoSidedLighting) {
    if (theRenderParams->backfaceMode == lit) {
      glMaterialubv(GL_BACK, GL_AMBIENT_AND_DIFFUSE, theRenderParams->backDiffuse);
      glMaterialubv(GL_BACK, GL_SPECULAR, theRenderParams->backSpecular);
      glMaterialf(GL_BACK, GL_SHININESS, theRenderParams->backShininess);
    }
    else if (theRenderParams->backfaceMode == emissive)
      glMaterialubv(GL_BACK, GL_EMISSION, theRenderParams->backDiffuse);
  }

  // default to solid color unless otherwise overridden
  colorSize = 0;
  colorSource = RigidScan::colorNone;

  if (theRenderParams->hiddenLine
      && theRenderParams->polyMode == GL_FILL)
  {
    // "fill the depth buffer" pass -- render black
    // so reset the only materials that were not set to nocolor above
    glMaterialfv(GL_FRONT, GL_SPECULAR, nocolor);
    glMaterialf (GL_FRONT, GL_SHININESS, 0);
    glColor4fv (nocolor);

    // we need to go through the below switch statement to set the
    // showColor and showConfidence vars, but we don't want to make
    // any more glMaterial/glColor calls...
    bDontReset = true;
  }

  GLenum matMode = theRenderParams->useEmissive ?
    GL_EMISSION : GL_AMBIENT_AND_DIFFUSE;
  if (!bDontReset)
    glMaterialubv(GL_FRONT, matMode, theRenderParams->diffuse);

  switch (theRenderParams->colorMode) {
  case falseColor:
    if (!bDontReset) {
      diff[0] = colorFalse[0]/255.0;
      diff[1] = colorFalse[1]/255.0;
      diff[2] = colorFalse[2]/255.0;
      diff[3] = alpha;

      glMaterialfv (GL_FRONT, matMode, diff);
      glColor4fv (diff);
    }
    break;

  case boundaryColor:
  case confidenceColor:
  case intensityColor:
  case trueColor:
    colorSize = 4;
    switch (theRenderParams->colorMode) {
    case confidenceColor:
      colorSource = RigidScan::colorConf; break;
    case intensityColor:
      colorSource = RigidScan::colorIntensity; break;
    case trueColor:
      colorSource = RigidScan::colorTrue; break;
    case boundaryColor:
      colorSource = RigidScan::colorBoundary; break;
    }

    if (!bDontReset) {
      if (theRenderParams->useEmissive)
	glMaterialfv(GL_FRONT, GL_SPECULAR, nocolor);
      glColorMaterial(GL_FRONT, matMode);
    }
    break;

  case registrationColor:
    if (!bDontReset) {
      if (this == theSelectedScan) {
	glMaterialfv (GL_FRONT, matMode, regThisColor);
      } else {
	assert (theScene->globalReg);
	bool bTrans = atoi (Tcl_GetVar (g_tclInterp, "regColorTransitive",
					TCL_GLOBAL_ONLY));
	if (theScene->globalReg->pairRegistered
	    (this->getMeshData(), theSelectedScan->getMeshData(), bTrans)) {
	  glMaterialfv (GL_FRONT, matMode, regYesColor);
	} else {
	  glMaterialfv (GL_FRONT, matMode, regNoColor);
	}
      }
    }
    break;

  case noColor:
    glColor4ubv (theRenderParams->diffuse);
    break;

  case textureColor:
    if (myTexture) {
      myTexture->setupTexture();
      glMaterialfv(GL_FRONT, matMode, white);
      glColor4fv(white);
      break;
    }
    // else fall through...

  case grayColor:
  default:
    // just set color in case it's not set from lighting
    // material properties are already ok (they have to be, since this
    // is the fallback case and may be invoked without warning if we
    // fail to extract color info requested from the RigidScan)
    if (!bDontReset) {
      diff[0] = diff[1] = diff[2] = 0.7;
      diff[3] = alpha;
      glColor4fv (diff);
    }
    break;
  }

  // set polygon mode, so we can draw points/lines/polys with same
  // series of calls to glBegin/glVertex/glEnd
  if (theRenderParams->polyMode == GL_FILL) {
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
  }
  else if (theRenderParams->polyMode == GL_POINT) {
    glPolygonMode (GL_FRONT_AND_BACK, GL_POINT);
    glPointSize(theRenderParams->pointSize);
  }
  else { // if (theRenderParams->polyMode == GL_LINE)
    glLineWidth(theRenderParams->lineWidth);
    glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
  }
}


void
DisplayableRealMesh::drawBoundingBox (void)
{
  // assumes mesh transformation is in place, and draws bounding box edges
  // in local (mesh) coordinates

  ::drawBoundingBox (meshData->localBbox());
}


void
DisplayableRealMesh::setName (const char* baseName)
{
  char bName[1024];
  strcpy(bName, baseName);

  char* begin = strrchr (bName, '/');
  char tempName[500];

  if (begin == NULL)
    {
      begin=bName;
      strcpy(tempName, bName);
    }
  else
    {
      begin[0]='\0';
      // printf ("suffix:%s\n",begin-3);
      // Check if this is really a sweep, if so name funkyness
      if (strcmp(begin-3,".sd")==0)
	{

	  char* begin2 = strrchr (bName, '/');
	  if (begin2 == NULL)
	    {
	      begin2=bName;
	    }
	  else
	    {
	      begin2++;
	    }
	  // printf ("begin2:%s\n",begin2);
	  *(begin-3)='\0';
	  strcpy(tempName,begin2);
	  strcat(tempName,"__");
	  strcat(tempName,begin+1);

	}
      else
	{
	  //  printf("fail:%s\n",begin+1);
	begin++;
	strcpy(tempName,begin  );
	}
    }


  //printf ("tempname:%s\n",tempName);
  int suffix = 0;
  while (FindMeshDisplayInfo (tempName) != NULL) {
    ++suffix;
    sprintf (tempName + strlen(tempName), "%d", suffix);
  }

  DisplayableMesh::setName (tempName);
}


