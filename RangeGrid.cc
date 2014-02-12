#include <stdio.h>
#include "ply++.h"
#include <stdlib.h>
#include "strings.h"
#include "plvGlobals.h"
#include "RangeGrid.h"
#include "Mesh.h"
#include <iostream.h>
#ifdef WIN32
#include <io.h>
#endif
#include <malloc.h>
#include <math.h>
#include <fcntl.h>
#ifndef linux
#    include <stdexcept.h>
#endif
#include "cyfile.h"
#include "defines.h"
#include "Xform.h"
#include "Pnt3.h"

struct PlyVertex {
  float x,y,z;
  float confidence;
  float intensity;
  unsigned char red,grn,blu;
};

PlyProperty vert_std_props[] = {
  {"x", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,x), 0, 0, 0, 0},
  {"y", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,y), 0, 0, 0, 0},
  {"z", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,z), 0, 0, 0, 0},
  {"confidence", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,confidence), 0, 0, 0,0},
  {"intensity", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,intensity), 0, 0, 0,0},
  {"diffuse_red", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,red), 0, 0, 0, 0},
  {"diffuse_green", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,grn), 0, 0, 0, 0},
  {"diffuse_blue", PLY_UCHAR, PLY_UCHAR, offsetof(PlyVertex,blu), 0, 0, 0, 0},
  {"std_dev", PLY_FLOAT, PLY_FLOAT, offsetof(PlyVertex,confidence), 0, 0, 0,0},
};


struct RangePnt {
  unsigned char num_pts;
  int *pts;
};

// list of property information for a range data point
PlyProperty range_props[] = {
  {"vertex_indices", PLY_INT, PLY_INT, offsetof(RangePnt,pts),
   1, PLY_UCHAR, PLY_UCHAR, offsetof(RangePnt,num_pts)},
};


static float RANGE_DATA_SIGMA_FACTOR = 4;
static float RANGE_DATA_MIN_INTENSITY = 0.05;


RangeGrid::RangeGrid()
{
  coords = NULL;
  confidence = NULL;
  intensity = NULL;
  matDiff = NULL;
  indices = NULL;

  hasColor = FALSE;
  hasConfidence = FALSE;
  hasIntensity = FALSE;
  hasTexture = FALSE;

  obj_info = NULL;
  num_obj_info = 0;
  isModelMakerScan = FALSE;
}


RangeGrid::~RangeGrid()
{
  delete [] coords;

  delete [] indices;

  if (num_obj_info > 0) {
    for (int i = 0; i < num_obj_info; i++)
      delete [] obj_info[i];
    delete [] obj_info;
  }

  if (hasConfidence)
    delete [] confidence;

  if (hasColor)
    delete [] matDiff;

  if (hasIntensity)
    delete [] intensity;
}


int
is_range_grid_file(const char *filename)
{
  int nelems;
  char **elist;

  PlyFile ply;
  if (ply.open_for_reading(filename, &nelems, &elist) == 0)
    return 0;

  for (int i = 0; i < nelems; i++) {
    if (!strcmp(elist[i], "range_grid"))
      return 1;
  }
  return 0;
}


/*****************************************************************
Read range data from a PLY file.

Entry:
  name - name of PLY file to read from

Exit:
  returns pointer to data, or NULL if it couldn't read from file
******************************************************************/

int
RangeGrid::readRangeGrid(const char *name)
{
  int i,j,k,index, best_index;
  PlyFile ply;
  int num_elems;
  int nprops;
  int nelems;
  char **elist;
  PlyProperty **plist;
  int num_rows,num_cols;
  RangePnt range_pnt;
  PlyVertex vert;
  char *elem_name;
  int get_std_dev = 0;
  int get_confidence = 0;
  int get_intensity = 0;
  int get_color = 0;
  int has_red = 0;
  int has_green = 0;
  int has_blue = 0;
  int is_warped = 0;
  char temp[PATH_MAX];
  int isRightMirrorOpen = 1;
  //int found_mirror = 0;  //unused?
  int foundScanType = FALSE;

  float conf,std;
  float min_std,max_std,max;
  float avg_std = 0;

  if (ply.open_for_reading(name, &nelems, &elist) == 0)
    return 0;

  // parse the obj_info

  char **ply_obj_info;
  int ply_num_obj_info;
  ply_obj_info = ply.get_obj_info (&ply_num_obj_info);
  for (i = 0; i < ply_num_obj_info; i++) {
    if (strstr(ply_obj_info[i], "num_cols"))
      sscanf(ply_obj_info[i], "%s%d", temp, &num_cols);
    if (strstr(ply_obj_info[i], "num_rows"))
      sscanf(ply_obj_info[i], "%s%d", temp, &num_rows);
    if (strstr(ply_obj_info[i], "is_warped"))
      sscanf(ply_obj_info[i], "%s%d", temp, &is_warped);
    if (strstr(ply_obj_info[i], "optimum_std_dev"))
      sscanf(ply_obj_info[i], "%s%f", temp, &avg_std);
    if (strstr(ply_obj_info[i], "is_right_mirror_open")) {
      //found_mirror = TRUE;
      sscanf(ply_obj_info[i], "%s%d", temp, &isRightMirrorOpen);
    }
    if (strstr(ply_obj_info[i], "is_linear_scan")) {
      foundScanType = TRUE;
      sscanf(ply_obj_info[i], "%s%d", temp, &isLinearScan);
    }
  }

  min_std = avg_std / RANGE_DATA_SIGMA_FACTOR;
  max_std = avg_std * RANGE_DATA_SIGMA_FACTOR;

  // set up the range data structure
  nlg = num_cols;
  nlt = num_rows;
  intensity = NULL;
  confidence = NULL;
  matDiff = NULL;
  hasColor = 0;
  hasIntensity = 0;
  hasConfidence = 0;
  multConfidence = 0;
  hasTexture = 0;

  ltMin = 0;
  ltMax = nlt-1;
  lgMin = 0;
  lgMax = nlg-1;


  if (!foundScanType) {
    printf("Assumed to be linear scan...\n");
    isLinearScan = TRUE;
  }

  if (!is_warped) {
    viewDir.set(0, 0, -1);
  } else {
    if (isRightMirrorOpen) {
      viewDir.set(-sin(30*M_PI/180), 0, -cos(30*M_PI/180));
    } else {
      viewDir.set(sin(30*M_PI/180), 0, -cos(30*M_PI/180));
    }
  }

  num_obj_info = ply_num_obj_info;
  obj_info = new char*[ply_num_obj_info];
  for (i = 0; i < ply_num_obj_info; i++) {
    obj_info[i] = new char[strlen(ply_obj_info[i])+1];
    strcpy(obj_info[i], ply_obj_info[i]);
  }

  // see if we've got both vertex and range_grid data

  plist = ply.get_element_description("vertex", &num_elems, &nprops);
  if (plist == NULL) {
    fprintf (stderr, "file doesn't contain vertex data\n");
    return 0;
  }
  numSamples = num_elems;
  plist = ply.get_element_description("range_grid", &num_elems, &nprops);
  if (plist == NULL) {
    fprintf (stderr, "file doesn't contain range_grid data\n");
    return 0;
  }

  // read in the range data

  for (i = 0; i < nelems; i++) {
    elem_name = elist[i];
    plist = ply.get_element_description(elem_name, &num_elems, &nprops);

    if (equal_strings ("vertex", elem_name)) {

      // see if the file contains intensities
      for (j = 0; j < nprops; j++) {
        if (equal_strings ("std_dev", plist[j]->name))
          get_std_dev = 1;
        if (equal_strings ("confidence", plist[j]->name))
          get_confidence = 1;
        if (equal_strings ("intensity", plist[j]->name))
          get_intensity = 1;
        if (equal_strings ("diffuse_red", plist[j]->name))
          has_red = 1;
        if (equal_strings ("diffuse_green", plist[j]->name))
          has_green = 1;
        if (equal_strings ("diffuse_blue", plist[j]->name))
          has_blue = 1;
      }

      if (has_red && has_green && has_blue) {
        get_color = 1;
        hasColor = 1;
      }

      if (get_intensity)
        hasIntensity = 1;

      if (get_std_dev && is_warped) {
        hasConfidence = 1;
        multConfidence = 1;
      }
      else if (get_confidence)
        hasConfidence = 1;

      ply.get_property ("vertex", &vert_std_props[0]);
      ply.get_property ("vertex", &vert_std_props[1]);
      ply.get_property ("vertex", &vert_std_props[2]);
      if (get_confidence)
        ply.get_property ("vertex", &vert_std_props[3]);
      if (get_intensity)
        ply.get_property ("vertex", &vert_std_props[4]);
      if (get_color) {
        ply.get_property ("vertex", &vert_std_props[5]);
        ply.get_property ("vertex", &vert_std_props[6]);
        ply.get_property ("vertex", &vert_std_props[7]);
      }
      if (get_std_dev)
        ply.get_property ("vertex", &vert_std_props[8]);
      coords = new Pnt3[numSamples];

      if (get_confidence || get_std_dev)
	confidence = new float[numSamples];

      if (get_intensity)
	intensity = new float[numSamples];

      if (get_color)
	matDiff = new vec3uc[numSamples];

      for (j = 0; j < num_elems; j++) {
// STL Update
#define __STL_TRY try
#define __STL_CATCH_ALL catch(...)
        __STL_TRY {
	  ply.get_element ((void *) &vert);
	}
	__STL_CATCH_ALL {

	  return NULL;
	}


        coords[j].set(&vert.x);

        if (get_intensity) {
          intensity[j] = vert.intensity;
	}

        if (get_std_dev&&is_warped) {

          std = vert.confidence;

          if (std < min_std)
            conf = 0;
          else if (std < avg_std)
            conf = (std - min_std) / (avg_std - min_std);
          else if (std > max_std)
            conf = 0;
          else
	    conf = (max_std - std) / (max_std - avg_std);

	  // Unsafe to use vertex intensity, as aperture
	  // settings may change between scans.
	  // Instead, use std_dev confidence * orientation.
	  // conf *= vert.intensity;

	  if (get_intensity)
	    if (vert.intensity < RANGE_DATA_MIN_INTENSITY) conf = 0.0;

          confidence[j] = conf;
        }
        else if (get_confidence) {
          confidence[j] = vert.confidence;
        }

        if (get_color) {
          matDiff[j][0] = vert.red;
          matDiff[j][1] = vert.grn;
          matDiff[j][2] = vert.blu;
        }
      }
    }

    if (equal_strings ("range_grid", elem_name)) {
      indices = new int[nlt * nlg];

      ply.get_element_setup(elem_name, 1, range_props);
      for (j = 0; j < num_elems; j++) {

	__STL_TRY {
	  ply.get_element((void *) &range_pnt);
	}
	__STL_CATCH_ALL {
	  return NULL;
	}

        if (range_pnt.num_pts == 0)
          indices[j] = -1;
        else {
	  max = -FLT_MAX;
	  for (k = 0; k < range_pnt.num_pts; k++) {
	    index = range_pnt.pts[k];
	    // There will only be more than one point per sample
	    // if there are intensities
	    if (get_intensity) {
	      if (intensity[index] > max) {
		max = intensity[index];
		best_index = index;
	      }
	    }
	    else {
	      best_index = index;
	    }
	  }
	  index = best_index;
	  if (get_confidence || get_std_dev) {
	    if (confidence[index] > 0.0)
	      indices[j] = index;
	    else
	      indices[j] = -1;
	  } else {
	    indices[j] = index;
	  }

	  if (get_intensity) {
	    if (intensity[index] < RANGE_DATA_MIN_INTENSITY) {
	      indices[j] = -1;
	    }
	  }

	  delete range_pnt.pts;
	}
      }
    }
  }
  return 1;
}



/*****************************************************************
Create a triangle mesh from scan data.

Entry:
  sc         - scan data to make into triangle mesh
  level      - level of mesh (how detailed), in [0,1,2,3]
  table_dist - distance for spatial subdivision hash table

Exit:
  returns pointer to newly-created mesh
******************************************************************/

Mesh *
RangeGrid::toMesh(int subSamp, int hasTexture)
{
  int i,j;
  int ii,jj;
  int in1,in2,in3,in4,vin1,vin2,vin3,vin4;
  int max_tris;

  Mesh *mesh = new Mesh;

  int _ltMin = ltMax - (ltMax/subSamp)*subSamp;

  // create a list saying whether a vertex is going to be used

  int *vert_index = new int[numSamples];
  for (i = 0; i < numSamples; i++)
    vert_index[i] = -1;

  // Extra column of vertices for wrap-around
  int *vert_index_extra = new int[nlt];
  for (i = _ltMin; i <= ltMax; i++)
    vert_index_extra[i] = -1;


  // see which vertices will be used in a triangle

  int count = 0;
  for (j = _ltMin; j <= ltMax; j += subSamp) {
    for (i = lgMin; i <= lgMax; i += subSamp) {
      in1 = indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index[in1] = 1;
	count++;
      }
    }

    if (!isLinearScan) {
      i = lgMin;
      in1 = indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index_extra[j] = 1;
	count++;
      }
    }
  }

  printf("Num verts = %d\n", count);

  if (hasConfidence)
    mesh->vertConfidence = new float[count];

  if (hasTexture)
    mesh->texture = new vec2f[count];

  if (hasIntensity)
    mesh->vertIntensity = new float[count];

  if (matDiff)
    mesh->vertMatDiff = new vec3uc[count];

  max_tris = count * 6;
  mesh->tris.reserve(max_tris);

  // create the vertices

  count = 0;
  for (j = _ltMin; j <= ltMax; j += subSamp) {
    for (i = lgMin; i <= lgMax; i += subSamp) {
      in1 = indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index[in1] = count;
	mesh->vtx.push_back(coords[in1]);
	if (hasConfidence)
	  mesh->vertConfidence[count] = confidence[in1];

	if (hasIntensity)
	  mesh->vertIntensity[count] = intensity[in1];

	if (hasColor) {
	  mesh->vertMatDiff[count][0] = matDiff[in1][0];
	  mesh->vertMatDiff[count][1] = matDiff[in1][1];
	  mesh->vertMatDiff[count][2] = matDiff[in1][2];
	}

	count++;
      }
    }

    if (!isLinearScan) {
      i = lgMin;
      in1 = indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index_extra[j] = count;

	mesh->vtx.push_back(coords[in1]);
	if (hasConfidence)
	  mesh->vertConfidence[count] = confidence[in1];

	if (hasIntensity)
	  mesh->vertIntensity[count] = intensity[in1];

	if (hasColor) {
	  mesh->vertMatDiff[count][0] = matDiff[in1][0];
	  mesh->vertMatDiff[count][1] = matDiff[in1][1];
	  mesh->vertMatDiff[count][2] = matDiff[in1][2];
	}

	count++;
      }
    }
  }

  // Assign texture coordinates

  if (hasTexture) {
    for (j = _ltMin; j <= ltMax; j += subSamp) {
      for (i = lgMin; i <= lgMax; i += subSamp) {
	in1 = indices[i + j * nlg];
	if (in1 >= 0) {
	  vin1 = vert_index[in1];
	  mesh->texture[vin1][0] = float(i-lgMin)/(lgMax-lgMin+1);
	  mesh->texture[vin1][1] = float(j)/nlt;
	}
      }
      if (!isLinearScan) {
	i = lgMin;
	in1 = indices[i + j * nlg];
	if (in1 >= 0) {
	  vin1 = vert_index_extra[j];
	  mesh->texture[vin1][0] = 1 + 1.0/(lgMax - lgMin + 1);
	  mesh->texture[vin1][1] = float(j)/nlt;
	  count++;
	}
      }
    }
  }


  // create the triangles

  for (j = _ltMin; j <= ltMax - subSamp; j += subSamp) {
    for (i = lgMin; i <= lgMax - subSamp; i += subSamp) {

      ii = (i + subSamp) % nlg;
      jj = (j + subSamp) % nlt;

      // count the number of good vertices
      // 2 3
      // 1 4
      in1 = indices[ i +  j * nlg];
      in2 = indices[ i + jj * nlg];
      in3 = indices[ii + jj * nlg];
      in4 = indices[ii +  j * nlg];
      count = (in1 >= 0) + (in2 >= 0) + (in3 >= 0) + (in4 >=0);
      if (in1 >= 0) {
	vin1 = vert_index[in1];
      }
      if (in2 >= 0) {
	vin2 = vert_index[in2];
      }
      if (in3 >= 0) {
	vin3 = vert_index[in3];
      }
      if (in4 >= 0) {
	vin4 = vert_index[in4];
      }

      if (count == 4) {	// all 4 vertices okay, so make 2 tris
	// compute lengths of cross-edges
	float len1 = dist(mesh->vtx[vin1], mesh->vtx[vin3]);
	float len2 = dist(mesh->vtx[vin2], mesh->vtx[vin4]);

	if (len1 < len2) {
	  mesh->addTri(vin2, vin1, vin3);
	  mesh->addTri(vin1, vin4, vin3);
	} else {
	  mesh->addTri(vin2, vin1, vin4);
	  mesh->addTri(vin2, vin4, vin3);
	}
      }
      else if (count == 3) { // only 3 vertices okay, so make 1 tri
	if        (in1 == -1) {
	  mesh->addTri(vin2, vin4, vin3);
	} else if (in2 == -1) {
	  mesh->addTri(vin1, vin4, vin3);
	} else if (in3 == -1) {
	  mesh->addTri(vin2, vin1, vin4);
	} else { // in4 == -1
	  mesh->addTri(vin2, vin1, vin3);
	}
      }
    }
  }


  // Do the wrap around for cylindrical scans
  if (!isLinearScan) {

    ii = lgMin;

    for (j = _ltMin; j <= ltMax - subSamp; j += subSamp) {
      jj = (j + subSamp) % nlt;

      // count the number of good vertices
      in1 = indices[ i +  j * nlg];
      in2 = indices[ i + jj * nlg];
      in3 = indices[ii + jj * nlg];
      in4 = indices[ii +  j * nlg];
      count = (in1 >= 0) + (in2 >= 0) + (in3 >= 0) + (in4 >=0);

      if (in1 >= 0) {
	vin1 = vert_index[in1];
      }
      if (in2 >= 0) {
	vin2 = vert_index[in2];
      }
      if (in3 >= 0) {
	vin3 = vert_index_extra[jj];
      }
      if (in4 >= 0) {
	vin4 = vert_index_extra[j];
      }

      if (count == 4) {	// all 4 vertices okay, so make 2 tris
	// compute lengths of cross-edges
	float len1 = dist(mesh->vtx[vin1], mesh->vtx[vin3]);
	float len2 = dist(mesh->vtx[vin2], mesh->vtx[vin4]);

	if (len1 < len2) {
	  mesh->addTri(vin2, vin1, vin3);
	  mesh->addTri(vin1, vin4, vin3);
	} else {
	  mesh->addTri(vin2, vin1, vin4);
	  mesh->addTri(vin2, vin4, vin3);
	}
      }
      else if (count == 3) { // only 3 vertices okay, so make 1 tri
	if        (in1 == -1) {
	  mesh->addTri(vin2, vin4, vin3);
	} else if (in2 == -1) {
	  mesh->addTri(vin1, vin4, vin3);
	} else if (in3 == -1) {
	  mesh->addTri(vin2, vin1, vin4);
	} else { // in4 == -1
	  mesh->addTri(vin2, vin1, vin3);
	}
      }
    }
  }

  mesh->hasVertNormals = FALSE;

  mesh->initNormals(UseAreaWeightedNormals);

  if (!isLinearScan) {
    for (j = _ltMin; j <= ltMax; j += subSamp) {
      in1 = indices[lgMin + j * nlg];

      if (in1 >= 0) {
	vin1 = vert_index[in1];
	vin2 = vert_index_extra[j];
	mesh->nrm[3*vin1  ] += mesh->nrm[3*vin2  ];
	mesh->nrm[3*vin1+1] += mesh->nrm[3*vin2+1];
	mesh->nrm[3*vin1+2] += mesh->nrm[3*vin2+2];
	Pnt3 temp (mesh->nrm[3*vin1],
		   mesh->nrm[3*vin1+1], mesh->nrm[3*vin1+2]);
	temp /= 32767;
	temp.normalize();
	temp *= 32767;
	mesh->nrm[3*vin1  ] = mesh->nrm[3*vin2  ] = temp[0];
	mesh->nrm[3*vin1+1] = mesh->nrm[3*vin2+1] = temp[1];
	mesh->nrm[3*vin1+2] = mesh->nrm[3*vin2+2] = temp[2];
      }
    }
  }

  // free up the vertex index list
  delete [] vert_index;
  delete [] vert_index_extra;

  if (isModelMakerScan) mesh->remove_stepedges(50, 8);
     else mesh->remove_stepedges(50, 4);

  return mesh;
}


void
RangeGrid::addTriangleCheckNormal(Mesh *mesh,
				  int vin1, int vin2, int vin3,
				  Pnt3 &viewDir, float minDot)
{
  float _dot = fabs(dot(viewDir,
			normal(mesh->Vtx(vin1), mesh->Vtx(vin2),
			       mesh->Vtx(vin3))));
  int tooGrazing = _dot < minDot;
  if (tooGrazing)
    return;

  mesh->addTri(vin2, vin1, vin3);
}


void
RangeGrid::addTriangleCheckLength(Mesh *mesh,
				  int vin1, int vin2, int vin3,
				  float maxLength)
{
  const Pnt3 &c1 = mesh->Vtx(vin1);
  const Pnt3 &c2 = mesh->Vtx(vin2);
  const Pnt3 &c3 = mesh->Vtx(vin3);

  if (dist(c1,c2) > maxLength)
    return;

  if (dist(c1,c3) > maxLength)
    return;

  if (dist(c2,c3) > maxLength)
    return;

  mesh->addTri(vin2, vin1, vin3);
}


int
RangeGrid::readCyfile(const char *name)
{
  int xx, yy;
  float x, y, z;
  int count;
  long range;

  int fd = open(name, O_RDONLY, 0);

  GSPEC *gs;
  gs = cyread(NULL, fd);

  float lgincr = 1e-6*gs->lgincr;
  float ltincr = 1e-6*gs->ltincr;
  float theta;
  count = 0;
  for (yy = 0;  yy < gs->nlt; yy++) {
    for (xx = 0;  xx < gs->nlg; xx++) {
      range = getr(gs, yy, xx);
      if (range != VOIDGS(gs) ) {
	count++;
      }
    }
  }

  numSamples = count;
  isLinearScan = gs->flags & FLAG_CARTESIAN;
  nlg = gs->nlg;
  nlt = gs->nlt;
  coords = new Pnt3[numSamples];
  indices = new int[nlt * nlg];
  viewDir.set(0, 0, -1);

  count = 0;
  int diff;
  int range1, range2;

  if (!isLinearScan) {

    /* For moving targets (like heads), the begin and
       end points will differ noticably.  This is an attempt
       to counteract that motion by shifting the range data
       by an average amount of discrepancy.  A better job
       would be to find a more complex rigid or even
       non-rigid deformation that would match the points.
       Even better, find new matches iteratively as in Besl
       and McKay. */

    diff = 0;
    for (yy = 0;  yy < gs->nlt; yy++) {
      range1 = getr(gs, yy, 0);
      range2 = getr(gs, yy, gs->nlg-1);
      if (range1 != VOIDGS(gs) && range2 != VOIDGS(gs) ) {
	count++;
	diff += (range2 - range1);
      }
    }
    diff = int(float(diff)/count);
  }

  count = 0;
  int lg;
  int yymin = 100000;
  int yymax = -1;
  int xxmax = -1;
  int xxmin = 100000;

  for (yy = 0;  yy < gs->nlt; yy++) {
    for (xx = 0;  xx < gs->nlg; xx++) {
      range = getr(gs, yy, xx);
      if (isLinearScan) {
	lg = xx;
      } else {
	lg = gs->nlg - xx - 1;
      }
      if (range == VOIDGS(gs) ) {
	indices[lg + yy*nlg] = -1;
      }
      else {
	yymax = yy > yymax ? yy : yymax;
	xxmax = lg > xxmax ? lg : xxmax;
	yymin = yy < yymin ? yy : yymin;
	xxmin = lg < xxmin ? lg : xxmin;
	indices[lg + yy*nlg] = count;
	y = yy*ltincr;
	if (isLinearScan) {
	  x = xx*lgincr - (gs->nlg/2) * lgincr;
	  z = range*1e-6;
	} else {
	  //range -= int(diff*float(xx)/gs->nlg);
	  theta = xx*lgincr + M_PI;
	  x = -range*1e-6*sin(theta);
	  z = range*1e-6*cos(theta);
	}
	coords[count].set(x,y,z);
	count++;
      }
    }
  }

  if (!isLinearScan) {
    ltMax = yymax;
    lgMax = xxmax;
    ltMin = yymin;
    lgMin = xxmin;
  } else {
    ltMax = gs->nlt-1;
    lgMax = gs->nlg-1;
    ltMin = 0;
    lgMin = 0;
  }

  // To handle color shifts - ack!
  /*
    lgMin++;
    lgMax--;
  */

  cyfree(gs);
  return 1;
}

