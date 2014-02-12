// GroupScan.h                 RigidScan aggregation
// created 11/26/98            Matt Ginzton (magi@cs)


#ifndef _GROUPSCAN_H_
#define _GROUPSCAN_H_


#include "RigidScan.h"
#include "BailDetector.h"
#include "plvScene.h"
#include "GroupUI.h"
#include "VertexFilter.h"
class DisplayableMesh;


class GroupScan: public RigidScan
{
 public:
  // GroupScan manipulation
  GroupScan();
  GroupScan(const vector<DisplayableMesh*>& members, bool dirty);

  ~GroupScan();

  bool AddScan (DisplayableMesh* scan);
  bool RemoveScan (DisplayableMesh* scan);

 public:
  // RigidScan methods:

  // Display
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  virtual void computeBBox (void);
  virtual crope getInfo(void);

  ////////////////////////////////////////////////////////////////
  // Aggregation
  ////////////////////////////////////////////////////////////////
  virtual bool get_children (vector<RigidScan*>& children) const;

  ////////////////////////////////////////////////////////////////
  // ICP
  ////////////////////////////////////////////////////////////////
  virtual void subsample_points(float rate, vector<Pnt3> &p,
				vector<Pnt3> &n);
  virtual bool
  closest_point(const Pnt3 &p, const Pnt3 &n,
		Pnt3 &cl_pnt, Pnt3 &cl_nrm,
		float thr = 1e33, bool bdry_ok = 0);

  // need to support:
  // read, write
  // vertex filters: are a royal pain, because they implicitly have a
  //    transform built in, which would need to be diff. for each child
  // etc.

  bool write_metadata (MetaData data);
  virtual bool is_modified (void);
  virtual bool write(const crope &fname);

  bool get_children_for_display (vector<DisplayableMesh*>& children) const;
  virtual RigidScan* filtered_copy (const VertexFilter &filter);
  virtual bool filter_inplace      (const VertexFilter &filter);

  virtual bool load_resolution (int iRes);
  virtual bool release_resolution (int nPolys);

 protected:
  virtual bool switchToResLevel (int iRes);

 private:
  vector<DisplayableMesh*> children;
  void rebuildResolutions (void);
  bool bDirty;
};


#endif // _GROUPSCAN_H_
