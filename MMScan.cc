//############################################################
//
// MMScan.cc
//
// Jeremy Ginsberg
//
// Stores all information pertaining to a 3DScanners ModelMaker H
// scan "conglomerate"... this contains many small scan fragments,
// each of which can be characterized by a single viewing direction.
//
// Data structures revamped starting rev1.35 to better encapsulate the
// idea of a collection of scan fragments.
//
//
// $Date: 2001/07/02 17:56:26 $
// $Revision: 1.66 $
//
//############################################################

#include "MMScan.h"
#include "MeshTransport.h"
#include "TriMeshUtils.h"
#include "KDindtree.h"
#include "ColorUtils.h"
#include "Progress.h"
#include "VertexFilter.h"

#include "Random.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "sys/stat.h"

#include "plvScene.h"


#define STRIPE_PTS 312          // max number of points in a given stripe
#define SUBSAMPS 6              // number of resolutions to create


MMScan::MMScan()
{
  haveScanDir = false;
  isDirty_mem = false;
  isDirty_disk = false;
  scans.clear();
}

MMScan::~MMScan()
{
  while (kdtree.size()) {
    delete (kdtree.back());
    kdtree.pop_back();
  }

  scans.clear();
}

static bool
getXformFilename (const char* meshName, char* xfName)
{
  if (meshName == NULL || meshName[0] == 0)
    return FALSE;

  strcpy (xfName, meshName);
  char* pExt = strrchr (xfName, '.');

  if (pExt == NULL)
    pExt = xfName + strlen (xfName);

  strcpy (pExt, ".xf");
  return TRUE;
}


void
MMScan::absorb_xforms(char *basename)
{

  char buffer[PATH_MAX];
  strcpy(buffer, basename);
  char *pExt = strrchr(buffer, '.');
  if (pExt == NULL)
    pExt = buffer + strlen(buffer);

  for (int i = 0; i < scans.size(); i++) {
    char temp[15];
    if (i < 10) sprintf(temp, "_00%d.xf", i);
      else if (i < 100) sprintf(temp, "_0%d.xf", i);
        else sprintf(temp, "_%d.xf", i);
    strcpy(pExt, temp);
    TbObj tempObj;
    Xform<float> newXF;
    tempObj.readXform(buffer);
    newXF = tempObj.getXform();
    cout << "Filename sought: " << buffer << endl;
    cout << newXF << endl;
    for (int j = 0; j < scans[i].meshes.size(); j++) {
      for (int k = 0; k < scans[i].meshes[j].vtx.size(); k++) {
	newXF(scans[i].meshes[j].vtx[k]);
      }
    }
  }
  computeBBox();
}


int
MMScan::write_xform_ply(int whichRes, char *directory,
			crope &confLines, bool applyXform)
{
  if (!load_resolution(whichRes)) {
    cerr << "MMScan::write_xform_ply couldn't load desired resolution"
	 << endl;
    return 0;
  }
  if (scans[0].meshes[whichRes].tris.size() == 0) {
    // we don't have triangles; only t-strips... do something about this
    for (int i = 0; i < scans.size(); i++) {
      mmResLevel& res = scans[i].meshes[whichRes];
      triangulate(&scans[0] + i, &res, 1 << whichRes);
    }
  }

  int numFragsSaved = 0;

  // get basename for ply files

  char buffer[PATH_MAX], *slash;
  strcpy(buffer, get_name().c_str());
  for (slash = buffer; *slash; slash++)
     if (*slash == '/') *slash = '_';

  char *pExt = strrchr(buffer, '.');
  if (pExt == NULL)
    pExt = buffer + strlen(buffer);

  /*
    okay so we don't really want confidence

    // build confidence if we don't have it
    if (scans[0].meshes[0].confidence.size() == 0)
    calcConfidence();
  */
  vector<Pnt3> scanVtx;

  Pnt3 axis;
  float rotAngle, quat[4];
  Xform<float> xf, invXF;

  // stuff for global alignment
  Xform<float> meshXF;
  Xform<float> comboXF;
  float comboTrans[3];

  meshXF = this->getXform();

  // set up conf strings

  //confLines = new char *[scans.size()];

  // save each scan to a separate file
  for(int i = 0; i < scans.size(); i++) {

    // discard absurdly tiny meshes
    if (scans[i].stripes.size() < 3) {
      cout << "DISCARDING MESH WITH < 3 STRIPES " << endl;
    }
    else {
       //cout << "Scan # " << i << ":" << endl;

      if (i < 10) sprintf(pExt, "_00%d.ply", i);
      else if (i < 100) sprintf(pExt, "_0%d.ply", i);
      else sprintf(pExt, "_%d.ply", i);

#if 0
      // An effort to combat flipped view direction
      Pnt3 nrm(0,0,0);
      for(int j = 0; j < scans[i].meshes[whichRes].nrm.size(); j++) {
	 nrm[0] += scans[i].meshes[whichRes].nrm[j*3+0];
	 nrm[1] += scans[i].meshes[whichRes].nrm[j*3+1];
	 nrm[2] += scans[i].meshes[whichRes].nrm[j*3+2];
      }

      if (dot(nrm, scans[i].viewDir) < 0)
	 scans[i].viewDir = -scans[i].viewDir;
#endif


      // calculate transformation necessary to align viewDir with Z-axis
      xf.identity();
      axis.set(-1 * scans[i].viewDir[1], scans[i].viewDir[0], 0);
      axis.normalize();

      rotAngle = M_PI + acos(-1 * scans[i].viewDir[2]);
      //cout << "rotating " << rotAngle << " rads around " << axis << endl;
      xf.rot(rotAngle, axis[0], axis[1], axis[2]);
      //cout << scans[i].num_vertices(whichRes) << " points" << endl;

      invXF = xf;
      invXF.invert();
      comboXF = meshXF * invXF;
      comboXF.toQuaternion(quat);
      comboXF.getTranslation(comboTrans);
      if (applyXform) {
	char line[PATH_MAX + 100];

	// save transformation to conf string
	sprintf(line, "{bmesh %s %g %g %g %g %g %g %g}",
		buffer, comboTrans[0], comboTrans[1], comboTrans[2],
		-1 * quat[1], -1 * quat[2], -1 * quat[3], quat[0]);
	//confLines[numFragsSaved] = new char[strlen(buffer)];
	//strcpy(confLines[numFragsSaved], buffer);
	if (!confLines.empty()) {
	   confLines += " ";
	}
	confLines += line;
      }

      scanVtx.reserve(scans[i].meshes[whichRes].vtx.size());

      // apply transformation to points
      for(int k = 0; k < scans[i].meshes[whichRes].vtx.size(); k++) {
	Pnt3 pt = scans[i].meshes[whichRes].vtx[k];
	if (applyXform)
	  xf(pt);
	scanVtx.push_back(pt);
      }

      char full_path[PATH_MAX];
      sprintf(full_path, "%s/%s", directory, buffer);
      write_ply_file(full_path, scanVtx,
		     scans[i].meshes[whichRes].tris, false
		     );
      scanVtx.clear();


      // write xf
      strcpy (full_path + strlen(full_path) - 3, "xf");
      cout << "Writing " << full_path << " ... " << flush;
      ofstream xffile (full_path);
      if (applyXform)
	 xffile << comboXF;
      else
	 xffile << meshXF;

      if (xffile.fail()) {
	 cerr << "Scan " << buffer
	      << " failed to write xform!" << endl;
	 break;
      }

      numFragsSaved++;

    }
  }

  return numFragsSaved;
}

MeshTransport*
MMScan::mesh(bool perVertex, bool stripped,
	     ColorSource color, int colorSize)
{
  if (stripped && !perVertex) {
    cerr << "No t-strips without per-vertex properties";
    return NULL;
  }

  int resNum = current_resolution_index();
  if (!resolutions[resNum].in_memory) load_resolution(resNum);
  int numVerts = num_vertices(resNum);

  MeshTransport* mt = new MeshTransport;

  for (int i = 0; i < scans.size(); i++) {
    if (scans[i].isVisible) {
      mmResLevel *res = &(scans[i].meshes[0]) + resNum;
      mt->setVtx (&res->vtx, MeshTransport::share);
      if (perVertex) {
	mt->setNrm (&res->nrm, MeshTransport::share);
      } else {
	return NULL;
	/* no support now
	if (!res->triNrm.size()) {
	  if (!res->tris.size()) strips_to_tris(res->tstrips, res->tris);
	  calcTriNormals (res->tris, res->triNrm);
	}
	mt->setNrm (&res->triNrm, MeshTransport::share);
	*/
      }

      if (stripped) {
	if (!res->tstrips.size()) {
	  int subSamp = 1 << resNum;
	  make_tstrips(&scans[0] + i, res, subSamp);
	}
	mt->setTris (&res->tstrips, MeshTransport::share);
      } else { // regular triangles
	if (!res->tris.size() && res->tstrips.size())
	  strips_to_tris(res->tstrips, res->tris);
	mt->setTris (&res->tris, MeshTransport::share);
      }

      if (color != colorNone)
	setMTColor (mt, i, perVertex, color, colorSize);
    }
  }

  return mt;
}


void
MMScan::setMTColor (MeshTransport* mt, int iScan, bool perVertex,
		    ColorSource source, int colorsize)
{
  int resNum = current_resolution_index();
  mmResLevel* res = &(scans[iScan].meshes[resNum]);

  /* don't know how to check ahead of time yet
  if ((!conf_col) && (vtxIntensity.size() == 0) && (colorsize != 3))
    return false;   // if we have no intensity data for some unknown reason
                    // and we're not looking for per-scan color or confidence
  */

  vector<uchar>* colors = new vector<uchar>;
  mt->setColor (colors, MeshTransport::steal);

  if (perVertex)
    colors->reserve (res->vtx.size() * colorsize);
  else return;
    /* no support now
    colors->reserve (num_tris(resNum) / 3 * colorsize);
    */

  int j;
  switch (source) {
  case colorConf:
    // confidence zone
    if (perVertex) {
      // per-vertex confidence zone
      if (res->confidence.size() == 0) return;

      for (j = 0; j < res->confidence.size(); j++)
	pushColor(*colors, colorsize, res->confidence[j]);
    }
    else {
      return;
      /* no support now
      // per-face confidence zone
      if (res->confidence.size() == 0) return;

      for (j = 0; j < res->tris.size(); j+=3)
	pushColor(*colors, colorsize,
		  res->confidence[res->tris[j+0]],
		  res->confidence[res->tris[j+1]],
		  res->confidence[res->tris[j+2]]);
      */
    }
    break; // end confidence color zone

  case colorTrue:
    // MMScan doesn't have true-color information, so it interprets this
    // flag as a request for per-fragment coloring.
    // begin per-scan coloring to replace diffuse color
    pushColor(*colors, colorsize, scans[iScan].falseColor);
    break;

  case colorIntensity:
  default:
    // intensity coloring
    // on filtered vertex sets, i'm not sure how the intensity
    // is going to be derived, so check and see where that data
    // is coming from if you're concerned.

    if (perVertex) {
      // per-vertex intensities

      if (res->intensity.size() == 0) return;
      for (j = 0; j < res->vtx.size(); j++)
	pushColor(*colors, colorsize, res->intensity[j]);
    }
    else {
      // per-face intensities
      return;
      /* no support now
      if (res->intensity.size() == 0) return;
      for (j = 0; j < res->tris.size(); j+=3)
	pushColor(*colors, colorsize,
		  res->intensity[res->tris[j+0]],
		  res->intensity[res->tris[j+1]],
		  res->intensity[res->tris[j+2]]);
      */
    } // end intensity-coloring zone
  } // end switch
}


crope
MMScan::getInfo(void)
{
  char buf[1000];
  sprintf(buf, "MMScan: %d scans, %d stripes, %d points,"
	  " %d resolutions\n\n",
	  num_scans(), num_stripes(), num_vertices(0), num_resolutions());

  return crope (buf) + RigidScan::getInfo();
}


int
MMScan::num_tris(int res)
{
  if (scans.size() == 0) return 0;
  int numResolutions = scans[0].meshes.size();
  if (numResolutions == 0) return 0;
  if ((res >= numResolutions) || (res < 0)) return 0;

  int numTris = 0;
  for (int i = 0; i < scans.size(); i++) {
    if (scans[i].isVisible)
      numTris += scans[i].meshes[res].tris.size() / 3;
  }
  return numTris;
}

int
MMScan::num_stripes()
{
  int sum = 0;
  for (int i = 0; i < scans.size(); i++) {
    sum += scans[i].stripes.size();
  }
  return sum;
}

int
MMScan::num_vertices(void)
{
  int total = 0;
  int resNum = current_resolution_index();

  for (int i = 0; i < scans.size(); i++) {
    if (scans[i].isVisible) {
      if ((resNum < scans[i].meshes.size()) && (resNum >= 0))
	total += scans[i].meshes[resNum].vtx.size();
    }
  }
  return total;
}

int
MMScan::num_vertices(int resNum)
{
  int total = 0;

  for (int i = 0; i < scans.size(); i++) {
    if (scans[i].isVisible)
      if ((resNum < scans[i].meshes.size()) && (resNum >= 0))
	total += scans[i].meshes[resNum].vtx.size();
  }
  return total;
}


int
MMScan::create_resolution_absolute(int budget, Decimator dec)
{
  if (budget == 0) return 0;
  int resNum = 0; // always decimate from the master copy, at least for now
  int totalTris = num_tris(resNum);
  int decimatedTris = 0;

  mmResLevel *resolutions = new mmResLevel[scans.size()];
  mmResLevel *newRes;

  cerr << "Target size = " << budget << ": decimating... ";
  for (int i = 0; i < scans.size(); i++) {
    // check to see if we have triangles
    mmResLevel *res = &(scans[i].meshes[0]) + resNum;

    if (!res->tris.size()) strips_to_tris(res->tstrips, res->tris);

    cerr << "mesh tris = " << scans[i].meshes[resNum].num_tris() << endl;;
    int fragBudget = budget * ((double)(scans[i].meshes[resNum].num_tris())
			       / totalTris);
    fprintf(stderr, "Mesh #%d:\t%d tris...\t", i, fragBudget);

    newRes = resolutions + i;

    quadric_simplify(scans[i].meshes[0].vtx, scans[i].meshes[0].tris,
		     newRes->vtx, newRes->tris,
		     fragBudget, PLACE_OPTIMAL, 0, 1000);
    if (newRes->tris.size()/3 != fragBudget)
      fprintf(stderr, "ended up with %ld tris\n", newRes->tris.size()/3);
    else fprintf(stderr, "done\n");
    decimatedTris += newRes->tris.size() / 3;

    getVertexNormals(newRes->vtx, newRes->tris, false, newRes->nrm, true);

  }
  insert_resolution(decimatedTris, get_name(), true, true);

  int iPos = findLevelForRes(decimatedTris);

  for (i = 0; i < scans.size(); i++) {
// STL Update
    scans[i].meshes.insert(scans[i].meshes.begin() + iPos, resolutions[i]);
  }
  delete[] resolutions;

  return decimatedTris;
}

void
MMScan::computeBBox (void)
{
  bbox.clear();
  int resNum = current_resolution_index();

  for (int k = 0; k < scans.size(); k++) {
    if (scans[k].isVisible) {
      for (int i = 0; i < scans[k].meshes[resNum].num_vertices(); i++) {
	bbox.add(scans[k].meshes[resNum].vtx[i]);
      }
    }
  }
  if (scans.size() == 0) {
    rot_ctr = Pnt3();
  } else {
    rot_ctr = bbox.center();
  }
}

void
MMScan::flipNormals (void)
{
  for (int i = 0; i < scans.size(); i++) {
    for (int j = 0; j < scans[i].meshes.size(); j++) {
      // triangle normals
      for (int k = 0; k < scans[i].meshes[j].tris.size(); k+=3) {
	swap(scans[i].meshes[j].tris[k], scans[i].meshes[j].tris[k+1]);
      }
      // tstrip normals
      flip_tris(scans[i].meshes[j].tstrips, true);
      // per-vertex normals
      for (k = 0; k < scans[i].meshes[j].nrm.size(); k++) {
	scans[i].meshes[j].nrm[k] *= -1;
      }
    }
    for (j = 0; j < scans[i].stripes.size(); j++) {
      // this isn't completely effective; causes z-axis align problems
      scans[i].stripes[j].sensorVec *= -1;
      if (scans[i].stripes[j].scanDir == 1) scans[i].stripes[j].scanDir = 2;
      else scans[i].stripes[j].scanDir = 1;
    }
    scans[i].viewDir *= -1;
  }
}

void
MMScan::flipNormals (int scanNum)
{
  for (int j = 0; j < scans[scanNum].meshes.size(); j++) {
    // triangle normals
    for (int k = 0; k < scans[scanNum].meshes[j].tris.size(); k+=3) {
      swap(scans[scanNum].meshes[j].tris[k],
	   scans[scanNum].meshes[j].tris[k+1]);
    }
    // tstrip normals
    flip_tris(scans[scanNum].meshes[j].tstrips, true);
    // per-vertex normals
    for (k = 0; k < scans[scanNum].meshes[j].nrm.size(); k++) {
      scans[scanNum].meshes[j].nrm[k] *= -1;
    }
  }
  for (j = 0; j < scans[scanNum].stripes.size(); j++) {
    // sensor vectors (so that it will triangulate flipped next time
    scans[scanNum].stripes[j].sensorVec *= -1;
    if (scans[scanNum].stripes[j].scanDir == 1)
      scans[scanNum].stripes[j].scanDir = 2;
    else scans[scanNum].stripes[j].scanDir = 1;
  }
  scans[scanNum].viewDir *= -1;
}

bool
MMScan::isValidPoint(int scanNum, int stripeNum, int colNum)
  // returns value of bitmap of specified stripe in specified column
  // of a specified scan
{
  if (scanNum >= scans.size()) return false;
  if (stripeNum >= scans[scanNum].stripes.size()) return false;
  if (colNum > STRIPE_PTS) return false;
  return (scans[scanNum].stripes[stripeNum].validMap[colNum / 8]
	  & (1 << (colNum % 8)));
}

static void
addTri(vector<int> &tris, int v1, int v2, int v3, char scanDir)
{

  // if it's normal or if we don't know the scan direction...
  if ((scanDir == 1) || (scanDir == 0)) {
    tris.push_back(v1);
    tris.push_back(v2);
    tris.push_back(v3);
  }
  // otherwise flip the beans out of it
  else if (scanDir == 2) {
    tris.push_back(v2);
    tris.push_back(v1);
    tris.push_back(v3);
  }
}

void
MMScan::calcScanDir(void)
{
  Pnt3 scanDir, stripeDir, crossDir;
  int numPts, i, j, k;

  // loop through all scans
  for (k = 0; k < scans.size(); k++) {

    if (scans[k].indices.size() == 0) calcIndices();

    mmScanFrag *scan = &scans[0] + k;
    mmResLevel *mesh = &(scan->meshes[0]);
    // stop before last stripe since we look ahead one stripe
    for (j = 0; j < scans[k].stripes.size() - 1; j++) {

      scanDir.set(0,0,0);
      stripeDir.set(0,0,0);
      numPts = 0;
      for (i = 0; i < STRIPE_PTS - 1; i++) {
	// first get scan direction
	if (isValidPoint(k,j,i) && isValidPoint(k,j+1,i)) {
	  scanDir += mesh->vtx[scan->indices[i + (j+1) * STRIPE_PTS]];
	  scanDir -= mesh->vtx[scan->indices[i + j * STRIPE_PTS]];
	  numPts++;
	}
      }
      // then get stripe direction
      stripeDir += mesh->vtx[scan->stripes[j].startIdx +
			       scan->stripes[j].numPoints - 1];
      stripeDir -= mesh->vtx[scan->stripes[j].startIdx];
      if (numPts == 0) {
	// not enough information to determine scanning direction
	scan->stripes[j].scanDir = 0;
	cout << "couldn't obtain scan direction for a stripe" << endl;
      }
      else {
	scanDir /= numPts;
	scanDir.normalize();
	stripeDir.normalize();

	// figure out where the scan direction is expected to be
	crossDir = cross(scan->stripes[j].sensorVec, stripeDir);

	// if the actual doesn't match expected, we need to flip the triangles
	// by marking the scanDir flag as 2 instead of 1
	if (dot(crossDir, scanDir) > 0) scan->stripes[j].scanDir = 1;
	   else scan->stripes[j].scanDir = 2;

      }
    }
    // here we are in the last stripe of a scan, so set it's direction to 0
    scan->stripes[j].scanDir = 0;
  }

      /*
    cout << "stripe " << j << endl << "  stripe dir: " << stripeDir << endl;
    cout << "  sensor vec: " << stripes[j].sensorVec << endl;
    cout << "   cross vec: " << crossDir << endl;
    cout << "    scan dir: " << scanDir << endl;
    cout << "    num pts : " << numPts << endl;
    if (stripes[j].scanDir == 1) cout << "normal" << endl;
    else if (stripes[j].scanDir == 2) cout << "flipped" << endl;
    else cout << "fucked!!" << endl;
    */

  haveScanDir = true;
}

void
MMScan::calcIndices()
{
  int i, j, k, count;
  fprintf(stderr, "Calculating triangulation indices...\n");
  for (k = 0; k < scans.size(); k++) {
    mmScanFrag *scan = &(scans[0]) + k;
    scan->indices.clear();
    scan->indices.reserve(scan->stripes.size() * STRIPE_PTS);
    count = 0;

    for (i = 0; i < scan->stripes.size(); i++) {
      for (j = 0; j < STRIPE_PTS; j++) {
	if (isValidPoint(k,i,j)) {
	  scan->indices.push_back(count++);
	}
	else
	  scan->indices.push_back(-1);
      }
    }
  }
}

/* not needed anymore
void
MMScan::calcTriNormals(const vector<int> &tris, vector<short> &triNrm)
{
  int i, j, k;

  for (i = 0; i < scans.size(); i++) {
    for (j = 0; j < scans[i].meshes.size(); j++) {
      mmResLevel *mesh = &(scans[i].meshes[j]);
      mesh->triNrm.clear();
      mesh->triNrm.reserve(3 * mesh->num_tris());
      for(k = 0; k < mesh->tris.size(); k +=3) {
	Pnt3 n;
	n = mesh->nrm[mesh->tris[k+0]] +
	    mesh->nrm[mesh->tris[k+1]] +
	    mesh->nrm[mesh->tris[k+2]];
	n /= 3.0;
	n *= 32767;
	mesh->triNrm.push_back(n[0]);
	mesh->triNrm.push_back(n[1]);
	mesh->triNrm.push_back(n[2]);
      }
    }
  }
}
*/

static Random rnd;

bool
MMScan::load_resolution (int iRes)
{
  if (resolutions[iRes].in_memory)
    return true;

  int subSamp = 1 << iRes;

  Progress progress(scans.size(), "%s: build mesh",
		    name.c_str());

  int numTris = 0;

  cerr << name << ": build mesh (~" << resolutions[iRes].abs_resolution
       << ") from " << scans.size() << " fragments: " << flush;

  for (int i = 0; i < scans.size(); i++) {
    progress.update(i);
    mmScanFrag *scan = &scans[0] + i;
    mmResLevel& res = scan->meshes[iRes];
    triangulate(scan, &res, subSamp);
    /*    if (subSamp == 1) {
      make_tstrips(scan, res.tstrips);
      numTris += count_tris(res.tstrips);
    }
    else {
      triangulate(scan, &res, subSamp);
      tris_to_strips(res.vtx.size(), res.tris, res.tstrips);*/
      res.nrm.reserve(scan->meshes[0].nrm.size() / subSamp);
      getVertexNormals (res.vtx, res.tris, false, res.nrm);
      numTris += res.tris.size()/3;
      //    }
  }

  cerr << " done." << endl;
  progress.update(i);

  resolutions[iRes].abs_resolution = numTris;
  resolutions[iRes].in_memory = true;
  return true;
}


void
MMScan::triangulate(mmScanFrag *scan, mmResLevel *res, int subSamp)
{
  int i,j,ii,jj;
  int in1,in2,in3,in4,vin1,vin2,vin3,vin4;
  int count;
  int numStripes = scan->stripes.size();

  // clear the list of triangles
  res->tris.clear();

  // i don't know what ltMin does. kari+curless do it, so i will too
  int ltMin = (numStripes-1) - ((numStripes-1)/subSamp)*subSamp;

  if (scan->indices.size() == 0) calcIndices();
  if (!haveScanDir) calcScanDir();

  // create a list saying whether a vertex is going to be used
  // int *vert_index = new int[vtx.size()];
  // for (i = 0; i < vtx.size(); i++)
  //   vert_index[i] = -1;

  int *vert_remap;

  bool alreadySubSampled = (res->vtx.size() > 0);

  if (subSamp != 1) {
    // see which vertices will be used in the new mesh
    vert_remap = new int[scan->num_vertices(0)];

    if (!alreadySubSampled) {
      res->vtx.reserve(scan->num_vertices(0) / subSamp);
      res->confidence.reserve(scan->meshes[0].confidence.size() / subSamp);
      res->intensity.reserve(scan->meshes[0].intensity.size() / subSamp);
    }
    count = 0;
    for (j = ltMin; j < numStripes; j += subSamp) {
      for (i = 0; i < STRIPE_PTS; i += subSamp) {
	in1 = scan->indices[i + j * STRIPE_PTS];
	if (in1 >= 0) {
	  //	vert_index[in1] = count;
	  vert_remap[in1] = count;
	  if (!alreadySubSampled) {
	    res->vtx.push_back(scan->meshes[0].vtx[in1]);
	    if (scan->meshes[0].confidence.size())
	      res->confidence.push_back(scan->meshes[0].confidence[in1]);
	    if (scan->meshes[0].intensity.size())
	      res->intensity.push_back(scan->meshes[0].intensity[in1]);
	  }
	  count++;
	}
      }
    }
  }
  else count = scan->num_vertices(0);

  //if (subSamp == 1) fprintf(stderr, "%d verts... ", count);

  int max_tris = count * 6;
  res->tris.reserve(max_tris);

  // create the triangles
  for (j = ltMin; j < numStripes - subSamp; j += subSamp) {

    //    if (scan->stripes[j].scanDir != scan->stripes[j+subSamp].scanDir) {
    //      cout << "not triangulating across a gap" << endl;
    //      continue;
    //    }

    for (i = 0; i < STRIPE_PTS - subSamp; i += subSamp) {

      ii = (i + subSamp) % STRIPE_PTS;
      jj = (j + subSamp) % numStripes;

      // count the number of good vertices
      // 2 3
      // 1 4
      in1 = scan->indices[ i +  j * STRIPE_PTS];
      in2 = scan->indices[ i + jj * STRIPE_PTS];
      in3 = scan->indices[ii + jj * STRIPE_PTS];
      in4 = scan->indices[ii +  j * STRIPE_PTS];
      count = (in1 >= 0) + (in2 >= 0) + (in3 >= 0) + (in4 >=0);

      // note about vin_x vs. in_x :
      // vin's should be used if new vertex lists are being generated.
      // on subsamp == 1, we aren't remapping. otherwise, we are.

      if (in1 >= 0) {
	if (subSamp == 1) vin1 = in1;
	  else vin1 = vert_remap[in1];
      }
      if (in2 >= 0) {
	if (subSamp == 1) vin2 = in2;
	  else vin2 = vert_remap[in2];
      }
      if (in3 >= 0) {
	if (subSamp == 1) vin3 = in3;
	  else vin3 = vert_remap[in3];
      }
      if (in4 >= 0) {
	if (subSamp == 1) vin4 = in4;
	  else vin4 = vert_remap[in4];
      }

      // these are squared distances, let's not forget, scaled by subSamp
      float maxLen1 = 1.0 * 1.0 * subSamp * subSamp;
      float maxLen2 = 1.25 * 1.25 * subSamp * subSamp;

      if (count == 4) {	// all 4 vertices okay, so make 2 tris

	bool badTri = false;

	// check to make sure that stripe edges aren't too long
	if (dist2(res->vtx[vin2], res->vtx[vin3]) > maxLen1) badTri = true;
	if (dist2(res->vtx[vin1], res->vtx[vin4]) > maxLen1) badTri = true;
	// check to make sure that inter-stripe distances aren't too long
	if (dist2(res->vtx[vin1], res->vtx[vin2]) > maxLen2) badTri = true;
	if (dist2(res->vtx[vin3], res->vtx[vin4]) > maxLen2) badTri = true;

	if (!badTri) {

	  // compute lengths of cross-edges
	  float len1 = dist(res->vtx[vin1], res->vtx[vin3]);
	  float len2 = dist(res->vtx[vin2], res->vtx[vin4]);

	  if (len1 < len2) {
	    addTri(res->tris, vin2, vin1, vin3, scan->stripes[j].scanDir);
	    addTri(res->tris, vin1, vin4, vin3, scan->stripes[j].scanDir);
	  } else {
	    addTri(res->tris, vin2, vin1, vin4, scan->stripes[j].scanDir);
	    addTri(res->tris, vin2, vin4, vin3, scan->stripes[j].scanDir);
	  }
	}
      }
      else if (count == 3) { // only 3 vertices okay, so make 1 tri
	if        (in1 == -1) {
	  if ((dist2(res->vtx[vin2], res->vtx[vin3]) < maxLen1) &&
	      (dist2(res->vtx[vin3], res->vtx[vin4]) < maxLen2))
	  addTri(res->tris, vin2, vin4, vin3, scan->stripes[j].scanDir);
	} else if (in2 == -1) {
	  if ((dist2(res->vtx[vin1], res->vtx[vin4]) < maxLen1) &&
	      (dist2(res->vtx[vin3], res->vtx[vin4]) < maxLen2))
	  addTri(res->tris, vin1, vin4, vin3, scan->stripes[j].scanDir);
	} else if (in3 == -1) {
	  if ((dist2(res->vtx[vin1], res->vtx[vin4]) < maxLen1) &&
	      (dist2(res->vtx[vin1], res->vtx[vin2]) < maxLen2))
	  addTri(res->tris, vin2, vin1, vin4, scan->stripes[j].scanDir);
	} else { // in4 == -1
	  if ((dist2(res->vtx[vin2], res->vtx[vin3]) < maxLen1) &&
	      (dist2(res->vtx[vin1], res->vtx[vin2]) < maxLen2))
	  addTri(res->tris, vin2, vin1, vin3, scan->stripes[j].scanDir);
	}
      }
    }
  }


  // old solution before properly using vin's
  /*
  if (subSamp != 1) {
    // re-map triangle indices

    for (i = 0; i < res->tris.size(); i++)
      res->tris[i] = vert_remap[res->tris[i]];
  }
  */

  if (subSamp != 1)
    delete[] vert_remap;

  // I originally called this with the last 2 parameters: 50, 8
  // but that seems to be quite incorrect. changed on 10/19/98
  // don't call anymore
  //  remove_stepedges(res->vtx, res->tris, 4, 90);

  cerr << "." << flush;
}

void
MMScan::make_tstrips(mmScanFrag *scan, mmResLevel *res, int subSamp)
{
  int i,j,ii,jj;
  int in1,in2,in3,in4;
  int vin1, vin2, vin3, vin4;
  int count;
  int numStripes = scan->stripes.size();
  int numStrips = 0;
  bool newStrip = true;
  int inc, iStart, iFinish;

  res->tstrips.clear();

  // i don't know what ltMin does. kari+curless do it, so i will too
  int ltMin = (numStripes-1) - ((numStripes-1)/subSamp)*subSamp;

  // this flag is used to differentiate the 2 kinds of diagonals which occur
  bool whichDiag = true;

  if (scan->indices.size() == 0) calcIndices();
  if (!haveScanDir) calcScanDir();


  bool alreadySubSampled = (res->vtx.size() > 0);

  int *vert_remap;

  if (subSamp != 1) {
    // see which vertices will be used in the new mesh
    vert_remap = new int[scan->num_vertices(0)];
    if (!alreadySubSampled) {
      res->vtx.reserve(scan->num_vertices(0) / subSamp);
      res->confidence.reserve(scan->meshes[0].confidence.size() / subSamp);
      res->intensity.reserve(scan->meshes[0].intensity.size() / subSamp);
    }
    count = 0;
    for (j = ltMin; j < numStripes; j += subSamp) {
      for (i = 0; i < STRIPE_PTS; i += subSamp) {
	in1 = scan->indices[i + j * STRIPE_PTS];
	if (in1 >= 0) {
	  //	vert_index[in1] = count;
	  vert_remap[in1] = count;
	  if (!alreadySubSampled) {
	    res->vtx.push_back(scan->meshes[0].vtx[in1]);
	    if (scan->meshes[0].confidence.size())
	      res->confidence.push_back(scan->meshes[0].confidence[in1]);
	    if (scan->meshes[0].intensity.size())
	      res->intensity.push_back(scan->meshes[0].intensity[in1]);
	  }
	  count++;
	}
      }
    }
  }
  else count = scan->num_vertices(0);

  // usually about 2 * numVerts triangles, apply avg. length of 10
  int max_strips = (count * 2) * 13 / 10;
  res->tstrips.reserve(max_strips);

  cout << numStripes << " stripes, " << count << " vertices." << endl;

  // create the triangle strips
  for (j = ltMin; j < numStripes - subSamp; j+= subSamp) {

    int numSteps = (STRIPE_PTS / subSamp);

    if (scan->stripes[j].scanDir == 1) {
      // normal scan direction
      inc = subSamp;
      iStart = 0;
      iFinish = (numSteps - 1) * subSamp;
    }
    else {
      // reverse scan direction
      inc = -1 * subSamp;
      iStart = (numSteps - 1) * subSamp;
      iFinish = 0;
    }
    for (i = iStart; i != iFinish; i+= inc) {

      ii = (i + inc) % STRIPE_PTS;
      jj = (j + subSamp) % numStripes;

      // count the number of good vertices
      // 2 3
      // 1 4
      in1 = scan->indices[ i +  j * STRIPE_PTS];
      in2 = scan->indices[ i + jj * STRIPE_PTS];
      in3 = scan->indices[ii + jj * STRIPE_PTS];
      in4 = scan->indices[ii +  j * STRIPE_PTS];
      count = (in1 >= 0) + (in2 >= 0) + (in3 >= 0) + (in4 >=0);

      // note about vin_x vs. in_x :
      // vin's should be used if new vertex lists are being generated.
      // on subsamp == 1, we aren't remapping. otherwise, we are.

      if (in1 >= 0) {
	if (subSamp == 1) vin1 = in1;
	  else vin1 = vert_remap[in1];
      }
      if (in2 >= 0) {
	if (subSamp == 1) vin2 = in2;
	  else vin2 = vert_remap[in2];
      }
      if (in3 >= 0) {
	if (subSamp == 1) vin3 = in3;
	  else vin3 = vert_remap[in3];
      }
      if (in4 >= 0) {
	if (subSamp == 1) vin4 = in4;
	  else vin4 = vert_remap[in4];
      }

      // squared distances; don't forget
      float maxLen1 = 1.0 * 1.0 * subSamp * subSamp;
      float maxLen2 = 1.25 * 1.25 * subSamp * subSamp;

      if (count == 4) {	// all 4 vertices okay, so make 2 tris

	bool badTri = false;

	// note: these are crude checks... 1mm isn't always what we want
	// check to make sure that stripe edges aren't too long
	if (dist2(res->vtx[vin2], res->vtx[vin3]) > maxLen1) badTri = true;
	if (dist2(res->vtx[vin1], res->vtx[vin4]) > maxLen1) badTri = true;
	// check to make sure that inter-stripe distances aren't too long
	if (dist2(res->vtx[vin1], res->vtx[vin2]) > maxLen2) badTri = true;
	if (dist2(res->vtx[vin3], res->vtx[vin4]) > maxLen2) badTri = true;

	if (badTri) {
	    if (!newStrip) {
	      res->tstrips.push_back(-1);
	      newStrip = true;
	      numStrips++;
	    }
	}
	else {
	  // compute lengths of cross-edges
	  float len1 = dist2(res->vtx[vin1], res->vtx[vin3]);
	  float len2 = dist2(res->vtx[vin2], res->vtx[vin4]);

	  if (len1 < len2) {
	    // entering the normal-diagonal zone
	    // (meaning diagonal lies between 1 and 3, instead of 2 and 4)

	    if (!(whichDiag || newStrip)) {
	      // if we're in the middle of a reverse-diagonal strip...
	      // end the strip
	      res->tstrips.push_back(-1);
	      newStrip = true;
	      numStrips++;
	    }
	    if (newStrip) {
	      // start a normal-diagonal strip
	      res->tstrips.push_back(vin2);
	      res->tstrips.push_back(vin1);
	      whichDiag = true;
	      newStrip = false;
	    }
	    // always add the two newest indices
	    res->tstrips.push_back(vin3);
	    res->tstrips.push_back(vin4);

	  } else {
	    // we're in the reverse-diagonal zone now

	    // if we're starting a new strip or ending a normal-diagonal,
	    // we add a single triangle and begin again
	    if (whichDiag || newStrip) {
	      if (newStrip) {
		// 1st 2 vertices of triangle needed for new strip
		res->tstrips.push_back(vin2);
		res->tstrips.push_back(vin1);
	      }
	      res->tstrips.push_back(vin4);
	      res->tstrips.push_back(-1);
	      // end the strip and the triangle, start the new strip
	      res->tstrips.push_back(vin2);
	      numStrips++;
	      newStrip = false;
	      whichDiag = false;
	    }

	    // always add the two newest indices
	    res->tstrips.push_back(vin4);
	    res->tstrips.push_back(vin3);
	  }
	}
      }
      else if (count == 3) { // only 3 vertices okay, so make 1 tri
	if (in1 == -1) {
	  // longedge check:
	  if ((dist2(res->vtx[vin2], res->vtx[vin3]) > maxLen1) ||
	      (dist2(res->vtx[vin3], res->vtx[vin4]) > maxLen2)) {
	    if (!newStrip) {
	      res->tstrips.push_back(-1);
	      newStrip = true;
	      numStrips++;
	    }
	  }
	  else {
	    if (!newStrip) {
	      res->tstrips.push_back(-1);
	      numStrips++;
	    }
	    res->tstrips.push_back(vin2);
	    res->tstrips.push_back(vin4);
	    res->tstrips.push_back(vin3);
	    newStrip = false;
	    whichDiag = false;
	  }
	}
	else if (in2 == -1) {
	  // longedge check:
	  if ((dist2(res->vtx[vin1], res->vtx[vin4]) > maxLen1) ||
	      (dist2(res->vtx[vin3], res->vtx[vin4]) > maxLen2)) {
	    if (!newStrip) {
	      res->tstrips.push_back(-1);
	      newStrip = true;
	      numStrips++;
	    }
	  }
	  else {
	    if (!newStrip) {
	      res->tstrips.push_back(-1);
	      numStrips++;
	    }
	    res->tstrips.push_back(vin1);
	    res->tstrips.push_back(vin4);
	    res->tstrips.push_back(vin3);
	    res->tstrips.push_back(-1);
	    numStrips++;
	    newStrip = true;
	    whichDiag = true;
	  }
	}
	else if (in3 == -1) {
	  // longedge check:
	  if ((dist2(res->vtx[vin1], res->vtx[vin4]) > maxLen1) ||
	      (dist2(res->vtx[vin1], res->vtx[vin2]) > maxLen2)) {
	    if (!newStrip) {
	      res->tstrips.push_back(-1);
	      newStrip = true;
	      numStrips++;
	    }
	  }
	  else {
	    if (newStrip) {
	      res->tstrips.push_back(vin2);
	      res->tstrips.push_back(vin1);
	    }
	    res->tstrips.push_back(vin4);
	    res->tstrips.push_back(-1);
	    numStrips++;
	    newStrip = true;
	    whichDiag = true;
	  }
	}
	else { // in4 == -1
	  // longedge check:
	  if ((dist2(res->vtx[vin2], res->vtx[vin3]) > maxLen1) ||
	      (dist2(res->vtx[vin1], res->vtx[vin2]) > maxLen2)) {
	    if (!newStrip) {
	      res->tstrips.push_back(-1);
	      newStrip = true;
	      numStrips++;
	    }
	  }
	  else {
	    if (newStrip) {
	      res->tstrips.push_back(vin2);
	      res->tstrips.push_back(vin1);
	    }
	    res->tstrips.push_back(vin3);
	    res->tstrips.push_back(-1);
	    numStrips++;
	    newStrip = true;
	    whichDiag = true;
	  }
	}
      }
      else if ((count == 2) || (count == 1)) {
	// we need to end any current strips because there is going to be
	// a hole in this row... fucking bug that now shall finally die
	if (!newStrip) {
	  res->tstrips.push_back(-1);
	  newStrip = true;
	  numStrips++;
	}
      }
    }

    // at end of a stripe, so end any strip
    if (!newStrip) {
      res->tstrips.push_back(-1);
      newStrip = true;
      numStrips++;
    }

  }

  // if we haven't ended the current strip, end it
  if (!newStrip) {
    res->tstrips.push_back(-1);
    newStrip = true;
    numStrips++;
  }
  /*
  getVertexNormals (res->vtx, tstrips, true, res->nrm);
  */
  if (numStrips == 0) {
    cerr << "No triangle strips created." << endl;
    res->tstrips.push_back(-1);
  }
  else {
    fprintf(stderr, "%d t-strips (", numStrips);
    fprintf(stderr, "%5.2f avg length)\n",
	    (res->tstrips.size() - (3 * numStrips)) / (double) numStrips);
  }

  if (subSamp != 1) delete[] vert_remap;
}

void
MMScan::subsample_points(float rate, vector<Pnt3> &p, vector<Pnt3> &n)
{
  int nv = num_vertices(curr_res);
  int totalNum = (int)(rate * nv);

  if (totalNum > nv) totalNum = nv;

  p.clear(); p.reserve(totalNum);
  n.clear(); n.reserve(totalNum);

  for (int i=0; i < scans.size(); i++) {

    mmResLevel& res = scans[i].meshes[curr_res];
    int nv = res.vtx.size();
    int num = (int)(rate * nv);
    int end = nv;
    for (int j = 0; j < end; j++) {
      if (rnd(nv) <= num) {
	p.push_back(res.vtx[j]);                  // save point
	pushNormalAsPnt3 (n, res.nrm.begin(), j); // and norm
	num--;
      }
      nv--;
    }
    assert(num == 0);
    if (p.size() > totalNum)
      printf("Selected too many points in the MMScan subsample proc.\n");
  }
}

MMScan::mergedRegData&
MMScan::getRegData (void)
{
  mergedRegData& reg = regData[curr_res];
  if (!isDirty_mem && (reg.vtx.size() > 0)) return reg;

  cout << "Making new global vertex array" << endl;
  int numVerts = num_vertices(curr_res);
  reg.vtx.clear(); reg.vtx.reserve(numVerts);
  reg.nrm.clear(); reg.nrm.reserve(numVerts * 3);
  reg.tris.clear(); reg.tris.reserve(num_tris(curr_res) * 3);

  // used to reindex triangle vertices in the global array
  int reIndexFactor;

  for (int i = 0; i < scans.size(); i++) {
    reIndexFactor = reg.vtx.size();
    mmResLevel& res = scans[i].meshes[curr_res];
    for (int j = 0; j < res.vtx.size(); j++) {
      reg.vtx.push_back(res.vtx[j]);
      reg.nrm.push_back(res.nrm[3 * j + 0]);
      reg.nrm.push_back(res.nrm[3 * j + 1]);
      reg.nrm.push_back(res.nrm[3 * j + 2]);
    }
    for (j = 0; j < res.tris.size(); j++)
      reg.tris.push_back(res.tris[j] + reIndexFactor);
  }

  mark_boundary (reg);
  delete kdtree[curr_res];

  isDirty_mem = false;
  return reg;
}

void
MMScan::mark_boundary(mergedRegData& reg)
{
  if (!isDirty_mem && (reg.boundary.size() > 0)) return;
  reg.boundary.clear();

  int nVtx = reg.vtx.size();
  reg.boundary.reserve(nVtx);
  for (int j = 0; j < nVtx; j++)
    reg.boundary.push_back(0);

  ::mark_boundary_verts(reg.boundary, reg.tris);
  cerr << "Marked boundary vertices for " << get_name() << endl;
  reg.tris.clear();     // not needed any more

  isDirty_mem = false;
}


KDindtree*
MMScan::get_kdtree()
{
  if ((!isDirty_mem) && (kdtree[curr_res] != NULL))
    return kdtree[curr_res];

  // necessary but evil step here: create global list of all
  // vertices and normals and triangles for this res,
  mergedRegData& reg = getRegData();

  delete kdtree[curr_res];
  kdtree[curr_res] = CreateKDindtree (reg.vtx.begin(),
				      reg.nrm.begin(),
				      reg.vtx.size());

  isDirty_mem = false;

  return kdtree[curr_res];
}


bool
MMScan::closest_point(const Pnt3 &p, const Pnt3 &n,
		      Pnt3 &cp, Pnt3 &cn,
		      float thr, bool bdry_ok)
{
  int ind, ans;
  KDindtree* tree = get_kdtree();
  mergedRegData& reg = getRegData();
  ans = tree->search(reg.vtx.begin(), reg.nrm.begin(), p, n, ind, thr);

  if (ans) {
    if (bdry_ok == 0) {
      // disallow closest points that are on the mesh boundary
      if (reg.boundary[ind]) return 0;
    }
    cp = reg.vtx[ind];
    cn = reg.nrm[ind];
  }

  return ans;
}


void
MMScan::update_res_ctrl()
{
  for (int i = 0; i < resolutions.size(); i++) {
    resolutions[i].abs_resolution = num_tris (i);
  }
}


bool
MMScan::filter_inplace(const VertexFilter &filter)
{
  int i,j,k;
// STL Update
  vector<mmScanFrag>::iterator scan;
  bool changedScan = false;

  // #1: loop through all scans
  for (j = 0; j < scans.size(); j++) {
// STL Update
    scan = scans.begin() + j;
    cout << "looking at scan #" << j << endl;
    if (scan->isVisible) {
      changedScan = false;
      int maxVerts = scan->num_vertices(0);
      int *vert_remap = new int[maxVerts];
      for (i = 0; i < maxVerts; i++)
	vert_remap[i] = -1;

      int count = 0, vtxIndex = 0;
// STL Update
      vector<mmStripeInfo>::iterator stripe;
      for (k = 0; k < scan->stripes.size(); k++) {
// STL Update
	stripe = scan->stripes.begin() + k;
	for (i = 0; i < STRIPE_PTS; i++) {
	  if (isValidPoint(j, k, i)) {
	    if (filter.accept(scan->meshes[0].vtx[vtxIndex])) {
	      vert_remap[vtxIndex] = count;
	      count++;
	      vtxIndex++;
	    }
	    else {
	      stripe->validMap[i/8] &= ~(1 << (i % 8));
	      stripe->numPoints--;
	      vtxIndex++;
	      changedScan = true;
	    }
	  }
	}
	if (stripe->numPoints > 0) {
	  int newIndex = vert_remap[stripe->startIdx];
	  while (newIndex == -1) {
	    stripe->startIdx++;
	    newIndex = vert_remap[stripe->startIdx];
	  }
	  stripe->startIdx = newIndex;
	}
	else {
	  scan->stripes.erase(stripe);
	  k--;
	}
      }

      // either ditch the scan if it's now empty, or clip its resolutions
      // if it has been changed at all

      if (scan->stripes.size() == 0) {
	cout << "scan #" << j << " didn't make it" << endl;
	scans.erase(scan);
	j--;
      }
      else if (changedScan) {
// STL Update
	vector<mmResLevel>::iterator res;
	for (res = scan->meshes.begin(); res != scan->meshes.end(); res++) {

	  if (res != scan->meshes.begin()) {
	    cout << "res other than the first one" << endl;
	    continue;
	    // do something to find out about remapping
	  }

	  if (count == 0) {
	    // no vertices made it... only applicable in lower res's i guess
	    cout << "count equalled 0; scan bites the dust " << endl;
	    scan->meshes.erase(res);
	    res--;
	    continue;
	  }

	  // delete the tstrips from the res since they are hard to clip
	  res->tstrips.clear();

	  vector<Pnt3> newVerts;
	  vector<float> confidence;
	  vector<uchar> intensity;
	  vector<short> nrm;

	  newVerts.reserve(count);
	  if (res->confidence.size())
	    confidence.reserve(count);
	  if (res->intensity.size())
	    intensity.reserve(count);
	  if (res->nrm.size())
	    nrm.reserve(3 * count);

	  for (i = 0; i < res->vtx.size(); i++) {
	    if (vert_remap[i] >= 0) {
	      newVerts.push_back(res->vtx[i]);
	      if (res->confidence.size()) {
		confidence.push_back(res->confidence[i]);
	      }
	      if (res->intensity.size()) {
		intensity.push_back(res->intensity[i]);
	      }
	      if (res->nrm.size()) {
		nrm.push_back(res->nrm[i * 3 + 0]);
		nrm.push_back(res->nrm[i * 3 + 1]);
		nrm.push_back(res->nrm[i * 3 + 2]);
	      }
	    }
	  }

	  res->vtx.clear(); res->vtx = newVerts; newVerts.clear();
	  res->confidence.clear(); res->confidence = confidence;
	    confidence.clear();
	  res->intensity.clear(); res->intensity = intensity;
	    intensity.clear();
	  res->nrm.clear(); res->nrm = nrm; nrm.clear();

	  cout << "handling triangles " << endl;

	  vector<int> newTris;

	  // assume 2 times as many tris as points
	  newTris.reserve(2 * count * 3);

	  for (i = 0; i < res->tris.size(); i += 3) {
	    int a, b, c;
	    a = vert_remap[res->tris[i+0]];
	    b = vert_remap[res->tris[i+1]];
	    c = vert_remap[res->tris[i+2]];
	    if ((a >= 0) && (b >= 0) &&	(c >= 0)) {
	      newTris.push_back(a);
	      newTris.push_back(b);
	      newTris.push_back(c);
	    }
	  }

	  res->tris.clear(); res->tris = newTris; newTris.clear();
	  delete[] vert_remap;
	}
      }
    }
  }
  update_res_ctrl();
  isDirty_mem = true;
  isDirty_disk = true;
  // important that we rebuild indices
  calcIndices();
  return true;
}



// copy the part of the mesh that is in the clip box
// into a new mmscan

RigidScan*
MMScan::filtered_copy(const VertexFilter& filter)
{
  int i, j, k;
  MMScan* newMesh = new MMScan;

  assert(newMesh != NULL);

  newMesh->setXform(getXform());
  newMesh->haveScanDir = false;

  // #1: loop through all scans
  for (j = 0; j < scans.size(); j++) {

    if (scans[j].isVisible) {

      mmScanFrag newScan;
      memcpy(newScan.falseColor, scans[j].falseColor, 3);

      // delete the auto-created mesh for our new scan
      newScan.meshes.pop_back();

      int vertIndex = 0;
      int vertCount = 0;
      vector<int> newIndices;

      // #2a: loop through each stripe in the scan,
      // to figure out which verts
      // make it and to generate valid stripe information
      // for the new mmscan

      for (k = 0; k < scans[j].stripes.size(); k++) {
	mmStripeInfo stripe;
	for (i = 0; i < 40; i++)
	  stripe.validMap[i] = '\0';

	for (i = 0; i < STRIPE_PTS; i++) {
	  if (isValidPoint(j, k, i)) {
	    if (filter.accept (scans[j].meshes[0].vtx[vertIndex])) {
	      if (stripe.numPoints == 0) stripe.startIdx = vertCount;
	      stripe.numPoints++;
	      stripe.validMap[i/8] |= (1 << (i % 8));

	      newIndices.push_back(1);
	      vertCount++;
	    }
	    else newIndices.push_back(-1);
	    vertIndex++;
	  }
	}

	if (stripe.numPoints > 0) {
	  stripe.sensorVec = scans[j].stripes[k].sensorVec;
	  stripe.scanDir = 0;
	  newScan.viewDir += stripe.scanDir;
	  newScan.stripes.push_back(stripe);
	}
      }

      if (newScan.stripes.size() > 0) {

	// average and normalize current scan's view direction

	newScan.viewDir /= (double)newScan.stripes.size();
	newScan.viewDir.normalize();

	// #2b: BEGIN looping through each mesh in the current scan.
	// We're looking for resolutions that have their own vertices
	// and tris based on those vertices.
	// We basically clip and treat these exactly like the
	// generic scan clips each of its meshes.

	for (k = 0; k < scans[j].meshes.size(); k++) {
	  mmResLevel newRes;
	  mmResLevel *oldRes = &(scans[j].meshes[k]);

	  int triCount = 0;

	  // #3a: figure out which verts make the cut, except on the first
	  // mesh where we already know

	  if (k > 0) {
	    vertCount = 0;
	    for (i = 0; i < oldRes->vtx.size(); i++) {
	      if (filter.accept (oldRes->vtx[i])) {
		newIndices.push_back(1);
		vertCount++;
	      }
	      else newIndices.push_back(-1);
	    }
	  }

	  // #3b: figure out which tris make the cut

	  for (i = 0; i < oldRes->tris.size(); i += 3) {
	    if (newIndices[oldRes->tris[i  ]] >= 0 &&
		newIndices[oldRes->tris[i+1]] >= 0 &&
		newIndices[oldRes->tris[i+2]] >= 0) {
	      triCount++;
	    }
	  }

	  // #3c: copy the surviving vertices
	  int count = 0;
	  newRes.vtx.reserve (vertCount);
	  if (oldRes->nrm.size())
	    newRes.nrm.reserve (vertCount);
	  if (oldRes->confidence.size())
	    newRes.confidence.reserve (vertCount);
	  if (oldRes->intensity.size())
	    newRes.intensity.reserve (vertCount);

	  for (i = 0; i < oldRes->vtx.size(); i++) {
	    if (newIndices[i] > 0) {
	      newIndices[i] = count;

	      newRes.vtx.push_back(oldRes->vtx[i]);
	      if (oldRes->nrm.size()) {
		newRes.nrm.push_back(oldRes->nrm[i * 3 + 0]);
		newRes.nrm.push_back(oldRes->nrm[i * 3 + 1]);
		newRes.nrm.push_back(oldRes->nrm[i * 3 + 2]);
	      }
	      if (oldRes->confidence.size())
		newRes.confidence.push_back(oldRes->confidence[i]);
	      if (oldRes->intensity.size())
		newRes.intensity.push_back(oldRes->intensity[i]);

	      count++;
	    }
	  }

	  // #3d: copy and reindex the surviving faces
	  count = 0;
	  newRes.tris.reserve (triCount * 3);
	  for (i = 0; i < oldRes->tris.size(); i += 3) {
	    if (newIndices[oldRes->tris[i  ]] >= 0 &&
		newIndices[oldRes->tris[i+1]] >= 0 &&
		newIndices[oldRes->tris[i+2]] >= 0) {

	      for (int z = 0; z < 3; z++)
		newRes.tris.push_back(-1);
	      newRes.tris[count  ] = newIndices[oldRes->tris[i  ]];
	      newRes.tris[count+1] = newIndices[oldRes->tris[i+1]];
	      newRes.tris[count+2] = newIndices[oldRes->tris[i+2]];

	      count += 3;
	    }
	  }

	  // we're done with this resolution
	  newScan.meshes.push_back(newRes);
	  newIndices.clear();

	} // end "for (k = 0; k < meshes.size(); k++)"

	newMesh->scans.push_back(newScan);

      } // end "if (numStripes > 0)"

    } // end "if isVisible"
    else {

      // don't copy the scan... this line copies it if i change my mind
      // newMesh->scans.push_back(scans[j]);
    }

  } // end scan loop

  // seems like a bad idea, but we'll see

  if (newMesh->scans.size() == 0) return NULL;

  // finalize mmscan details

  for (i = 0; i < newMesh->scans[0].meshes.size(); i++) {
    int count = newMesh->num_tris(i);
    cerr << "mesh num " << i << " has " << count << " tris." << endl;
    char info[20];
    sprintf (info, ".clip%d", count);
    crope fragName (get_basename() + info);
    newMesh->insert_resolution(count, fragName, true, true);
  }

  newMesh->curr_res = curr_res;

  newMesh->isDirty_mem = newMesh->isDirty_disk = true;
  newMesh->computeBBox();

  crope clipName(get_basename());
  char info[20];
  sprintf(info, "clip.%d.mms", newMesh->num_vertices());
  clipName += info;
  newMesh->set_name (clipName);

  return newMesh;
}

//Apply filer... to every point in the highest resolution version
//of the file...
bool
MMScan::filter_vertices (const VertexFilter& filter, vector<Pnt3>& p)
{
// STL Update
  vector<mmScanFrag>::iterator endScanFrag = scans.end();
    //for every mmScanFrag in scans vector
    for(vector<mmScanFrag>::iterator curScanFrag = scans.begin();
	curScanFrag < endScanFrag; curScanFrag++)
    {

      //get the current points vector (full resolution)
      vector<Pnt3> *curPtVector;
      curPtVector = &(curScanFrag->meshes[0].vtx);
      //for every point apply the function
// STL Update
      vector<Pnt3>::iterator vtxend = curPtVector->end();
      for(vector<Pnt3>::iterator pnt = curPtVector->begin(); pnt < vtxend; pnt++)
	{
	  if(filter.accept (*pnt))
	    p.push_back(*pnt);
	}

    }
  return true;
}
/* OLD_VERSION FROM GENERIC_SCAN
bool
MMScan::filter_vertices (const VertexFilter& filter, vector<Pnt3>& p)
{
  Mesh* mesh = currentMesh();

  Pnt3* vtxend = mesh->vtx.end();
  for (Pnt3* pnt = mesh->vtx.begin(); pnt < vtxend; pnt++) {
    if (filter.accept (*pnt))
      p.push_back (*pnt);
  }

  return true;
}
*/

/*
// featherBoundary()
//
// takes a boundary-like array, containing 1 at boundary vertices, 2 at their
// adjacent vertices, etc.
// returns that same array, expanded to one more level

static void
featherBoundary(vector<char> &boundPlus, int level, vector<int> &tri_inds)
{
  for (int i = 0; i < tri_inds.size(); i+=3) {
    if ((boundPlus[tri_inds[i+0]] == level) ||
	(boundPlus[tri_inds[i+1]] == level) ||
	(boundPlus[tri_inds[i+2]] == level)) {
      for (int j = 0; j < 3; j++) {
	if (boundPlus[tri_inds[i+j]] == 0)
	  boundPlus[tri_inds[i+j]] = level + 1;
      }
    }
  }
}
*/

#define FEASTEPS 15
#define FEARANGE 1.0
#define FEADIST  5.0

void
MMScan::calcConfidence()
{

  for (int k = 0; k < scans.size(); k++) {
    /*
    // method one: only connectivity
    vector<char> boundPlus = scans[k].boundary;

    for (int i = 1; i < FEASTEPS; i++)
      featherBoundary(boundPlus, i, scans[k].meshes[0].tris);

    scans[k].meshes[0].confidence.clear();
    scans[k].meshes[0].confidence.reserve(scans[k].num_vertices());

    for (i = 0; i < scans[k].num_vertices(); i++) {
      float conf = 1.0;
      if (boundPlus[i] > 0)
	conf = (1.0 - FEARANGE) + (boundPlus[i] / (float)FEASTEPS) * FEARANGE;

      scans[k].meshes[0].confidence.push_back(conf);
    }
    boundPlus.clear();
    */
    // method two: kari

    vector<float> distances;

    cout << "calculating distances" << endl;
    distance_from_boundary(distances, scans[k].meshes[0].vtx,
			   scans[k].meshes[0].tris);
    cout << "done" << endl << endl;

    mmResLevel& scan = scans[k].meshes[0];
    scan.confidence.clear();
    scan.confidence.reserve(scan.num_vertices());

    for (int i = 0; i < scan.num_vertices(); i++) {
      float conf;
      if (distances[i] < FEADIST)
	conf = (distances[i] / FEADIST);
      else conf = 1.0;
      scan.confidence.push_back(conf);
    }
  }
}

// check whether some vertices in vtx are not being used in tri
// if so, remove them and also adjust the tri indices
// stolen from TriMeshUtils.cc and modified for stripe purposes
static void
remove_unused_vtxs(vector<Pnt3> &vtx,
		   vector<int>  &tri, vector<int> &vtx_ind)
{
  // assume vtx_ind is initialized to -1's

  // march through the triangles
  // and mark the vertices that are actually used
  int n = tri.size();
  for (int i=0; i<n; i+=3) {
    vtx_ind[tri[i+0]] = tri[i+0];
    vtx_ind[tri[i+1]] = tri[i+1];
    vtx_ind[tri[i+2]] = tri[i+2];
  }

  // remove the vertices that were not marked,
  // also keep tab on how the indices change
  int cnt = 0;
  n = vtx.size();
  for (i=0; i<n; i++) {
    if (vtx_ind[i] != -1) {
      vtx_ind[i] = cnt;
      vtx[cnt] = vtx[i];
      cnt++;
    }
  }
// STL Update
  vtx.erase(vtx.begin() + cnt, vtx.end());
  // march through triangles and correct the indices
  n = tri.size();
  for (i=0; i<n; i+=3) {
    tri[i+0] = vtx_ind[tri[i+0]];
    tri[i+1] = vtx_ind[tri[i+1]];
    tri[i+2] = vtx_ind[tri[i+2]];
    assert(tri[i+0] >=0 && tri[i+0] < vtx.size());
    assert(tri[i+1] >=0 && tri[i+1] < vtx.size());
    assert(tri[i+2] >=0 && tri[i+2] < vtx.size());
  }
}
/*
void
MMScan::filter_unused_vtx(mmScanFrag *scan)
{
  int numVerts = scan->num_vertices();
  int numStripes = scan->num_stripes();

  vector<int> vtx_ind(numVerts, -1);
  remove_unused_vtxs(scan->meshes[0]->vtx, scan->meshes[0]->tris, vtx_ind);

  int numChecked = 0, numKept = 0, expected = 0;
  for(int i = 0; i < numStripes; i++) {
    mmStripeInfo *stripe = scan->stripes + i;
    stripe->startIdx = numKept;
    for (int j = 0; j < STRIPE_PTS; j++) {
      if (scan->stripes[i].validMap[j / 8] & (1 << (j % 8))) {
	// found a valid point in the map... check to see if it died
	if (vtx_ind[numChecked] != expected) {
	      stripe->validMap[j/8] &= ~(1 << (j % 8));


}
*/

/*
void
MMScan::drawSensorVecs(vector<int> &tris)

  // draws a triangle at every 30th point in the mesh, where the triangle is
  // oriented towards the sensor vector at that point
{
  Pnt3 perp, p1, p2, dir;

  for(int i = 0; i < scans.size(); i++) {
    // loop through all scans
    int count = 0;

    for(int j = 0; j < scans[i].stripes.size(); j++) {
      for(int k = 0; k < STRIPE_PTS; k++) {
	if (isValidPoint(i,j,k)) {
	  if (count % 30 == 0) {
	    dir = scans[i].stripes[j].sensorVec;
	    perp.set(1 * dir[1], -1 * dir[0], 0.0);
	    perp.normalize();
	    perp /= 5.0;
	    p2 = scans[i].meshes[0].vtx[count] + dir + perp;
	    p1 = scans[i].meshes[0].vtx[count] + dir - perp;
	    scans[i].meshes[0].vtx.push_back(p2);
	    scans[i].meshes[0].vtx.push_back(p1);
	    Pnt3 newnrm = cross(perp, dir).normalize();
	    newnrm *= 32767;
	    scans[i].meshes[0].nrm.push_back(newnrm[0]);
	    scans[i].meshes[0].nrm.push_back(newnrm[1]);
	    scans[i].meshes[0].nrm.push_back(newnrm[2]);
	    scans[i].meshes[0].nrm.push_back(newnrm[0]);
	    scans[i].meshes[0].nrm.push_back(newnrm[1]);
	    scans[i].meshes[0].nrm.push_back(newnrm[2]);
	    scans[i].meshes[0].tris.push_back(count);
	    scans[i].meshes[0].tris.push_back(scans[i].num_vertices(0)-2);
	    scans[i].meshes[0].tris.push_back(scans[i].num_vertices(0)-1);
	  }
	  count++;
	}
      }
    }
  }
}
*/

////////////////////////////////////////////
// UTILITY I/O FUNCTIONS
////////////////////////////////////////////


// --------------------------
// struct mmIO
//    o  used to organize data for input and output to CTA files

typedef struct {
  double vtx[3];
  double nrm[3];
  int inten;       // these two ints are just used to capture 8 bytes
  int mode;        // all of the info seems to be in bytes 7 and 8.
} mmIO;

// struct MMSio
//    o  used to organize data for i/o to the new MMS format files

typedef struct {
  float v[3];
  char  bytes[4];   // make struct word-aligned
                    // intensity is first byte, mode second
} MMSio;

typedef struct {
  unsigned char validMap[40];
  int    numPoints;
  double sensorVec[3];
  char   scanDir[4]; // 4 chars for word-alignment; scanDir is 1st byte
} MMSstripe;



static void
CopyReverseWord (void* dest, void* src, int nWords)
  // like memcpy but copies 32-bit words instead of bytes,
  // and flips byte order
  // to convert endian-ness as it copies.

  // ***NOTE***: this version is set-up to do nothing on win32 machines and
  // to swap on unix machines... should be used for modelmaker files only

{
  #ifdef WIN32
  if (src == dest) return;
  memcpy(dest, src, nWords * 4);
  return;
  #else
  for (int i = 0; i < nWords; i++) {
    unsigned int temp = *(unsigned int*)src;

    temp = (temp >> 24) | ((temp >> 8) & 0xFF00) |
      ((temp << 8) & 0xFF0000) | (temp << 24);

    (*(unsigned int*)dest) = temp;
    src  = ((char*)src)  + 4;
    dest = ((char*)dest) + 4;
  }
  #endif
}

static double
WSwap(double in)
// switches the word-order in an 8-byte double
// DOES NOTHING on a win32 machine (for modelmaker conversions to unix only)
{

  #ifdef WIN32
  return in;
  #else
  double out;
  *((int *)&out) = *(((int *)&in) + 1);
  *(((int *)&out) + 1) = *((int *)&in);
  return out;
  #endif
}

// overloaded MMS version of the other ReversePoints

static void
ReversePoints(MMSio *points, int numPoints) {

  // switches word-byte order for an array of inputted points
  // does nothing on win32 machines since the endian is correct there

  #ifdef WIN32
     return;
  #else
  CopyReverseWord(points, points, numPoints * sizeof(MMSio) / 4);

  // shouldn't need to swap word order since we're dealing with floats

  #endif
}

static void
ReversePoints(mmIO *points, int numPoints) {

  // switches word-byte order for an array of inputted points
  // does nothing on win32 machines since the endian is correct there

  #ifdef WIN32
     return;
  #else
  CopyReverseWord(points, points, numPoints * sizeof(mmIO) / 4);
  for (int i = 0; i < numPoints; i++) {
    for (int j = 0; j < 3; j++) {
      points[i].vtx[j] = WSwap(points[i].vtx[j]);
      points[i].nrm[j] = WSwap(points[i].nrm[j]);
    }
  }
  #endif
}


static void
triSet(float *dest, Pnt3 *src)
  // converts Pnt3 -> float[3]; used to store mms files
  // needs to be rewritten for doubles if we bring back cta writing
{
  dest[0] = (*src)[0];
  dest[1] = (*src)[1];
  dest[2] = (*src)[2];
}

// returns true if the indexed stripe and its successor are so similar
// that we can trash one of them
bool
MMScan::stripeCompare(int strNum, mmScanFrag *scan)
{
  if (strNum < 0) return false;
  mmStripeInfo *str1 = &scan->stripes[0] + strNum;
  mmStripeInfo *str2 = &scan->stripes[0] + strNum + 1;

  // if we're gaining more points as we go along, something interesting
  // is probably happening and we want to keep everything

  if (abs(str1->numPoints - str2->numPoints) > 5) return false;

  float avgDist = 0, numLargeDist = 0;
  int numPts = 0, count1 = 0, count2 = 0;

  for (int i = 0; i < STRIPE_PTS; i++) {
    bool valid1 = (str1->validMap[i / 8] & (1 << (i % 8)));
    bool valid2 = (str2->validMap[i / 8] & (1 << (i % 8)));
    if (valid1 && valid2) {
      // they have a point in common
      numPts++;
      float d = dist(scan->meshes[0].vtx[str1->startIdx + count1],
		     scan->meshes[0].vtx[str2->startIdx + count2]);
      if (d > 0.25) {
	numLargeDist++;
      }
      avgDist += d;
    }
    if (valid1) count1++;
    if (valid2) count2++;
  }

  // if less than 95% of the points are in common, something interesting
  // might be happening. this also catches the div by 0 on the next command
  if (numPts < (0.95 * str1->numPoints)) return false;

  avgDist /= numPts;

  if ((avgDist < 0.1) && (numLargeDist < 5)) return true;
  return false;
}


// *****************************************
// I/O FUNCTIONS: read and write
// *****************************************


bool
MMScan::read_mms(const crope &fname)
{
  mmStripeInfo stripe;
  mmScanFrag *scan;

  ColorSet cset;
  int numStripes, numPoints, numScans;
  int *numVerts; // tallies for each scanfrag for vertex allocs
  int i, j;
  MMSstripe *stripes;
  MMSio *points;

  //bool versionT = false, versionN = false;

  const char *filename = fname.c_str();

  FILE *pFile = fopen(filename, "rb");

  // check that file opened OK
  if( !pFile )
    return false;

  // read file version info
  char szFileID[10] = "";
  fread( szFileID, 10, 1, pFile );

  if ((strcmp(szFileID, "BinVer1.0") == 0) ||
      (strncmp(szFileID, "MMSBin", 6) == 0)) {
    cerr << "------------------" << endl;
    cerr << "This is an antiquated preliminary version of the MMS format."
	 << endl << "We're not dealing with it anymore. My apologies."
	 << endl;
    cerr << "------------------" << endl;
    return false;
  }
  else if (strncmp(szFileID, "MMScan", 6) == 0) {
    //versionT = versionN = false;
    for (int i = 6; i < 9; i++) {
      switch(szFileID[i]) {
        case 'T' : //versionT = true;
  	           cerr << "Found pretriangulation" << endl;
		   break;
        case 'N' : //versionN = true;
	           cerr << "Found per-vertex normals" << endl;
		   break;
        case 'x' : break;
        default  : cerr << endl << "Unrecognized flag in MMScan string: "
			<< szFileID[i] << endl;
	           return false;
      }
    }
  }
  else {
    cerr << "Bad version information for " << filename << ": string " <<
      szFileID << endl;
    return false;
  }

  // get number of stripes
  fread(&numStripes, sizeof( int ), 1, pFile );
  //  CopyReverseWord(&numStripes, &numStripes, 1);

  // get number of points
  fread(&numPoints, sizeof( int ), 1, pFile);
  //  CopyReverseWord(&numPoints, &numPoints, 1);

  // get number of scans
  fread(&numScans, sizeof( int ), 1, pFile);

  cout << "Reading " << numPoints << " points, "
       << numStripes << " stripes, "
       << numScans << " scans." << endl;

  // get stripes

  Progress progress (numStripes, "%s: reading stripes", filename);

  stripes = new MMSstripe[numStripes];

  if (stripes == NULL) {
    cerr << "Couldn't allocate enough memory for "
	 << numStripes << " stripes" << endl;
    return false;
  }

  fread(stripes, sizeof (MMSstripe) * numStripes, 1, pFile);

  scans.reserve(numScans);

  // prepare first scan
  scan = new mmScanFrag;

  numVerts = new int[numScans];
  for (i = 0; i < numScans; i++)
    numVerts[i] = 0;

  // set up stripes and organize them into scan fragments (no points yet)

  for(i = 0; i < numStripes; i++ )
  {
    if (stripes[i].numPoints > 0) {
      // keep this stripe, add it to current scanfrag

      for (j = 0; j < 40; j++)
	stripe.validMap[j] = stripes[i].validMap[j];
      stripe.numPoints = stripes[i].numPoints;
      stripe.scanDir = stripes[i].scanDir[0];
      stripe.sensorVec.set(stripes[i].sensorVec);

      stripe.startIdx = numVerts[scans.size()];
      numVerts[scans.size()] += stripe.numPoints;

      scan->stripes.push_back(stripe);
      scan->viewDir += stripe.sensorVec;
    }
    else if (scan->num_stripes() > 1) {
      // nothing in this stripe, but we need to end the scanfrag
      scan->viewDir /= (double)scan->num_stripes();
      scan->viewDir.normalize();
      cset.chooseNewColor(scan->falseColor);

      // reserve geometry and intensity space for our soon-to-arrive points
      scan->meshes[0].vtx.reserve(numVerts[scans.size()]);
      scan->meshes[0].intensity.reserve(numVerts[scans.size()]);

      // prepare for next scanfrag
      scans.push_back(*scan);
      delete scan;
      scan = new mmScanFrag;
    }
    else {
      // shouldn't ever be here if filtered properly... no points in stripe
      // and not enough stripes in scan to save it
      delete scan;
      scan = new mmScanFrag;
    }
    if ((i % 100) == 0) progress.update(i+1);
  }

  // get rid of i/o struct for stripes
  delete[] stripes;

  // set up points i/o

  points = new MMSio[numPoints];
  if (!points) {
    cerr << "Couldn't allocate memory for " << numPoints
	 << " points." << endl;
    return false;
  }

  fread(points, sizeof(MMSio) * numPoints, 1, pFile);

  // put points into scanfrags

  Progress progress2(numPoints, "%s: reading points", filename);

  int numSoFar = 0;
  for (i = 0; i < numScans; i++) {
    for (j = 0; j < numVerts[i]; j++, numSoFar++) {
      Pnt3 p;
      p.set(points[numSoFar].v);
      scans[i].meshes[0].vtx.push_back(p);
      scans[i].meshes[0].intensity.push_back(points[numSoFar].bytes[0]);
    }
    progress2.update(numSoFar);
  }

  // close file
  fclose( pFile );

  return true;
}

bool
MMScan::read_cta(const crope &fname)
{
  mmIO points[STRIPE_PTS], sensorVec;
  mmStripeInfo stripe;
  mmScanFrag *scan;
  mmResLevel *mesh;
  Pnt3 point, norm;
  uchar intensity;
  int numStripes, numSkipped;
  ColorSet cset;

  const char *filename = fname.c_str();

  FILE *pFile = fopen(filename, "rb");

  // check that file opened OK
  if( !pFile )
    return false;

  // read file version info
  char szFileID[10] = "";
  fread( szFileID, 10, 1, pFile );

  // get number of stripes

  fread(&numStripes, sizeof( int ), 1, pFile );
  CopyReverseWord(&numStripes, &numStripes, 1);

  Progress progress (numStripes, "%s: read CTA file", filename);

  // prepare first scan

  scan = new mmScanFrag;
  mesh = &(scan->meshes[0]);

  bool doFilter = true;
  char *result =
    Tcl_GetVar(theScene->interp, "noMMFilter", TCL_GLOBAL_ONLY);

  if (result)
    if ((strcmp(result, "1") == 0) || (strcmp(result, "true") == 0)) {
      doFilter = false;
      cout << "NOT FILTERING CTA FILE AS REQUESTED IN .RC" << endl;
    }

  // load stripes
  for( int i = 0; i < numStripes; i++ )
  {

    // load map
    fread( &stripe.validMap, 40, 1, pFile );

    // load number of points in stripe
    fread(&stripe.numPoints, sizeof( int ), 1, pFile );
    CopyReverseWord(&stripe.numPoints, &stripe.numPoints, 1);

    stripe.startIdx = scan->num_vertices(0);

    // load points if there are any
    if(stripe.numPoints > 0 )
    {

      // load and reverse points
      fread(points, sizeof(mmIO)* stripe.numPoints, 1, pFile );
      ReversePoints(points, stripe.numPoints);

      // store points
      for (int j = 0; j < stripe.numPoints; j++) {
	point.set(points[j].vtx);

	// note: we're discarding normals in the cta file because
	// they're not measured, just generated really poorly

	// intensity is the 4th byte of the first variable
	intensity = *((char *)(&points[j].inten)+3);

	mesh->vtx.push_back(point);
	mesh->intensity.push_back(intensity);
      }
    }

    // read sensor vector (using first 24 bytes of mmIO struct)
    fread(&sensorVec, 3 * sizeof(double), 1, pFile );
    ReversePoints(&sensorVec, 1);
    stripe.sensorVec.set(sensorVec.vtx);

    // save stripe ONLY if there are 5 points on the stripe.. otherwise end
    // the scan.

    if ((stripe.numPoints < 5) && (stripe.numPoints > 0)) {
      numSkipped++;
    }
    else if (stripe.numPoints > 0) {
      scan->stripes.push_back(stripe);
      if (doFilter && stripeCompare(scan->stripes.size() - 2, scan)) {
	for (int j = 0; j < stripe.numPoints; j++) {
	  mesh->vtx.pop_back();
	  mesh->intensity.pop_back();
	}
	scan->stripes.pop_back();
	numSkipped++;
      }
      else
	scan->viewDir += stripe.sensorVec;
    }
    else if (scan->num_stripes() > 2) {
      // end current scan and save if it has at least 3 stripes
      scan->viewDir /= (double)scan->num_stripes();
      scan->viewDir.normalize();
      cset.chooseNewColor(scan->falseColor);
      scans.push_back(*scan);
      delete scan;
      scan = new mmScanFrag;
      mesh = &(scan->meshes[0]);
    }
    else { // no points on stripe and not enough information in scan
      numSkipped += scan->num_stripes();
      delete scan;
      scan = new mmScanFrag;
      mesh = &(scan->meshes[0]);
    }
    if ((i % 100) == 0) progress.update(i+1);
  }

  // close file
  fclose( pFile );

  if (numSkipped) cout << "SKIPPED " << numSkipped
		       << "STRIPES while loading." << endl;
  return true;
}


bool
MMScan::read(const crope &fname)
{
  set_name(fname);

  // can only read cta and mms files
  if (has_ending(".mms")) {
    if (!read_mms(fname)) return false;
  }
  else if (!(has_ending(".cta") && read_cta(fname)))
    return false;

  // should be OK here

  /* debug code
  for(i=0; i < scans.size(); i++) {
    printf("scan %d: %d stripes\n", i, scans[i].stripes.size());
    printf("         (%5.2f, %5.2f, %5.2f) viewDir\n", scans[i].viewDir[0],
	   scans[i].viewDir[1], scans[i].viewDir[2]);
    printf("         %d points\n", scans[i].vtx.size());
    for(int j=0; j < scans[i].stripes.size(); j++) {
      printf("  stripe %d: %d points\n", j, scans[i].stripes[j].numPoints);
      printf("     %d pointIdx\n", scans[i].stripes[j].startIdx);
      printf("     (%5.2f, %5.2f, %5.2f) sensor\n",
	     scans[i].stripes[j].sensorVec[0],
	     scans[i].stripes[j].sensorVec[1],
	     scans[i].stripes[j].sensorVec[2]);
    }
  }
  */

  // insert resolutions... real and subsampled
  int ntris = scans[0].num_tris();
  if (!ntris) { // compute estimate
    for (int ifrag = 0; ifrag < scans.size(); ifrag++)
      ntris += scans[ifrag].meshes[0].vtx.size() * 2;
  }

  for (int iss = 0; iss < SUBSAMPS; iss++) {
    insert_resolution (ntris / (1 << (2*iss)), fname, false, false);
    kdtree.push_back (NULL);
    regData.push_back(mergedRegData());
    if (iss > 0) {
      for (int ifrag = 0; ifrag < scans.size(); ifrag++)
	scans[ifrag].meshes.push_back(mmResLevel());
    }
  }

  /*
  fprintf(stderr, "Calculating confidence...\n");
  calcConfidence();
  */

  /*
  fprintf(stderr, "Removing unused vertices... ");
  for (int i = 0; i < scans.size(); i++) {
    filter_unused_vtx(scans[i]);
  }
  */

  // try to load .xf file
  TbObj::readXform (get_basename());

  // we just wiped out whatever was in memory, but it's identical to what's
  // on disk, so set the memory update flag on
  isDirty_mem = true;

  select_coarsest();
  computeBBox();

  return true;
}


// writes an mms file

bool
MMScan::write(const crope &fname)
{
  MMSstripe blankStripe;

  int i, j, k;
  MMSstripe *stripes;
  MMSio *points;

  if (fname.empty()) {
    // try to save to default name; quit if there isn't one
    if (name.empty()) return false;
  }
  else {
    if (name != fname) {
      cout << "Saving to filename " << fname << endl;
      set_name(fname);
    }
  }

  for (i=0; i < 3; i++) {
    blankStripe.sensorVec[i] = 0.0;
  }

  for (i=0; i < 40; i++)
    blankStripe.validMap[i] = '\0';

  blankStripe.numPoints = 0;
  blankStripe.scanDir[0] = 0;

  Progress progress (num_stripes(), "%s: writing stripes", name.c_str());

  FILE *pFile = fopen(name.c_str(), "wb");

  if (!pFile)
    return false;

  // file version
  char szFileID[10] = "MMScanxxx";
  if( !fwrite( szFileID, 10, 1, pFile ) )
  {
    fclose( pFile );
    return false;
  }

  int numStripes, numPoints, numScans;

  // number of stripes plus blank ones
  numStripes = num_stripes() + scans.size();
  numScans = num_scans();
  numPoints = num_vertices(0);

  //  CopyReverseWord(intBuf, &tmp, 1);
  if(!fwrite(&numStripes, sizeof(int), 1, pFile)) {
    fclose(pFile);
    return false;
  }

  if(!fwrite(&numPoints, sizeof(int), 1, pFile)) {
    fclose(pFile);
    return false;
  }

  if (!fwrite(&numScans, sizeof(int), 1, pFile)) {
    fclose(pFile);
    return false;
  }

  int numSoFar = 0;

  stripes = new MMSstripe[numStripes];

  // loop through all scans, build up master stripe list

  for (i = 0; i < numScans; i++) {
    for (j = 0; j < scans[i].stripes.size(); j++, numSoFar++) {
      for (k = 0; k < 40; k++)
	stripes[numSoFar].validMap[k] = scans[i].stripes[j].validMap[k];
      stripes[numSoFar].numPoints = scans[i].stripes[j].numPoints;
      for (k = 0; k < 3; k++)
	stripes[numSoFar].sensorVec[k] = scans[i].stripes[j].sensorVec[k];
      stripes[numSoFar].scanDir[0] = scans[i].stripes[j].scanDir;
      progress.update(numSoFar);
      if (numSoFar == numStripes)
	cerr << "reached the end of our array " << endl;

    }
    stripes[numSoFar] = blankStripe;
    numSoFar++;
    progress.update(numSoFar);
  }

  if (numSoFar != numStripes) {
    cerr << "Mismatch happened while building stripe i/o array" << endl;
    cerr << "numSoFar = " << numSoFar << ", numStripes = " << numStripes <<
      endl;
    return false;
  }

  if (!fwrite(stripes, sizeof(MMSstripe) * numStripes, 1, pFile)) {
    fclose(pFile);
    cerr << "Couldn't write stripes to disk whilst saving. " << endl;
    return false;
  }

  delete[] stripes;

  points = new MMSio[numPoints];

  Progress progress2(numScans, "%s: writing points", name.c_str());

  numSoFar = 0;
  for (i = 0; i < numScans; i++) {
    progress2.update(i);
    for (j = 0; j < scans[i].meshes[0].vtx.size(); j++, numSoFar++) {
      for (k = 0; k < 3; k++)
	points[numSoFar].v[k] = scans[i].meshes[0].vtx[j][k];
      points[numSoFar].bytes[0] = scans[i].meshes[0].intensity[j];
    }
  }

  if (numSoFar != numPoints) {
    cerr << "Mismatch whilst setting up vertex i/o arrays." << endl;
    return false;
  }

  if (!fwrite(points, sizeof(MMSio) * numPoints, 1, pFile)) {
    fclose(pFile);
    cerr << "Couldn't write points to disk whilst saving. " << endl;
    return false;
  }

  delete[] points;

  // close file
  fclose( pFile );
  return true;
}
