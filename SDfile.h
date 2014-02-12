//############################################################
//
// SDfile.h
//
// Kari Pulli
// Tue Feb 23 13:09:27 CET 1999
//
// Read, write, and store the raw data of an *.sd file
//
//############################################################

#ifndef __SDFILE_H__
#define __SDFILE_H__

#include <ext/rope>
#include <fstream>
#include "CyberXform.h"
#include "Bbox.h"
using namespace __gnu_cxx;
class VertexFilter;
typedef unsigned char uchar;

struct sd_raw_pnt {
  unsigned int config;
  short        y,z;
  float        nod, turn, tr_h;
};

class SDfile {

private:
  unsigned int    version;
  unsigned int    header_size;
public:
  float           other_screw;
  float           scanner_trans;
  float           scanner_vert;
  unsigned int    scanner_config;
private:
  float           laser_intensity;
  float           camera_sensitivity;

public:
  CyberXform      xf;
private:
  float           frame_pitch;// screw pitch between frames
  float           scan_screw; // "center" scan screw value
  unsigned int    n_pts;      // size of z_data and intensity_data
  unsigned int    n_valid_pts;
  unsigned int    pts_per_frame;
  unsigned int    n_frames;   // number of frames
  unsigned int   *row_start;  // index to z_data for start of row
  unsigned short *first_good; // the first good column of row
  unsigned short *first_bad;  // the first missing column of row
  unsigned short *z_data;     // the raw z data
  unsigned char  *intensity_data;

  // preprocessing for space carving
  bool  axis_proj_done;
  float axis_proj_min, axis_proj_max;

  void  prepare_axis_proj(void);

  void make_tstrip_horizontal(vector<int>  &tstrips,
			      vector<char> &bdry);
  void make_tstrip_vertical(vector<int>  &tstrips,
			    vector<char> &bdry);

  enum SubSampMode { fast, conf, holesGrow, filterAvg };

  void fill_index_array(int  step,
			int  row,
			SubSampMode subSampMode,
			vector<int> &p,
			vector<Pnt3>  &pnts,
			vector<int>   &ssmap,
			vector<uchar> &intensity,
			vector<uchar> &confidence);

  bool find_row_range(float screw, float radius, const Pnt3 &ctr,
		      int rowrange[2]);

public:
  SDfile(void);
  ~SDfile(void);

  bool read (const crope &fname);
  bool write (const crope &fname);

  void fill_holes(int max_missing, int thresh);

  int  valid_pts(void) { return n_valid_pts; }

  void make_tstrip(vector<int>  &tstrips,
		   vector<char> &bdry);

  void subsampled_tstrip(int            step,
			 char          *behavior,
			 vector<int>   &tstrips,
			 vector<char>  &bdry,
			 vector<Pnt3>  &pnts,
			 vector<uchar> &intensity,
			 vector<uchar> &confidence,
			 vector<int>   &ssmap);

  void set_xf(int row, bool both = true);

  void get_pnts_and_intensities(vector<Pnt3>  &pnts,
				vector<uchar> &intensity);

  // access a data point, transformed or in laser coords
  // row is the frame, col is a point on scanline,
  // col even: frame 0; col odd: frame 1
  // returns false if there is no such point
  bool get_point(int r, int c, Pnt3 &p, bool transformed = true);

  Xform<float> vrip_reorientation_frame(void);

  bool find_data(float screw, float y_in,
		 int &row,unsigned short &y,
		 unsigned short &z);

  int  count_valid_pnts(void);

  void filtered_copy(const VertexFilter &filter,
		     SDfile &sdnew);
  void get_piece(int firstFrame, int lastFrame,
		 SDfile &sdnew);

  int  num_frames(void) { return n_frames; }
  Bbox get_bbox(void);

  // for space carving
  int sphere_status(const Pnt3 &ctr, float r);

  // for auto calibration
  int dump_pts_laser_subsampled(ofstream &out, int nPnts);

  Pnt3 raw_for_ith      (int   i, sd_raw_pnt &data);
  Pnt3 raw_for_ith_valid(int   i, sd_raw_pnt &data);
};

#endif
