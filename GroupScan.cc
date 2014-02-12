// GroupScan.cc                RigidScan aggregation
// created 11/26/98            Matt Ginzton (magi@cs)


#include "GroupScan.h"
#include "MeshTransport.h"
#include "DisplayMesh.h"


// STL Update
#define FOR_EACH_CHILD(it) \
   for (vector<DisplayableMesh*>::iterator it = children.begin(); it < children.end(); it++)
#define FOR_EACH_CONST_CHILD(it) \
   for (vector<DisplayableMesh*>::const_iterator it = children.begin(); \
        it < children.end(); it++)

GroupScan::GroupScan()
{
}


GroupScan::GroupScan (const vector<DisplayableMesh*>& members, bool dirty)
{
  bDirty = dirty;
  children = members;
  rebuildResolutions();
  computeBBox();
}


GroupScan::~GroupScan()
{
  cout << "Destroying scan group: " << children.size() << " members" << endl;
  //  FOR_EACH_CHILD (it)    delete (*it);
}

bool
GroupScan::AddScan (DisplayableMesh* scan)
{
  children.push_back (scan);
  computeBBox();
  rebuildResolutions();
  return true;
}


bool
GroupScan::RemoveScan (DisplayableMesh* scan)
{
  FOR_EACH_CHILD (it) {
    if (*it == scan) {
      children.erase (it);
      rebuildResolutions();
      computeBBox();

      // this scan has been xformed by whatever our xform is...
      RigidScan* rs = scan->getMeshData();
      rs->setXform (getXform() * rs->getXform());
      return true;
    }
  }

  return false;
}

bool
GroupScan::write(const crope &fname)
{
  if (fname.empty()) {
      // try to save to default name; quit if there isn't one
    if (name.empty()) return false;
  } else {
    if (name != fname)  {
      cout << "Saving to filename " << fname << endl;
      set_name(fname);
    }
  }

  ofstream fout (name.c_str());
  if (fout) {
    FOR_EACH_CHILD(it) {
      const char* str = (*it)->getMeshData()->get_name().c_str();
      for (int i = 0; i < strlen(str); i++)
	fout.put(str[i]);
      fout.put('\n');
    }
    fout.put ('\n');
    fout.close();
    bDirty = false;
    return true;
  }
  return false;
}

MeshTransport*
GroupScan::mesh(bool perVertex, bool stripped,
		ColorSource color, int colorSize)
{
  //cout << "GroupScan::mesh called for " << children.size() << " children"
  //     << endl;

  if (stripped && !perVertex) {
    cerr << "No t-strips without per-vertex properties";
    return NULL;
  }

  MeshTransport* mt = new MeshTransport;
  bool bAnySucceeded = false;
  FOR_EACH_CHILD (it) {
    RigidScan* rs = (*it)->getMeshData();
    MeshTransport* cmt = rs->mesh (perVertex, stripped, color, colorSize);
    if (cmt) {
      // append geometry and topology
      mt->appendMT (cmt, rs->getXform());
      delete cmt;
      bAnySucceeded = true;

      //cout << " . " << tri_inds.size() << endl;
    }
  }

  if (!bAnySucceeded) {
    delete mt;
    mt = NULL;
  }

  return mt;
}


void
GroupScan::computeBBox (void)
{
  bbox.clear();
  rot_ctr = Pnt3();
  FOR_EACH_CHILD (it) {
    RigidScan* rs = (*it)->getMeshData();
    Bbox childBox = rs->worldBbox();
    if (childBox.valid()) {
      bbox.add (childBox);
      rot_ctr += rs->worldCenter();
    }
  }

  if (children.size())
    rot_ctr /= children.size();

  rot_ctr = bbox.center();
}


crope
GroupScan::getInfo (void)
{
  char info[1000];

  sprintf (info, "Scan group containing %ld members.\n\n",
	   children.size());

  return crope (info) + RigidScan::getInfo();
}

bool
GroupScan::is_modified (void) {
  return bDirty;
}

bool
GroupScan::get_children (vector<RigidScan*>& _children) const
{
  _children.reserve (children.size());
  FOR_EACH_CONST_CHILD (it) {
    _children.push_back ((*it)->getMeshData());
  }

  return true;
}


bool
GroupScan::get_children_for_display (vector<DisplayableMesh*>& _children) const
{
  _children = children;
  return true;
}


void
GroupScan::rebuildResolutions (void)
{
  // we have as many resolutions as our best-endowed child...
  // find out how many that is.
  int nRes = 0;
  FOR_EACH_CHILD (it) {
    nRes = max (nRes, (*it)->getMeshData()->num_resolutions());
  }

  // now, build our res data (snapping to nearest existing resolution of
  // poorer children, if necessary)
  int iPolysVis = 0;
  resolutions.clear();
  for (int i = 0; i < nRes; i++) {
    int nThisRes = 0;
    bool bInMem = true;
    bool bDesiredInMem = true;
    FOR_EACH_CHILD (it) {
      RigidScan* rs = (*it)->getMeshData();
      vector<res_info> childres;
      rs->existing_resolutions (childres);
      int iChildRes = i * childres.size() / nRes;
      nThisRes += childres[iChildRes].abs_resolution;
      if (!childres[iChildRes].in_memory)
	bInMem = false;
      if (!childres[iChildRes].desired_in_mem)
	bDesiredInMem = false;

      if (i == 0)  // only do this once for each child
	iPolysVis += childres[rs->current_resolution_index()].abs_resolution;
    }

    insert_resolution (nThisRes, crope(), bInMem, bDesiredInMem);
  }

  // switch to best approximation of average current resolution
  for (i = 0; i < nRes; i++) {
    if (iPolysVis >= resolutions[i].abs_resolution)
      break;
  }
  if (i == nRes)
    select_coarsest();
  else
    switchToResLevel (i);
}


bool
GroupScan::switchToResLevel (int iRes)
{
  if (!ResolutionCtrl::switchToResLevel (iRes))
    return false;

  int ok = true;
  FOR_EACH_CHILD (it) {
    ResolutionCtrl* rc = (*it)->getMeshData();
    vector<res_info> ri;
    rc->existing_resolutions (ri);
    if (!rc->select_by_count (ri[iRes].abs_resolution))
      ok = false;
  }

  return ok;
}


bool
GroupScan::load_resolution (int iRes)
{
  if (resolutions[iRes].in_memory)
    return true;

  FOR_EACH_CHILD (it) {
    if (!(*it)->getMeshData()->load_resolution (iRes))
      return false;
  }

  resolutions[iRes].in_memory = true;
  return true;
}

bool
GroupScan::release_resolution (int nPolys)
{
   int iRes = findLevelForRes(nPolys);
   if (iRes == -1) {
    cerr << "Group resolution doesn't exist!" << endl;
    return false;
   }
   if (!resolutions[iRes].in_memory)
     return true;

   FOR_EACH_CHILD (it) {
     RigidScan *scan = (*it)->getMeshData();

     if (!scan->release_resolution (scan->findResForLevel(iRes)))
       return false;
   }

  resolutions[iRes].in_memory = false;
  return true;
}
bool
GroupScan::filter_inplace (const VertexFilter &filter)
{
  return false;
}



RigidScan*
GroupScan::filtered_copy (const VertexFilter &filter)
{
  BailDetector bail;
  RigidScan *newchild, *oldchild;
  vector <DisplayableMesh*> newchildren;
  VertexFilter *childFilter;

  cout << "Clip " << children.size() << " children: " << flush;
  FOR_EACH_CHILD(it) {

    oldchild = (*it)->getMeshData();
    // need to transform each child by the group transform b4 filtering
    childFilter = filter.transformedClone((float*)oldchild->getXform());
    newchild = oldchild->filtered_copy(*childFilter);

    if (newchild) {
      if (newchild->num_resolutions() > 0 ) {
	crope newname;
	newname = oldchild->get_basename()
	  + "Clip." + oldchild->get_nameending();

	newchild->set_name(newname);

	DisplayableMesh *dm;
	GroupScan *g = dynamic_cast<GroupScan*>(newchild);
	if (!g) {
	  if (newchild->num_vertices() >= 3) {
	    dm = theScene->addMeshSet(newchild, false);
	    newchildren.push_back(dm);
	  }
	} else {
	  dm = GetMeshForRigidScan(newchild);
	  assert(dm);
	  newchildren.push_back(dm);
	}
	//newchildren.push_back(dm);
      }
    }

    cout << "." << flush;
    if (bail()) {
      cerr << "Warning: group clip interrupted; results are partial" << endl;
      break;
    }
  }
  cout << " " << newchildren.size() << " of my kids made it." << endl;
  char buf[256];
  sprintf (buf, "%sClip.%s", basename.c_str(), ending.c_str());

  // groupScans will make its own copy of buf

  if (!newchildren.size()) return new GroupScan;
  DisplayableMesh *groupmesh = groupScans (newchildren, buf, true);

  groupmesh->getMeshData()->setXform(getXform());

  return groupmesh->getMeshData();
}





void
GroupScan::subsample_points(float rate, vector<Pnt3> &p,
			    vector<Pnt3> &n)
{
  vector<Pnt3> cp, cn;
  p.clear();
  n.clear();

  FOR_EACH_CHILD (it) {
    RigidScan* rs = (*it)->getMeshData();

    cp.clear();
    cn.clear();
    rs->subsample_points (rate, cp, cn);

    // apply local transformations
    Xform<float> xf = rs->getXform();
    if (xf != Xform<float>()) {
// STL Update
      for (vector<Pnt3>::iterator pi = cp.begin(); pi < cp.end(); pi++) {
	xf (*pi);
      }
      xf.removeTranslation();
      for (pi = cn.begin(); pi < cn.end(); pi++) {
	xf (*pi);
      }
    }

    //cout << "- adding " << cp.size() << " points from child" << endl;
    p.insert (p.end(), cp.begin(), cp.end());
    n.insert (n.end(), cn.begin(), cn.end());
  }

  //cout << "added " << p.size() << " points total" << endl;
}


bool
GroupScan::closest_point(const Pnt3 &p, const Pnt3 &n,
			 Pnt3 &cl_pnt, Pnt3 &cl_nrm,
			 float thr, bool bdry_ok)
{
  Pnt3 cp, cn;
  Pnt3 mp, mn;
  float closest = 1e33;
  bool bAny = false;

  Xform<float> xf, xfn;
  RigidScan* winner = NULL;

  //cout << "closest_point for " << children.size() << " children" << endl;
  FOR_EACH_CHILD (it) {
    RigidScan* rs = (*it)->getMeshData();
    xf = rs->getXform();
    xfn = xf;
    xfn.removeTranslation();

    mp = p; mp.invxform (xf);
    mn = n; mn.invxform (xfn);

    if (!rs->closest_point (mp, mn, cp, cn, thr, bdry_ok))
      continue;
    bAny = true;

    float dist = (mp-cp).norm2();
    //cout << "- distance^2 from child is " << dist << endl;

    if (dist < closest) {
      winner = rs;
      cl_pnt = cp;
      cl_nrm = cn;
      closest = dist;
    }
  }

  // the output is still in the coordinate system of the winning subscan
  if (bAny) {
    assert (winner != NULL);
    xf = winner->getXform();
    xf (cl_pnt);
    xf.removeTranslation();
    xf (cl_nrm);
  }

  //cout << "distance^2 from group is " << closest << endl;
  return bAny;
}


bool
GroupScan::write_metadata (MetaData data)
{
  bool success = true;

  FOR_EACH_CHILD (it) {
    RigidScan* rs = (*it)->getMeshData();
    Xform<float> saved_xf;

    if (data == md_xform) {
      saved_xf = rs->getXform();
      rs->setXform(getXform() * saved_xf);
    }

    if (!rs->write_metadata (data))
      success = false;

    if (data == md_xform) {
      rs->setXform(saved_xf);
    }
  }

  return success;
}
