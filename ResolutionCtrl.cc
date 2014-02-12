 //############################################################
//
// ResolutionCtrl.cc
//
// Kari Pulli
// Fri Jul  3 11:52:55 PDT 1998
//
//############################################################

#include "ResolutionCtrl.h"
#include "plvGlobals.h"
#include "plvScene.h"


void
ResolutionCtrl::split_name(void)
{
  basename = name;

  crope::const_iterator it = basename.end()-1;

  // Special case - gzipped scans
  if ((basename.size() > 3) &&
      (*(it-2) == '.') &&
      (*(it-1) == 'g') &&
      (*(it  ) == 'z'))
          it -= 3;

  // Search for a dot
  while (*it != '.' && it != basename.begin()) it--;

  if (it == basename.begin()) it = basename.end();
  ending = basename.substr(it+1, basename.end());
  basename = basename.substr (basename.begin(), it);
}

void
ResolutionCtrl::set_name(const char  *n)
{
  name = crope(n);
  // remove trailing slash, if a directory name
  if (name.back() == '/' || name.back() == '\\') name.pop_back();
  split_name();
}

void
ResolutionCtrl::set_name(const crope &n)
{
  name = n;
  // remove trailing slash, if a directory name
  if (name.back() == '/' || name.back() == '\\') name.pop_back();
  split_name();
}


crope
ResolutionCtrl::get_name(void)
{
  return name;
}

crope
ResolutionCtrl::get_basename(void)
{
  return basename;
}

crope
ResolutionCtrl::get_nameending(void)
{
  return ending;
}

bool
ResolutionCtrl::has_ending(const char *end)
{
  if (end[0] == '.') return ending == crope(end+1);
  else               return ending == crope(end);
}

bool
ResolutionCtrl::select_finest(void)
{
  if (!resolutions.size()) return false;
  return switchToResLevel (0);
}

bool
ResolutionCtrl::select_coarsest(void)
{
  if (!resolutions.size()) return false;
  return switchToResLevel (resolutions.size()-1);
}

bool
ResolutionCtrl::select_finer(void)
{
  if (!resolutions.size()) return false;
  if (curr_res == 0) return false;
  if (switchToResLevel (curr_res - 1)) return true;

  return false;
  //return select_finest();
}

bool
ResolutionCtrl::select_coarser(void)
{
  if (!resolutions.size()) return false;
  if (curr_res == resolutions.size()-1) return false;
  if (switchToResLevel (curr_res + 1)) return true;

  return false;
  //return select_coarsest();
}

bool
ResolutionCtrl::select_by_count(int n)
{
  return switchToResLevel (findLevelForRes (n));
}


int
ResolutionCtrl::create_resolution_absolute(int budget, Decimator dec)
{ return 0; }

bool
ResolutionCtrl::delete_resolution (int abs_res)
{ return false; }

void
ResolutionCtrl::insert_resolution (int abs, crope filename,
				   bool in_mem, bool desired_mem)
{
  res_info res;
  res.abs_resolution = abs;
  res.filename = filename;
  res.in_memory = in_mem;
  res.desired_in_mem = desired_mem;

  int n = resolutions.size();
  resolutions.push_back (res);
// STL Update
  inplace_merge (resolutions.begin(), resolutions.begin() + n, resolutions.end());
}


bool
ResolutionCtrl::set_load_desired (int res, bool desired)
{
  int i = findLevelForRes (res);
  if (i >= 0) {
    resolutions[i].desired_in_mem = desired;
    return true;
  }

  return false;
}


void
ResolutionCtrl::existing_resolutions(vector<res_info> &res)
{ res = resolutions; }

ResolutionCtrl::res_info
ResolutionCtrl::current_resolution(void)
{
  if (curr_res < 0 || curr_res >= resolutions.size())
  {
    static res_info bogus;
    memset (&bogus, 0, sizeof(bogus));
    return bogus;
  }

  return resolutions[curr_res];
}

bool
ResolutionCtrl::load_resolution(int i)
{ return 0; }

bool
ResolutionCtrl::release_resolution(int nPolys)
{
  cerr << "Warning: no memory was actually freed" << endl;
  return 0;
}

bool
ResolutionCtrl::switchToResLevel (int iRes)
{
  if (iRes < 0 || iRes >= resolutions.size())
    return false;

  if (!resolutions[iRes].in_memory) {
    if (!load_resolution (iRes))
      return false;
  }

  curr_res = iRes;
  return true;
}

int
ResolutionCtrl::current_resolution_index (void)
{
  int globalRes = theScene->getMeshResolution();

  switch (globalRes) {
  case Scene::resTempHigh:
    return 0;

  case Scene::resTempLow:
    return resolutions.size() - 1;

  case Scene::resDefault:
  default:
    assert (curr_res >= 0 && curr_res < resolutions.size());
    return curr_res;
  }
}

int
ResolutionCtrl::findLevelForRes (int n)
{
  for (int i=0; i<resolutions.size(); i++) {
    if (n == resolutions[i].abs_resolution) {
      return i;
    }
  }

  return -1;
}

int
ResolutionCtrl::findResForLevel (int n)
{
  assert (n >= 0 && n < resolutions.size());

  return resolutions[n].abs_resolution;

}

