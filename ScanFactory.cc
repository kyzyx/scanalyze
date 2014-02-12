// ScanFactory.cc              Create objects of individual scan classes
// created 11/26/98            Matt Ginzton (magi@cs)
//
// No other file but this one should need to reference any *Scan except
// at the RigidScan level.

#include "Mesh.h"
#include "GenericScan.h"
#include "CyberScan.h"
#include "CyraScan.h"
#include "MMScan.h"
#include "SyntheticScan.h"
#include "GroupScan.h"
#include "ProxyScan.h"


RigidScan*
CreateScanFromGeometry (const vector<Pnt3>& vtx,
			const vector<int>& tris,
			const crope& name)
{
  Mesh* mesh = new Mesh (vtx, tris);
  GenericScan* gs = new GenericScan (mesh, name);
  return gs;
}


static bool
has_ending(const crope& filename, const crope& ending)
{
  int lenf = filename.size();
  int lene = ending.size();
  if (lenf <= lene)
    return FALSE;

  return (ending == filename.substr (lenf - lene, lenf));
}


RigidScan*
CreateScanFromFile (const crope& filename)
{
  RigidScan *scan = NULL;
  if      (has_ending(filename, ".ply"))
    scan = new GenericScan;
  else if (has_ending(filename, ".ply.gz"))
    scan = new GenericScan;
  else if (has_ending(filename, ".set"))
    scan = new GenericScan;
  else if (has_ending(filename, ".sd"))
    scan = new CyberScan;
  else if (has_ending(filename, ".sd.gz"))
    scan = new CyberScan;
  else if (has_ending(filename, ".cta"))
    scan = new MMScan;
  else if (has_ending(filename, ".mms"))
    scan = new MMScan;
  else if (has_ending(filename, ".pts"))
    scan = new CyraScan;
  else
    scan = new GenericScan;

  if (scan) {
    if (!scan->read (filename)) {
      delete scan;
      scan = NULL;
    }
  }

  return scan;
}


RigidScan*
CreateScanFromThinAir (float size, int type)
{
  // note -- types not supported; just for future options

  RigidScan* scan = new SyntheticScan (size);
  return scan;
}


RigidScan*
CreateScanGroup (const vector<DisplayableMesh*>& members, const char *nameToUse, bool bDirty)
{
  RigidScan* scan = new GroupScan (members, bDirty);
  scan->set_name (strdup(nameToUse));

  return scan;
}


vector<DisplayableMesh*>
BreakScanGroup (RigidScan* scan)
{
  vector<DisplayableMesh*> members;
  GroupScan* group = dynamic_cast<GroupScan*> (scan);
  if (group) {
    if (group->get_children_for_display (members)) {
// STL Update
      for (vector<DisplayableMesh*>::iterator mem = members.begin();
	   mem < members.end(); mem++) {
	group->RemoveScan (*mem);
      }
    }
  } else {
    members.clear();
  }

  return members;
}


RigidScan*
CreateScanFromBbox (const crope& name, const Pnt3& min, const Pnt3& max)
{
  return new ProxyScan (name, min, max);
}


bool
isScanLoaded (const RigidScan* scan)
{
  return dynamic_cast<const ProxyScan*> (scan) == NULL;
}
