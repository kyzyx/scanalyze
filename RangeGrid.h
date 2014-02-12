#ifndef _RANGE_GRID_
#define _RANGE_GRID_

#include <limits.h>
#include "defines.h"
#include "Pnt3.h"

typedef uchar vec3uc[3];
class Mesh;
class cStripeServer;

class RangeGrid {
private:
  void addTriangleCheckLength(Mesh *mesh,
			      int vin1, int vin2, int vin3,
			      float maxLength);
  void addTriangleCheckNormal(Mesh *mesh,
			      int vin1, int vin2, int vin3,
			      Pnt3 &viewDir, float minDot);
public:

  int nlg;		// number of range columns (samples in x)
  int nlt;		// number of rows (samples in y)
  int interlaced;       // is it interlaced data?
  int numSamples;       // number of range samples
  Pnt3 *coords;	        // the range samples
  float *confidence;    // confidence
  float *intensity;     // intensity
  vec3uc *matDiff;      // color
  int *indices;	        // row x col indices to the positions
  int hasColor;         // color information?
  int hasIntensity;     // intensity information?
  int hasConfidence;    // confidence information?
  int multConfidence;   // multiply confidence?
  int hasTexture;
  int isLinearScan;
  int ltMax;
  int lgMax;
  int ltMin;
  int lgMin;
  char **obj_info;
  int num_obj_info;
  Pnt3 viewDir;

  int isModelMakerScan;

  RangeGrid();
  ~RangeGrid();

  int readRangeGrid(const char *name);
  int readCyfile(const char *name);
  Mesh *toMesh(int subSamp, int hasTexture);

};


int is_range_grid_file(const char *filename);

#endif
