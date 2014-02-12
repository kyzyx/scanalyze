//############################################################
//
// RigidScan.cc
//
// Kari Pulli
// Mon Jun 29 11:51:05 PDT 1998
//
// An abstract base class for holding all the scanner specific
// information of a complete scan.
//
// This class is meant to form an abstraction between the raw
// (or minimally processed) data from various scanners and
// the algorithms that are used to (at least)
//    o  display the data for view planning, registration, etc.,
//       possibly using a limited polygon budget;
//    o  determine a volumetric approximate model of the target;
//    o  fit an accurate surface description of the target using
//       the range data;
//    o  associate possible input color with scanned geometry.
//
// The logical unit is a single rigidly registered scan.
// This can mean different things for different scanners.
//    o  Cyrax: probably just a single scan.
//    o  Cyberware custom statue scanner: it may include
//       dozens of scan sweeps as long as the base of the
//       scanner was not moved.
//    o  Modelmaker the data from a single Faro position.
//    o  Generic scanner: Any other scanner from which we
//       don't know much more than range maps.
//
//############################################################

#include "RigidScan.h"
#include "MeshTransport.h"
#include "TriMeshUtils.h"
#include "plvScene.h"      // for meshes_written_stripped()
#include "plvDraw.h"       // to know what color properties to write
#include <fstream.h>       // for write_metadata


RigidScan::RigidScan()
{
}

bool
RigidScan::render_self (ColorSource color)
{ return false; }

int
RigidScan::num_vertices(void)
{ return 0; }

void
RigidScan::subsample_points(float rate, vector<Pnt3> &p,
			    vector<Pnt3> &n)
{ }

void
RigidScan::flipNormals (void)
{ }

void
RigidScan::computeBBox (void)
{ }

RigidScan*
RigidScan::filtered_copy(const VertexFilter &filter)
{
  return NULL;
}

bool
RigidScan::filter_vertices (const VertexFilter& filter, vector<Pnt3>& p)
{
  return false;
}

bool
RigidScan::filter_inplace (const VertexFilter &filter)
{
  return false;
}

crope
RigidScan::getInfo(void)
{
  float q[4], t[3];
  Xform<float> xf = getXform();
  xf.toQuaternion(q);
  xf.getTranslation(t);
  Bbox wb = worldBbox();

  char info[1024 + PATH_MAX];
  sprintf(info,
	  "filename: %s\n"
	  "trans: %.3f %.3f %.3f\n"
	  "orientation: %.3f %.3f %.3f %.3f\n"
	  "\n"
	  "extents in world coordinates:\n"
	  "  X: %.3f to %.3f\n"
	  "  Y: %.3f to %.3f\n"
	  "  Z: %.3f to %.3f",
	  get_name().c_str(),
	  t[0], t[1], t[2], q[0], q[1], q[2], q[3],
	  wb.min()[0], wb.max()[0], wb.min()[1], wb.max()[1],
	  wb.min()[2], wb.max()[2]);

  return crope (info);
}

OccSt
RigidScan::carve_cube  (const Pnt3 &ctr, float side)
{ return INDETERMINATE; }

OccSt
RigidScan::carve_sphere(const Pnt3 &ctr, float radius)
{ return INDETERMINATE; }

bool
RigidScan::closest_point(const Pnt3 &p, const Pnt3 &n,
			 Pnt3 &cl_pnt, Pnt3 &cl_nrm,
			 float thr, bool brdy_ok)
{ return 0; }

#if 0
float
RigidScan::closest_point(const Pnt3 &p, Pnt3 &cl_pnt)
{ return 0.0f; }

float
RigidScan::closest_point(const Pnt3 &p, Pnt3 &cl_pnt, Pnt3 &cl_nrm)
{ return 0.0f; }
#endif

float
RigidScan::closest_point_on_mesh(const Pnt3 &p, Pnt3 &cl_pnt,
				 OccSt &status_p)
{ return 0.0f; }

float
RigidScan::closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
				       OccSt &status_p)
{ return 0.0f; }

float
RigidScan::closest_along_line(const Pnt3 &p, const Pnt3 &dir,
			      Pnt3 &cp, OccSt &status_p)
{ return 0.0f; }

float
RigidScan::color_along_line_of_sight(const Pnt3 &p, float rgb[3])
{ return 0.0f; }

float
RigidScan::color_along_line_of_sight(const Pnt3 &p, uchar rgb[3])
{ return 0.0f; }

bool
RigidScan::read(const crope &fname)
{ return false; }

bool
RigidScan::is_modified (void)
{ return false; }

bool
RigidScan::write(const crope& fname)
{ return false; }

bool
RigidScan::write_resolution_mesh (int nPolys, const crope& fname,
				  Xform<float> xfBy)
{
  // This is a basic implementation that will work for any RigidScan,
  // by writing the geometry/topology information that is returned from
  // RigidScan::mesh().  Feel free to override it if you can implement
  // it in a more efficient scan-specific fashion (like GenericScan,
  // for example, which stores its data in a Mesh class that knows how to
  // write itself anyway and can thus avoid the call to mesh()).

  int nOldRes = resolutions[curr_res].abs_resolution;
  if (!select_by_count (nPolys))
    return false;

  bool success = false;
  bool bStrips = theScene->meshes_written_stripped();
  // BUGBUG support other color modes than truecolor, intensity?
  MeshTransport* mt = mesh (true, bStrips,
			    (theRenderParams &&
			    (theRenderParams->colorMode == intensityColor)) ?
			    colorIntensity : colorTrue);
  if (mt) {
    // if there's only one fragment and it doesn't need any
    // transformation, we can write directly from the fragment
    if (mt->vtx.size() == 1
	&& xfBy.isIdentity()
	&& mt->xf[0].isIdentity()) {
      // can just write the geometry
      cerr << "Writing single mesh... " << flush;

      if (mt->color.size() == 1
	  && mt->color[0]->size() >= mt->vtx[0]->size()) {
	cerr << "(in color)... " << flush;
	write_ply_file (fname.c_str(), *(mt->vtx[0]),
			*(mt->tri_inds[0]), bStrips,
			*(mt->color[0]));
      } else {
	write_ply_file (fname.c_str(), *(mt->vtx[0]),
			*(mt->tri_inds[0]), bStrips);
      }
      success = true;
    } else {
      // either multiple fragments, or needs xform before write, so
      // need to build conglomerate vectors with all vertices and
      // consistent triangle indexing
      cerr << "Writing mesh consisting of " << mt->vtx.size()
	   << " fragments... " << flush;
      vector<Pnt3> vtx;
      vector<int> tris;
      vector<uchar> color;

      int nvtx = 0;
      int ntris = 0;
      int ncolors = 0;
      for (int i = 0; i < mt->vtx.size(); i++) {
	nvtx += mt->vtx[i]->size();
	ntris += mt->tri_inds[i]->size();
	if (i < mt->color.size() && mt->color[i])
	  ncolors += mt->color[i]->size();
      }

      vtx.reserve (nvtx);
      tris.reserve (ntris);
      color.reserve (ncolors);

      nvtx = 0;
      ntris = 0;
      ncolors = 0;
      for (i = 0; i < mt->vtx.size(); i++) {
// STL Update
	vector<Pnt3>::iterator newVtxStart = vtx.end();

	vtx.insert (vtx.end(), mt->vtx[i]->begin(), mt->vtx[i]->end());
// STL Update
	vector<int>::iterator tend = tris.end();
	tris.insert (tend, mt->tri_inds[i]->begin(), mt->tri_inds[i]->end());

// STL Update
	const vector<int>::iterator newtend = tris.end();
	for (vector<int>::iterator t = tend; t < newtend; t++) {
	  if (*t != -1)
	    *t += nvtx;
	}
	// update triangle indices
	nvtx += mt->vtx[i]->size();

	if (i < mt->color.size() && mt->color[i])
	color.insert (color.end(), mt->color[i]->begin(), mt->color[i]->end());

	Xform<float> xfThis = xfBy * mt->xf[i];
	if (!xfThis.isIdentity()) {
	  cerr << "applying xform... " << flush;
// STL Update
	  for (vector<Pnt3>::iterator pi = newVtxStart; pi < vtx.end(); pi++) {
	    xfThis (*pi);
	  }
	}
      }

      if (color.size() >= vtx.size())
	write_ply_file (fname.c_str(), vtx, tris, bStrips, color);
      else
	write_ply_file (fname.c_str(), vtx, tris, bStrips);
      success = true;
    }
    delete mt;
  }
  select_by_count (nOldRes);

  if (success)
    cerr << "done." << endl;
  else
    cerr << "failed!" << endl;
  return success;
}


bool
RigidScan::write_metadata (MetaData data)
{
  bool success = false;

  switch (data) {
  case md_xform:
    success = TbObj::writeXform (get_basename());
    break;
  }

  return success;
}


bool
RigidScan::get_children (vector<RigidScan*>& children) const
{ return false; }
