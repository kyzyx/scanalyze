#include "MeshSet.h"

Mesh *readPlyFile(char *filename);
int writePlyFile(char *filename, MeshSet *meshSet,
		 int level, int useColorNotTexture, int writeNormals = FALSE);
