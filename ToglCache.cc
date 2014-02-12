// ToglCache.cc              cache togl frame buffer for fast redraws
// created 10/27/98          magi


#include <assert.h>
#include <iostream.h>
#include "togl.h"
#include "ToglCache.h"
#include "plvGlobals.h"
#include "plvDraw.h"
#include "Trackball.h"



DisplayCache::DisplayCache (struct Togl* togl)
{
  mTogl = togl;
  mData = NULL;
  mZData = NULL;
  mValid = false;
  mX = mY = 0;
  mEnabled = true;

  // save callbacks for chaining
  mOldDestroyProc = Togl_GetDestroyFunc (togl);
  mOldReshapeProc = Togl_GetReshapeFunc (togl);
  mOldDisplayProc = Togl_GetDisplayFunc (togl);

  // then replace them
  Togl_SetDestroyFunc (togl, DC_ToglDestroy);
  Togl_SetReshapeFunc (togl, DC_ToglReshape);
  Togl_SetDisplayFunc (togl, DC_ToglDisplay);

  AddToglDC (togl, this);
  Allocate();
}


DisplayCache::~DisplayCache (void)
{
  Free();
}


bool
DisplayCache::Render (void)
{
  if (!mEnabled)
    return false;

  // write from our cache to frame buffer
  if (!mValid) {
#if TOGLCACHE_DEBUG
    cout << "can't render from cache" << endl;
#endif
    return false;
  }

  glDrawBuffer (GL_FRONT);
  while (glGetError() != GL_NO_ERROR)
    ; // clear error state since we don't care if this fails

  glMatrixMode (GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glDisable (GL_DEPTH_TEST);
  glRasterPos2f (-1, -1);
  glDepthMask(GL_FALSE);
  glDrawPixels (mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, mData);
  glRasterPos2f (-1, -1);
  glDepthMask(GL_TRUE);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDrawPixels (mX, mY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, mZData);
  if (glGetError() != GL_NO_ERROR) {
    cout << "ERROR in glDrawPixels!" << endl;
  }
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glPopMatrix();
  glMatrixMode (GL_PROJECTION);
  glPopMatrix();
  glMatrixMode (GL_MODELVIEW);
  glEnable (GL_DEPTH_TEST);

  glDrawBuffer (GL_BACK);

#if TOGLCACHE_DEBUG
  cout << "mX, mY = " << mX << "," << mY << endl;
  cout << "render from cache" << endl;
#endif
  return true;
}


bool
DisplayCache::Update (void)
{
  mValid = false;
  if (mEnabled) {
    // read cache from frame buffer
    glReadBuffer (GL_BACK);
    while (glGetError() != GL_NO_ERROR)
      ; // clear error state since we don't care if this fails

    glReadPixels (0, 0, mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, mData);
    glReadPixels (0, 0, mX, mY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, mZData);
    if (!glGetError()) {
#if TOGLCACHE_DEBUG
      cout << "creating cache" << endl;
#endif
      mValid = true;
    } // buffer read succeeded
  } // enabled

  return mValid;
}


bool
DisplayCache::Invalidate (void)
{
  mValid = false;
  return true;
}


void
DisplayCache::Enable (bool bEnable)
{
  mEnabled = bEnable;
}


bool
DisplayCache::Allocate (void)
{
  Free();

#if TOGLCACHE_DEBUG
  cout << "Reallocating cache" << endl;
#endif

  mValid = false;
  mX = Togl_Width (mTogl);
  mY = Togl_Height (mTogl);
  mData = new char [4 * mX * mY];
  mZData = new char [sizeof(unsigned int) * mX * mY];
  return true;
}


bool
DisplayCache::Free (void)
{
  if (mData) {
    delete[] mData;
    mData = NULL;
    delete[] mZData;
    mZData = NULL;
    mX = mY = 0;
    mValid = false;
  }

  return true;
}


void
DisplayCache::InvalidateToglCache (struct Togl* togl)
{
  DisplayCache* dc = FindToglDC (togl);
  if (dc != NULL)
    dc->Invalidate();
}


void
DisplayCache::UpdateToglCache (struct Togl* togl)
{
  DisplayCache* dc = FindToglDC (togl);
  if (dc != NULL)
    dc->Update();
}


void
DisplayCache::EnableToglCache (struct Togl* togl, bool bEnable)
{
  DisplayCache* dc = FindToglDC (togl);
  assert (dc);

  dc->Enable (bEnable);
}


void
DisplayCache::DC_ToglDestroy (struct Togl* togl)
{
  DisplayCache* dc = FindToglDC (togl);
  assert (dc);

  if (dc->mOldDestroyProc)
    dc->mOldDestroyProc (togl);
  delete dc;
}


void
DisplayCache::DC_ToglReshape (struct Togl* togl)
{
  DisplayCache* dc = FindToglDC (togl);
  assert (dc);

  dc->Allocate();
  if (dc->mOldReshapeProc)
    dc->mOldReshapeProc (togl);
}


void
DisplayCache::DC_ToglDisplay (struct Togl* togl)
{
  DisplayCache* dc = FindToglDC (togl);
  assert (dc);

  if (!dc->Render()) {
    dc->mOldDisplayProc (togl);
  }
}


#include <hash_map.h>
struct eqtoglptr
{
  bool operator()(unsigned long t1, unsigned long t2) const
  {
    return (t1 == t2);
  }
};

typedef hash_map<unsigned long, DisplayCache*, hash<unsigned long>,
  eqtoglptr> DCToglHash;

static DCToglHash dcHash;


void
DisplayCache::AddToglDC (struct Togl* togl, DisplayCache* dc)
{
  unsigned long hash = (unsigned long)togl;
  dcHash[hash] = dc;
}


DisplayCache*
DisplayCache::FindToglDC (struct Togl* togl)
{
  unsigned long hash = (unsigned long)togl;
  return dcHash[hash];
}
