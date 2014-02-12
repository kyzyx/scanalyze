//############################################################
//
// CyberXform.cc
//
// Kari Pulli
// Fri Dec 11 11:40:42 CET 1998
//
// Class for transforming raw Cyberware data (the Digital
// Michelangelo scanner) into 3D, as well as projecting
// 3D points into scans
//
//############################################################

#include "CyberXform.h"
#include "CyberCalib.h"
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#ifdef WIN32
#	include "defines.h"
#endif


#define SHOW(X) cout << #X " = " << X << endl

static CyberCalib cc;           // calibration data

                                // and some other calibration
                                // data that hasn't been integrated
                                // into CyberCalib
// these numbers come from the CyTemplate-WAG8007.iv file
// and describe the range of the raw data for y and z
static int CorrRowOffset = 4;
static int CorrNumRows   = 236;
static int CorrZOffset   = 60;
static int CorrZSize     = 2880;
static int otableSize = 9;
// The calibration values and derived calibration constants used
// by these macros are defined in CyberCalib.h
// a <- dtssp
// b <- dtsep
// c <- offt
// minscrew <- -6
// function ang = screw_to_angle(screw, a, b, c, minscrew)
//   ang = angle(a, b, screw-minscrew+c) - angle(a, b, c);
//
// function res = angle(a,b,c)
//   % get the angle opposite to c in a triangle a,b,c
//   res = acos((a*a+b*b-c*c)/(2*a*b));

#define TURN_SCREW_TO_ANGLE(scr) \
(acos((cc.t_h2-((scr)+cc.t_h1)*((scr)+cc.t_h1))*cc.t_h3) - cc.t_h4)

#define NOD_SCREW_TO_ANGLE(scr) \
(acos((cc.n_h2-((scr)+cc.n_h1)*((scr)+cc.n_h1))*cc.n_h3) - cc.n_h4)

#define SCREW_TO_ANGLE(scr) \
(acos((_h2-((scr)+_h1)*((scr)+_h1))*_h3) - _h4)

#define ANGLE_TO_SCREW(ang) \
(sqrtf(_h2 - cos(ang+_h4)/_h3) - _h1)

static float corners[4][2] = {
  { CorrRowOffset*2, CorrZOffset },
  { (CorrRowOffset+CorrNumRows)*2, CorrZOffset },
  { CorrRowOffset*2, CorrZOffset+CorrZSize },
  { (CorrRowOffset+CorrNumRows)*2, CorrZOffset+CorrZSize }
};


// the answer is laser_ctr_in_laser_coords() = [0 18.9049 -1215.23]
Pnt3
CyberXform::laser_ctr_in_laser_coords(void)
{
  Pnt3 a(0, corners[0][0], corners[0][1]);
  Pnt3 b(0, corners[1][0], corners[1][1]);
  Pnt3 c(0, corners[2][0], corners[2][1]);
  Pnt3 d(0, corners[3][0], corners[3][1]);
  raw_to_laser(a);
  raw_to_laser(b);
  raw_to_laser(c);
  raw_to_laser(d);
  Pnt3 ans;
  bool ok = lines_intersect(a,c,b,d, ans);
  assert(ok);
  return ans;
}


void
CyberXform::bounds_for_circle_on_laser_in_raw(float ly, float lz, float r,
					      short ywin[2], short zwin[2])
{
  Xform<double> laser_to_raw = raw_to_laser;
  laser_to_raw.invert();

  //SHOW("...");

  // need to convert from laser coordinates back to raw
  Pnt3 p(0, ly+r, lz);
  //SHOW(p);
  laser_to_raw(p);
  //SHOW(p);
  ywin[0] = ywin[1] = p[1];
  zwin[0] = zwin[1] = p[2];
  p.set(0, ly-r, lz);
  //SHOW(p);
  laser_to_raw(p);
  //SHOW(p);
  if      (ywin[0] > p[1]) ywin[0] = p[1];
  else if (ywin[1] < p[1]) ywin[1] = p[1];
  if      (zwin[0] > p[2]) zwin[0] = p[2];
  else if (zwin[1] < p[2]) zwin[1] = p[2];
  p.set(0, ly, lz+r);
  //SHOW(p);
  laser_to_raw(p);
  //SHOW(p);
  if      (ywin[0] > p[1]) ywin[0] = p[1];
  else if (ywin[1] < p[1]) ywin[1] = p[1];
  if      (zwin[0] > p[2]) zwin[0] = p[2];
  else if (zwin[1] < p[2]) zwin[1] = p[2];
  p.set(0, ly, lz-r);
  //SHOW(p);
  laser_to_raw(p);
  //SHOW(p);
  if      (ywin[0] > p[1]) ywin[0] = p[1];
  else if (ywin[1] < p[1]) ywin[1] = p[1];
  if      (zwin[0] > p[2]) zwin[0] = p[2];
  else if (zwin[1] < p[2]) zwin[1] = p[2];
  //SHOW(ywin[0]);
  //SHOW(ywin[1]);
  //SHOW(zwin[0]);
  //SHOW(zwin[1]);
}



// with the current optical transform, the raw z grows toward
// scanner, the laser z grows away from it
// raw and laser y behave more nicely

void
CyberXform::setup(bool vscan, bool tup, int tconf, bool tright,
		  float hor_trans, float screw, float vert_trans)
{
  vertical_scan = vscan;
  // the transformation goes as follows:
  // first, apply the optical correction to the data
  // (the raw data comes in two shorts,
  // y [8,479] and z [60,2939])
  // then, apply the optical frame transform
  // then, apply the nod,
  // then, apply the turn,
  // finally, translate
  // only the nod is variable, all the rest can be
  // collapsed (optical correction and opt. frame xform,
  // turn and translate)

  // get the optical transformation first

  // map the raw data to range [1,otableSize]
  double scaley = float(otableSize-1) / (2*CorrNumRows-1);
  double scalez = float(otableSize-1) / (CorrZSize-1);
  raw_to_laser.identity();

  // remove offset
  raw_to_laser.translate(0.0,
			 - (2*CorrRowOffset),
			 - CorrZOffset);
  // scale to [0..scale]
  raw_to_laser.scale(1.0, scaley, scalez);
  // move up by one
  raw_to_laser.translate(0., 1., 1.);

  // do the optical table correction
  raw_to_laser *= Xform<double>(cc.A);

  // do the laser-to-nod frame transformation
  laser_to_scanaxis = Xform<double>(cc.opt_frm_v);

  scanaxis_to_horz.identity();
  if (vertical_scan) {
    _h1 = cc.n_h1; _h2 = cc.n_h2; _h3 = cc.n_h3; _h4 = cc.n_h4;
    axdir[0] = cc.axnod[0]; axdir[1] = cc.axnod[1]; axdir[2] = cc.axnod[2];
    ax0[0]  = cc.axnod0[0]; ax0[1]  = cc.axnod0[1]; ax0[2]  = cc.axnod0[2];
    scanaxis_to_horz.rot(TURN_SCREW_TO_ANGLE(screw),
			 cc.axturn[0], cc.axturn[1], cc.axturn[2],
			 cc.axturn0[0], cc.axturn0[1], cc.axturn0[2]);
  } else {
    _h1 = cc.t_h1; _h2 = cc.t_h2; _h3 = cc.t_h3; _h4 = cc.t_h4;
    axdir[0] = cc.axturn[0];axdir[1] = cc.axturn[1];axdir[2] = cc.axturn[2];
    ax0[0]  = cc.axturn0[0]; ax0[1] = cc.axturn0[1]; ax0[2] = cc.axturn0[2];
    laser_to_scanaxis *= Xform<double>(cc.opt_frm_h);
    laser_to_scanaxis.rot(NOD_SCREW_TO_ANGLE(screw),
			  cc.axnod[0], cc.axnod[1], cc.axnod[2],
			  cc.axnod0[0], cc.axnod0[1], cc.axnod0[2]);
  }

  // Add translation
  if (!tright) tup = !tup;

  if (tup)  {
    if (tconf == 1)  {
      scanaxis_to_horz.translate(hor_trans * cc.arm_up1[0],
				 hor_trans * cc.arm_up1[1],
				 hor_trans * cc.arm_up1[2] );
    } else if (tconf == 2)  {
      scanaxis_to_horz.translate(hor_trans * cc.arm_up2[0],
				 hor_trans * cc.arm_up2[1],
				 hor_trans * cc.arm_up2[2] );
    } else if (tconf == 3)  {
      scanaxis_to_horz.translate(hor_trans * cc.arm_up3[0],
				 hor_trans * cc.arm_up3[1],
				 hor_trans * cc.arm_up3[2] );
    } else {
      scanaxis_to_horz.translate(hor_trans * cc.arm_up4[0],
				 hor_trans * cc.arm_up4[1],
				 hor_trans * cc.arm_up4[2] );
    }
  } else {
    if (tconf == 1)  {
      scanaxis_to_horz.translate(hor_trans * cc.arm_down1[0],
				 hor_trans * cc.arm_down1[1],
				 hor_trans * cc.arm_down1[2] );
    } else if (tconf == 2)  {
      scanaxis_to_horz.translate(hor_trans * cc.arm_down2[0],
				 hor_trans * cc.arm_down2[1],
				 hor_trans * cc.arm_down2[2] );
    } else if (tconf == 3)  {
      scanaxis_to_horz.translate(hor_trans * cc.arm_down3[0],
				 hor_trans * cc.arm_down3[1],
				 hor_trans * cc.arm_down3[2] );
    } else {
      scanaxis_to_horz.translate(hor_trans * cc.arm_down4[0],
				 hor_trans * cc.arm_down4[1],
				 hor_trans * cc.arm_down4[2] );
    }
  }
  scanaxis_to_horz.translate(0,0,vert_trans);

  // precalculations for backprojection
  laser_n = &((double *) laser_to_scanaxis)[0];
  laser_t = &((double *) laser_to_scanaxis)[12];
  n_dot_t = laser_n[0]*laser_t[0]+laser_n[1]*laser_t[1]
    +laser_n[2]*laser_t[2];
  // vector s gives the direction of the line that we
  // get when we drop a perpendicular from a 3D point
  // to the nod axis, and rotate it so it cuts the laser
  // plane
  // s = cross(n,nodaxis)
  s[0] = laser_n[1]*axdir[2] - laser_n[2]*axdir[1];
  s[1] = laser_n[2]*axdir[0] - laser_n[0]*axdir[2];
  s[2] = laser_n[0]*axdir[1] - laser_n[1]*axdir[0];
  double sinvlen = 1.0 / sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
  s[0] *= sinvlen; s[1] *= sinvlen; s[2] *= sinvlen;

  q[0] = s[1]*axdir[2] - s[2]*axdir[1];
  q[1] = s[2]*axdir[0] - s[0]*axdir[2];
  q[2] = s[0]*axdir[1] - s[1]*axdir[0];

  n_dot_q = laser_n[0]*q[0]+laser_n[1]*q[1]+laser_n[2]*q[2];
  s_dot_q = s[0]*q[0] + s[1]*q[1] + s[2]*q[2];

  raw_to_scanaxis     = laser_to_scanaxis * raw_to_laser;

  raw_to_scanaxis_inv = raw_to_scanaxis;
  raw_to_scanaxis_inv.invert();
  toYZ = (const double *) raw_to_scanaxis_inv;

  // figure out some limits for testing whether a point
  // can project to within the working volume
  axislimit_min = 1.e33; axislimit_max = -axislimit_min;
  axisdist_near = 1.e33; axisdist_far = -1.0;
  for (int i=0; i<4; i++) {
    Pnt3 p(0, corners[i][0], corners[i][1]);
    raw_to_scanaxis(p);
    // change to vector from a0 to p
    p[0] -= ax0[0]; p[1] -= ax0[1]; p[2] -= ax0[2];
    float tmp = p[0]*axdir[0] + p[1]*axdir[1] + p[2]*axdir[2];
    if (tmp < axislimit_min) axislimit_min = tmp;
    if (tmp > axislimit_max) axislimit_max = tmp;
    // calculate also the squared distance to the axis
    tmp = dot(p, p) - tmp*tmp; // pythagorean rule
    assert(tmp > 0.0);
    if (tmp < axisdist_near) axisdist_near = tmp;
    if (tmp > axisdist_far ) axisdist_far  = tmp;
  }
  axisdist_near = sqrtf(axisdist_near);
  axisdist_far  = sqrtf(axisdist_far);
  /*
  SHOW(vertical_scan);
  Pnt3 lzr = laser_ctr_in_laser_coords();
  laser_to_scanaxis(lzr);
  Pnt3 t1 = lzr - Pnt3(ax0);
  float t = dot(t1, Pnt3(axdir));
  SHOW(sqrtf(t1.norm2() - t*t));
  assert(0);
  */
}


void
CyberXform::set_screw(float even_scr, float odd_scr)
{
  odd_xform = even_xform = raw_to_scanaxis;
  even_xform.rot(SCREW_TO_ANGLE(even_scr),
		 axdir[0], axdir[1], axdir[2],
		 ax0[0], ax0[1], ax0[2]);
  odd_xform.rot (SCREW_TO_ANGLE(odd_scr),
		 axdir[0], axdir[1], axdir[2],
		 ax0[0], ax0[1], ax0[2]);
  even_xform *= scanaxis_to_horz;
  odd_xform  *= scanaxis_to_horz;
  even_xf = (const double *) even_xform;
  odd_xf  = (const double *) odd_xform;

  Xform<double> blah = laser_to_scanaxis;
  blah.rot(SCREW_TO_ANGLE(even_scr),
	   axdir[0], axdir[1], axdir[2],
	   ax0[0], ax0[1], ax0[2]);
  blah *= scanaxis_to_horz;
  Pnt3 lzr(0, 18.9, -1215.2);
  blah(lzr);
  laser_ctr = lzr;
}


void
CyberXform::set_screw(float even_scr)
{
  even_xform = raw_to_scanaxis;
  even_xform.rot(SCREW_TO_ANGLE(even_scr),
		 axdir[0], axdir[1], axdir[2],
		 ax0[0], ax0[1], ax0[2]);
  even_xform *= scanaxis_to_horz;

  even_xf = (const double *) even_xform;
}


// use this only if you want to see how the scanhead
// moves, e.g., for color camera stuff
Xform<double>
CyberXform::set_and_get_geometric_xform(float screw)
{
  Xform<double> xf = laser_to_scanaxis;
  xf.rot(SCREW_TO_ANGLE(screw),
	 axdir[0], axdir[1], axdir[2],
	 ax0[0], ax0[1], ax0[2]);
  xf *= scanaxis_to_horz;
  return xf;
}


Pnt3
CyberXform::apply_xform(short y, short z)
{
  if (y%2) {
    double invw =
      1.0 / (odd_xf[7]*y+odd_xf[11]*z+odd_xf[15]);
    return Pnt3((odd_xf[4]*y+odd_xf[8]*z+odd_xf[12])*invw,
		(odd_xf[5]*y+odd_xf[9]*z+odd_xf[13])*invw,
		(odd_xf[6]*y+odd_xf[10]*z+odd_xf[14])*invw);
  } else {
    double invw =
      1.0 / (even_xf[7]*y+even_xf[11]*z+even_xf[15]);
    return Pnt3((even_xf[4]*y+even_xf[8]*z+even_xf[12])*invw,
		(even_xf[5]*y+even_xf[9]*z+even_xf[13])*invw,
		(even_xf[6]*y+even_xf[10]*z+even_xf[14])*invw);
  }
}

void
CyberXform::apply_xform(short y, short z, Pnt3 &p)
{
  if (y%2) {
    double invw =
      1.0 / (odd_xf[7]*y+odd_xf[11]*z+odd_xf[15]);
    p.set((odd_xf[4]*y+odd_xf[8]*z+odd_xf[12])*invw,
	  (odd_xf[5]*y+odd_xf[9]*z+odd_xf[13])*invw,
	  (odd_xf[6]*y+odd_xf[10]*z+odd_xf[14])*invw);
  } else {
    double invw =
      1.0 / (even_xf[7]*y+even_xf[11]*z+even_xf[15]);
    p.set((even_xf[4]*y+even_xf[8]*z+even_xf[12])*invw,
	  (even_xf[5]*y+even_xf[9]*z+even_xf[13])*invw,
	  (even_xf[6]*y+even_xf[10]*z+even_xf[14])*invw);
  }
}

// used by CyberTurn...
Pnt3
CyberXform::apply_raw_to_laser(short y, short z)
{
  Pnt3 p(0,y,z);
  raw_to_laser(p);
  return p;
}


// move a raw data point to the same coordinates as
// the scan axis, project it to it, get a float
// that expresses the point's position on axis
float
CyberXform::axis_project(short y, short z)
{
  Pnt3 p(0,y,z);
  raw_to_scanaxis(p);
  return ((p[0]-ax0[0])*axdir[0] +
	  (p[1]-ax0[1])*axdir[1] +
	  (p[2]-ax0[2])*axdir[2]);
}


static double axis_proj_pos;
static double scan_angle;

// return the scr,y,z raw coordinates that would have
// exactly scanned p_in
// return false if the point is not within the viewing frustum
// (can be in front or back of the actual working volume,
//  but not on the side)
bool
CyberXform::back_project(const Pnt3 &p_in, Pnt3 &p_out,
			 bool check_frustum)
{
  // the sequence of events:
  // * remove the transformation from horizontal translation
  //   until the scan axis
  // * project the point p onto the scan axis (call that pa)
  // * can already check whether in working volume
  //   (too far left or right, too far, too close,
  //    although close one's can be used for carving)
  // * find the intersection of the laser plane with p
  //   think about rotating not the plane but p
  //   find where they intersect, from that get the
  //   y,z coordinates, and also the rotation angle,
  //   from which get the screw length
  // * calculate the rotation intersection by first
  //   going from pa to laser plane,
  //   then continue along the plane so far that
  //   the distance from pa becomes the same as |p-pa|.

  float *p = &p_out[0];
  // * remove the horizontal translation, maybe also turn
  scanaxis_to_horz.apply_inv(p_in, p);

  // * project the point p onto the scan axis (call that pa)
  axis_proj_pos = ((p[0]-ax0[0])*axdir[0] +
		   (p[1]-ax0[1])*axdir[1] +
		   (p[2]-ax0[2])*axdir[2]);
  // within frustum?
  if (check_frustum &&
      (axis_proj_pos < axislimit_min ||
       axis_proj_pos > axislimit_max))
    return false;
  double pa[3],r[3];
  pa[0] = axis_proj_pos*axdir[0]+ax0[0];
  pa[1] = axis_proj_pos*axdir[1]+ax0[1];
  pa[2] = axis_proj_pos*axdir[2]+ax0[2];

  // r is pa to p
  r[0] = p[0]-pa[0];
  r[1] = p[1]-pa[1];
  r[2] = p[2]-pa[2];
  double r2 = r[0]*r[0]+r[1]*r[1]+r[2]*r[2];

  // find the intersection with laser plane
  // alpha = n.t - n.pa (is also distance of pa from laser plane)
  double alpha = (n_dot_t - (laser_n[0]*pa[0]+
			     laser_n[1]*pa[1]+
			     laser_n[2]*pa[2])) / n_dot_q;

  double alpha2 = alpha*alpha;

  if (r2 < alpha2) {
    cerr << "Really weird input to backproject!" << endl;
    cerr << "(too close to scan axis)" << endl;
    return false;
  }

  double beta;
  double tmp = sqrtf(alpha2*s_dot_q*s_dot_q - (alpha2-r2));
  if (vertical_scan) beta = -alpha*s_dot_q - tmp;
  else               beta = -alpha*s_dot_q + tmp;

  // get the "rotated" point on the laser plane
  double rp[3];
  rp[0] = pa[0] + alpha*q[0] + beta*s[0];
  rp[1] = pa[1] + alpha*q[1] + beta*s[1];
  rp[2] = pa[2] + alpha*q[2] + beta*s[2];

  // get the y,z coords
  // don't need to calculate x because it should become 0
  tmp =
    1.0/(toYZ[3]*rp[0]+toYZ[7]*rp[1]+toYZ[11]*rp[2]+toYZ[15]);
  p[1] =(toYZ[1]*rp[0]+toYZ[5]*rp[1]+toYZ[9]*rp[2]+toYZ[13])*tmp;
  p[2] =(toYZ[2]*rp[0]+toYZ[6]*rp[1]+toYZ[10]*rp[2]+toYZ[14])*tmp;

  // get the screw
  scan_angle = acos((alpha * (r[0]*q[0]+r[1]*q[1]+r[2]*q[2]) +
		     beta *  (r[0]*s[0]+r[1]*s[1]+r[2]*s[2]))/r2);
  // try to deal with slightly negative angles
  if (scan_angle > .75 * M_PI) scan_angle -= M_PI;

  p[0] = ANGLE_TO_SCREW(scan_angle);
  return true;
}




// return values
// 0 not in frustum
// 1 not fully in frustum
// 2 possibly fully in frustum

int
CyberXform::sphere_status(const Pnt3 &ctr, float radius,
			  float &screw,
			  float ax_min, float ax_max,
			  short ywin[2], short zwin[2],
			  float swin[2])
{
  // first, back project the center
  Pnt3 p;
  scanaxis_to_horz.apply_inv(ctr, p);

  axis_proj_pos = ((p[0]-ax0[0])*axdir[0] +
		   (p[1]-ax0[1])*axdir[1] +
		   (p[2]-ax0[2])*axdir[2]);

  // check whether in viewing frustum
  if (axis_proj_pos < ax_min - radius ||
      axis_proj_pos > ax_max + radius) {
    //cout << "wide " << radius << endl;
    return 0; // NOT_IN_FRUSTUM
  }
  double pa[3],r[3];
  pa[0] = axis_proj_pos*axdir[0]+ax0[0];
  pa[1] = axis_proj_pos*axdir[1]+ax0[1];
  pa[2] = axis_proj_pos*axdir[2]+ax0[2];

  // r is pa to p
  r[0] = p[0]-pa[0];
  r[1] = p[1]-pa[1];
  r[2] = p[2]-pa[2];
  double r2 = r[0]*r[0]+r[1]*r[1]+r[2]*r[2];
  double dist_axis = sqrt(r2);

  if (dist_axis > axisdist_far + radius) {
    //cout << "far " << radius << endl;
    return 0; // NOT_IN_FRUSTUM
  }

  // check whether can be fully in viewing frustum
  if (axis_proj_pos > ax_min + radius &&
      axis_proj_pos < ax_max - radius) {
    // can't be fully in frustum
    return 1;
  }
  if (dist_axis > axisdist_far - radius) {
    // can't be fully in frustum
    return 1;
  }

  Pnt3 bp;
  back_project(ctr, bp, false);
  // how much can we rotate laser plane and still hit sphere?
  // BUGBUG: not accurate at all...

  float alpha = asin(radius / dist_axis);
  swin[0] = scan_angle - alpha;
  swin[1] = scan_angle + alpha;

  if (swin[0] < 0 || swin[1] > M_PI * .75) return 1;

  // the corners of the working volume in laser coordinates
  // are
  // (-2.5, 142.4)
  // (143.2, 140.9
  // (-.3, 1)
  // (130.3, .4)
  // Bbox: [(-2.5, .4); (143.2, 142.4)]

  // convert the y,z to laser coordinates, in mm
  Pnt3 lzr(0,bp[1],bp[2]);
  raw_to_laser(lzr);

  // could the sphere be fully within the working volume?
  bounds_for_circle_on_laser_in_raw(lzr[1], lzr[2], radius,
				    ywin, zwin);

  if (ywin[0] <  CorrRowOffset*2 ||
      ywin[1] >= (CorrRowOffset+CorrNumRows)*2) {
    // can't be fully in frustum
    return 1;
  }

  if (zwin[0] <  CorrZOffset) {
    //zwin[1] >= (CorrZOffset+CorrZSize)) {
    // can't be fully in frustum
    return 1;
  }

  swin[0] = ANGLE_TO_SCREW(swin[0]);
  swin[1] = ANGLE_TO_SCREW(swin[1]);

  // could be fully within frustum (for space carving purposes)
  screw = bp[0];

  return 2;
}



#if 0

void
main(void)
{
  CyberXform xf;
  xf.setup(1, 1, 1, 1, 10, 10);
  xf.set_nod(10,20);
  SHOW(xf.even_xform);
  SHOW(xf.odd_xform);
}
#endif
