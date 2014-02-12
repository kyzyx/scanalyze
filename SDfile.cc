//############################################################
//
// SDfile.cc
//
// Kari Pulli
// Tue Feb 23 13:09:27 CET 1999
//
// Read, write, and store the raw data of an *.sd file
//
//############################################################

#include "SDfile.h"
#include "VertexFilter.h"
#include "Random.h"
#include "Median.h"
#include <algorithm>


#ifdef WIN32
#	define __median _Median
#	define gzclose fclose
int gzread (FILE* file, void* buf, int size)
{
	return fread (buf, 1, size, file);
}
#else
#	include <zlib.h>
#endif


#if 1
#	include "plvGlobals.h"
#else
bool g_bNoIntensity = false;
bool g_verbose = false;
#endif

#if defined(WIN32) || defined(i386)
#define LITTLE_ENDIAN
#endif


#ifdef LITTLE_ENDIAN

#define swap_short(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )
#define swap_long(x) ( ((x) << 24) | \
                       (((x) << 8) & 0x00ff0000) | \
		       (((x) >> 8) & 0x0000ff00) | \
		       ((x) >> 24) )

static short sg_swapbuf_short;
static float sg_swapbuf_float;
static int sg_swapbuf_int;

#define FIX_SHORT(x) (*(unsigned short *)&(x) = \
		      swap_short(*(unsigned short *)&(x)))
#define FIX_LONG(x) (*(unsigned *)&(x) = \
		     swap_long(*(unsigned *)&(x)))
#define FIX_FLOAT(x) FIX_LONG(x)

#define WRITE_UINT(x) \
 ((sg_swapbuf_int = x), FIX_LONG(sg_swapbuf_int) , (fwrite(&sg_swapbuf_int, sizeof(unsigned int),1,sdfile) != 1))
#define WRITE_FLOAT(x) \
 ((sg_swapbuf_float = x), FIX_FLOAT(sg_swapbuf_float) , (fwrite(&sg_swapbuf_float, sizeof(float),1,sdfile) != 1))
#define WRITE_USHORT(x) \
 ((sg_swapbuf_short = x), FIX_SHORT(sg_swapbuf_short) , (fwrite(&sg_swapbuf_short, sizeof(unsigned short),1,sdfile) != 1))

#else

#define FIX_SHORT(x) (x)
#define FIX_LONG(x) (x)
#define FIX_FLOAT(x) (x)

#define WRITE_UINT(x) \
 (fwrite(&x, sizeof(unsigned int),1,sdfile) != 1)
#define WRITE_FLOAT(x) \
 (fwrite(&x, sizeof(float),1,sdfile) != 1)
#define WRITE_USHORT(x) \
 (fwrite(&x, sizeof(unsigned short),1,sdfile) != 1)

#endif


// These return true on error
#define READ_UINT(x) \
 ((gzread(sdfile, &x, sizeof(unsigned int)) != sizeof(unsigned int)) || (FIX_LONG(x) && 0))
#define READ_FLOAT(x) \
 ((gzread(sdfile, &x, sizeof(float)) != sizeof(float)) || (FIX_FLOAT(x) && 0))
#define READ_USHORT(x) \
 ((gzread(sdfile, &x, sizeof(unsigned short)) != sizeof(unsigned short)) || (FIX_SHORT(x) && 0))



void
SDfile::prepare_axis_proj(void)
{
  if (axis_proj_done) return;

  axis_proj_done = true;

  axis_proj_min = axis_proj_max =
    xf.axis_project(first_good[0], z_data[0]);

  for (int i=0; i<n_frames; i++) {

    if (first_good[i] >= first_bad[i]) continue;

    float pr = xf.axis_project(first_good[i],
			       z_data[row_start[i]]);
    if      (pr < axis_proj_min) axis_proj_min = pr;
    else if (pr > axis_proj_max) axis_proj_max = pr;

    int cnt = first_bad[i] - first_good[i];
    pr = xf.axis_project(first_bad[i]-1,
			 z_data[row_start[i]+cnt]);
    if      (pr < axis_proj_min) axis_proj_min = pr;
    else if (pr > axis_proj_max) axis_proj_max = pr;

  }
}



// The image below describes how the point indices are assumed
// to be arranged. There's two index arrays, both n long.
// 1 *---*---*...*---*
//    \ / \ / \...\ / \
// 0   *---*---*...*---*
// If the situation looks like
// 1   *---*...*---*---*
//    / \ / \...\ / \ /
// 0 *---*---*...*---*
// the flag upper_row_starts should be set false.
// This routine creates ccw triangles, but the strip can be
// flipped by setting a flag.
static void
create_strip(vector<int>        &tstrips,
	     const vector<int>  &p_inds_0,
	     const vector<int>  &p_inds_1,
	     bool               upper_row_starts = true,
	     bool               flipped = false)
{
  bool str_on = false;
  // 1 *---*
  //    \1/2\
  // 0   *---*
  int n = p_inds_0.size();
  int end = n-1;

  const int *p0 = &p_inds_0[0];
  const int *p1 = &p_inds_1[0];

  if (flipped) {
    p0 = &p_inds_1[0];
    p1 = &p_inds_0[0];
    upper_row_starts = !upper_row_starts;
  }

  if (upper_row_starts == false) {
    if (p0[0] != -1 &&
	p0[1] != -1 &&
	p1[0] != -1) {
      tstrips.push_back(p1[0]);
      tstrips.push_back(p0[0]);
      tstrips.push_back(p0[1]);
      tstrips.push_back(-1);
    }
    p0  = p0+1;
    end = n-2;
  }

  for (int i=0; i<end; i++) {
    // triangle 1
    if (str_on) {
      // push the index
      int j = p1[i+1];
      tstrips.push_back(j);
      // still going along a strip?
      str_on = (j != -1);
    } else {
      // should we start a strip?
      if (p1[i  ] != -1 &&
	  p1[i+1] != -1 &&
	  p0[i  ] != -1) {
	tstrips.push_back(p1[i]);
	tstrips.push_back(p0[i]);
	tstrips.push_back(p1[i+1]);
	str_on = true;
      }
    }
    // triangle 2
    if (str_on) {
      // push the index
      int j = p0[i+1];
      tstrips.push_back(j);
      // still going along a strip?
      str_on = (j != -1);
    } else {
      // bad place to start a strip, but a single tri?
      if (p0[i  ] != -1 &&
	  p0[i+1] != -1 &&
	  p1[i+1] != -1) {
	tstrips.push_back(p1[i+1]);
	tstrips.push_back(p0[i]);
	tstrips.push_back(p0[i+1]);
	tstrips.push_back(-1);
      }
    }
  }
  if (upper_row_starts == false) {
    if (p1[n-1] != -1) {
      if (str_on) {
	tstrips.push_back(p1[n-1]);
      } else {
	p0--; // p0 was shifted, shift it back!
	if (p1[n-2] != -1 && p0[n-1] != -1) {
	  // single triangle
	  tstrips.push_back(p1[n-1]);
	  tstrips.push_back(p1[n-2]);
	  tstrips.push_back(p0[n-1]);
	  tstrips.push_back(-1);
	}
      }
    }
  }
  // close strip?
  if (str_on) tstrips.push_back(-1);
}


inline void
mark_all_empty(const vector<int> &curr,
	       vector<char>      &bdry)
{
  vector<int>::const_iterator it = curr.begin();
  for (;it != curr.end(); it++)
    if (*it != -1) bdry[*it] = 1;
}


inline void
mark_maybe_empty(const vector<int> &curr,
		 const vector<int> &prev,
		 const vector<int> &next,
		 vector<char> &bdry)
{
  int n = curr.size();
  if (curr[0] != -1)   bdry[curr[0]] = 1;
  int end = n-1;
  if (curr[end] != -1) bdry[curr[end]] = 1;
  for (int j=1; j<end; j++) {
    int k = curr[j];
    if (k != -1 && (curr[j-1] == -1 ||
		    curr[j+1] == -1 ||
		    prev[j]   == -1 ||
		    prev[j-1] == -1 ||
		    next[j]   == -1 ||
		    next[j-1] == -1)) {
      bdry[k] = 1;
    }
  }
}


void
SDfile::make_tstrip_horizontal(vector<int>  &tstrips,
			       vector<char> &bdry)
{
  // do horizontal stripping

  int i,j,k;
  assert(pts_per_frame % 2 == 0);
  int n = pts_per_frame / 2;
  // previous, current indices
  vector<int> p_0(n), p_1(n);
  vector<int> c_0(n), c_1(n);
  fill(c_0.begin(), c_0.end(), -1);
  fill(c_1.begin(), c_1.end(), -1);

  int p_idx = 0;

  // initialize the point indices
  int end = first_bad[0] - first_good[0];
  for (i=0; i<end; i++) {
    if (z_data[row_start[0]+i]) {
      j = first_good[0]+i;
      if (j%2) c_1[j/2] = p_idx++;
      else     c_0[j/2] = p_idx++;
    }
  }
  // first row: part of boundary
  mark_all_empty(c_0, bdry);

  bool flip = (frame_pitch > 0.0);
  bool vscan = scanner_config & 0x00000001;
  if (!vscan) flip = !flip;
  // first frame, within the two fields
  create_strip(tstrips, c_0, c_1, false, flip);
  for (i=1; i<n_frames; i++) {

    end = first_bad[i] - first_good[i];

    if (end == 0) {
      if (first_good[i-1] != first_bad[i-1]) {
	mark_all_empty(c_1, bdry);
      }
      continue;
    }

    // initialize and copy
    copy(c_0.begin(), c_0.end(), p_0.begin());
    copy(c_1.begin(), c_1.end(), p_1.begin());
    fill(c_0.begin(), c_0.end(), -1);
    fill(c_1.begin(), c_1.end(), -1);
    // fill with indices
    for (j=0; j<end; j++) {
      if (z_data[row_start[i]+j]) {
	k = first_good[i]+j;
	if (k%2) c_1[k/2] = p_idx++;
	else     c_0[k/2] = p_idx++;
      }
    }

    // between the first field and the previous frame's
    // second field
    if (first_good[i-1] != first_bad[i-1]) {
      create_strip(tstrips, p_1, c_0, true, flip);
      // check boundary status of p_inds[1]
      mark_maybe_empty(p_1, p_0, c_0, bdry);
    }

    // between the fields of this frame
    create_strip(tstrips, c_0, c_1, false, flip);
    // check boundary status of c_inds[0]
    if (first_good[i-1] != first_bad[i-1]) {
      mark_maybe_empty(c_0, p_1, c_1, bdry);
    } else {
      mark_all_empty(c_0, bdry);
    }
  }

  // last row: all valids are part of boundary
  mark_all_empty(c_1, bdry);
}


static int
get_index(int col, int idx, int n,
	  const vector<int> &indices,
	  int good, int bad)
{

  if (col < good || col >= bad || idx < 0 || idx >= n)
    return -1;
  return indices[idx];
}


#define IsValidTstripIndex(ind,base,range) \
  (((ind)==-1) || ((ind)>=(base) && (ind)<((base)+(range))))

// the comments make more sense for the make_tstrip_horizontal,
// this was modified from it
void
SDfile::make_tstrip_vertical(vector<int>  &tstrips,
			     vector<char> &bdry)
{
  // do vertical stripping

  int i,j,k;
  assert(pts_per_frame % 2 == 0);
  int n = n_frames;
  // previous, current indices
  vector<int> p_0(n), p_1(n);
  vector<int> c_0(n), c_1(n);
  fill(c_0.begin(), c_0.end(), -1);
  fill(c_1.begin(), c_1.end(), -1);

  vector<int> indices(n_pts);
  k = 0;
  int p_idx = 0;
  for (i=0; i<n; i++) {
    int end = first_bad[i] - first_good[i];
    assert(k == row_start[i]);
    for (j=0; j<end; j++) {
      if (z_data[k]) indices[k++] = p_idx++;
      else           indices[k++] = -1;
    }
  }

  // initialize the point indices
  for (i=0; i<n; i++) {
    c_0[i] = get_index(0, row_start[i], n_pts, indices,
		       first_good[i], first_bad[i]);
    c_1[i] = get_index(1, row_start[i]+1, n_pts, indices,
		       first_good[i], first_bad[i]);
  }

# ifndef NDEBUG
  for (i=0; i<n_pts; i++) {
    assert (IsValidTstripIndex (indices[i], 0, k));
  }
  for (i=0; i<n; i++) {
    assert (IsValidTstripIndex (c_0[i], 0, k));
    assert (IsValidTstripIndex (c_1[i], 0, k));
  }
# endif

  bool flip = (frame_pitch > 0.0);
  bool vscan = scanner_config & 0x00000001;
  if (!vscan) flip = !flip;
  // first column: part of boundary
  mark_all_empty(c_0, bdry);
  // first frame, within the two fields
  create_strip(tstrips, c_1, c_0, true, flip);
  for (i=2; i<pts_per_frame; i+=2) {
    int oddi = i+1;
    // copy
    copy(c_0.begin(), c_0.end(), p_0.begin());
    copy(c_1.begin(), c_1.end(), p_1.begin());
    // fill with indices
    for (j=0; j<n; j++) {
      int tmp = row_start[j]-first_good[j];
      c_0[j] = get_index(i, tmp+i, n_pts, indices,
			 first_good[j], first_bad[j]);
      c_1[j] = get_index(oddi, tmp+oddi, n_pts, indices,
			 first_good[j], first_bad[j]);
      assert(IsValidTstripIndex (c_0[j], 0, k));
      assert(IsValidTstripIndex (c_1[j], 0, k));
    }

    // between the first field and the previous frame's
    // second field
    create_strip(tstrips, c_0, p_1, false, flip);
    // check boundary status of p_inds[1]
    mark_maybe_empty(p_1, p_0, c_0, bdry);

    // between the fields of this frame
    create_strip(tstrips, c_1, c_0, true, flip);
    // check boundary status of c_inds[0]
    mark_maybe_empty(c_0, p_1, c_1, bdry);
  }

  // last row: all valids are part of boundary
  mark_all_empty(c_1, bdry);
}





void
SDfile::fill_index_array(int step,
			 int row,
			 SubSampMode subSampMode,
			 vector<int> &p,
			 vector<Pnt3>  &pnts,
			 vector<int>   &ssmap,
			 vector<uchar> &intensity,
			 vector<uchar> &confidence)
{
  int half_step = step/2;
  int i,j,k;
  int zAccum;

  i = row * half_step;

  set_xf(i, false);

  unsigned short *z = &z_data[row_start[i]];

  int firstrow = max(0, i - half_step / 2);
  int lastrow  = min(int(n_frames), i + (half_step+1) / 2);
  int end = first_bad[i] - first_good[i];
  if (subSampMode == holesGrow) {
    for (j=0; j<end;) {
      if (z[j]) {
	k = first_good[i]+j;
	if (k % step == 0) {
	  // now need to search other rows to see how many
	  // samples are ok
	  int mincol = k - half_step;
	  int maxcol = k + half_step;
	  for (int irow = firstrow; irow < lastrow; irow++) {
	    if (mincol <  first_good[irow] ||
		maxcol >= first_bad[irow]) {
	      goto loop_exit;
	    }
	    int off = row_start[irow] - first_good[irow];
	    unsigned short *zz   = &z_data[mincol + off];
	    unsigned short *zend = &z_data[maxcol + off];
	    for (; zz != zend; zz++) {
	      if (*zz == 0) goto loop_exit;
	    }
	  }

	  // keep the point
	  p[k/step] = pnts.size();
	  pnts.push_back(xf.apply_xform(k, z[j]));
	  ssmap.push_back(row_start[i]+j);
	  if (!g_bNoIntensity)
	    intensity.push_back (intensity_data[row_start[i]+j]);
	}
      }
    loop_exit: j++;
    }
  } else { // don't conserve holes
    for (j=0; j<end; j++) {
      if (z[j]) {
	k = first_good[i]+j;
	if (k % step == 0) {
	  if (subSampMode == conf || subSampMode == filterAvg) {
	    // if filtering, keep totals to average:
	    int iAccum = 0, kAccum = 0;
	    zAccum = 0;

	    // now need to search other rows to see how many
	    // samples are ok
	    int mincol = k - half_step;
	    int maxcol = k + half_step;
	    int nPossibleContrib =
	      (maxcol - mincol) * (lastrow - firstrow);
	    int nContrib = 0;
	    for (int irow = firstrow; irow < lastrow; irow++) {
	      int fg = first_good[irow];
	      int rs = row_start[irow];
	      int firstcol = max(0, mincol - fg) + rs;
	      int lastcol = min(maxcol, int(first_bad[irow]))+rs-fg;
	      unsigned short *zbeg = &z_data[firstcol],
		*zend = &z_data[lastcol];
	      int _k = mincol;
	      for (unsigned short* zz = zbeg; zz < zend; zz++) {
		if (*zz) {
		  nContrib++;
		  if (subSampMode == filterAvg) {
		    iAccum += irow;
		    kAccum += _k++;
		    zAccum += *zz;
		  }
		}
	      }
	    }

	    // exaggerate distance from full confidence
	    float conf = float(nContrib) / float(nPossibleContrib);
	    conf = 1.0 - 4.0*(1.0 - conf);
	    confidence.push_back (255.0*max(0.0f, conf));

	    if (subSampMode == filterAvg) {
	      set_xf (iAccum / nContrib, true);
	      k = kAccum / nContrib;
	      zAccum /= nContrib;
	    }
	  }

	  // keep the point
	  p[k/step] = pnts.size();
	  if (subSampMode == filterAvg)
	    pnts.push_back(xf.apply_xform(k, zAccum));
	  else
	    pnts.push_back(xf.apply_xform(k, z[j]));
	  ssmap.push_back(row_start[i]+j);
	  if (!g_bNoIntensity)
	    intensity.push_back (intensity_data[row_start[i]+j]);
	}
      }
    }
  }
}


// return false if fully out of frustum
bool
SDfile::find_row_range(float screw, float radius, const Pnt3 &ctr,
		       int rowrange[2])
{
  float rowf = (screw - scan_screw) / frame_pitch + .5*n_frames-.25;

  if (rowf < 0) {
    // check whether the first row intersects sphere
    if (1) {
      // intersect
      rowrange[0] = rowrange[1] = 0;
      return true;
    } else {
      return false;
    }
  }

  if (rowf >= n_frames) {
    // check whether the last row intersects sphere
    if (1) {
      // intersect
      rowrange[0] = rowrange[1] = 0;
      return true;
    } else {
      return false;
    }
  }
  return true;
}



SDfile::SDfile(void)
  : row_start(NULL), first_good(NULL), first_bad(NULL),
    z_data(NULL), intensity_data(NULL), n_valid_pts(0),
    axis_proj_done(false)
{

}


SDfile::~SDfile(void)
{
  if (row_start)
    delete[] row_start;
  if (first_good)
    delete[] first_good;
  if (first_bad)
    delete[] first_bad;
  if (z_data)
    delete[] z_data;
  if (intensity_data)
    delete[] intensity_data;
}


bool
SDfile::read(const crope &fname)
{
  if(g_verbose) cout << "SDfile::read(" << fname << ")... ";
  //
  // Open the .sd file for reading
  //

#ifdef WIN32
  FILE* sdfile;
  if (!(sdfile = fopen (fname.c_str(), "rb"))) {
#else
  gzFile sdfile;
  if (!(sdfile = gzopen(fname.c_str(), "rb"))) {
#endif
    cerr << "Can't open file " << fname << endl;
    return false;
  }

  //
  // Read and check the magic number
  //

  unsigned int magic_num;
  if (READ_UINT(magic_num)) {
    cerr << "Can't read magic number" << endl;
    return false;
  }
  if (magic_num == 0x444d5001)
    version = 1;
  else if (magic_num == 0x444d5002)
    version = 2;
  else if (magic_num == 0x444d5003)
    version = 3;
  else if (magic_num == 0x444d5004)
    version = 4;
  else if (magic_num == 0x444d5005)
    version = 5;
  else if (magic_num == 0x444d5006)
    version = 6;
  else if (magic_num == 0x444d5007)
    version = 7;
  else  {
    cerr << "Invalid magic number" << endl;
    return false;
  }

  if (version != 7 && version != 6 && version != 5) {
    cerr << "Only versions 5, 6, and 7 of sd supported..." << endl;
    gzclose(sdfile);
    return false;
  }

  if(g_verbose) cout << "v" << version << "... " << flush;

  scanner_vert = 0;

  if (version == 7 || version == 6) {
    //
    // Read the rest of the .sd header information
    //

    if (READ_UINT(header_size) ||
	READ_UINT(n_frames) ||
	READ_UINT(pts_per_frame) ||
	READ_UINT(n_pts) ||
	READ_FLOAT(frame_pitch) ||
	READ_FLOAT(scan_screw) ||
	READ_FLOAT(other_screw) ||
	READ_FLOAT(scanner_trans)) {
      cerr << "Can't read sd file header" << endl;
      return false;
    }

    if (version == 7)  {
      if (READ_FLOAT(scanner_vert))  {
        cerr << "Can't read sd file header" << endl;
        return false;
      }
    }

    if (READ_UINT(scanner_config) ||
	READ_FLOAT(laser_intensity) ||
	READ_FLOAT(camera_sensitivity)) {
      cerr << "Can't read sd file header" << endl;
      return false;
    }

    first_good = new unsigned short[n_frames];
    first_bad  = new unsigned short[n_frames];
    z_data     = new unsigned short[n_pts];
    if (!g_bNoIntensity)
      intensity_data = new unsigned char[n_pts];

    //
    // Read the block of start/stop indices
    //

    for (int i=0; i<n_frames; ++i) {
      if (READ_USHORT(first_good[i]) ||
	  READ_USHORT(first_bad[i]) ) {
	cerr << "Can't read sd file row indices" << endl;
	return false;
      }
    }

    //
    // Read the block of range data values
    //

    if (gzread(sdfile, z_data, sizeof(unsigned short) * n_pts) !=
		sizeof(unsigned short) * n_pts) {
      cerr << "Can't read sd file range data values" << endl;
      gzclose(sdfile);
      return false;
    }

    //
    // Read the block of intensity data values
    //
    if (!g_bNoIntensity) {
      if (gzread(sdfile, intensity_data, sizeof(unsigned char) * n_pts) !=
      		sizeof(unsigned char) * n_pts) {
	cerr << "Can't read sd file intensity data values" << endl;
	gzclose(sdfile);
	return false;
      }
    }
  } else { // version 5
    //
    // Read the rest of the .sd header information
    //

    if (READ_UINT(n_frames) ||
	READ_UINT(pts_per_frame) ||
	READ_UINT(n_pts) ||
	READ_FLOAT(frame_pitch) ||
	READ_FLOAT(scan_screw) ||
	READ_FLOAT(other_screw) ||
	READ_FLOAT(scanner_trans) ||
	READ_UINT(scanner_config)) {
      cerr << "Can't read sd file header" << endl;
      return false;
    }

    //
    // Read data a frame at a time
    //

    first_good = new unsigned short[n_frames];
    first_bad  = new unsigned short[n_frames];
    z_data     = new unsigned short[n_pts];

    int cnt = 0;
    for (int i=0; i<n_frames; ++i) {

      if (READ_USHORT(first_good[i]) ||
	  READ_USHORT(first_bad[i]) ) {
	cerr << "Can't read sd file row indices" << endl;
	return false;
      }

      int toread = first_bad[i] - first_good[i];

      if (gzread(sdfile, &z_data[cnt], sizeof(unsigned short) * toread) !=
      		sizeof(unsigned short) * toread)  {
	cerr << "Can't read sd file data values" << endl;
	gzclose(sdfile);
	return false;
      }
      cnt += toread;
    }
  }
  gzclose(sdfile);

#ifdef LITTLE_ENDIAN
    for (int z = 0; z < n_pts; z++)
    	FIX_SHORT(z_data[z]);
#endif

  // initialize the CyberXform member variable
  // we would REALLY like to do the following:
  //xf.setup(scanner_config, scanner_trans, other_screw, scanner_vert);
  // but that hoses things that were already registered before
  // scanner_vert was taken into account, so we won't yet.
  xf.setup(scanner_config, scanner_trans, other_screw);

  // initialize the row_start array
  row_start  = new unsigned int[n_frames];
  row_start[0] = 0;
  for (int i=1; i<n_frames; ++i) {
    int j = i-1;
    row_start[i] = row_start[j] + first_bad[j] - first_good[j];
  }

  return true;
}


bool
SDfile::write(const crope &fname)
{
  FILE *sdfile;
  unsigned int magic_num =  0x444d5007;
  unsigned int header_size = 52;

  if (!(sdfile = fopen(fname.c_str(), "wb")))
    return false;

  //
  // Write the header block
  //

  if (WRITE_UINT(magic_num) ||
      WRITE_UINT(header_size) ||
      WRITE_UINT(n_frames) ||
      WRITE_UINT(pts_per_frame) ||
      WRITE_UINT(n_pts) ||
      WRITE_FLOAT(frame_pitch) ||
      WRITE_FLOAT(scan_screw) ||
      WRITE_FLOAT(other_screw) ||
      WRITE_FLOAT(scanner_trans) ||
      WRITE_FLOAT(scanner_vert) ||
      WRITE_UINT(scanner_config) ||
      WRITE_FLOAT(laser_intensity) ||
      WRITE_FLOAT(camera_sensitivity)) {
    cerr << "Can't write sd file header" << endl;
    fclose(sdfile);
    return false;
  }

  //
  // Write the block of start/stop indices
  //

  for (int i=0; i<n_frames; ++i) {
    if (WRITE_USHORT(first_good[i]) ||
	WRITE_USHORT(first_bad[i])) {
      cerr << "Can't write sd file row indices" << endl;
      fclose(sdfile);
      return false;
    }
  }

  //
  // Write the block of range data values
  //

#ifdef LITTLE_ENDIAN
  unsigned short *fixed_z = new unsigned short[n_pts];
  for (int z = 0; z < n_pts; z++)
    fixed_z[z] = swap_short(z_data[z]);

  if (fwrite(fixed_z, sizeof(unsigned short), n_pts, sdfile) != n_pts)  {
    delete [] fixed_z;
    cerr << "Can't write sd file range data values" << endl;
    fclose(sdfile);
    return false;
  }
  delete [] fixed_z;
#else
  if (fwrite(z_data, sizeof(unsigned short), n_pts, sdfile) != n_pts)  {
    cerr << "Can't write sd file range data values" << endl;
    fclose(sdfile);
    return false;
  }

#endif

  //
  // Write the block of intensity data values
  //

  if (g_bNoIntensity) {
    cerr << "No intensity data in memory, writing zeros..." << endl;
    char c = 0;
    for (i=0; i<n_pts; i++) {
      fwrite(&c, sizeof(unsigned char), 1, sdfile);
    }
  } else {
    if (fwrite(intensity_data, sizeof(unsigned char), n_pts, sdfile) !=
	n_pts) {
      cerr << "Can't write sd file intensity data values" << endl;
      fclose(sdfile);
      return false;
    }
  }

  fclose(sdfile);
  return true;
}

// Fill single-sample holes in .sd files
void
SDfile::fill_holes(int max_missing, int thresh)
{
  //Median<unsigned short> m;
  Median<float> m;
  for (int row = 0; row < n_frames; row++) {
    int numcols = first_bad[row]-first_good[row];
    if (!numcols)
      continue;
    for (int col = max_missing; col < numcols-max_missing; col++) {
      int index = row_start[row]+col;
      unsigned short me = z_data[index];
      unsigned short med;
      if (max_missing == 1) {
	med = __median(z_data[index-1],
		       me,
		       z_data[index+1]);
#if 1
      } else if (max_missing == 2) {
				// Fast approx. to median of 5
	med = __median(z_data[index-1],
		       me,
		       z_data[index+1]);
	med = __median(z_data[index-2],
		       med,
		       z_data[index+2]);
#endif
      } else {
				// Slow general-case code
	for (int i = index-max_missing; i <= index+max_missing; i++)
	  m += z_data[i];
	med = m.find();
	m.zap();
      }
      if (med && (abs(med - me) > thresh)) {
	int tot = 0;
	int num = 0;
	for (int i = index-max_missing; i <= index+max_missing; i++)
	  if (abs(z_data[i] - med) < thresh) {
	    tot += z_data[i];
	    num++;
	  }
	if (num)
	  z_data[index] = tot / num;
      }
    }
  }
}


void
SDfile::make_tstrip(vector<int>  &tstrips,
		    vector<char> &bdry)
{
  if (fabs(frame_pitch) > .07)
    make_tstrip_horizontal(tstrips, bdry);
  else
    make_tstrip_vertical(tstrips, bdry);
}


void
SDfile::subsampled_tstrip(int            step,
			  char          *behavior,
			  vector<int>   &tstrips,
			  vector<char>  &bdry,
			  vector<Pnt3>  &pnts,
			  vector<uchar> &intensity,
			  vector<uchar> &confidence,
			  vector<int>   &ssmap)
{
  int n_s = pts_per_frame / step;
  int end = 2*n_frames/step;

  SubSampMode subSampMode = !strcmp (behavior, "holes")
    ? holesGrow
    : !strcmp (behavior, "conf") ? conf
    : !strcmp (behavior, "filter") ? filterAvg
    : fast;

  if (n_s > 1 && end > 1) {
    // do horizontal stripping

    vector<int> p_inds[2], old_inds(n_s);
    p_inds[0].reserve(n_s);
    p_inds[0].insert(p_inds[0].begin(), n_s, -1);
    p_inds[1].reserve(n_s);
    p_inds[1].insert(p_inds[1].begin(), n_s, -1);

    vector<char> tmp_bdry(end*n_s); // a temporary bdry vector
    // first row
    fill_index_array(step, 0, subSampMode, p_inds[0],
		     pnts, ssmap, intensity, confidence);
    mark_all_empty(p_inds[0], tmp_bdry);
    bool flip = (frame_pitch > 0.0);
    bool vscan = scanner_config & 0x00000001;
    if (!vscan) flip = !flip;
    // second row and first strip
    fill_index_array(step, 1, subSampMode, p_inds[1],
		     pnts, ssmap, intensity, confidence);
    create_strip(tstrips, p_inds[0], p_inds[1],
		 true, flip);
    // rest of the rows
    int i,j;
    for (i=2,j=0; i<end; i++, j^=1) {
      copy(p_inds[j].begin(), p_inds[j].end(),old_inds.begin());
      fill(p_inds[j].begin(), p_inds[j].end(), -1);
      fill_index_array(step, i, subSampMode, p_inds[j],
		       pnts, ssmap, intensity, confidence);
      mark_maybe_empty(p_inds[!j], old_inds, p_inds[j], tmp_bdry);
      create_strip(tstrips, p_inds[!j], p_inds[j],
		   true, flip);
    }
    mark_all_empty(p_inds[!j], tmp_bdry);
    // copy the boundary info
// STL Update
    copy(tmp_bdry.begin(), tmp_bdry.begin() + pnts.size(),
	 back_insert_iterator<vector<char> > (bdry));
  }
}


void
SDfile::set_xf(int row, bool both)
{
  if (both) {
    // get the average screw length for the given row
    float scr = scan_screw + (row - .5*n_frames+.5) * frame_pitch;
    // the even field is a quarter pitch down, odd field up
    xf.set_screw(scr - 0.25*frame_pitch, scr + 0.25*frame_pitch);
  } else {
    // only even screw
    float scr = scan_screw + (row -.5*n_frames+.25) * frame_pitch;
    xf.set_screw(scr);
  }
}


void
SDfile::get_pnts_and_intensities(vector<Pnt3>  &pnts,
				 vector<uchar> &intensity)
{
  int cnt = 0;
  for (int i = 0; i < n_frames; ++i) {
    set_xf(i);
    for (int j=first_good[i]; j<first_bad[i]; j++, cnt++)  {
      if (z_data[cnt] == 0)
	continue; // missing data marked by zero
      pnts.push_back(xf.apply_xform(j, z_data[cnt]));
      if (!g_bNoIntensity)
	intensity.push_back (intensity_data[cnt]);
    }
  }
}



// access a data point, transformed or in laser coords
// row is the frame, col is a point on scanline,
// col even: frame 0; col odd: frame 1
// returns false if there is no such point
bool
SDfile::get_point(int row, int col, Pnt3 &p,
		  bool transformed)
{
  // row within range?
  if (row < 0 || row >= n_frames)
    return false;
  // column within range?
  if (first_good[row] > col || first_bad[row] <= col)
    return false;
  // valid point?
  short z = z_data[row_start[row] + col - first_good[row]];
  if (z == 0) return false;
  if (transformed) {
    set_xf(row);  // should check whether this already is valid
    p = xf.apply_xform(col, z);
  } else {
    p = xf.apply_raw_to_laser(col, z);
  }
  return true;
}


// Calculate the laser direction at the middle frame,
// find also the rotation axis, and create a frame such
// that the laser direction is aligned with negative z
// and the scanning axis is aligned with x (pos or neg irrelevant)
Xform<float>
SDfile::vrip_reorientation_frame(void)
{
  set_xf(n_frames/2);

  // get the laser direction
  short mid_column = 480/2;
  short front = 60;
  short back  = 2940;
  Pnt3 p1 = xf.apply_xform(mid_column, front);
  Pnt3 p2 = xf.apply_xform(mid_column, back);
  Pnt3 laser_dir = (p2 - p1).normalize();

  // get the center point
  short mid_z = (front + back) / 2;
  Pnt3 ctr = xf.apply_xform(mid_column, mid_z);

  // assume that a constant z-row is aligned in 3D with
  // scan axis
  p1 = xf.apply_xform(  0, mid_z);
  p2 = xf.apply_xform(480, mid_z);
  Pnt3 new_x = (p2-p1).normalize();

  // create the frame
  Pnt3 new_z = laser_dir;
  Pnt3 new_y = cross(new_z, new_x);
  new_x = cross(new_y, new_z);

  Xform<float> a,b;
  a.translate(-ctr);
  b.fromFrame(new_x, new_y, new_z, Pnt3());
  return b * a;
}


// given a screw length and a position on the scanline
// find the closest data point in the sweep
// return false if that data point is invalid
//
// a preliminary version, should be optimized and
// the logic should be improved...
bool
SDfile::find_data(float screw, float y_in,
		  int            &row,
		  unsigned short &y,
		  unsigned short &z)
{
  if (y_in < 0.0) {
    //SHOW(y_in);
    return false;
  }
  // first, calculate row
  // invert the equation in set_xf()
  // treat the screw input as the "average screw of the frame"
  // change it to the even field screw...
  float rowf = (screw - scan_screw) / frame_pitch + .5*n_frames-.25;

  //SHOW(screw / frame_pitch);
  //SHOW(25*frame_pitch);
  //rowf -= 25;

  float rowfloor = floor(row);
  float frac = row - rowfloor;
  // if frac: [0.0,.25] -> even field
  //          (.25,.75] -> odd field
  //          (.75,1.0) -> next row even field
  bool field_even;
  if (frac > .75) {
    row = int(rowf) + 1;
    field_even = true;
  } else {
    row = int(rowf);
    field_even = (frac <= .25);
  }
  if (row < 0 || row >= n_frames)
    return false;
  // if even field, snap to closest even integer,
  // similarly for odd
  if (field_even) {
    y = int(floor(y_in*.5+.5)) * 2;
  } else {
    y = int(floor(y_in*.5)) * 2 + 1;
  }
  if (y < first_good[row] || y >= first_bad[row]) {
    //SHOW(row);
    //SHOW(y);
    //SHOW(first_good[row]);
    //SHOW(first_bad[row]);
    return false;
  }
  z = z_data[row_start[row]+y-first_good[row]];
  return (z != 0);
}


int
SDfile::count_valid_pnts(void)
{
  if (n_valid_pts == 0) {
    unsigned short *end = z_data+n_pts;
    for (unsigned short *sp = z_data; sp != end; sp++) {
      if (*sp) n_valid_pts++;
    }
  }
  return n_valid_pts;
}


void
SDfile::filtered_copy(const VertexFilter &filter,
		      SDfile &newsd)
{
  // copy the data, first shallow copy
  newsd = *this;
  // then do a deeper, filtered copy
  newsd.row_start  = new unsigned int[n_frames];
  newsd.first_good = new unsigned short[n_frames];
  newsd.first_bad  = new unsigned short[n_frames];

  // initialize these to null
  newsd.intensity_data = NULL;
  newsd.z_data = NULL;

  // for z_data and intensity data, we need temporary
  // buffers, we don't know how big they'll be
  unsigned short *_z_data = new unsigned short[n_pts];
  unsigned char  *_bw_data = new unsigned char[n_pts];
  int cnt = 0;

  // march over all the points, apply transformations,
  // test for filtering, update the arrays above
  for (int i=0; i<n_frames; i++) {

    // initialize
    newsd.row_start[i] = cnt;
    int  FirstGood, LastGood;
    bool empty_row = true;
    newsd.set_xf(i);

    // check all valid ones of a row...
    for (int j=first_good[i]; j<first_bad[i]; j++) {

      int k = row_start[i] + j - first_good[i];

      // valid data, filter it
      if (z_data[k] != 0 &&
	  filter.accept(newsd.xf.apply_xform(j, z_data[k]))) {
	// accepted
	if (empty_row) { FirstGood = j; empty_row = false; }
	LastGood = j;
	_z_data[cnt]    = z_data[k];
	if (!g_bNoIntensity)
	  _bw_data[cnt] = intensity_data[k];
	cnt++;
      } else if (!empty_row) {
	// rejected
	_z_data[cnt]    = 0;
	_bw_data[cnt++] = 0;
      }
    }

    if (empty_row) {
      // row has no data
      newsd.first_good[i] = 0;
      newsd.first_bad[i]  = 0;
    } else {
      // row has filtered data
      newsd.first_good[i] = FirstGood;
      newsd.first_bad[i]  = LastGood+1;
      // remove the last bad entries from _z_data and _bw_data
      cnt -= (first_bad[i] - newsd.first_bad[i]);
    }
  }

  // copy the z and intensity data to the new copy
  newsd.n_pts  = cnt;
  if (cnt) {
    newsd.z_data = new unsigned short[cnt];
    memcpy(newsd.z_data, _z_data, sizeof(unsigned short) * cnt);
    if (!g_bNoIntensity) {
      newsd.intensity_data = new unsigned char[cnt];
      memcpy(newsd.intensity_data, _bw_data, cnt);
    }
  }
  delete[] _z_data;
  delete[] _bw_data;
}


void
SDfile::get_piece(int firstFrame, int lastFrame,
		  SDfile &newsd)
{
  // copy the data, first shallow copy
  newsd = *this;

  newsd.n_frames = lastFrame - firstFrame + 1;
  // then do a deeper, filtered copy
  newsd.row_start  = new unsigned int[newsd.n_frames];
  newsd.first_good = new unsigned short[newsd.n_frames];
  newsd.first_bad  = new unsigned short[newsd.n_frames];

  newsd.scan_screw = scan_screw +
    (firstFrame + (int(newsd.n_frames) - int(n_frames))*0.5)*frame_pitch;

  int offset = row_start[firstFrame];
  for (int i = firstFrame; i <= lastFrame; i++) {
    newsd.row_start[i-firstFrame]  = row_start[i] - offset;
    newsd.first_good[i-firstFrame] = first_good[i];
    newsd.first_bad[i-firstFrame]  = first_bad[i];
  }

  newsd.n_pts = newsd.row_start[newsd.n_frames-1] +
    newsd.first_bad[newsd.n_frames-1] -
    newsd.first_good[newsd.n_frames-1];

  if (newsd.n_pts) {
    // copy the z and intensity data to the new copy
    newsd.z_data = new unsigned short[newsd.n_pts];
    memcpy(newsd.z_data, z_data + row_start[firstFrame],
	   sizeof(unsigned short) * newsd.n_pts);
    if (!g_bNoIntensity) {
      newsd.intensity_data = new unsigned char[newsd.n_pts];
      memcpy(newsd.intensity_data,
	     intensity_data+row_start[firstFrame],
	     newsd.n_pts);
    }
  }
}


Bbox
SDfile::get_bbox(void)
{
  int cnt = 0;
  Bbox bb;
  for (int i = 0; i < n_frames; ++i) {
    set_xf(i);
    for (int j=first_good[i]; j<first_bad[i]; j++, cnt++)  {
      if (z_data[cnt] == 0)
	continue; // missing data marked by zero
      bb.add(xf.apply_xform(j, z_data[cnt]));
    }
  }
  return bb;
}


static int inside = 0;
static int outside = 0;
static int status_cnt = 0;
int
SDfile::sphere_status(const Pnt3 &ctr, float r)
{
  float screw, swin[2];
  short ywin[2], zwin[2];
  prepare_axis_proj();
  int ans = xf.sphere_status(ctr, r, screw,
			     axis_proj_min, axis_proj_max,
			     ywin, zwin, swin);

  status_cnt++;

  if (ans == 0) {
    return 4; // NOT_IN_FRUSTUM
  }

  if (ans == 1) {
    return 1; // BOUNDARY;
  }

  cout << ywin[0] << " "
       << ywin[1] << " "
       << zwin[0] << " "
       << zwin[1] << " "
       << swin[0] << " "
       << swin[1] << " " << r << endl;

  int start_row = (swin[0] - scan_screw) / frame_pitch + .5*n_frames-.25;
  int end_row   = (swin[1] - scan_screw) / frame_pitch + .5*n_frames-.25;

  if (end_row < 0 || start_row >= n_frames) {
    return 4; // NOT_IN_FRUSTUM
  }
  if (start_row < 0 || end_row >= n_frames) {
    // partially within frustum
    return 1; // BOUNDARY (force subdivision)
  }

  // really check the status

  bool all_behind = true;
  bool all_front  = true;
  bool all_within = true;

  SHOW(start_row);
  SHOW(end_row);

  int span = ywin[1] - ywin[0] + 1;
  for (int i=start_row; i<=end_row; i++) {
    if (first_good[i] >  ywin[0]) return 1; // invalid, force subdiv
    if (first_bad[i]  <= ywin[1]) return 1; // invalid, force subdiv
    unsigned short *z = &z_data[row_start[i] + ywin[0] - first_good[i]];
    for (int j=0; j<span; j++) {
      if      (z[j] < zwin[0]) { all_front  = all_within = false; }
      else if (z[j] > zwin[1]) { all_behind = all_within = false; }
      else                     { all_front  = all_behind = false; }
    }
    if (!all_behind && !all_front && !all_within)
      break;
  }

  if (all_behind) { cout << "OUTSIDE" << ++outside << endl; }
  if (all_front)  { cout << "INSIDE" << ++inside << endl; }

  if (all_within) return 1; // BOUNDARY
  if (all_behind) return 0; // OUTSIDE
  if (all_front)  return 3; // INSIDE
  return 2; // SILHOETTE
}


int
SDfile::dump_pts_laser_subsampled(ofstream &out, int nPnts)
{
  // go through all the valid points, do a uniform
  // subsampling
  int valids_left = n_valid_pts;
  int still_need  = nPnts;
  Random rnd;
  int cnt = 0;

  for (int i=0; i<n_frames; i++) {

    // see set_xf()
    float scr = scan_screw + (i - .5*n_frames+.5) * frame_pitch;
    float evenscr = scr - 0.25*frame_pitch;
    float oddscr  = scr + 0.25*frame_pitch;
    set_xf(i);

    // check all valid ones of a row...
    for (int j=first_good[i]; j<first_bad[i]; j++) {

      int k = row_start[i] + j - first_good[i];

      // valid data, filter it
      if (z_data[k]) {

	if (rnd() < float(still_need)/float(valids_left)) {
	  cnt++;
	  still_need--;
	  // transform to laser coords
	  Pnt3 p(0, j, z_data[k]);
	  xf.raw_to_laser(p);
	  assert(fabs(p[0]) < 1e-5);
	  out.write((char*)&p[1], sizeof(float));
	  out.write((char*)&p[2], sizeof(float));
	  if (j%2) out.write((char*)&oddscr,  sizeof(float));
	  else     out.write((char*)&evenscr, sizeof(float));
	  out.write((char*)&other_screw, sizeof(float));
	  out.write((char*)&scanner_trans, sizeof(float));
	  /*
	  // DELETE FROM HERE
	  if (j%2) SHOW(oddscr);
	  else     SHOW(evenscr);
	  SHOW(other_screw);
	  Pnt3 pp(0, j, z_data[k]);
	  SHOW(pp);
	  xf.raw_to_laser(pp);
	  SHOW(pp);
	  xf.laser_to_scanaxis(pp);
	  SHOW(pp);
	  SHOW(xf.apply_xform(j, z_data[k]));
	  return cnt;
	  // DELETE UP TO HERE
	  */
	}
	valids_left--;
      }
    }
  }
  return cnt;
}


Pnt3
SDfile::raw_for_ith(int   j, sd_raw_pnt &data)
{
  assert(j < n_pts);

  // find the row
  int bot = 0;
  int top = n_frames;
  while (top - bot > 1) {
    int i = (bot + top) / 2;
    if (row_start[i] > j) top = i;
    else                  bot = i;
  }
  int row = bot;

  // find the column
  int col = j - row_start[row] + first_good[row];

  data.y = col;
  data.z = z_data[j];
  data.config = scanner_config;
  float scr = scan_screw + (row - .5*n_frames+.5) * frame_pitch;
  if (j%2) scr += .25*frame_pitch;
  else     scr -= .25*frame_pitch;
  if (data.config & 0x00000001) {
    // vertical
    data.nod  = scr;
    data.turn = other_screw;
  } else {
    // horizontal
    data.turn = scr;
    data.nod  = other_screw;
  }
  data.tr_h = scanner_trans;
  // now, convert the raw to 3D for verification
  set_xf(row);
  return xf.apply_xform(data.y,data.z);
}


Pnt3
SDfile::raw_for_ith_valid(int   i, sd_raw_pnt &data)
{
  assert(i < n_valid_pts);

  // first, find the index for the ith valid point
  unsigned short *z = z_data;
  i++;
  while (i) { if (*(z++)) i--; }
  z--;

  return raw_for_ith(int(z-z_data), data);
}











