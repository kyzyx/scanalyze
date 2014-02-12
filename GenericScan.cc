//############################################################
//
// GenericScan.cc
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


#include <fstream>
#include <sys/stat.h>
#include "GenericScan.h"
#include "defines.h"
#include "Mesh.h"
#include "RangeGrid.h"
#include "plvGlobals.h"
#include "plvScene.h"
#include "plvDraw.h"
#include "KDindtree.h"
#include "ColorUtils.h"
#include "TriMeshUtils.h"
#include "FileNameUtils.h"
#include "MeshTransport.h"
#include "VertexFilter.h"


GenericScan::GenericScan ()
{
  _Init();
}


void
GenericScan::_Init()
{
  bDirty = true;
  bNameSet = false;
  pushcount = 0;
  myRangeGrid = NULL;
}


GenericScan::GenericScan (Mesh* mesh, const crope& name)
{
  _Init();
  insertMesh (mesh, name);
  computeBBox();
  set_name (name);
}


GenericScan::~GenericScan ()
{
  while (meshes.size()) {
    delete meshes.back();
    meshes.pop_back();
  }

  while (kdtree.size()) {
    delete kdtree.back();
    kdtree.pop_back();
  }

  delete myRangeGrid;
}


MeshTransport*
GenericScan::mesh(bool perVertex, bool stripped,
		  ColorSource color, int colorSize)
{
  if (stripped && !perVertex) {
    cerr << "No t-strips without per-vertex properties";
    return NULL;
  }

  Mesh* mesh = currentMesh();

  MeshTransport* mt = new MeshTransport;
  mt->setVtx (&mesh->vtx, MeshTransport::share);
  if (perVertex)
    mt->setNrm (&mesh->nrm, MeshTransport::share);
  else {
    vector<short>* faceNrm = new vector<short>;
    mesh->simulateFaceNormals (*faceNrm);
    mt->setNrm (faceNrm, MeshTransport::steal);
  }
  if (stripped)
    mt->setTris (&mesh->getTstrips(), MeshTransport::share);
  else
    mt->setTris (&mesh->getTris(), MeshTransport::share);

  if (color != colorNone)
    setMTColor (mesh, mt, perVertex, color, colorSize);

  return mt;
}


void
GenericScan::setMTColor (Mesh* mesh, MeshTransport* mt,
			 bool perVertex, ColorSource source,
			 int colorsize)
{
  if (source == colorBoundary) {
      mesh->mark_boundary_verts();
  } else if (source == colorConf) {
    if (!mesh->vertConfidence)
      return;  // confidence data requested but does not exist
  } else {
    if (!mesh->vertMatDiff && !mesh->vertIntensity &&
	(perVertex || !mesh->triMatDiff))
      return;  // no color data at all, or only per-face but per-vert requested
  }

  // we know color data can at the least be synthesized... do it!
  vector<uchar>* colors = new vector<uchar>;
  mt->setColor (colors, MeshTransport::steal);

  if (source == colorConf) {
    if (perVertex) {
      // per-vertex confidence
      colors->reserve (colorsize * mesh->vtx.size());
      for (int i = 0; i < mesh->vtx.size(); i++)
	pushConf (*colors, colorsize, mesh->vertConfidence[i]);
    } else {
      // per-face confidence
      colors->reserve (colorsize * mesh->getTris().size()/3);
      for (int i = 0; i < mesh->tris.size(); i+=3) {
	pushConf (*colors, colorsize,
		  (mesh->vertConfidence[mesh->tris[i+0]]
		   + mesh->vertConfidence[mesh->tris[i+1]]
		   + mesh->vertConfidence[mesh->tris[i+2]]) / 3);
      }
    }
  } else if (source == colorBoundary) {
      colors->reserve (colorsize * mesh->bdry.size());
// STL Update
      vector<char>::iterator end = mesh->bdry.end();
      for (vector<char>::iterator c = mesh->bdry.begin(); c < end; c++)
	pushConf (*colors, colorsize, (uchar)(*c ? 0 : 255));
  } else { // real diffuse color, not confidence
    if (perVertex) {
      // per-vertex truecolor or intensity
      colors->reserve (colorsize * mesh->vtx.size());
      for (int i = 0; i < mesh->vtx.size(); i++) {
	if (source == colorTrue) { // prefer rgb over intensity
	  if (mesh->vertMatDiff)
	    pushColor (*colors, colorsize, mesh->vertMatDiff[i]);
	  else
	    pushColor (*colors, colorsize, mesh->vertIntensity[i]);
	} else { // prefer intensity over rgb
	  if (mesh->vertIntensity)
	    pushColor (*colors, colorsize, mesh->vertIntensity[i]);
	  else
	    pushColor (*colors, colorsize,
		       intensityFromRGB (mesh->vertMatDiff[i]));
	}
      }
    } else {
      // per-face truecolor or intensity
      colors->reserve (colorsize * mesh->getTris().size()/3);
      for (int i = 0; i < mesh->tris.size(); i+=3) {
	if (source == colorTrue) { // prefer rgb from faces,
	  // then rgb from verts, then intensity
	  if (mesh->triMatDiff)
	    pushColor (*colors, colorsize, mesh->triMatDiff[i/3]);
	  else if (mesh->vertMatDiff)
	    pushColor (*colors, colorsize,
		       mesh->vertMatDiff[mesh->tris[i+0]],
		       mesh->vertMatDiff[mesh->tris[i+1]],
		       mesh->vertMatDiff[mesh->tris[i+2]]);
	  else
	    pushColor (*colors, colorsize,
		       mesh->vertIntensity[mesh->tris[i+0]],
		       mesh->vertIntensity[mesh->tris[i+1]],
		       mesh->vertIntensity[mesh->tris[i+2]]);
	} else { // prefer intensity, then rgb from faces, then rgb from verts
	  if (mesh->vertIntensity)
	    pushColor (*colors, colorsize,
		       mesh->vertIntensity[mesh->getTris()[i+0]],
		       mesh->vertIntensity[mesh->tris[i+1]],
		       mesh->vertIntensity[mesh->tris[i+2]]);
	  else if (mesh->triMatDiff)
	    pushColor (*colors, colorsize, mesh->triMatDiff[i/3]);
	  else
	    pushColor (*colors, colorsize,
		       uchar (((int)intensityFromRGB (mesh->vertMatDiff[mesh->getTris()[i+0]])
			+ intensityFromRGB (mesh->vertMatDiff[mesh->tris[i+1]])
			+ intensityFromRGB (mesh->vertMatDiff[mesh->tris[i+2]]))/3));
	}
      }
    }
  }
}


void
GenericScan::computeBBox()
{
  bool nonEmpty;

  bbox.clear();
  for (int i = 0; i < meshes.size(); i++) {
    if (meshes[i]->num_verts() != 0) {
      bbox.add(meshes[i]->bbox);
      nonEmpty = 1;
    }
  }

  if (nonEmpty) {
    rot_ctr = bbox.center();
  } else {
    rot_ctr = Pnt3();
  }
}

int
GenericScan::num_vertices(void)
{
  return currentMesh()->num_verts();
}

void
GenericScan::subsample_points(float rate, vector<Pnt3> &p,
			      vector<Pnt3> &n)
{
  currentMesh()->subsample_points(rate, p, n);
}


bool
GenericScan::read (const crope &fname)
{
  bool success = FALSE;

  // initialize name and remove trailing slash, if it's a directory
  set_name (fname);

  // check if it's a .set file -- or a directory containing a .set
  if ( has_ending(".set") ) {
    success = readSet (fname);
  } else {
    // treat filename as directory name, and look for .set file
    crope fn(get_name());
    fn += crope("/") + get_name() + ".set";
    if (0 == access (fn.c_str(), R_OK)) {
      success = readSet (fn);
      if (success)
	set_name(fn);
    }
  }

  // if none of the directory/set forms exist, look for single
  // fully-specified file
  if (!success)
    success = readSingleFile (get_name());

  if (success) {
    // try reading transformation
    pushd();
    TbObj::readXform(get_basename());
    popd();

    bDirty = false;
    bNameSet = true;
  }

  return success;
}


void
GenericScan::setd (const crope& dir, bool bCreate)
{
  char setDir [PATH_MAX];
  strcpy (setDir, dir.c_str());
  char* psep = strrchr (setDir, '/');
  if (psep != NULL) {
    // terminate before filename part
    *psep = 0;
    setdir = setDir;
  } else {
    // default to current directory
    getcwd (setDir, PATH_MAX);
  }

  cout << "Set directory is " << setDir << "/" << endl;
  if (bCreate)
    portable_mkdir (setDir, 00775);
}


void
GenericScan::pushd (void)
{
  //cout << "push: count is " << pushcount << endl;
  if (pushcount++ == 0) {
    assert (pusheddir.empty());

    char szCwd [PATH_MAX];
    getcwd (szCwd, PATH_MAX);
    pusheddir = szCwd;

    chdir (setdir.c_str());
  }
}


void
GenericScan::popd (void)
{
  if (--pushcount == 0) {
    assert (!pusheddir.empty());

    chdir (pusheddir.c_str());
    pusheddir = crope();
  }
  //cout << "pop: count is " << pushcount << endl;
}


bool
GenericScan::readSet (const crope& fn)
{
  //read .set file
  ifstream in (fn.c_str());
  if (in.fail()) {
    cout << "FAILED: " << fn << " does not exist." << endl;
    return false;
  }

  int nMeshes;
  int nDefaultRes;

  in.ignore (20, '=');
  in >> nMeshes;
  in.ignore (20, '=');
  in >> nDefaultRes;

  cout << "Set has " << nMeshes << " meshes\n";
  cout << "Default res is " << nDefaultRes << endl;

  // semantics of filenames in set files: relative pathnames are relative
  // to directory containing set file
  setd (fn);
  pushd();

  //read meshes mentioned in .set file
  meshes.clear();
  for (int i = 0; i < nMeshes; i++) {
    char load[20];
    int nRes;
    char path[PATH_MAX];

    in >> load >> nRes >> path;

    bool bPreload = !strcmp (load, "preload");
    if (nRes == nDefaultRes)   //always load the default resolution
      bPreload = true;

    if (bPreload) {  // otherwise leave blank
      Mesh* mesh = readMeshFile (path);
      if (mesh)
	insertMesh (mesh, path, true, true, nRes);
      //else
      //cerr << "Mesh " << path
      //     << " named in .set file does not exist" << endl;
    } else {         // insert dummy mesh
      insertMesh (new Mesh, path, false, false, nRes);
    }
  }

  popd();

  //set default mesh according to .set file
  if (!select_by_count (nDefaultRes))  // didn't find default
    select_coarsest();                 // so set to lowest res
  computeBBox();

  return true;
}


bool
GenericScan::readSingleFile (const crope& fn)
{
  const char* filename = fn.c_str();

  if (is_range_grid_file(filename)) {

    RangeGrid* rangeGrid = new RangeGrid();
    if (rangeGrid->readRangeGrid(filename) == 0) {
	  delete rangeGrid;
	  return false;
    }

    int subSamp = 1;
    int nAutoGenRes = 4;
    int iPolys = rangeGrid->numSamples * 2;

    for (int i = 0; i < nAutoGenRes; i++) {

      char name[PATH_MAX];
      strcpy (name, filename);
      char* ext = strrchr (name, '.');
      if (ext == NULL)
	ext = name + strlen (name);
      sprintf (ext, "_samp%d.ply", subSamp);
      insertMesh (new Mesh, name, false, false, iPolys / subSamp);
      subSamp *= (SubSampleBase * SubSampleBase);
    }

    myRangeGrid = rangeGrid;

  } else {

    Mesh* mesh = readMeshFile (filename);
    if (!mesh)
      return false;

    insertMesh (mesh, filename);
  }

  // set current directory here
  set_name (filename);
  setd();

  select_coarsest();
  computeBBox();

  return true;
}


Mesh*
GenericScan::readMeshFile (const char* name)
{
  Mesh* mesh = new Mesh;

  cout << "Reading mesh " << name << "... " << flush;
  if (!mesh->readPlyFile (name)) {
    cout << "failed!" << endl;
    delete mesh;
    return NULL;
  }

  cout << "calculating normals... " << flush;
  mesh->updateScale();
  mesh->initNormals(UseAreaWeightedNormals);
  cout << "done." << endl;

  return mesh;
}


int
GenericScan::create_resolution_absolute(int budget, Decimator dec)
{
  Mesh *origMesh = highestRes();
  if (origMesh->num_tris() == 0) {
    cerr <<  "No triangles in the original mesh" << endl;
    return 0;
  }

  cerr << "targetSize = " << budget << endl;
  cerr << "num meshes = " << num_resolutions() << endl;

  Mesh *newMesh = origMesh->Decimate(budget, PLACE_OPTIMAL,
				     0, 1000, dec);
  char numberEnding[20];
  sprintf(numberEnding, ".%d.ply", newMesh->num_tris());
  cerr << "output vertSize = " << newMesh->num_verts() << endl;
  cerr << "output triSize = " << newMesh->num_tris() << endl;
  newMesh->bNeedsSave = TRUE;   //mark that this is worth keeping

  insertMesh(newMesh, basename + numberEnding);
  computeBBox();

  cerr << "new num meshes = " << num_resolutions() << endl;
  theScene->invalidateDisplayCaches();

#if 0
  for (int i=0; i<meshes.size(); i++) {
    cout << i << endl;
    cout << meshes[i]->num_tris() << endl;
    cout << meshes[i]->num_verts() << endl;
    cout << meshes[i]->vtx[100] << endl;
    cout << meshes[i]->nrm[100] << endl;
    cout << meshes[i]->tris[100] << endl;
  }
#endif

  bDirty = true;
  return newMesh->num_tris();
}


bool
GenericScan::delete_resolution (int nPolys)
{
  int iRes = findLevelForRes (nPolys);
  if (iRes < 0) {
    cerr << "FAILED: Attempt to delete resolution " << nPolys << endl;
    return false;
  }

  delete meshes[iRes];
// STL Update
  meshes.erase (meshes.begin() + iRes);
  resolutions.erase (resolutions.begin() + iRes);

  select_coarser();
  return true;
}


void
GenericScan::flipNormals (void)
{
  for (int i = 0; i < meshes.size(); i++)
    meshes[i]->flipNormals();
  bDirty = true;
}


void
GenericScan::insertMesh(Mesh *m, const crope& filename,
		    bool bLoaded, bool bAlwaysLoad,
		    int nRes)
{
  // insert into resolution vector (sorted by vertex count)
  if (nRes == 0)                  // allow override of
    nRes = m->num_tris();         // temporary resolution
  insert_resolution (nRes, filename, bLoaded, bAlwaysLoad);

  // now add to mesh vector, sorted parallel to resolution vector
  int iPos = findLevelForRes (nRes);
// STL Update
  meshes.insert (meshes.begin() + iPos, m);
  kdtree.insert (kdtree.begin() + iPos, NULL);
}


Mesh*
GenericScan::currentMesh (void)
{
  int i = current_resolution_index();
  if (!resolutions[i].in_memory) {
    load_resolution (i);
  }

  return meshes[i];
}


bool
GenericScan::load_resolution (int i)
{
  if (resolutions[i].in_memory)
    return true;

  Mesh* loaded = NULL;

  if (myRangeGrid) {
    // build mesh from range grid
    int subSamp = 1;
    for (int j = 0; j < i; j++) {
      subSamp *= SubSampleBase;
    }
    loaded = myRangeGrid->toMesh(subSamp, false);
    loaded->updateScale();
    loaded->initNormals(UseAreaWeightedNormals);
    loaded->bNeedsSave = true;
  } else {
    // mesh should be plyfile on disk

    pushd();
    loaded = readMeshFile (resolutions[i].filename.c_str());
    if (!loaded) {
      popd();
      return false;
    }
    popd();
  }

  delete meshes[i];
  meshes[i] = loaded;
  resolutions[i].in_memory = true;
  resolutions[i].abs_resolution = loaded->num_tris();
  computeBBox();

  return true;
}


bool
GenericScan::release_resolution (int nPolys)
{
  int iRes = findLevelForRes (nPolys);
  if (iRes < 0) {
    cerr << "FAILD: Attempt to release resolution " << nPolys << endl;
    return false;
  }

  if (!resolutions[iRes].in_memory)
    return true;

  delete meshes[iRes];
  meshes[iRes] = new Mesh;
  resolutions[iRes].in_memory = false;
  return true;
}


bool
GenericScan::getXformFilename (const char* meshName, char* xfName)
{
  const char* meshFile = meshName;
  const char* dir = setdir.c_str();
  int dirLen = strlen(dir);
  if (dirLen && !strncmp (meshName, dir, dirLen))
    meshFile += dirLen + 1;

  return get_filename_new_ending (meshFile, ".xf", xfName);
}


KDindtree*
GenericScan::get_current_kdtree()
{
  int iTree = current_resolution_index();
  assert (iTree < kdtree.size());
  if (kdtree[iTree] != NULL)
    return kdtree[iTree];

  Mesh* mesh = currentMesh();
// STL Update
  kdtree[iTree] = CreateKDindtree(mesh->vtx.begin(),
				  mesh->nrm.begin(),
				  mesh->vtx.size());

  return kdtree[iTree];
}


bool
GenericScan::closest_point(const Pnt3 &p, const Pnt3 &n,
			   Pnt3 &cp, Pnt3 &cn,
			   float thr, bool bdry_ok)
{
  KDindtree* tree = get_current_kdtree();
  if (!tree)
    return false;

  int ind, ans;
  Mesh* mesh = currentMesh();
// STL Update
  ans = tree->search(&*(mesh->vtx.begin()), &*(mesh->nrm.begin()), p, n, ind, thr);
  if (ans) {
    if (bdry_ok == 0) {
      // disallow closest points that are on the mesh boundary
      mesh->mark_boundary_verts();
      if (mesh->bdry[ind]) return 0;
    }
    cp = mesh->vtx[ind];
    short *sp = &mesh->nrm[ind*3];
    cn.set(sp[0]/32767.0,
	   sp[1]/32767.0,
	   sp[2]/32767.0);
  }
  return ans;
}


crope
GenericScan::getInfo (void)
{
  char info[512];
  sprintf(info, "Polygon file: %ld resolutions\n"
	  "verts %d tris %ld strips %ld\n\n",
	  resolutions.size(),
	  currentMesh()->num_verts(),
	  currentMesh()->tris.size() / 3,
	  currentMesh()->tstrips.size());

  return crope(info) + RigidScan::getInfo();
}


Mesh*
GenericScan::getMesh (int level)
{
  if (level >= meshes.size())
    level = meshes.size() - 1;
  if (level < 0) level = 0;

  if (!resolutions[level].in_memory)
    load_resolution (level);

  return meshes[level];
}


inline Mesh*
GenericScan::highestRes (void)
{
  return getMesh (0);
}

/* For each triangle in the ply file, this function
   prints out the coordinates of the voxel from which
   it came.  This function is called from plvClipBoxCmds.cc
   which has already called filtered_copy to generate a list
   of the trianlges enclosed by a user-specified box.

   N.B.'s:

   1. This feature should only be called on a *small* set of
   polygons since information is printed out for every polygon
   chosen.

   2. This feature uses the same mechanism as clipping to decide
   which polygons are inside the box.  So, just as clipping returns
   polygons which are inside and facing the box as well as inside
   and facing away from the box, this feature prints information
   for all polygons regardless of whether they are forwards or
   backwards facing.

   - leslie 2001
*/
void GenericScan::PrintVoxelInfo()
{
 printf("Printing voxel information...\n");

 Mesh *mesh = currentMesh();
 if (!mesh->hasVoxels) {
   printf("There is no voxel information stored\n"); return;
 }
 int triSize = mesh->getTris().size();
 if (triSize != mesh->fromVoxels.size()) {
   printf("Voxel information error: %d tris, %d voxels\n", triSize,
	  mesh->fromVoxels.size());
   return;
 }

 for (int i = 0; i < triSize; i += 3) {
   printf("Triangle with vertices at\n");
   printf("(%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n",
	  mesh->vtx[mesh->tris[i]][0], mesh->vtx[mesh->tris[i]][1],
	  mesh->vtx[mesh->tris[i]][2],
	  mesh->vtx[mesh->tris[i+1]][0], mesh->vtx[mesh->tris[i+1]][1],
	  mesh->vtx[mesh->tris[i+1]][2],
	  mesh->vtx[mesh->tris[i+2]][0], mesh->vtx[mesh->tris[i+2]][1],
	  mesh->vtx[mesh->tris[i+2]][2]);
   printf("came from voxel at\n");
   printf("(%f, %f, %f)\n", mesh->fromVoxels[i],
	  mesh->fromVoxels[i+1], mesh->fromVoxels[i+2]);
 }

}


// copy the part of the mesh that is in the clip box
// into the newMeshSet (clipmesh)
RigidScan*
GenericScan::filtered_copy(const VertexFilter& filter)
{
  int i, j;
  GenericScan* newMeshSet = new GenericScan;
  assert(newMeshSet != NULL);
  Mesh *newMesh, *oldMesh;

  newMeshSet->setXform(getXform());

  for (j = 0; j < meshes.size(); j++) {
    vector<int> newIndices;
    newMesh = new Mesh;
    oldMesh = getMesh(j);

    // figure out which vertices remain
    int vertCount = 0;
    for (i = 0; i < oldMesh->num_verts(); i++) {
      if (filter.accept (oldMesh->vtx[i])) {
	newIndices.push_back(1);
	vertCount++;
      } else {
	newIndices.push_back(-1);
      }
    }

    // and then which faces remain
    int triCount = 0;
    for (i = 0; i < oldMesh->getTris().size(); i += 3) {
      if (newIndices[oldMesh->tris[i  ]] >= 0 &&
	  newIndices[oldMesh->tris[i+1]] >= 0 &&
	  newIndices[oldMesh->tris[i+2]] >= 0) {
	triCount++;
      }
    }

    // copy other mesh properties
    newMesh->hasVertNormals = oldMesh->hasVertNormals;
    if (oldMesh->vertMatDiff != NULL)
      newMesh->vertMatDiff = new vec3uc[vertCount];
    if (oldMesh->vertIntensity != NULL)
      newMesh->vertIntensity = new float[vertCount];
    if (oldMesh->vertConfidence != NULL)
      newMesh->vertConfidence = new float[vertCount];
    if (oldMesh->texture != NULL)
      newMesh->texture = new vec2f[vertCount];
    if (oldMesh->triMatDiff != NULL)
      newMesh->triMatDiff = new vec3uc[triCount];

    // copy the surviving vertices
    int count = 0;
    newMesh->vtx.reserve (vertCount);
    if (oldMesh->nrm.size())
      newMesh->nrm.reserve (vertCount);
    for (i = 0; i < oldMesh->num_verts(); i++) {
      if (newIndices[i] > 0) {
	newIndices[i] = count;
	newMesh->copyVertFrom (oldMesh, i, count);
	count++;
      }
    }

    // copy and reindex the surviving faces
    count = 0;
    newMesh->tris.reserve (triCount * 3);
    for (i = 0; i < oldMesh->tris.size(); i += 3) {
      if (newIndices[oldMesh->tris[i  ]] >= 0 &&
	  newIndices[oldMesh->tris[i+1]] >= 0 &&
	  newIndices[oldMesh->tris[i+2]] >= 0) {

	newMesh->copyTriFrom (oldMesh, i/3, count/3);
	newMesh->tris[count  ] = newIndices[oldMesh->tris[i  ]];
	newMesh->tris[count+1] = newIndices[oldMesh->tris[i+1]];
	newMesh->tris[count+2] = newIndices[oldMesh->tris[i+2]];

	// copy fromVoxels info - for voxel display feature
	if (oldMesh->hasVoxels) {
	  newMesh->fromVoxels[count] = oldMesh->fromVoxels[i];
	  newMesh->fromVoxels[count+1] = oldMesh->fromVoxels[i+1];
	  newMesh->fromVoxels[count+2] = oldMesh->fromVoxels[i+2];
	}

	count += 3;
      }
    }

    // and finalize this resolution
    newMesh->bNeedsSave = true;
    newMesh->computeBBox();

    // and insert it into new GenericScan
    crope clipName(get_basename());
    char info[20];
    sprintf (info, "Clip.%d.ply", triCount);
    clipName += info;
    newMeshSet->insertMesh (newMesh, clipName);
  }

  // done!
  newMeshSet->computeBBox();
  newMeshSet->curr_res = curr_res;
  return newMeshSet;
}


bool
GenericScan::filter_inplace(const VertexFilter &filter)
{
  for (int iRes = 0; iRes < resolutions.size(); iRes++) {
    vector<int> newIndices;
    Mesh* newMesh = new Mesh;
    Mesh* oldMesh = getMesh(iRes);

    // figure out which vertices remain
    int vertCount = 0;
    for (int i = 0; i < oldMesh->num_verts(); i++) {
      if (filter.accept (oldMesh->vtx[i])) {
	newIndices.push_back(1);
	vertCount++;
      } else {
	newIndices.push_back(-1);
      }
    }

    // and then which faces remain
    int triCount = 0;
    for (i = 0; i < oldMesh->getTris().size(); i += 3) {
      if (newIndices[oldMesh->tris[i  ]] >= 0 &&
	  newIndices[oldMesh->tris[i+1]] >= 0 &&
	  newIndices[oldMesh->tris[i+2]] >= 0) {
	triCount++;
      }
    }

    // copy other mesh properties
    newMesh->hasVertNormals = oldMesh->hasVertNormals;
    if (oldMesh->vertMatDiff != NULL)
      newMesh->vertMatDiff = new vec3uc[vertCount];
    if (oldMesh->vertIntensity != NULL)
      newMesh->vertIntensity = new float[vertCount];
    if (oldMesh->vertConfidence != NULL)
      newMesh->vertConfidence = new float[vertCount];
    if (oldMesh->texture != NULL)
      newMesh->texture = new vec2f[vertCount];
    if (oldMesh->triMatDiff != NULL)
      newMesh->triMatDiff = new vec3uc[triCount];

    // copy the surviving vertices
    int count = 0;
    newMesh->vtx.reserve (vertCount);
    if (oldMesh->nrm.size())
      newMesh->nrm.reserve (vertCount);
    for (i = 0; i < oldMesh->num_verts(); i++) {
      if (newIndices[i] > 0) {
	newIndices[i] = count;
	newMesh->copyVertFrom (oldMesh, i, count);
	count++;
      }
    }

    // copy and reindex the surviving faces
    count = 0;
    newMesh->tris.reserve (triCount * 3);
    for (i = 0; i < oldMesh->tris.size(); i += 3) {
      if (newIndices[oldMesh->tris[i  ]] >= 0 &&
	  newIndices[oldMesh->tris[i+1]] >= 0 &&
	  newIndices[oldMesh->tris[i+2]] >= 0) {
	// newIndices holds renumbered vertices
	newMesh->copyTriFrom (oldMesh, i/3, count/3);
	newMesh->tris[count  ] = newIndices[oldMesh->tris[i  ]];
	newMesh->tris[count+1] = newIndices[oldMesh->tris[i+1]];
	newMesh->tris[count+2] = newIndices[oldMesh->tris[i+2]];
	// copy from voxel info if we're supporting clipping later
	// for voxel display feature
	//if (oldMesh->hasVoxels) {
	//newMesh->fromVoxels[count] = oldMesh->fromVoxels[i];
	//newMesh->fromVoxels[count+1] = oldMesh->fromVoxels[i+1];
	//newMesh->fromVoxels[count+2] = oldMesh->fromVoxels[i+2];
	//}

	count += 3;
      }
    }

    // and finalize this resolution
    newMesh->bNeedsSave = true;
    newMesh->computeBBox();

    // and insert it in place of original
    delete oldMesh;
    meshes[iRes] = newMesh;
    resolutions[iRes].abs_resolution = newMesh->num_tris();

    // nuke any kdtree for this resolution
    delete kdtree[iRes];
    kdtree[iRes] = NULL;
  }

  // done!
  computeBBox();
  bDirty = true;
  return true;
}


bool
GenericScan::filter_vertices (const VertexFilter& filter, vector<Pnt3>& p)
{
  Mesh* mesh = currentMesh();

// STL Update
  vector<Pnt3>::iterator vtxend = mesh->vtx.end();
  for (vector<Pnt3>::iterator pnt = mesh->vtx.begin(); pnt < vtxend; pnt++) {
    if (filter.accept (*pnt))
      p.push_back (*pnt);
  }

  return true;
}


bool
GenericScan::is_modified (void)
{
  return bDirty;
}


bool
GenericScan::write_metadata (MetaData data)
{
  bool success = false;

  pushd();   // don't return without matching popd!

  switch (data) {
  case md_xform: {
    success = TbObj::writeXform(get_basename());
    break;
  }
  }

  popd();
  return success;
}


bool
GenericScan::write(const crope &fname)
{
  if (fname.empty()) {  // no filename given...
    if (!bNameSet)      // and we don't already have one
      return false;     // so bail
  } else if (name != fname) {
    cout << "Scan " << name << " being saved as " << fname << endl;

    // need to force all meshes to load before changing name
    for (int i = 0; i < resolutions.size(); i++) {
      load_resolution (i);
      meshes[i]->bNeedsSave = true;
    }

    set_name (fname);
    bNameSet = false;    // mark that we need to regenerate child names
  }

  if (ending == crope("set")) {
  } else {
    //need to generate .set name from current name
    const char* dirName = strrchr (name.c_str(), '/');
    if (dirName == NULL) {
      dirName = name.c_str();
    } else {
      dirName++; // skip past the /
    }
    name += crope("/") + crope(dirName);

    set_name (name + ".set");
    cout << "Creating new set file " << name << endl;

    setd (name, true);
  }

  cout << "Writing whole scan to " << name << endl;
  ofstream out (name.c_str());

  out << "NumMeshes = " << meshes.size() << endl;
  out << "DefaultRes = " << meshes[current_resolution_index()]->num_tris()
      << endl;

  pushd();
  for (int i = 0; i < meshes.size(); i++) {
    if (!bNameSet) {
      char stupidbuf[10];
      sprintf (stupidbuf, "%d", meshes[i]->num_tris());

      resolutions[i].filename = basename
	+ "." + stupidbuf + ".ply";
      // need to rename associated meshes
    }

    out << (resolutions[i].desired_in_mem ? "preload" : "noload") << " ";
    out << resolutions[i].abs_resolution << " ";
    out << resolutions[i].filename << endl;

    if (resolutions[i].in_memory && meshes[i]->bNeedsSave) {
      cout << "Writing mesh " << resolutions[i].filename << endl;
      meshes[i]->writePlyFile(resolutions[i].filename.c_str(), true, false);
    }
  }
  popd();
  cout << "Done." << endl;

  // save transform
  write_metadata (md_xform);

  bDirty = false;
  bNameSet = true;
  return true;
}


bool
GenericScan::write_resolution_mesh (int nPolys, const crope& fname,
				    Xform<float> xfBy)
{
  if (!xfBy.isIdentity()) {
    // can't handle this any better than default implementation
    return RigidScan::write_resolution_mesh (nPolys, fname, xfBy);
  }

  int iRes = findLevelForRes (nPolys);
  if (iRes < 0 || iRes >= resolutions.size())
    return false;

  crope& meshName = resolutions[iRes].filename;
  if (!fname.empty())
    meshName = fname;

  if (meshName.empty())
    return false;

  cout << "Writing plyfile " << meshName << "... "<< flush;
  bool ok = getMesh(iRes)->writePlyFile (meshName.c_str(), false, false);
  if (ok)
    cout << "done." << endl;
  else
    cout << "failed!" << endl;

  return ok;
}


int
PlvWritePlyForVripCmd(ClientData clientData,
		      Tcl_Interp *interp,
		      int argc, char* argv[])
{
  if (argc < 3) {
      interp->result =
	"Usage: PlvWritePlyForVripCmd <res level, 0=max> <dir> ";
      return TCL_ERROR;
  }

  int iRes = atoi (argv[1]);
  char* dir = argv[2];
  portable_mkdir (dir, 00775);

  char name[PATH_MAX];
  crope result;
  bool success = true;

  // vrip can't read tstrips
  char* oldStripVal = Tcl_GetVar (interp, "meshWriteStrips",
				  TCL_GLOBAL_ONLY);
  Tcl_SetVar (interp, "meshWriteStrips", "never",
	      TCL_GLOBAL_ONLY);

  char confFN[PATH_MAX];
  sprintf (confFN, "%s/vrip.conf", dir);
  ofstream conffile (confFN, ios::app);

// STL Update
  vector<DisplayableMesh*>::iterator dm = theScene->meshSets.begin();
  for (; dm < theScene->meshSets.end(); dm++) {
    if (!(*dm)->getVisible())
      continue;

    RigidScan* sd = (*dm)->getMeshData();
    GenericScan* gs = dynamic_cast<GenericScan*> (sd);
    if (!gs)
      continue;

    vector<ResolutionCtrl::res_info> ri;
    int thisIRes;

    const char* filename = sd->get_basename().c_str();
    const char* slash;
    if (NULL != (slash = strrchr (filename, '/')))
      filename = slash + 1;
    else if (NULL != (slash = strrchr (filename, '\\')))
      filename = slash + 1;

    sprintf(name, "%s/%s-v.ply", dir, filename);

    if (access (name, R_OK)) {
	  cout << "Writing " << name << " ... " << flush;
    } else {
	  cout << "Skipping " << name << " (already exists) ... " << endl;
	  continue;
    }

    sd->existing_resolutions (ri);
    thisIRes = MIN (iRes, ri.size() - 1);
    gs->load_resolution (thisIRes);

    sd->existing_resolutions (ri);
    int res = ri[thisIRes].abs_resolution;

    if (res) {

	  // write plyfile
	  if (!gs->write_resolution_mesh (res, name, Xform<float>())) {
	    cerr << "Scan " << sd->get_name()
		 << " failed to write self!"  << endl;
	    return TCL_ERROR;
	  }

	  cout << "done." << endl;

	  // write config string:
	  // bmesh filename x y z i j k r
	  char bmesh[PATH_MAX + 200];
	  float t[3];
	  float q[4];
	  Xform<float> xf = sd->getXform();
	  xf.toQuaternion (q);
	  xf.getTranslation (t);
	  sprintf (bmesh, "bmesh %s %g %g %g %g %g %g %g",
		   1+strrchr(name,'/'),
		   t[0], t[1], t[2], -q[1], -q[2], -q[3], q[0]);

	  // write xf
	  strcpy (name + strlen(name) - 3, "xf");
	  cout << "Writing " << name << " ... " << flush;
	  ofstream xffile (name);
	  xffile << xf;
	  if (xffile.fail()) {
	    cerr << "Scan " << sd->get_name()
		 << " failed to write xform!" << endl;
	    success = false;
	    break;
	  }
	  cout << "done." << endl;

	  conffile << bmesh << endl;

    }

  }

  // cleanup
  Tcl_SetVar (interp, "meshWriteStrips", oldStripVal,
	      TCL_GLOBAL_ONLY);

  if (!success)
    return TCL_ERROR;

  return TCL_OK;
}

void GenericScan::dequantizationSmoothing(int iterations, double maxDisplacement)
{
  Mesh *mesh=currentMesh();

  // Get the original verts into both lists
  mesh->restoreOrigVerts();

  for (int i=0;i<iterations;i++)
    mesh->dequantizationSmoothing(maxDisplacement);
}

void GenericScan::commitSmoothingChanges()
{
  Mesh *mesh=currentMesh();

  // Copy the vtx -> orig_vtx, so the real orig is gone
  mesh->saveOrigVerts();
}

