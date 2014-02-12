//############################################################
//
// MMScan.h
//
// Jeremy Ginsberg
// $Date: 2001/07/02 17:56:26 $
//
// Stores all information pertaining to a 3DScanners ModelMaker H
// scan "conglomerate"... this contains many small scan fragments,
// each of which can be characterized by a single viewing direction.
//
// Data structures revamped starting today to better encapsulate the
// idea of a collection of scan fragments.
//
//############################################################

#ifndef _MMSCAN_H_
#define _MMSCAN_H_

#include "RigidScan.h"
#include "ResolutionCtrl.h"

class KDindtree;

class MMScan : public RigidScan {

public:

  // -----------------------------------------
  // struct mmStripeInfo (MMScan.mmStripeInfo)
  //    o  per-stripe information
  //    o  the points for each stripe are conglomerated into vector vtx of
  //       a given scan fragment

  struct mmStripeInfo {
    int numPoints;
    int startIdx;                  // index of stripe's first point in vtx
    Pnt3 sensorVec;                // scannning / viewing direction
    unsigned char validMap[40];    // bitmap for placement along stripe
                                   // 1 = valid pt    0 = invalid pt
    char scanDir;                  // 0 = unknown; 1 = normal; 2 = backwards

    mmStripeInfo(void)
      {
	numPoints = 0; startIdx = -1; sensorVec.set(0,0,0); scanDir = 0;
      }
  };

  // -----------------------------------------
  // struct mmResLevel (MMScan.mmResLevel)
  //    o triangle and t-strip information for a particular resolution
  //    o more or less equivalent to the Mesh.cc class

  struct mmResLevel {
    vector<Pnt3>  vtx;
    vector<short> nrm;
    vector<uchar> intensity;
    vector<float> confidence;
    vector<int>   tris;
    vector<int>   tstrips;
    // no more per-face normals
    //    vector<short> triNrm;

    mmResLevel(void)
      {
	tris.clear(); tstrips.clear();
	vtx.clear(); nrm.clear(); intensity.clear(); confidence.clear();
      }
    ~mmResLevel(void)
      {
	tris.clear(); tstrips.clear();
	vtx.clear(); nrm.clear(); intensity.clear(); confidence.clear();
      }
    int num_tris(void)
      {
	return tris.size() / 3;
      }
    int num_vertices(void)
      {
	return vtx.size();
      }
  };

typedef uchar vec3uc[3];

  // -----------------------------------------
  // struct mmScanFrag (MMScan.mmScanFrag)
  //    o  a scan fragment is a series of meshes which can
  //       be represented by a single vifilter_verticesew direction.
  //    o  this is now the most basic level of organization of an MMScan
  //    o  the raw data from the scanner and its connectivity information
  //       are now stored in meshes[0], instead of directly in the scanFrag

  struct mmScanFrag {
    vector<mmStripeInfo> stripes;
    vector<mmResLevel>   meshes;
    vector<int>          indices;      // used for triangulation
    Pnt3    viewDir;
    vec3uc  falseColor;
    bool    isVisible;
    //    vector<char> boundary;   needed only for older confidence calcs

    mmScanFrag(void)
      {
	//stripes.clear(); meshes.clear();
	//viewDir.set(0,0,0); indices.clear();
	mmResLevel mainRes;
	meshes.push_back(mainRes);
	falseColor[0] = falseColor[1] = falseColor[2] = '\0';
	isVisible = true;
      }
    ~mmScanFrag(void)
      {
	//stripes.clear(); meshes.clear(); indices.clear();
      }
    int num_vertices(int level)
      {
	if (meshes.size() == 0) return 0;
	if (!isVisible) return 0;
	return meshes[level].vtx.size();
      }
    int num_stripes(void)
      {
	return stripes.size();
      }
    int num_tris(void)
      {
	if (meshes.size() == 0) return 0;
	if (!isVisible) return 0;
	return meshes[0].tris.size() / 3;
      }
  };

  MMScan(void);
  ~MMScan(void);

  // perVertex: colors and normals for every vertex (not every 3)
  // stripped:  triangle strips instead of triangles
  // color: one of the enum values from RigidScan.h
  // colorsize: # of bytes for color
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  bool read(const crope &fname);
  bool write(const crope &fname);

  crope getInfo(void);
  int num_vertices();
  int num_vertices(int resNum);

  void computeBBox (void);
  void flipNormals (void);

  bool filter_vertices (const VertexFilter& filter, vector<Pnt3>& p);

  void subsample_points(float rate, vector<Pnt3> &p, vector<Pnt3> &n);
  bool closest_point(const Pnt3 &p, const Pnt3 &n,
		     Pnt3 &cp, Pnt3 &cn,
		     float thr = 1e33, bool bdry_ok = 0);

  bool load_resolution (int iRes);
  int create_resolution_absolute(int budget = 0,
				 Decimator dec = decQslim);


  RigidScan* filtered_copy(const VertexFilter& filter);
  bool filter_inplace(const VertexFilter &filter);

  bool is_modified(void) {
    return isDirty_disk;
  }

  // --- NOT INHERITED FROM RIGIDSCAN

  int num_tris(int res = 0);
  int num_stripes();
  int num_scans(void) { return scans.size(); };
  int num_resolutions(void) {
    // assume that each scan has the same number of resolutions
    if (scans.size() == 0) return 0;
    return scans[0].meshes.size();
  };

  int write_xform_ply(int whichRes, char *dir,
		      crope &confLines, bool applyXform = true);

  void flipNormals (int scanNum);

  void absorb_xforms(char *basename);
  void update_res_ctrl();
  const vec3uc& getScanFalseColor(int scanNum)
    {
      if (scanNum >= scans.size()) scanNum = scans.size() - 1;
      if (scanNum < 0) scanNum = 0;
      return scans[scanNum].falseColor;
    }
  bool isScanVisible(int scanNum)
    {
      if (scanNum >= scans.size()) return false;
      if (scanNum < 0) return false;
      return scans[scanNum].isVisible;
    }
  void setScanVisible(int scanNum, int shouldBeVisible)
    {
      if ((scanNum >= scans.size()) || (scanNum < 0)) return;
      scans[scanNum].isVisible = shouldBeVisible;
    }
  void deleteScan(int scanNum)
    {
      if (scanNum < 0) return;
      if (scanNum >= num_scans()) return;

// STL Update
      scans.erase(scans.begin() + scanNum);

      isDirty_mem = isDirty_disk = true;
      computeBBox();
    }


private:

  //--- DATA DIRECTLY FROM CTA FILE

  vector<mmScanFrag> scans;         // scan information

  //--- DERIVED FROM CTA DATA

  // evil global arrays repeating all of the vertices and meshes in the
  // scan fragments... i can't think of any other way to create a kdtree and
  // boundary for the conglomerate, so we keep these around solely for that
  // purpose.

  struct mergedRegData {
    vector<Pnt3>  vtx;
    vector<short> nrm;
    vector<int>   tris;
    vector<char>  boundary;
  };

  vector<mergedRegData> regData;
  vector<KDindtree*>    kdtree;

  bool haveScanDir;

  // dirty bits, for saving files, updating global arrays, etc.
  bool isDirty_mem;
  bool isDirty_disk;

  //--- PRIVATE HELPERS FNS

  bool read_mms(const crope &fname);
  bool read_cta(const crope &fname);
  mmResLevel *currentRes(int scanNum);
  bool isValidPoint(int scanNum, int stripeNum, int colNum);
  void triangulate(mmScanFrag *scan, mmResLevel *res, int subSamp = 1);
  void make_tstrips(mmScanFrag *scan, mmResLevel *res, int subSamp = 1);
  //  void drawSensorVecs(vector<int> &tris);
  //  void calcTriNormals(const vector<int> &tris, vector<short> &triNrm);
  void calcIndices(void);
  void calcScanDir(void);
  void calcConfidence(void);
  mergedRegData& getRegData (void);
  void mark_boundary (mergedRegData& reg);
  KDindtree* get_kdtree(void);
  bool stripeCompare(int strNum, mmScanFrag *scan);
  void setMTColor (MeshTransport* mt, int iScan, bool perVertex,
		   ColorSource source, int colorSize);
};

//typedef MMScan::mmStripeInfo mmStripeInfo;
typedef MMScan::mmResLevel mmResLevel;
typedef MMScan::mmScanFrag mmScanFrag;

#endif
