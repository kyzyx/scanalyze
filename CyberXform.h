//############################################################
//
// CyberXform.h
//
// Kari Pulli
// Fri Dec 11 11:40:42 CET 1998
//
// Class for transforming raw Cyberware data (the Digital
// Michelangelo scanner) into 3D, as well as projecting
// 3D points into scans.
// For a single turn only!
//
//############################################################

#ifndef _CyberXform_h_
#define _CyberXform_h_

#include "Xform.h"

class CyberXform {
  //private:
public:

  Xform<double> raw_to_laser;
  Xform<double> laser_to_scanaxis;
  Xform<double> raw_to_scanaxis;
  Xform<double> raw_to_scanaxis_inv;
  Xform<double> scanaxis_to_horz;
  Xform<double> even_xform, odd_xform;
  const double *even_xf, *odd_xf, *toYZ;

  bool  vertical_scan;
  // following define the laser plane before scan rotation
  const double *laser_n, *laser_t;
  double n_dot_t, n_dot_q, s_dot_q;
  double axdir[3], ax0[3];
  double s[3]; // cross(nodaxis, laser_n)
  double q[3]; // cross(nodaxis, laser_n)
  double axislimit_min, axislimit_max;
  double axisdist_far, axisdist_near;
  double _h1, _h2, _h3, _h4;

  Pnt3  laser_ctr_in_laser_coords(void);
  Pnt3  laser_ctr;

  void  bounds_for_circle_on_laser_in_raw(float ly, float lz, float r,
					  short ywin[2], short zwin[2]);
public:
  CyberXform(void) {}

  CyberXform(unsigned config, float hor_trans, float screw,
	     float vert_trans = 0)
    {
      setup((config & 0x00000001),
	    (config & 0x00000002) >> 1,
	    (config & 0x0000000c) >> 2,
	    (config & 0x00000010) >> 4,
	    hor_trans, screw, vert_trans);
    }
  CyberXform(bool vscan, bool tup, int tconf, bool tright,
	     float hor_trans, float screw, float vert_trans = 0)
    {
      setup(vscan, tup, tconf, tright, hor_trans, screw, vert_trans);
    }

  void setup(bool vscan, bool tup, int tconf, bool tright,
	     float hor_trans, float screw, float vert_trans = 0);

  void setup(unsigned config, float hor_trans, float screw,
	     float vert_trans = 0)
    {
      setup((config & 0x00000001),
	    (config & 0x00000002) >> 1,
	    (config & 0x0000000c) >> 2,
	    (config & 0x00000010) >> 4,
	    hor_trans, screw, vert_trans);
    }

  void set_screw(float even_scr, float odd_scr);
  void set_screw(float even_scr);

  Xform<double> get_xform_even(void)    { return even_xf; }
  Xform<double> get_xform_odd(void)     { return odd_xf; }

  Xform<double> set_and_get_geometric_xform(float screw);

  Pnt3 apply_xform(short y, short z);
  void apply_xform(short y, short z, Pnt3 &p);

  Pnt3 apply_raw_to_laser(short y, short z);

  float axis_project(short y, short z);

  bool back_project(const Pnt3 &p_in, Pnt3 &p_out,
		    bool  check_frustum = true);

  int  sphere_status(const Pnt3 &ctr, float r,
		     float &screw,
		     float ax_min, float ax_max,
		     short ywin[2], short zwin[2],
		     float swin[2]);

  Pnt3 normal_to_laser(void)            { return Pnt3(laser_n); }
};

#endif
