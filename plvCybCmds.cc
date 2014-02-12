#include <vector.h>
#include "plvGlobals.h"
#include "plvCybCmds.h"
#include "plvMeshCmds.h"
#include "RangeGrid.h"
#include "plvImageCmds.h"
#include "plvScene.h"
#include "DisplayMesh.h"
#include "GenericScan.h"
#include "Mesh.h"

int
PlvReadCybCmd(ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
  //char *texFile = NULL;

  RangeGrid rangeGrid;
  if (rangeGrid.readCyfile(argv[1]) == 0)
    return TCL_ERROR;

  //if (argc > 2) {
  //if (EQSTR(argv[2], "-tex")) {
  //  texFile = argv[3];
  //}
  //}

  // BUGBUG need a smart factory for file reading
  GenericScan *meshSet = new GenericScan;
  meshSet->read(argv[1]);
  DisplayableMesh* dm = theScene->addMeshSet(meshSet);

  /*
  if (texFile != NULL) {
    strcpy(meshSet->texFileName, texFile);
    for (int i = 0; i < numMeshes; i++)
      meshSet->meshes[i]->setName(texFile);

    loadTexture(meshSet, texFile, rangeGrid.isLinearScan);
  }
  */

  Tcl_SetResult(interp, (char*)dm->getName(), TCL_VOLATILE);

  return TCL_OK;
}


int
PlvShadeCybCmd(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
#if 1
  cerr << "Command disabled" << endl;
  return TCL_ERROR;
#else
  int i, j, in1, vin1, hasTexture;
  char *texFile;
  float lx, ly, lz;

    //numMeshes = 4;
  RangeGrid rangeGrid;
  if (rangeGrid.readCyfile(argv[1]) == 0)
    return TCL_ERROR;

  hasTexture = FALSE;

  texFile = argv[2];

  lx = 0;
  ly = 0;
  lz = 1;
  if (argc > 3) {
    lx = atof(argv[3]);
    ly = atof(argv[4]);
    lz = atof(argv[5]);
  }

  printf("Building mesh level 1\n");
  Mesh *mesh = rangeGrid.toMesh(1, hasTexture);
  int subSamp = 1;

  int nlt = rangeGrid.nlt;
  int nlg = rangeGrid.nlg;

  int ltMax = rangeGrid.ltMax;
  int ltMin = ltMax - (ltMax/subSamp)*1;

  int lgMax = rangeGrid.lgMax;
  int lgMin = rangeGrid.lgMin;


  int *vert_index = new int[rangeGrid.numSamples];
  for (i = 0; i < rangeGrid.numSamples; i++)
    vert_index[i] = -1;

  /* Extra column of vertices for wrap-around */
  int *vert_index_extra = new int[nlt];
  for (i = ltMin; i <= ltMax; i++)
    vert_index_extra[i] = -1;


  /* see which vertices will be used in a triangle */

  int count = 0;
  for (j = ltMin; j <= ltMax; j += subSamp) {
    for (i = lgMin; i <= lgMax; i += subSamp) {
      in1 = rangeGrid.indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index[in1] = 1;
	count++;
      }
    }

    if (!rangeGrid.isLinearScan) {
      i = lgMin;
      in1 = rangeGrid.indices[i + j * rangeGrid.nlg];
      if (in1 >= 0) {
	vert_index_extra[j] = 1;
	count++;
      }
    }

  }

  count = 0;
  for (j = ltMin; j <= ltMax; j += subSamp) {
    for (i = lgMin; i <= lgMax; i += subSamp) {
      in1 = rangeGrid.indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index[in1] = count;
	mesh->verts[count].coord = rangeGrid.coords[in1];
	if (rangeGrid.hasConfidence)
	  mesh->vertConfidence[count] = rangeGrid.confidence[in1];

	if (rangeGrid.hasIntensity)
	  mesh->vertIntensity[count] = rangeGrid.intensity[in1];

	if (rangeGrid.hasColor) {
	  mesh->vertMatDiff[count][0] = rangeGrid.matDiff[in1][0];
	  mesh->vertMatDiff[count][1] = rangeGrid.matDiff[in1][1];
	  mesh->vertMatDiff[count][2] = rangeGrid.matDiff[in1][2];
	}

	count++;
      }
    }

    if (!rangeGrid.isLinearScan) {
      i = lgMin;
      in1 = rangeGrid.indices[i + j * rangeGrid.nlg];
      if (in1 >= 0) {
	vert_index_extra[j] = count;

	mesh->verts[count].coord = rangeGrid.coords[in1];
	if (rangeGrid.hasConfidence)
	  mesh->vertConfidence[count] = rangeGrid.confidence[in1];

	if (rangeGrid.hasIntensity)
	  mesh->vertIntensity[count] = rangeGrid.intensity[in1];

	if (rangeGrid.hasColor) {
	  mesh->vertMatDiff[count][0] = rangeGrid.matDiff[in1][0];
	  mesh->vertMatDiff[count][1] = rangeGrid.matDiff[in1][1];
	  mesh->vertMatDiff[count][2] = rangeGrid.matDiff[in1][2];
	}

	count++;
      }
    }


  }


  printf ("%f %f %f\n", lx, ly, lz);
  int color;
  ImgUchar *img = new ImgUchar(rangeGrid.nlg, rangeGrid.nlt, 1);
  for (j = ltMin; j <= ltMax; j += 1) {
    for (i = lgMin; i <= lgMax; i += 1) {
      in1 = rangeGrid.indices[i + j * nlg];
      if (in1 >= 0) {
	vin1 = vert_index[in1];
	color = int(mesh->verts[vin1].norm[0]*lx*255 +
		    mesh->verts[vin1].norm[2]*lz*255);
	if (color < 0) color = 0;
	if (color > 255) color = 255;
	img->elem(i,j) = uchar(color);
      }
    }
    /*
      if (!rangeGrid.isLinearScan) {
      i = lgMin;
      in1 = rangeGrid.indices[i + j * rangeGrid.nlg];
      if (in1 >= 0) {
      vin1 = vert_index_extra[j];
      mesh->texture[vin1][0] = 1 + 1.0/(lgMax - lgMin + 1);
      mesh->texture[vin1][1] = float(j)/nlt;
      count++;
      }
      }
    */
  }

  /*
    int xx, yy, color;
    ImgUchar *img = new ImgUchar(rangeGrid.nlg, rangeGrid.nlt, 1);
    for (yy = 0; yy < rangeGrid.nlt; yy++) {
       for (xx = 0; xx < rangeGrid.nlg; xx++) {
	  color = -mesh->verts[xx+yy*rangeGrid.nlg].norm[2]*255;
	  if (color < 0) color = 0;
	  if (color > 255) color = 255;
	  img->elem(xx,yy) = uchar(color);
       }
    }
    */


  img->writeIris(texFile);

  delete mesh;
  delete img;

  return TCL_OK;
#endif
}


int
PlvBumpCybCmd(ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
#if 1
  cerr << "Command disabled" << endl;
  return TCL_ERROR;
#else
  int i, j, in1, vin1, hasTexture;
  char *texFile;

  //numMeshes = 4;
  RangeGrid rangeGrid;
  if (rangeGrid.readCyfile(argv[1]) == 0)
    return TCL_ERROR;

  hasTexture = FALSE;

  texFile = argv[2];

  printf("Building mesh level 1\n");
  Mesh *mesh = rangeGrid.toMesh(1, hasTexture);
  int subSamp = 1;

  int nlt = rangeGrid.nlt;
  int nlg = rangeGrid.nlg;

  int ltMax = rangeGrid.ltMax;
  int ltMin = ltMax - (ltMax/subSamp)*1;

  int lgMax = rangeGrid.lgMax;
  int lgMin = rangeGrid.lgMin;


  int *vert_index = new int[rangeGrid.numSamples];
  for (i = 0; i < rangeGrid.numSamples; i++)
    vert_index[i] = -1;

  /* Extra column of vertices for wrap-around */
  int *vert_index_extra = new int[nlt];
  for (i = ltMin; i <= ltMax; i++)
    vert_index_extra[i] = -1;


  /* see which vertices will be used in a triangle */

  int count = 0;
  for (j = ltMin; j <= ltMax; j += subSamp) {
    for (i = lgMin; i <= lgMax; i += subSamp) {
      in1 = rangeGrid.indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index[in1] = 1;
	count++;
      }
    }

    if (!rangeGrid.isLinearScan) {
      i = lgMin;
      in1 = rangeGrid.indices[i + j * rangeGrid.nlg];
      if (in1 >= 0) {
	vert_index_extra[j] = 1;
	count++;
      }
    }

  }

  count = 0;
  for (j = ltMin; j <= ltMax; j += subSamp) {
    for (i = lgMin; i <= lgMax; i += subSamp) {
      in1 = rangeGrid.indices[i + j * nlg];
      if (in1 >= 0) {
	vert_index[in1] = count;
	mesh->verts[count].coord = rangeGrid.coords[in1];
	if (rangeGrid.hasConfidence)
	  mesh->vertConfidence[count] = rangeGrid.confidence[in1];

	if (rangeGrid.hasIntensity)
	  mesh->vertIntensity[count] = rangeGrid.intensity[in1];

	if (rangeGrid.hasColor) {
	  mesh->vertMatDiff[count][0] = rangeGrid.matDiff[in1][0];
	  mesh->vertMatDiff[count][1] = rangeGrid.matDiff[in1][1];
	  mesh->vertMatDiff[count][2] = rangeGrid.matDiff[in1][2];
	}

	count++;
      }
    }

    if (!rangeGrid.isLinearScan) {
      i = lgMin;
      in1 = rangeGrid.indices[i + j * rangeGrid.nlg];
      if (in1 >= 0) {
	vert_index_extra[j] = count;

	mesh->verts[count].coord = rangeGrid.coords[in1];
	if (rangeGrid.hasConfidence)
	  mesh->vertConfidence[count] = rangeGrid.confidence[in1];

	if (rangeGrid.hasIntensity)
	  mesh->vertIntensity[count] = rangeGrid.intensity[in1];

	if (rangeGrid.hasColor) {
	  mesh->vertMatDiff[count][0] = rangeGrid.matDiff[in1][0];
	  mesh->vertMatDiff[count][1] = rangeGrid.matDiff[in1][1];
	  mesh->vertMatDiff[count][2] = rangeGrid.matDiff[in1][2];
	}

	count++;
      }
    }


  }


  //printf ("%f %f %f\n", lx, ly, lz);
  int color;
  ImgUchar *img = new ImgUchar(rangeGrid.nlg, rangeGrid.nlt, 3);
  for (j = ltMin; j <= ltMax; j += 1) {
    for (i = lgMin; i <= lgMax; i += 1) {
      in1 = rangeGrid.indices[i + j * nlg];
      if (in1 >= 0) {
	vin1 = vert_index[in1];

	color = int(mesh->verts[vin1].norm[0]*127 + 127);
	if (color < 0) color = 0;
	if (color > 255) color = 255;
	img->elem(i,j,0) = char(color);

	color = int(mesh->verts[vin1].norm[1]*127 + 127);
	if (color < 0) color = 0;
	if (color > 255) color = 255;
	img->elem(i,j,1) = char(color);

	color = int(mesh->verts[vin1].norm[2]*127 + 127);
	if (color < 0) color = 0;
	if (color > 255) color = 255;
	img->elem(i,j,2) = char(color);

      }
    }
    /*
      if (!rangeGrid.isLinearScan) {
      i = lgMin;
      in1 = rangeGrid.indices[i + j * rangeGrid.nlg];
      if (in1 >= 0) {
      vin1 = vert_index_extra[j];
      mesh->texture[vin1][0] = 1 + 1.0/(lgMax - lgMin + 1);
      mesh->texture[vin1][1] = float(j)/nlt;
      count++;
      }
      }
    */
  }

  /*
    int xx, yy, color;
    ImgUchar *img = new ImgUchar(rangeGrid.nlg, rangeGrid.nlt, 1);
    for (yy = 0; yy < rangeGrid.nlt; yy++) {
       for (xx = 0; xx < rangeGrid.nlg; xx++) {
	  color = -mesh->verts[xx+yy*rangeGrid.nlg].norm[2]*255;
	  if (color < 0) color = 0;
	  if (color > 255) color = 255;
	  img->elem(xx,yy) = uchar(color);
       }
    }
    */


  img->writeIris(texFile);

  delete mesh;
  delete img;

  return TCL_OK;
#endif
}


