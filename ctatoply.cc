/* ctatoply.cc
 * jjg / dmich '98
 * ----
 * simple util to turn a cta into rangegrid and save the triangulated mesh of
 * highest resolution
 */

#include "MeshSet.h"
#include "ctafile.h"
#include "RangeGrid.h"
#include "Pnt3.h"

int main (int argc, char **argv)
{
  RangeGrid *rg;
  Mesh *mesh;
  Pnt3 *avgNorm, avgView;
  Pnt3 diff;
  int infoFlag;

  if (argc < 3) {
    printf("\nctatoply in.cta out.ply\n");
    printf("-------\n");
    printf("will output a fully sampled mesh from a cta file via rangegrid\n");
    printf("\n");
    return 0;
  }
  if ((argc >= 4) && (argv[3][0] == '-') && (argv[3][1] == 'i')) {
    infoFlag = true;
  }
  else infoFlag = false;
  rg = ImportCTAFile(argv[1], &avgView);
  if (rg == NULL) {
    printf("Couldn't import the CTA as rangegrid... something went wrong.\n");
    return -1;
  }

  mesh = rg->toMesh(1, FALSE);

  avgNorm = mesh->getAvgNormal();
  diff = *avgNorm - avgView;
  if (infoFlag) {
    cout << "Avg Norm is " << *avgNorm << endl;
    cout << "Avg View is " << avgView << endl;
  }
  cout << "Difference has length " << diff.norm() << endl;
  if (diff.norm() > 1.41) {
    printf("Flipping normals...\n");
    mesh->flipNormals();
  }
  if (!mesh->writePlyFile(argv[2], FALSE, FALSE, NULL)) {
    printf("Couldn't write out the PLY file\n");
    return -1;
  }
  return 1;
}

