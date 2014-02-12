//############################################################
//
// CyberScan.h
//
// Kari Pulli
// Wed Jul  8 13:15:25 PDT 1998
//
// Store range scan information from a Cyberware custom made
// range scanner for the Digital Michelangelo project.
//
//############################################################

#ifndef _CyberSCAN_H_
#define _CyberSCAN_H_

#include <string>
#include "RigidScan.h"
//#include "CyberXform.h"
#include "DrawObj.h"
#include "SDfile.h"

class KDindtree;

class CyberSweep;

// this data used only for registration
struct regLevelData
{
  vector<Pnt3>  *pnts;
  vector<short> *nrms;
  vector<char>  *bdry;
  bool bFree;

  regLevelData()
    { pnts = NULL; nrms = NULL; bdry = NULL; bFree = false; }
  ~regLevelData()
    { if (bFree) { delete pnts; delete nrms; delete bdry; } }
};


// carving spheres...
struct carveStackEntry {
  Pnt3  ctr;
  float radius;
  vector<CyberSweep*> sweeps;

  carveStackEntry(const Pnt3 &c, float r, const vector<CyberSweep*> &sw)
    : ctr(c), radius(r)
    {
      for (int i=0; i<sw.size(); i++) {
	sweeps.push_back(sw[i]);
      }
    }

  carveStackEntry(const Pnt3 &c, float r)
    : ctr(c), radius(r)
    {}

  bool can_be_child(const Pnt3 &c, float r)
    {
      return dist(c,ctr) + r <= radius;
    }
};


class CyberScan : public RigidScan {
private:

  // the raw data
  vector<CyberSweep*> sweeps;
  bool bDirty;

  // for registration
  vector<KDindtree*> kdtree;
  vector<regLevelData*> reglevels;

  KDindtree*     get_current_kdtree (void);
  regLevelData*  getHighestRegLevel (void);
  regLevelData*  getCurrentRegLevel (void);
  regLevelData*  getRegLevelFor (int iRes);

  void read_postprocess(const char *filename);

  // for space carving
  vector<carveStackEntry> carveStack;

public:

  CyberScan(void);
  ~CyberScan(void);

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

  RigidScan* filtered_copy (const VertexFilter &filter);
  RigidScan* get_piece (int sweepNumber, int frameStart, int frameFinish);

  bool filter_inplace      (const VertexFilter &filter);
  bool filter_vertices     (const VertexFilter &filter,
			    vector<Pnt3>& p);

  bool closest_point(const Pnt3 &p, const Pnt3 &n,
		     Pnt3 &cp, Pnt3 &cn,
		     float thr = 1e33, bool bdry_ok = 0);

  void computeBBox();
  void flipNormals();
  virtual crope getInfo(void);

  unsigned int get_scanner_config(void);
  float get_scanner_vertical(void);

  // These two methods are for jedavis's image stuff...
  bool worldCoordToSweepCoord(const Pnt3 &wc, int *sweepIndex, Pnt3 &sc);
  void sweepCoordToWorldCoord(int sweepIndex, const Pnt3 &sc, Pnt3 &wc);

  bool read(const crope &fname);
  bool write(const crope &fname);
  bool is_modified (void);

  bool load_resolution (int iRes);
  int  create_resolution_absolute(int budget = 0,
				  Decimator dec = decQslim);
  virtual bool release_resolution(int nPolys);

  // for auto-aligning
  vector< vector<CyberSweep*> > get_ordered_sweeps (void);
  vector<CyberSweep*> get_sweep_list (void);
  void dump_pts_laser_subsampled(std::string fname,
				 int nPnts);
  bool get_raw_data(const Pnt3 &p, sd_raw_pnt &data);
  // for volumetric processing
  virtual float
  closest_point_on_mesh(const Pnt3 &p, Pnt3 &cl_pnt, OccSt &status_p);
  virtual float
  closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
			      OccSt &status_p);
  virtual OccSt carve_cube  (const Pnt3 &ctr, float side);

protected:
  virtual bool switchToResLevel (int iRes);
};


// this stores the actual rendering (and registration) data
struct levelData {
  vector<Pnt3>   pnts;
  vector<short>  nrms;
  vector<int>    tstrips;
  vector<uchar>  intensity;
  vector<char>   bdry; // 1 for boundary vtx (for registration)

  // only important for subsampled levels:
  // ratio of samples that were valid.
  vector<uchar>  confidence;

  // this is a mapping from the indices of the sampled
  // data to the unsampled (raw) data
  // for level 0 this vector is empty (the mapping is identity)
  vector<int>    map_sampled_to_unsampled;
};


// This class contains a set of scanlines for a single
// other_screw value (turn if vertical scan, nod if horizontal).
class CyberSweep : public RigidScan, public DrawObj {
  friend class CyberScan;

private: // mesh data for rendering
  vector<levelData*> levels;
  uchar              falseColor[3];

private:

  SDfile sd;

  // implemented but stubbed out; CyberScan does the work
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  void init_leveldata(void);
  void insert_possible_resolutions(void);

  void simulateFaceNormals(vector<short> &faceNormals, int currentRes);

public:
  CyberSweep(void);
  ~CyberSweep(void);

  void computeBBox();
  void flipNormals();

  bool read (const crope &fname);
  bool write (const crope &fname);

  bool load_resolution (int iRes);

  vector<Pnt3> start, end;
  void drawthis(void);

  // stuff we have to support for self-alignment among sweeps
private:
  vector<KDindtree*> kdtree;
  KDindtree*         get_current_kdtree (void);

public:
  void subsample_points(float rate, vector<Pnt3> &p,
			vector<Pnt3> &n);

  RigidScan *filtered_copy(const VertexFilter &filter);

  RigidScan *get_piece(int firstFrame, int lastFrame);

  bool closest_point(const Pnt3 &p, const Pnt3 &n,
		     Pnt3 &cp, Pnt3 &cn,
		     float thr = 1e33, bool bdry_ok = 0);

  int num_frames(void)  { return sd.num_frames(); }

  Xform<float> vrip_reorientation_frame()
    { return sd.vrip_reorientation_frame(); }

  // debugging accessor
  crope get_description (void) const;

  virtual float
  closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
			      OccSt &status_p);
  OccSt carve_sphere(const Pnt3 &ctr, float r);
};

#endif /* _CyberSCAN_H_ */
