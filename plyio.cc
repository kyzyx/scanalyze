#include <stdio.h>
#include "ply++.h"
#include <stdlib.h>
#include "strings.h"
#include <iostream.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include "timer.h"

#include "vector.h"

#include "Mesh.h"
#include "plyio.h"

#define SHOW(x) cout << #x " = " << x << endl

static int Verbose = 1;

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

struct PlyFace {
  uchar nverts;
  int verts[MAX_FACE_VERTS];
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

static PlyProperty face_props[] = {
  {"vertex_indices", PLY_INT, PLY_INT, 0, 1, PLY_UCHAR, PLY_UCHAR, 0},
  //{"vertex_indices", PLY_INT, PLY_INT, 4, 1, PLY_INT, PLY_INT, 0},
};

static PlyProperty tstrips_props[] = {
  {"vertex_indices", PLY_INT, PLY_INT, 4, 1, PLY_INT, PLY_INT, 0},
};

static Mesh *oldReadPlyFile(char *filename);
static Mesh *mattReadPlyFile(char *filename);

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
  }
}

Mesh *
readPlyFile(char *filename)
{
  /*
  //Mesh* mesh = mattReadPlyFile (filename);
  //if (mesh == NULL) {
    Mesh *mesh = oldReadPlyFile (filename);
    if (mesh != NULL)
      //printf ("Warning: file %s does not conform to dmich's\n"
      //      "expected ply layout; cannot use fastread code.\n",
      //      filename);
      printf ("Warning: using oldReadPlyFile()\n");
    else
      printf ("Cannot read plyfile %s\n", filename);
    //}
  */
  Mesh *mesh = new Mesh;
  if (mesh->readPlyFile (filename)) {
    return mesh;
  }
  delete mesh;
  return NULL;
}


//
// Read in data from the ply file
//
/*
Mesh *
oldReadPlyFile(char *filename)
{
  int i, j;
  int nelems;
  char **elist;
  char **obj_info;
  int num_obj_info;
  char *elem_name;
  int nprops;
  int num_elems;
  int hasNormals = 0;
  int hasConfidence = 0;
  int hasIntensity = 0;
  int hasStdDev = 0;
  int hasDiffuseColors = 0;
  int notATriangleMesh = 0;
  int hasTexture = 0;
  int skipTexture = 1;
  char texFile[PATH_MAX];

  Timer t ("Read ply file (via plylib)");

  face_props[0].offset = offsetof(PlyFace, verts);
  face_props[0].count_offset = offsetof(PlyFace, nverts);  // count offset

  PlyFile ply;
  if (ply.open_for_reading(filename, &nelems, &elist) == 0)
    return NULL;

  Mesh *mesh = new Mesh;

  // Look for a texture file reference
  char temp[PATH_MAX];
  obj_info = ply.get_obj_info (&num_obj_info);
  for (i = 0; i < num_obj_info; i++) {
    if (strstr(obj_info[i], "texture_file")) {
      sscanf(obj_info[i], "%s%s", temp, texFile);
      FILE *fp = fopen(texFile, "r");
      if (fp != NULL) {
	fclose(fp);
	skipTexture = 0;
	strcpy(mesh->texFileName, texFile);
      }
    }
  }

  vector<PlyProperty> vert_props;

  set_offsets();

  Vertex *vert;
  PlyVertex plyVert;
  PlyFace plyFace;

  for (i = 0; i < nelems; i++) {

    // get the description of the first element
    elem_name = elist[i];
    ply.get_element_description(elem_name, &num_elems, &nprops);

    // if we're on vertex elements, read them in
    if (equal_strings ("vertex", elem_name)) {

      Timer tv ("Read vertices");

      mesh->numVerts = num_elems;
      mesh->verts = new Vertex[num_elems];

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
	hasNormals = 1;
      }
      mesh->hasVertNormals = hasNormals;

      if (ply.is_valid_property("vertex", vert_prop_intens.name)) {
	ply.get_property(elem_name, &vert_prop_intens);
	hasIntensity = 1;
      }

      if (ply.is_valid_property("vertex", vert_prop_std_dev.name)){
	ply.get_property(elem_name, &vert_prop_std_dev);
	hasStdDev = 1;
      }

      if (ply.is_valid_property("vertex",
				vert_prop_confidence.name)) {
	ply.get_property(elem_name, &vert_prop_confidence);
	hasConfidence = 1;
	mesh->vertConfidence = new float[num_elems];
      }

      if (ply.is_valid_property("vertex", "diffuse_red") &&
	  ply.is_valid_property("vertex", "diffuse_green") &&
	  ply.is_valid_property("vertex", "diffuse_blue")) {
	ply.get_property(elem_name, &vert_prop_diff_r);
	ply.get_property(elem_name, &vert_prop_diff_g);
	ply.get_property(elem_name, &vert_prop_diff_b);
	hasDiffuseColors = 1;
	mesh->vertMatDiff = new vec3uc[num_elems];
      }

      if (!skipTexture) {
	if (ply.is_valid_property("vertex", "texture_u") &&
	    ply.is_valid_property("vertex", "texture_v")) {
	  ply.get_property(elem_name, &vert_prop_texture_u);
	  ply.get_property(elem_name, &vert_prop_texture_v);
	  hasTexture = 1;
	  mesh->texture = new vec2f[num_elems];
	}
      }

      if (Verbose) cout << "Reading vertices..." << endl;

      SHOW(mesh->numVerts);

      // grab all the vertex elements
      for (j = 0; j < mesh->numVerts; j++) {
	ply.get_element ((void *) &plyVert);
	vert = &mesh->verts[j];

	vert->coord.setValue(plyVert.x, plyVert.y, plyVert.z);

	if (hasNormals) {
	  vert->norm.setValue(plyVert.nx, plyVert.ny, plyVert.nz);
	}

	if (hasDiffuseColors) {
	  mesh->vertMatDiff[j][0] = plyVert.diff_r;
	  mesh->vertMatDiff[j][1] = plyVert.diff_g;
	  mesh->vertMatDiff[j][2] = plyVert.diff_b;
	}

	if (hasConfidence) {
	  mesh->vertConfidence[j] = plyVert.confidence;
	}

	if (hasTexture) {
	  mesh->texture[j][0] = plyVert.tex_u;
	  mesh->texture[j][1] = plyVert.tex_v;
	}
      }

      if (Verbose) cout << "Done." << endl;

    }

    if (equal_strings ("face", elem_name)) {

      Timer tf ("Read faces");

      ply.get_property(elem_name, face_props);

      assert(mesh->tris.size() == 0);

      if (num_elems == 0)
	continue;

      if (Verbose) cout << "Reading Tris..." << endl;

      SHOW(num_elems);
      for (j = 0; j < num_elems; j++) {
	ply.get_element_noalloc((void *) &plyFace);
	if (plyFace.nverts != 3)
	  notATriangleMesh = 1;
	assert(plyFace.nverts <= MAX_FACE_VERTS);
	mesh->tris.push_back(Triangle(plyFace.verts[0],
				      plyFace.verts[1],
				      plyFace.verts[2]));
	//delete[] plyFace.verts;
      }

      if (notATriangleMesh) {
	cerr << "readPlyFile: Not a triangle mesh!" << endl;
      }

      if (Verbose) cout << "Done." << endl;
    }

    if (equal_strings ("tristrips", elem_name)) {

      struct {
	int nverts; int* vertData;
      } info;

      if (num_elems != 1)
	continue;

      if (Verbose) cout << "Reading TStrips..." << endl;

      ply.get_property(elem_name, tstrips_props);
      ply.get_element ((void*)&info);
      mesh->tstrips = new vector<int> (info.nverts);
      for (int iv = 0; iv < info.nverts; iv++) {
	mesh->tstrips->push_back (info.vertData[iv]);
      }

      mesh->trisFromStrip();

      if (Verbose) cout << "Done." << endl;
    }
  }

  return mesh;
}
*/

int SizeOfProp[] = { 0, 1, 2, 4, 1, 2, 4, 4, 8 };
int
CalcPropertySize (PlyElement* elem)
{
  int size = 0;
  int type;

  /*
  for (int i = 0; i < elem->nprops; i++) {
    type = elem->props[i]->external_type;
    if (type <= PLY_START_TYPE || type >= PLY_END_TYPE) {
      printf ("Unknown type %d found for property %s in elem %s\n",
	      type, elem->props[i]->name, elem->name);
      return 0;
    }
  */
  for (int i = 0; i < elem->props.size(); i++) {
    type = elem->props[i].external_type;
    if (type <= PLY_START_TYPE || type >= PLY_END_TYPE) {
      printf ("Unknown type %d found for property %s in elem %s\n",
	      type, elem->props[i].name, elem->name);
      return 0;
    }
    size += SizeOfProp[type];
  }

  return size;
}


#if defined(WIN32) || defined(i386)
void
CopyReverseWord (void* dest, void* src, int nWords)
  //like memcpy but copies 32-bit words instead of bytes, and flips byte order
  //to convert endian-ness as it copies.  Used to read PLY files (which are
  //currently all big-endian) on PC's (which are little-endian).
{
  for (int i = 0; i < nWords; i++) {
    unsigned long temp = *(unsigned long*)src;

    temp = (temp >> 24) | ((temp >> 8) & 0xFF00) |
      ((temp << 8) & 0xFF0000) | (temp << 24);

    (*(unsigned long*)dest) = temp;
    src  = ((char*)src)  + 4;
    dest = ((char*)dest) + 4;
  }
}
#endif

Mesh*
mattReadPlyFile (char* filename)
{
  /*
    int nelems;
    char **elist;
    char **obj_info;
    int num_obj_info;
    int file_type;
    float version;
    bool skipTexture = FALSE;
    char texFile[PATH_MAX];

    Timer t ("Read ply file (Matt's way)");

    PlyFile *ply =
	ply_open_for_reading(filename, &nelems, &elist, &file_type, &version);

    if (!ply)
	return NULL;

    if (file_type != PLY_BINARY_BE)  //if it ever becomes an issue, the code
        return NULL;  //is already here to read PLY_BINARY_LE too
                      //but right now, none of these exist anyway

    Mesh *mesh = new Mesh;

    // Look for a texture file reference
    char temp[PATH_MAX];
    obj_info = ply_get_obj_info (ply, &num_obj_info);
    for (int i = 0; i < num_obj_info; i++) {
	if (strstr(obj_info[i], "texture_file")) {
	    sscanf(obj_info[i], "%s%s", temp, texFile);
	    FILE *fp = fopen(texFile, "r");
	    if (fp != NULL) {
		fclose(fp);
		skipTexture = TRUE;
		strcpy(mesh->texFileName, texFile);
	    }
	}
    }

    if (strcmp (elist[0], "vertex") != 0)
      return NULL;//first element must be vertex

    if (strcmp (elist[1], "face") != 0 && strcmp (elist[1], "tristrips") != 0)
      return NULL;//second element must be triangles or strips

    //read in vertex data
    int nVerts = ply->elems[0]->num;
    int cbVertex = ply->elems[0]->size;
    //if (cbVertex <= 0)
    cbVertex = CalcPropertySize (ply->elems[0]);
    char* vBuffer = new char[nVerts * cbVertex];
    if (nVerts != fread (vBuffer, cbVertex, nVerts, ply->fp)) {
        delete vBuffer;
	delete mesh;
	return NULL;
    }

    //read in face data, if available
#if 0  // pack pragma is not supported by n32 compiler
#pragma pack (1)
    typedef struct { uchar nV; int vindex[3]; } RAWFACE;
    static const int kRawFaceSize = sizeof(RAWFACE);
#pragma pack (0)
#else
    static const int kRawFaceSize = 13;
#endif

    int nFaces;
    char* pFaces;
    int nStrips;
    int* pStrips;
    if (!strcmp (elist[1], "face")) {
      nStrips = 0; pStrips = NULL;
      nFaces = ply->elems[1]->num;
      pFaces = new char [nFaces * kRawFaceSize];
      if (nFaces != fread ((char*)pFaces, kRawFaceSize, nFaces, ply->fp)) {
	  delete vBuffer;
	  delete pFaces;
	  delete mesh;
	  return NULL;
      }
    } else {
      //otherwise, tristrips
      nFaces = 0; pFaces = NULL;
      fread ((char*)&nStrips, sizeof(int), 1, ply->fp);
      mesh->tstrips = new vector<int> (nStrips); //reserve space
      pStrips = &(*mesh->tstrips)[0]; //and read right into it
      if (1 != fread ((char*)pStrips, sizeof(int), nStrips, ply->fp)) {
	  delete vBuffer;
	  delete mesh->tstrips;
	  delete mesh;
	  return NULL;
      }
    }

    //transfer
    int ofsX = -1, ofsY = -1, ofsZ = -1;
    int ofsNx = -1, ofsNy = -1, ofsNz = -1;
    int ofsIntensity = -1, ofsConf = -1;
    //int ofsStdDev = -1; //this was here but unused and causing warnings
    int ofsD_Red = -1, ofsD_Green = -1, ofsD_Blue = -1;
    int ofsTexU = -1, ofsTexV = -1;

    int ofs = 0;
    for (i = 0; i < ply->elems[0]->nprops; i++) {
      PlyProperty* pProp = ply->elems[0]->props[i];
      if      (!strcmp (pProp->name, "x"))
	ofsX = ofs;
      else if (!strcmp (pProp->name, "y"))
	ofsY = ofs;
      else if (!strcmp (pProp->name, "z"))
	ofsZ = ofs;
      else if (!strcmp (pProp->name, "nx"))
	ofsNx = ofs;
      else if (!strcmp (pProp->name, "ny"))
	ofsNy = ofs;
      else if (!strcmp (pProp->name, "nz"))
	ofsNz = ofs;
      else if (!strcmp (pProp->name, "intensity"))
	ofsIntensity = ofs;
      //else if (!strcmp (pProp->name, "std_dev"))
      //  ofsStdDev = ofs;
      else if (!strcmp (pProp->name, "confidence"))
	ofsConf = ofs;
      else if (!strcmp (pProp->name, "diffuse_red"))
	ofsD_Red = ofs;
      else if (!strcmp (pProp->name, "diffuse_green"))
	ofsD_Green = ofs;
      else if (!strcmp (pProp->name, "diffuse_blue"))
	ofsD_Blue = ofs;
      else if (!strcmp (pProp->name, "texture_u"))
	ofsTexU = ofs;
      else if (!strcmp (pProp->name, "texture_v"))
	ofsTexV = ofs;

      ofs += SizeOfProp [pProp->external_type];
    }

    assert (ofsX != -1 && ofsY != -1 && ofsZ != -1);
    mesh->numVerts = nVerts;
    mesh->verts = new Vertex[nVerts];
    if (ofsConf != -1)
      mesh->vertConfidence = new float[nVerts];
    if (ofsIntensity != -1)
      mesh->vertIntensity = new float[nVerts];
    if (ofsNx != -1 && ofsNy == -1 && ofsNz == -1)
      mesh->hasVertNormals = TRUE;
    else
      ofsNx = ofsNy = ofsNz = -1;
    if (ofsD_Red != -1 && ofsD_Green != -1 && ofsD_Blue != -1)
      mesh->vertMatDiff = new vec3uc[nVerts];
    else
      ofsD_Red = ofsD_Green = ofsD_Blue = -1;
    if (ofsTexU != -1 && ofsTexV != -1 && !skipTexture) {
      mesh->texture = new vec2f[nVerts];
      mesh->hasTexture = TRUE;
    }
    else
      ofsTexU = ofsTexV = -1;
    //don't actually know what to do with std_dev data

    //verify assumptions about what is stored contiguously
    assert (ofsY == ofsX + sizeof(float));
    assert (ofsZ == ofsY + sizeof(float));
    if (ofsNx != -1) {
      assert (ofsNy == ofsNx + sizeof(float));
      assert (ofsNz == ofsNy + sizeof(float));
    }
    if (ofsD_Red != -1) {
      assert (ofsD_Green == ofsD_Red + sizeof(uchar));
      assert (ofsD_Blue == ofsD_Green + sizeof(uchar));
    }
    if (ofsTexU != -1)
      assert (ofsTexV == ofsTexU + sizeof(float));

    char* vert;
    for (vert = vBuffer, i = 0; i < nVerts;
	 vert += cbVertex, i++) {
#if defined(WIN32) || defined(i386)
      CopyReverseWord (&mesh->verts[i].coord, vert + ofsX, 3);
      if (ofsNx != -1)
	CopyReverseWord (&mesh->verts[i].norm, vert + ofsNx, 3);
      if (ofsD_Red != -1)
	memcpy (mesh->vertMatDiff+i, vert + ofsD_Red, 3*sizeof(uchar));
      if (ofsTexU != -1)
	CopyReverseWord (mesh->texture+i, vert + ofsTexU, 2);
      if (ofsIntensity != -1)
	CopyReverseWord (mesh->vertIntensity+i, vert + ofsIntensity, 1);
      if (ofsConf != -1)
	CopyReverseWord (mesh->vertConfidence+i, vert + ofsConf, 1);
#else
      memcpy (&mesh->verts[i].coord, vert + ofsX, 3*sizeof(float));
      if (ofsNx != -1)
	memcpy (&mesh->verts[i].norm, vert + ofsNx, 3*sizeof(float));
      if (ofsD_Red != -1)
	memcpy (mesh->vertMatDiff+i, vert + ofsD_Red, 3*sizeof(uchar));
      if (ofsTexU != -1)
	memcpy (mesh->texture+i, vert + ofsTexU, 2*sizeof(float));
      if (ofsIntensity != -1)
	memcpy (mesh->vertIntensity+i, vert + ofsIntensity, sizeof(float));
      if (ofsConf != -1)
	memcpy (mesh->vertConfidence+i, vert + ofsConf, sizeof(float));
#endif
    }
    delete vBuffer;

    if (nFaces) {
      assert(mesh->tris.size() == 0);
      mesh->tris.reserve (nFaces);

      char* pFace;
      char* endFaces = pFaces + (nFaces*kRawFaceSize);
      for (pFace = pFaces; pFace < endFaces; pFace += kRawFaceSize) {
	if (*pFace !=3 ) fprintf(stderr, "nV = %d\n", (int)*pFace);
	assert (*pFace == 3);

	mesh->tris.push_back(Triangle());
	Triangle *pTri = mesh->tris.end()-1;
#if defined(WIN32) || defined(i386)
	CopyReverseWord (&pTri->vindex1, pFace + 1, 3);
#else
	memcpy (&pTri->vindex1, pFace + 1, kRawFaceSize - 1);
#endif
      }

      delete pFaces;

    } else {

      //already have data in tstrips vector
      mesh->trisFromStrip();
    }

    ply_close(ply);

    return mesh;
  */
  return NULL;
}



int
writePlyFile(char *filename, MeshSet *meshSet,
	     int level, int useColorNotTexture, int writeNormals)
{
#if 1
  return meshSet->meshes[level]->writePlyFile(filename,
					      useColorNotTexture,
					      writeNormals,
					      meshSet);
#else
  int i;
  int hasIntensity;
  int hasColor;
  int hasConfidence;
  int hasTexture;
  char *elem_names[] = {"vertex", "face"};

  Mesh *mesh = meshSet->meshes[level];

  hasIntensity = mesh->vertIntensity != NULL;
  hasColor = mesh->vertMatDiff != NULL;
  hasConfidence = mesh->vertConfidence != NULL;
  hasTexture = mesh->texture != NULL;


  PlyFile ply;
  if (!ply.open_for_writing(filename, 2, elem_names,PLY_BINARY_BE))
    return 0;

  if (hasTexture && !useColorNotTexture) {
    char obj_info[PATH_MAX];
    sprintf(obj_info, "%s %s", "texture_file", mesh->texFileName);
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

  ply.describe_element("vertex", mesh->numVerts,
			vert_props.size(), &vert_props[0]);
  ply.describe_element("face", mesh->tris.size(), 1, face_props);
  ply.header_complete();

    // set up and write the vertex elements
  PlyVertex plyVert;
  Vertex *vert;

  ply.put_element_setup("vertex");

  for (i = 0; i < mesh->numVerts; i++) {
    vert = &mesh->verts[i];
    plyVert.x = vert->coord.x;
    plyVert.y = vert->coord.y;
    plyVert.z = vert->coord.z;

    if (hasIntensity)
      plyVert.intensity = mesh->vertIntensity[i];

    if (hasConfidence)
      plyVert.confidence = mesh->vertConfidence[i];

    if (hasColor) {
      plyVert.diff_r = mesh->vertMatDiff[i][0];
      plyVert.diff_g = mesh->vertMatDiff[i][1];
      plyVert.diff_b = mesh->vertMatDiff[i][2];
    }

    if (writeNormals) {
      plyVert.nx = vert->norm.x;
      plyVert.ny = vert->norm.y;
      plyVert.nz = vert->norm.z;
    }

    if (hasTexture && !useColorNotTexture) {
      plyVert.tex_u = mesh->texture[i][0];
      plyVert.tex_v = mesh->texture[i][1];
    }
    else if (hasTexture && useColorNotTexture) {
      float texU = mesh->texture[i][0];
      float texV = mesh->texture[i][1];
      int xOff = int(texU*meshSet->texXdim);
      int yOff = int(texV*meshSet->texYdim);

      // shouldn't that be texXdim, not texYdim*yOff ??
      uchar *col = &meshSet->
	texture[3*(xOff + meshSet->texYdim*yOff)];

      plyVert.diff_r = col[0];
      plyVert.diff_g = col[1];
      plyVert.diff_b = col[2];
    }

    ply.put_element((void *) &plyVert);
  }
  PlyFace plyFace;
  Triangle *tri;

  ply.put_element_setup("face");

  for (i = 0; i < mesh->tris.size(); i++) {
    tri = &mesh->tris[i];
    plyFace.nverts = 3;
    /*
    vertIndices[0] = tri->vindex1;
    vertIndices[1] = tri->vindex2;
    vertIndices[2] = tri->vindex3;
    plyFace.verts = (int *)vertIndices;
    */
    plyFace.verts[0] = tri->vindex1;
    plyFace.verts[1] = tri->vindex2;
    plyFace.verts[2] = tri->vindex3;

    ply.put_element_static_strg((void *) &plyFace);
  }

  return 1;
#endif
}
