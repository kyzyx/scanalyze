#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <vector.h>
#include <hash_map.h>
#include <assert.h>
#include <math.h>

#include "Mesh.h"
#include "ply++.h"
#include "Timer.h"
#include "Random.h"
#include "plvGlobals.h"
#include "TriMeshUtils.h"
#include "Progress.h"
#include "plvScene.h"


#if 0
#  define VERBOSE 1
#else
#  define VERBOSE 0
#endif


Mesh::Mesh()
{
  init();
}

Mesh::Mesh (const vector<Pnt3>& _vtx, const vector<int>& _tris)
{
  init();
  vtx = _vtx;
  tris = _tris;

  initNormals(UseAreaWeightedNormals);
  computeBBox();
}


void Mesh::init()
{
  vertMatDiff = NULL;
  vertIntensity = NULL;
  vertConfidence = NULL;
  texture = NULL;
  triMatDiff = NULL;
  hasVertNormals = FALSE;
  bNeedsSave = FALSE;
  numTris = 0;
  hasVoxels = FALSE;
}


Mesh::~Mesh()
{
  if (vertMatDiff != NULL)
    delete [] vertMatDiff;

  if (vertIntensity != NULL)
    delete [] vertIntensity;

  if (vertConfidence != NULL)
    delete [] vertConfidence;

  if (texture != NULL)
    delete [] texture;

  if (triMatDiff != NULL)
    delete [] triMatDiff;
}


void
Mesh::initNormals(int useArea)
{
  if (!hasVertNormals) {
    if (tstrips.size())
      getVertexNormals(vtx, tstrips, true, nrm, useArea);
    else
      getVertexNormals(vtx, tris, false, nrm, useArea);
    hasVertNormals = true;
  }
}

void
Mesh::flipNormals()
{
  if (tris.size())
    flip_tris (tris, false);

  if (tstrips.size())
    flip_tris (tstrips, true);

  int n = nrm.size();
  for (int i = 0; i < n; i++)
    nrm[i] = -nrm[i];

  bNeedsSave = TRUE;
}


void
Mesh::computeBBox()
{
  bbox.clear();

  for (int i = 0; i < vtx.size(); i++) {
    bbox.add(vtx[i]);
  }

  if (vtx.size() == 0) {
    center = Pnt3();
    radius = 0;
  } else {
    center = bbox.center();
    radius = bbox.diag()*.5;
  }
}


void
Mesh::updateScale()
  // this exists because Mesh is only used to read .ply files, and
  // there's a lot of old legacy data that exists in meters instead
  // of millimeters.  Not only will it display at the wrong scale
  // (only a problem if you view it side-by-side with new data),
  // but things that are typical sizes for a scan with the older
  // CyberWare scanners, like the dragon (say 20 cm across), expressed in
  // meters, will have dimensions on the scale of 0.01.  Inter-point
  // spacing will be like .0001, and so cross products at triangle
  // vertices will have norms below float thresholds.  Thus the normals
  // used for lighting will be calculated incorrectly.  Unfortunately the
  // headers of these files don't say whether they're measured in meters
  // or millimeters.  So we'll try to guess from the size of the bbox.
{
  // if the largest dimension is smaller than 10, assume we're dealing
  // with millimeters and scale everything up by 1000.

  // will false-positive on millimeter-scale scans of objects smaller than
  // 1cm x 1cm x 1cm (and they'll look too big).

  // will false-negative on meter-scale scans of objects bigger than 10m
  // on any side (and they'll stay too small, and render black).

  // will not affect ply conversions of Cyra data, whose dimensions are
  // big enough to be in the bounds of the above test, because the test is
  // one-way, only growing things -- although if Cyra data were mistakenly
  // saved as meters, this test would probably not catch it (the false
  // negative described above).

  // But wait!  This hack sucks rocks for pvrip output.  Often
  // you get one or two chunks that are too small, so they scale
  // by 1000, and suddenly the viewing bbox is 1000x too big,
  // and everything else is 1000x too small (in comparison) and
  // so you only see that tiny chunk (or nothing at all) when
  // the viewer pops up.  You have to find and delete the scan
  // that got multiplied before you can rescale the viewer to
  // be able to see the rest of the mesh. UGLY Hack.  Turning
  // it off, legacy crud be damned.  -- Lucas

  computeBBox();
#if 0
  if (bbox.maxDim() < 10) {
    cerr << "Warning: scaling mesh from meters to millimeters" << endl;
// STL Update
    for (vector<Pnt3*>::iterator vp = vtx.begin(); vp != vtx.end(); vp++) {
      *vp *= 1000;
    }

    computeBBox();
  }
#endif
}


int
Mesh::num_tris (void)
{
  if (tris.size())
    return (numTris = tris.size() / 3);

  if (numTris)
    return numTris;

  // ok, have to count tstrips
  numTris = tstrips.size();
// STL Update
  for (vector<int>::iterator ti = tstrips.begin(); ti < tstrips.end(); ti++) {
    if ((*ti) == -1)
      numTris -= 3;
  }

  return numTris;
}

vector<int>&
Mesh::getTris (void)
{
  if (!tris.size()) {
    strips_to_tris(tstrips, tris, numTris);
    printf ("rehydrated %d tris\n", num_tris());
  }

  return tris;
}


vector<int>&
Mesh::getTstrips (void)
{
  if (!tstrips.size()) {
#if VERBOSE
    TIMER(Build_t_strips);
#endif
    tris_to_strips(vtx.size(), tris, tstrips);
  }

  return tstrips;
}


void
Mesh::freeTris (void)
{
  if (tstrips.size() == 0)
    printf ("Cannot free triangles because strips don't exist\n");
  else {
    tris.clear();
    delete [] triMatDiff;
    triMatDiff = NULL;
  }
}


void
Mesh::freeTStrips (void)
{
  if (tris.size() == 0)
    printf ("Cannot free tstrips because separate triangles don't exist\n");
  else {
    tstrips.clear();
  }
}


// src and dst are indices to the triangle face (i.e., vertex face / 3).

void
Mesh::copyTriFrom (Mesh *meshSrc, int src, int dst)
{
  if (meshSrc->hasVoxels) this->hasVoxels = true;
  for (int i = 0; i < 3; i++) {
    // make sure triangle is within bounds of vector
    assert(3*dst+i <= this->tris.size());

    if (3*dst+i < this->tris.size()) {
      this->tris[3*dst+i] = meshSrc->tris[3*src+i];
      // copy voxel in - for voxel display feature
      if (meshSrc->hasVoxels)
	this->fromVoxels[3*dst+i] = meshSrc->fromVoxels[3*src+i];

    } else {  // adding to the end
      this->tris.push_back (meshSrc->tris[3*src+i]);
      if (meshSrc->hasVoxels)
	this->fromVoxels.push_back(meshSrc->fromVoxels[3*src+i]);

    }
  }

  if (meshSrc->triMatDiff != NULL) {
    this->triMatDiff[dst][0] = meshSrc->triMatDiff[src][0];
    this->triMatDiff[dst][1] = meshSrc->triMatDiff[src][1];
    this->triMatDiff[dst][2] = meshSrc->triMatDiff[src][2];
  }
}


void
Mesh::copyVertFrom (Mesh *meshSrc, int src, int dst)
{
  assert (dst <= this->vtx.size());
  if (dst < this->vtx.size())
    this->vtx[dst] = meshSrc->vtx[src];
  else
    this->vtx.push_back(meshSrc->vtx[src]);

  if (this->nrm.capacity() && meshSrc->nrm.size()) {
    int nd = 3 * dst;
    int ns = 3 * src;
    if (nd < this->nrm.size()) {
      this->nrm[nd  ] = meshSrc->nrm[ns  ];
      this->nrm[nd+1] = meshSrc->nrm[ns+1];
      this->nrm[nd+2] = meshSrc->nrm[ns+2];
    } else {
      this->nrm.push_back(meshSrc->nrm[ns  ]);
      this->nrm.push_back(meshSrc->nrm[ns+1]);
      this->nrm.push_back(meshSrc->nrm[ns+2]);
    }
  }

  if (meshSrc->vertMatDiff != NULL) {
    this->vertMatDiff[dst][0] = meshSrc->vertMatDiff[src][0];
    this->vertMatDiff[dst][1] = meshSrc->vertMatDiff[src][1];
    this->vertMatDiff[dst][2] = meshSrc->vertMatDiff[src][2];
  }

  if (meshSrc->vertIntensity != NULL) {
    this->vertIntensity[dst] = meshSrc->vertIntensity[src];
  }

  if (meshSrc->vertConfidence != NULL) {
    this->vertConfidence[dst] = meshSrc->vertConfidence[src];
  }

  if (meshSrc->texture != NULL) {
    this->texture[dst][0] = meshSrc->texture[src][0];
    this->texture[dst][1] = meshSrc->texture[src][1];
  }
}




void
Mesh::mark_boundary_verts(void)
{
  if (bdry.size()) return;
  // initialize the boundary flags
  //bdry.erase(bdry.begin(), bdry.end());
  bdry.insert(bdry.end(), vtx.size(), 0);
  ::mark_boundary_verts(bdry, getTris());
}


void plycrunch_simplify (const vector<Pnt3>& vtx, const vector<int>& tris,
			 vector<Pnt3>& outVtx, vector<int>& outTris,
			 int approxDesired);

Mesh *
Mesh::Decimate(int numFaces, int optLevel,
	       float errLevel, float boundWeight,
	       ResolutionCtrl::Decimator dec)
{
  Mesh  *outMesh = new Mesh();

  switch (dec) {
  case ResolutionCtrl::decQslim:
    quadric_simplify(vtx, getTris(), outMesh->vtx, outMesh->tris,
		     numFaces, optLevel, errLevel, boundWeight);
    break;

  case ResolutionCtrl::decPlycrunch:
    plycrunch_simplify (vtx, getTris(), outMesh->vtx, outMesh->tris, numFaces);
    break;

  default:
    cerr << "Mesh::Decimate: unsupported decimator " << dec << endl;
    delete outMesh;
    return NULL;
  }

  outMesh->initNormals(0);
  outMesh->computeBBox();

  return outMesh;
}

struct PlyVertex {
  float x, y, z;
  float nx, ny, nz;
  uchar diff_r, diff_g, diff_b;
  float tex_u, tex_v;
  float intensity;
  float std_dev;
  float confidence;
};

const int MAX_FACE_VERTS = 100;

/* Added field to hold voxel from which the face was derived
   - for voxel display feature */
struct PlyFace {
  uchar nverts;
  int verts[MAX_FACE_VERTS];
  float fromVoxelX, fromVoxelY, fromVoxelZ;
};


struct CoordConfVert {
   float v[3];
   float conf;
};

struct TriLocal {
   uchar nverts;
   int index[3];
};


static PlyProperty vert_prop_x =
   {"x", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_y =
  {"y", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_z =
  {"z", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_nx =
   {"nx", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_ny =
  {"ny", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_nz =
  {"nz", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_texture_u =
  {"texture_u", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_texture_v =
  {"texture_v", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_intens =
  {"intensity", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_std_dev =
  {"std_dev", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_confidence =
  {"confidence", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_diff_r =
  {"diffuse_red", PLY_UCHAR, PLY_UCHAR, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_diff_g =
  {"diffuse_green", PLY_UCHAR, PLY_UCHAR, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_diff_b =
  {"diffuse_blue", PLY_UCHAR, PLY_UCHAR, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};

/* Added another face_prop that holds data for the voxel from which
   the face was derived - for voxel display feature */
static PlyProperty face_props[] = {
  {"vertex_indices", PLY_INT, PLY_INT, 0, 1, PLY_UCHAR, PLY_UCHAR, 0},
};

static PlyProperty face_prop_from_voxel_x =
  {"from_voxel_x", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty face_prop_from_voxel_y =
  {"from_voxel_y", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty face_prop_from_voxel_z =
  {"from_voxel_z", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};


struct tstrip_info {
  int nverts;
  int* vertData;
};

static PlyProperty tstrips_props[] = {
  {"vertex_indices", PLY_INT, PLY_INT, 4, 1, PLY_INT, PLY_INT, 0},
};

static void
set_offsets(void)
{
  static int first = 1;
  if (first) {
    first = 0;
    vert_prop_x.offset          = offsetof(PlyVertex,x);
    vert_prop_y.offset          = offsetof(PlyVertex,y);
    vert_prop_z.offset          = offsetof(PlyVertex,z);
    vert_prop_nx.offset         = offsetof(PlyVertex,nx);
    vert_prop_ny.offset         = offsetof(PlyVertex,ny);
    vert_prop_nz.offset         = offsetof(PlyVertex,nz);
    vert_prop_intens.offset     = offsetof(PlyVertex, intensity);
    vert_prop_std_dev.offset    = offsetof(PlyVertex, std_dev);
    vert_prop_confidence.offset = offsetof(PlyVertex, confidence);
    vert_prop_diff_r.offset     = offsetof(PlyVertex,diff_r);
    vert_prop_diff_g.offset     = offsetof(PlyVertex,diff_g);
    vert_prop_diff_b.offset     = offsetof(PlyVertex,diff_b);
    vert_prop_texture_u.offset  = offsetof(PlyVertex,tex_u);
    vert_prop_texture_v.offset  = offsetof(PlyVertex,tex_v);

    tstrips_props[0].offset     = offsetof(tstrip_info,vertData);
  }
}

static const int progress_update = 0xfff;

int
Mesh::readPlyFile(const char *filename)
{
  int i, j;
  int nelems;
  char **elist;
  char **obj_info;
  int num_obj_info;
  char *elem_name;
  int nprops;
  int num_elems;
  int hasConfidence = 0;
  //int hasIntensity = 0;
  //int hasStdDev = 0;
  int hasDiffuseColors = 0;
  int hasTexture = 0;
  int notATriangleMesh = 0;
  int skipTexture = 1;

#if VERBOSE
  TIMER(Read_ply_file);
#endif

  face_props[0].offset = offsetof(PlyFace, verts);
  face_props[0].count_offset = offsetof(PlyFace, nverts);

  // face_prop offsets for voxel display feature
  face_prop_from_voxel_x.offset = offsetof(PlyFace, fromVoxelX);
  face_prop_from_voxel_y.offset = offsetof(PlyFace, fromVoxelY);
  face_prop_from_voxel_z.offset = offsetof(PlyFace, fromVoxelZ);

  PlyFile ply;
  if (ply.open_for_reading((char*)filename, &nelems, &elist) == 0)
    return 0;

  // Look for a texture file reference
  char temp[PATH_MAX];
  obj_info = ply.get_obj_info (&num_obj_info);
  for (i = 0; i < num_obj_info; i++) {
    if (strstr(obj_info[i], "texture_file")) {
      sscanf(obj_info[i], "%s%s", temp, texFileName);
      FILE *fp = fopen(texFileName, "r");
      if (fp != NULL) {
	fclose(fp);
	skipTexture = 0;
      } else {
	texFileName[0] = 0;
      }
    }
  }

  vector<PlyProperty> vert_props;

  set_offsets();

  PlyVertex plyVert;
  PlyFace plyFace;

  for (i = 0; i < nelems; i++) {

    // get the description of the first element
    elem_name = elist[i];
    ply.get_element_description(elem_name, &num_elems, &nprops);

    // if we're on vertex elements, read them in
    if (equal_strings ("vertex", elem_name)) {

#if VERBOSE
      TIMER(Read_vertices);
#endif

      vtx.reserve(num_elems);

      if (ply.is_valid_property("vertex", vert_prop_x.name) &&
	  ply.is_valid_property("vertex", vert_prop_y.name) &&
	  ply.is_valid_property("vertex", vert_prop_z.name)) {
	ply.get_property(elem_name, &vert_prop_x);
	ply.get_property(elem_name, &vert_prop_y);
	ply.get_property(elem_name, &vert_prop_z);
      }

      if (ply.is_valid_property("vertex", vert_prop_nx.name) &&
	  ply.is_valid_property("vertex", vert_prop_ny.name) &&
	  ply.is_valid_property("vertex", vert_prop_nz.name) ) {
	ply.get_property(elem_name, &vert_prop_nx);
	ply.get_property(elem_name, &vert_prop_ny);
	ply.get_property(elem_name, &vert_prop_nz);
	hasVertNormals = 1;
	nrm.reserve(num_elems * 3);
      }

      if (ply.is_valid_property("vertex", vert_prop_intens.name)) {
	ply.get_property(elem_name, &vert_prop_intens);
	//hasIntensity = 1;
      }

      if (ply.is_valid_property("vertex", vert_prop_std_dev.name)){
	ply.get_property(elem_name, &vert_prop_std_dev);
	//hasStdDev = 1;
      }

      if (ply.is_valid_property("vertex",
				vert_prop_confidence.name)) {
	ply.get_property(elem_name, &vert_prop_confidence);
	hasConfidence = 1;
	vertConfidence = new float[num_elems];
      }

      if (ply.is_valid_property("vertex", "diffuse_red") &&
	  ply.is_valid_property("vertex", "diffuse_green") &&
	  ply.is_valid_property("vertex", "diffuse_blue")) {
	ply.get_property(elem_name, &vert_prop_diff_r);
	ply.get_property(elem_name, &vert_prop_diff_g);
	ply.get_property(elem_name, &vert_prop_diff_b);
	hasDiffuseColors = 1;
	vertMatDiff = new vec3uc[num_elems];
      }

      if (!skipTexture) {
	if (ply.is_valid_property("vertex", "texture_u") &&
	    ply.is_valid_property("vertex", "texture_v")) {
	  ply.get_property(elem_name, &vert_prop_texture_u);
	  ply.get_property(elem_name, &vert_prop_texture_v);
	  hasTexture = 1;
	  texture = new vec2f[num_elems];
	}
      }

      Progress progress (num_elems, "%s: read vertices", filename);

      // grab all the vertex elements
      for (j = 0; j < num_elems; j++) {
	if ((j & progress_update) == progress_update)
	  progress.update (j);

	ply.get_element ((void *) &plyVert);
	vtx.push_back(Pnt3(plyVert.x, plyVert.y, plyVert.z));

	if (hasVertNormals)
	  pushNormalAsShorts (nrm, Pnt3(plyVert.nx, plyVert.ny, plyVert.nz));

	if (hasDiffuseColors) {
	  vertMatDiff[j][0] = plyVert.diff_r;
	  vertMatDiff[j][1] = plyVert.diff_g;
	  vertMatDiff[j][2] = plyVert.diff_b;
	}

	if (hasConfidence) {
	  vertConfidence[j] = plyVert.confidence;
	}

	if (hasTexture) {
	  texture[j][0] = plyVert.tex_u;
	  texture[j][1] = plyVert.tex_v;
	}
      }
    }

    if (equal_strings ("face", elem_name)) {

#if VERBOSE
      TIMER(Read_faces);
#endif

      ply.get_property(elem_name, face_props);

      /* check to see if we have voxel information for voxel
         display feature */
      if (ply.is_valid_property("face", "from_voxel_x")
	  || ply.is_valid_property("face", "from_voxel_y")
	  || ply.is_valid_property("face", "from_voxel_z")) {
	printf("Voxel information present...\n");
	ply.get_property(elem_name, &face_prop_from_voxel_x);
	ply.get_property(elem_name, &face_prop_from_voxel_y);
	ply.get_property(elem_name, &face_prop_from_voxel_z);
	hasVoxels = 1;
      } else printf("No voxel information stored...\n");

      if (num_elems == 0)
	continue;

      assert(tris.size() == 0);
      tris.reserve (num_elems * 3);
      Progress progress (num_elems, "%s: read tris", filename);

      for (j = 0; j < num_elems; j++) {
	if ((j & progress_update) == progress_update)
	  progress.update (j);

	ply.get_element_noalloc((void *) &plyFace);
	if (plyFace.nverts != 3)
	  notATriangleMesh = 1;
	assert(plyFace.nverts <= MAX_FACE_VERTS);
	addTri(plyFace.verts[0],
	       plyFace.verts[1],
	       plyFace.verts[2]);
	if (hasVoxels) {
	  /* printf("adding voxel %f, %f, %f\n",
		 plyFace.fromVoxelX, plyFace.fromVoxelY,
		 plyFace.fromVoxelZ); */
	  /* store voxel here - for voxel display feature */

	  addVoxelInfo(plyFace.verts[0], plyFace.verts[1],
		       plyFace.verts[2],
		       plyFace.fromVoxelX, plyFace.fromVoxelY,
		       plyFace.fromVoxelZ);
	}

	//delete[] plyFace.verts;
      }

      if (notATriangleMesh) {
	cerr << "readPlyFile: Not a triangle mesh!" << endl;
      }
    }

    if (equal_strings ("tristrips", elem_name)) {

      tstrip_info info;

      if (num_elems != 1)
	continue;

#if VERBOSE
      TIMER(Read_stripped_faces);
#endif

      ply.get_property (elem_name, tstrips_props);
      ply.get_element ((void*)&info);
      tstrips.reserve (info.nverts);
      Progress progress (info.nverts, "%s: read t-strips", filename);

      int lastend = 0;
      for (int iv = 0; iv < info.nverts; iv++) {
	if ((iv & progress_update) == progress_update)
	  progress.update (iv);

	int vert = info.vertData[iv];
	bool ok = (vert >= 0 && vert < vtx.size())
	  || (vert == -1 && lastend >= 3);

	if (ok) {
	  // middle of strip
	  tstrips.push_back (vert);
	  if (vert == -1)
	    lastend = 0;
	  else
	    ++lastend;
	} else {
	  cerr << "\n\nRed alert: invalid index " << vert
	       << " in tstrip (index = " << iv
	       << ", valid range=0.." << vtx.size() - 1
	       << ")\n" << endl;
	  // remove last strip
	  if (lastend) {
	    while (--lastend)
	      tstrips.pop_back();
	  }
	}
      }
    }
  }

  return 1;
}

int
Mesh::writePlyFile (const char *filename, int useColorNotTexture,
		    int writeNormals)
{
#if 1
  vector<int>* pInds = NULL;
  bool bStrips = false;
  vector<uchar> color;
  vector<float> conf;

  if (theScene->meshes_written_stripped()) {
    pInds = &getTstrips();
    bStrips = true;
  } else {
    pInds = &getTris();
    bStrips = false;
  }

  if (vertMatDiff) {
    color.reserve (vtx.size() * 3);
    for (int i = 0; i < vtx.size(); i++) {
      color.push_back (vertMatDiff[i][0]);
      color.push_back (vertMatDiff[i][1]);
      color.push_back (vertMatDiff[i][2]);
    }
  } else if (vertIntensity) {
    color.reserve (vtx.size());
    for (int i = 0; i < vtx.size(); i++)
      color.push_back (vertIntensity[i]);
  }

  if (vertConfidence) {
    conf.reserve (vtx.size());
    for (int i = 0; i < vtx.size(); i++)
      conf.push_back (vertConfidence[i]);
  }

  if (color.size())
    if (conf.size())
      write_ply_file (filename, vtx, *pInds, bStrips, color, conf);
    else
      write_ply_file (filename, vtx, *pInds, bStrips, color);
  else
    if (conf.size())
      write_ply_file (filename, vtx, *pInds, bStrips, conf);
    else
      write_ply_file (filename, vtx, *pInds, bStrips);

  return true;
#else
  int i;
  char *elem_names[] = {"vertex", "face"};

  int hasIntensity  = vertIntensity != NULL;
  int hasColor      = vertMatDiff != NULL;
  int hasConfidence = vertConfidence != NULL;
  int hasTexture    = texture != NULL;

  PlyFile ply;
  if (!ply.open_for_writing((char*)filename, 2, elem_names,PLY_BINARY_BE))
    return 0;

  if (hasTexture && !useColorNotTexture) {
    char obj_info[PATH_MAX];
    sprintf(obj_info, "%s %s", "texture_file", texFileName);
    ply.put_obj_info(obj_info);
  }

  vector<PlyProperty> vert_props;
  set_offsets();

  vert_props.push_back(vert_prop_x);
  vert_props.push_back(vert_prop_y);
  vert_props.push_back(vert_prop_z);

  if (writeNormals) {
    vert_props.push_back(vert_prop_nx);
    vert_props.push_back(vert_prop_ny);
    vert_props.push_back(vert_prop_nz);
  }

  if (hasIntensity) {
    vert_props.push_back(vert_prop_intens);
  }

  if (hasConfidence) {
    vert_props.push_back(vert_prop_confidence);
    hasConfidence = 1;
  }

  if (hasColor || (hasTexture && useColorNotTexture)) {
    vert_props.push_back(vert_prop_diff_r);
    vert_props.push_back(vert_prop_diff_g);
    vert_props.push_back(vert_prop_diff_b);
  }

  if (hasTexture && !useColorNotTexture) {
    vert_props.push_back(vert_prop_texture_u);
    vert_props.push_back(vert_prop_texture_v);
  }

  // count offset
  face_props[0].offset = offsetof(PlyFace, verts);
  face_props[0].count_offset = offsetof(PlyFace, nverts);

  ply.describe_element("vertex", vtx.size(),
		       vert_props.size(), &vert_props[0]);
  ply.describe_element("face", tris.size()/3, 1, face_props);
  ply.header_complete();

    // set up and write the vertex elements
  PlyVertex plyVert;

  ply.put_element_setup("vertex");

  for (i = 0; i < vtx.size(); i++) {
    plyVert.x = vtx[i][0];
    plyVert.y = vtx[i][1];
    plyVert.z = vtx[i][2];
    if (hasIntensity)
      plyVert.intensity = vertIntensity[i];

    if (hasConfidence)
      plyVert.confidence = vertConfidence[i];

    if (hasColor) {
      plyVert.diff_r = vertMatDiff[i][0];
      plyVert.diff_g = vertMatDiff[i][1];
      plyVert.diff_b = vertMatDiff[i][2];
    }

    if (writeNormals) {
      plyVert.nx = nrm[i][0];
      plyVert.ny = nrm[i][1];
      plyVert.nz = nrm[i][2];
    }

    /*
    if (hasTexture && !useColorNotTexture) {
      plyVert.tex_u = texture[i][0];
      plyVert.tex_v = texture[i][1];
    }
    else if (hasTexture && useColorNotTexture) {
      float texU = texture[i][0];
      float texV = texture[i][1];
      int xOff = int(texU*meshSet->texXdim);
      int yOff = int(texV*meshSet->texYdim);

      // shouldn't that be texXdim, not texYdim*yOff ??
      uchar *col = &meshSet->
	texture[3*(xOff + meshSet->texYdim*yOff)];

      plyVert.diff_r = col[0];
      plyVert.diff_g = col[1];
      plyVert.diff_b = col[2];
    }
    */
    ply.put_element((void *) &plyVert);
  }
  PlyFace plyFace;

  ply.put_element_setup("face");

  for (i = 0; i < tris.size(); i+=3) {
    plyFace.nverts = 3;
    plyFace.verts[0] = tris[i+0];
    plyFace.verts[1] = tris[i+1];
    plyFace.verts[2] = tris[i+2];
    ply.put_element_static_strg((void *) &plyFace);
  }

  return 1;
#endif
}


static Random rnd;

void
Mesh::subsample_points(float rate, vector<Pnt3> &pts)
{
  subsample_points(int(rate*vtx.size()), pts);
}


void
Mesh::subsample_points(float rate, vector<Pnt3> &pts,
		       vector<Pnt3> &nrms)
{
  subsample_points(int(rate*vtx.size()), pts, nrms);
}


bool
Mesh::subsample_points(int n, vector<Pnt3> &pts)
{
  pts.clear(); pts.reserve(n);
  double nv = vtx.size();
  if (n > nv) return 0;
  double np = n;
  int end = nv;
  for (int i = 0; i<end; i++) {
    if (rnd(nv) <= np) {
      pts.push_back(vtx[i]);   // save point
      np--;
    }
    nv--;
  }
  assert(np == 0);
  return 1;
}


bool
Mesh::subsample_points(int n, vector<Pnt3> &pts,
		       vector<Pnt3> &nrms)
{
  pts.clear(); pts.reserve(n);
  nrms.clear(); nrms.reserve(n);
  double nv = vtx.size();
  if (n > nv) return 0;
  double np = n;
  int end = nv;
  for (int i = 0; i<end; i++) {
    if (rnd(nv) <= np) {
      pts.push_back(vtx[i]);    // save point
      pushNormalAsPnt3 (nrms, nrm.begin(), i);
      np--;
    }
    nv--;
  }
  assert(np == 0);
  return 1;
}


void
Mesh::remove_unused_vtxs(void)
{
  // prepare vertex index map
  vector<int> vtx_ind(vtx.size(), -1);

  // march through the triangles
  // and mark the vertices that are actually used
  int n = getTris().size();
  for (int i=0; i<n; i++) {
    assert (tris[i] >= 0 && tris[i] < vtx.size());
    vtx_ind[tris[i]] = tris[i];
  }

  // remove the vertices that were not marked,
  // also keep tab on how the indices change
  int cnt = 0;
  n = vtx.size();
  for (i=0; i<n; i++) {
    if (vtx_ind[i] != -1) {
      vtx_ind[i] = cnt;
      copyVertFrom (this, i, cnt);
      cnt++;
    }
  }
// STL Update
  vtx.erase(vtx.begin() + cnt, vtx.end());
  if (nrm.size()) nrm.erase(nrm.begin() + (cnt*3), nrm.end());
  // march through triangles and correct the indices
  n = tris.size();
  for (i=0; i<n; i++) {
    tris[i] = vtx_ind[tris[i]];
    assert (tris[i] >=0 && tris[i] < vtx.size());
  }
}

// find the median edge length
// if a triangle has an edge that's longer than
// factor times the median (or percentile), remove the triangle
void
Mesh::remove_stepedges(int percentile, int factor)
{
  ::remove_stepedges(vtx, getTris(), factor, percentile);
  remove_unused_vtxs();
  freeTStrips();
}


static Pnt3 GetNrm (const vector<short>& nrm, int ivert)
{
  ivert *= 3;
  Pnt3 n (nrm[ivert], nrm[ivert+1], nrm[ivert+2]);
  n /= 32767.0;
  return n;
}


void
Mesh::simulateFaceNormals (vector<short>& facen)
{
  facen.clear();
  facen.reserve (getTris().size());

  // note: Mesh doesn't store per tri normals any more
  for (int i = 0; i < tris.size(); i+=3) {
    Pnt3 n  = GetNrm (nrm, tris[i+0]);
    n += GetNrm (nrm, tris[i+1]);
    n += GetNrm (nrm, tris[i+2]);
    n /= 3.0;

    pushNormalAsShorts (facen, n);
  }
}


// added by wsh for warping violin meshes (test stuff)
// A little dangerous 'cuz we're fudging the verticies as we go
void
Mesh::Warp()
{
  printf("(%f, %f, %f)\n", center[0], center[1], center[2]);

  for ( vector<Pnt3>::iterator i = vtx.begin();
	i != vtx.end();
	i++ ) {
    (*i)[2] += 7 * cos(0.017674205 * (*i)[1]) +
      5 * cos(0.030921161  * (*i)[0]);
  }
}

/***************************************************************************
               D E Q U A N T I Z A T I O N S M O O T H I N G
***************************************************************************/
void Mesh::dequantizationSmoothing(double maxDisplacement)
{
  cout << "Number of verts: " << vtx.size() << endl;

  // Make sure we have the inverse table
  calcTriLists();

  // Get a structure for new points of appropriate size
  vector<Pnt3> nvtx;
  nvtx=vtx;
  printf ("size nvtx %ld\n",nvtx.size());

  // print info every how many tris? (mod count)
  int mc= 100000;

  // Loop over every vtx, blurring
  for (int i=0;i<vtx.size();i++)
    {
      if (i%mc==0) printf("Vertex Number %d tri count %ld\n ",
			  i,vtxTris[i].size());

      Pnt3 pnt(0,0,0);
      double w=0;
      // Loop over all tris touching this vert
      for (TriListI j=vtxTris[i].begin();j!=vtxTris[i].end();j++)
	{
	  if (i%mc==0) printf("  Tri Number: %d\n",(*j));

	  // Loop over all verts in adjacent tri
	  // note that we end up double counting verts
	  for (int k=0;k<3;k++)
	    {
	      w++;
	      Pnt3 npnt=vtx[tris[(*j)*3+k]];
	      if (i%mc==0) printf ("    Vtx %f  %f  %f\n",npnt[0],npnt[1],npnt[2]);
	      if (i%mc==0) printf ("      Pnt %f  %f  %f\n",pnt[0],pnt[1],pnt[2]);
	      pnt= pnt + npnt;
	      if (i%mc==0) printf ("      Pnt %f  %f  %f\n",pnt[0],pnt[1],pnt[2]);
	    }
	}
      // Divide the weight ,( number of verts) out
      pnt= pnt / w;

      // Check the distance, so that we constrain.

      // Assign the new value
      nvtx[i]=pnt;
    }

  printf ("size nvtx %ld\n",nvtx.size());

  // Copy new points to main store
  vtx=nvtx;

  puts ("finish");
}

/***************************************************************************
                      R E S T O R E O R I G V E R T S
***************************************************************************/
void Mesh::restoreOrigVerts()
{
  // Don't restore if we never saved away the original
  if (orig_vtx.size()==vtx.size())
    {
      vtx=orig_vtx;
    }
  else if (orig_vtx.size()==0)
    {
      saveOrigVerts();
    }
}

/***************************************************************************
                         S A V E O R I G V E R T S
***************************************************************************/
void Mesh::saveOrigVerts()
{
  orig_vtx=vtx;
}

/***************************************************************************
                          C A L C T R I L I S T S
***************************************************************************/
void Mesh::calcTriLists()
{

  // Don't bother if we already did it
  if (vtxTris.size()==vtx.size()) return;

  // Insure that we have the tris structure

  // Size the vtxTris structure correctly
  TriList empty;
  for (int j=0;j<vtx.size();j++)
    {
      vtxTris.push_back(empty);
    }

  cout << "sizes match?  " << vtxTris.size() << " " << vtx.size() << endl;

  vector<int>&ltris=getTris();

  int triSize=ltris.size()/3;
  int vtxSize=vtx.size();


  cout << "Number of tris " << triSize << endl;

  // Fill in the structure
  for (int i=0;i<triSize*3;i++)
    {
      int curtri=i/3;
      assert(curtri<triSize);

      int curvert=ltris[i];
      assert(curvert<vtxSize);

      //printf ("before %d\n",vtxTris[curvert].size());
      vtxTris[curvert].insert(curtri);
      //printf ("after %d\n",vtxTris[curvert].size());
    }
}
