// ScanFactory.h               Create objects of individual scan classes
// created 11/26/98            Matt Ginzton (magi@cs)
//
// All creation of *Scan objects should be done through these interfaces,
// and the objects can then be manipulated at the RigidScan level.
// It should not be necessary for other modules to directly create or
// access *Scan pointers.


#ifndef _SCANFACTORY_H_
#define _SCANFACTORY_H_


#include "RigidScan.h"
class DisplayableMesh;

RigidScan* CreateScanFromGeometry (const vector<Pnt3>& vtx,
				   const vector<int>& tris,
				   const crope& name = crope());

RigidScan* CreateScanFromFile (const crope& filename);

RigidScan* CreateScanFromThinAir (float size, int type = 0);

RigidScan* CreateScanFromBbox (const crope& filename,
			       const Pnt3& min, const Pnt3& max);

RigidScan* CreateScanGroup (const vector<DisplayableMesh*>& members,
			    const char *nameToUse, bool bDirty);
vector<DisplayableMesh*> BreakScanGroup (RigidScan* group);

bool isScanLoaded (const RigidScan* scan);

#endif // _SCANFACTORY_H_
