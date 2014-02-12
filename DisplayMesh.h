//############################################################
// DisplayMesh.h
// Matt Ginzton
// Wed Jul  1 15:08:21 PDT 1998
//
// Rendering code for mesh data
//############################################################


#ifndef _DISPLAYMESH_H_
#define _DISPLAYMESH_H_

#include "RigidScan.h"
#include "TextureObj.h"
#include "defines.h"
#include <vector>


class DisplayableOrganizingMesh;

// abstract base class
class DisplayableMesh
{
 public:
  DisplayableMesh();
  virtual ~DisplayableMesh();

  virtual void     drawSelf (bool bAllowList = true) = 0; // applies transform,
  // sets modes, etc.  Set bAllowList false if you might be rendering with
  // parameters other than those encapsulated in a display list.

  virtual void     drawList() = 0;      // entry points for things that want to
  virtual void     drawImmediate() = 0; // set their own modes/transforms
                                        // and just get the geometry

  // display list accessors
  virtual bool     useDisplayList (void) const = 0;
  virtual void     useDisplayList (bool bUseNow) = 0;
  virtual void     invalidateCachedData (void) = 0;

  // other accessors
  RigidScan*    getMeshData (void) const;
  void          resetMeshData (RigidScan* _meshData);
  const char*   getName (void) const;
  virtual void  setName (const char* baseName);

  bool     getVisible (void);
  void     setVisible (bool bVis = true);

  const vec3uc& getFalseColor (void) const;
  void          setFalseColor (const vec3uc& color);

  virtual void     setHome (void) = 0;
  virtual void     goHome  (void) = 0;

  virtual void     setBlend (bool newBlend, float newAlpha) = 0;
  virtual bool     transparent (void) = 0;

  virtual void     setTexture(Ref<TextureObj> newtexture) = 0;

 protected:

  RigidScan*          meshData;
  bool                bVisible;
  char*               displayName;
  vec3uc              colorFalse;
};


// concrete subclass representing a real mesh

class DisplayableRealMesh: public DisplayableMesh
{
 public:
  DisplayableRealMesh (RigidScan* _meshData, char* nameBase = NULL);
  ~DisplayableRealMesh();

  void     drawSelf (bool bAllowList = true); // applies transform,
  // sets modes, etc.  Set bAllowList false if you might be rendering with
  // parameters other than those encapsulated in a display list.

  void     drawList();       // entry points for things that want to
  void     drawImmediate();  // set their own modes/transforms and just
                             // get the geometry

  // display list accessors
  bool     useDisplayList (void) const;
  void     useDisplayList (bool bUseNow);
  void     invalidateCachedData (void);

  // other accessors
  //RigidScan*  getMeshData (void) const;
  //void        resetMeshData (RigidScan* _meshData);
  //const char* getName (void) const;

  //bool     getVisible (void);
  //void     setVisible (bool bVis = true);

  //const vec3uc& getFalseColor (void) const;
  //void          setFalseColor (const vec3uc& color);

  void     setHome (void);
  void     goHome  (void);

  void     setBlend (bool newBlend, float newAlpha);
  bool     transparent (void);

  void     setTexture(Ref<TextureObj> newtexture);

 private:   // private helpers
  typedef RigidScan::ColorSource ColorSource;

  void     drawBoundingBox();
  void     drawImmediateOnce();
  void     invalidateDisplayList();
  void     setMaterials (int& colorSize, RigidScan::ColorSource& colorSource);
  void     getMeshTransport (bool perVertex, bool strips,
			     ColorSource color, int cbColor);
  void     renderMeshTransport();

  void     renderMeshArrays();
  void     renderMeshSingle();

  void     setName (const char* baseName);

 private:    // private data
  bool       bUseDisplayList;
  int        iDisplayList;

  bool       bBlend;
  float      alpha;

  Ref<TextureObj> myTexture;

  TbObj      homePos;

  // stuff that's set each time through drawSelf
  class ScreenBox* mBounds;
  bool       bManipulating;

  // cached mesh data returned from meshData->mesh()
  class DrawData
    {
    public:
      bool           bPerVertex;
      bool           bStrips;
      ColorSource    color;
      int            cbColor;
      MeshTransport* mesh;
      vector<vector<int> > StripInds;
    };

  DrawData   cache[2]; // current res, lo res preview

  void     buildStripInds(DrawData& cache);
};


// safe access to mesh data from a possibly null DisplayableMesh
RigidScan* MeshData (DisplayableMesh* disp);


#endif //_DISPLAYMESH_H_
