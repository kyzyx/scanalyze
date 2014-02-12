//############################################################
//
// GenericScan.h
//
// Kari Pulli
// Mon Jun 29 11:51:05 PDT 1998
//
// Store range scan information from a generic range scanner
// that gives range maps.
// Not much if anything is known of the scanner properties,
// so orthographics scan geometry is assumed.
//
//############################################################

#ifndef _GENERICSCAN_H_
#define _GENERICSCAN_H_

#include "RigidScan.h"
#include "Mesh.h"

class KDindtree;
class RangeGrid;

class GenericScan : public RigidScan {
private:

  vector<Mesh*> meshes;
  vector<KDindtree *> kdtree;
  bool bDirty;
  bool bNameSet;

  RangeGrid* myRangeGrid;

  KDindtree* get_current_kdtree(void);

  void insertMesh(Mesh *m, const crope& filename,
		  bool bLoaded = true, bool bAlwaysLoad = true,
		  int nRes = 0);
  Mesh* currentMesh (void);
  Mesh* getMesh (int level);
  inline Mesh* highestRes (void);
  bool readSet (const crope& fn);
  bool readSingleFile (const crope& fn);

  void _Init();

public:

      // default constructor:
  GenericScan(void);
      // wrapper constructor to display a single mesh:
  GenericScan(Mesh* mesh, const crope& name);
  ~GenericScan(void);

  // perVertex: colors and normals for every vertex (not every 3)
  // stripped:  triangle strips instead of triangles
  // color: one of the enum values from RigidScan.h
  // colorsize: # of bytes for color
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  int  num_vertices(void);
  void subsample_points(float rate, vector<Pnt3> &p,
			vector<Pnt3> &n);

  void PrintVoxelInfo();  // added display voxel feature.  See
  // GenericScan.cc for documentation - leslie

  RigidScan* filtered_copy(const VertexFilter& filter);
  virtual bool filter_inplace(const VertexFilter &filter);
  virtual bool filter_vertices (const VertexFilter& filter, vector<Pnt3>& p);

  bool closest_point(const Pnt3 &p, const Pnt3 &n,
		     Pnt3 &cp, Pnt3 &cn,
		     float thr = 1e33, bool bdry_ok = 0);
  void computeBBox();
  void flipNormals();
  crope getInfo (void);

  // smoothing
  void dequantizationSmoothing(int iterations, double maxDisplacement);
  void commitSmoothingChanges();

  // file I/O methods
  bool read(const crope &fname);

  // is data worth saving?
  virtual bool is_modified (void);
  // save to given name: if default, save to existing name if there is
  // one, or return false if there's not
  virtual bool write(const crope& fname = crope());
  // for saving individual meshes
  virtual bool write_resolution_mesh (int npolys,
				      const crope& fname = crope(),
				      Xform<float> xfBy = Xform<float>());
  // for saving anything else
  virtual bool write_metadata (MetaData data);


  // ResolutionCtrl methods
  int create_resolution_absolute (int budget = 0, Decimator dec = decQslim);
  bool delete_resolution (int abs_res);

  bool load_resolution (int i);
  bool release_resolution (int nPolys);

 private:
  // file i/o helpers
  void setd (const crope& dir = crope(), bool bCreate = false);
  void pushd();
  void popd();

  crope setdir;
  crope pusheddir;
  int pushcount;

  Mesh* readMeshFile (const char* name);
  bool getXformFilename (const char* meshName, char* xfName);

  // color helper
  void setMTColor (Mesh* mesh, MeshTransport* mt,
		   bool perVertex, ColorSource source, int colorsize);
};

#endif /* _GENERICSCAN_H_ */
