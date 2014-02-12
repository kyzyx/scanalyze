//###############################################################
// VertexFilter.cc
// Matt Ginzton, Kari Pulli
// Thu Jul  2 18:29:04 PDT 1998
//
// Interface for creating a filtered copy of mesh geometry data
//###############################################################


#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include "VertexFilter.h"
#include "TbObj.h"
#include "Pnt3.h"
#include "plvGlobals.h"
#include "Trackball.h"
#include "defines.h"
#include "plvClipBoxCmds.h"
#include "Projector.h"


// macros to accept / reject based on inversion status,
// to keep things less confusing
#define ACCEPT   return (!inverted)
#define REJECT   return (inverted)


//#define DEBUG_HIT_RATE

// factory that creates a specific VertexFilter from a Selection

VertexFilter* filterFromSelection (TbObj* ms, const Selection& sel)
{
  VertexFilter* filter = NULL;

  if (sel.type == Selection::rect) {
    filter = new ScreenBox (ms,
			    sel[0].x, sel[2].x,
			    sel[0].y, sel[2].y);
  } else if (sel.type == Selection::shape) {
    filter = new ScreenPolyLine (ms, sel.pts);
  }

  return filter;
}


#ifdef DEBUG_HIT_RATE
static int yes, no;
#endif

// and implementations
VertexFilter::VertexFilter()
{
#ifdef DEBUG_HIT_RATE
  yes = no = 0;
#endif
}


VertexFilter::~VertexFilter()
{
#ifdef DEBUG_HIT_RATE
  cout << "VertexFilter: Yes=" << yes << "; No=" << no << endl;
#endif
}


ScreenBox::ScreenBox (TbObj *ms, int x1, int x2, int y1, int y2)
  : projector (ms ? tbView : NULL, ms)
{
  // for projector constructor above:
  // get modelview matrix -- if ms is specified, use its coordinate system
  // (apply camera transform and its transform); if ms is NULL, then use
  // current GL state.

  inverted = false;
  xmin = MIN(x1,x2);
  xmax = MAX(x1,x2);
  ymin = MIN(y1,y2);
  ymax = MAX(y1,y2);
}


ScreenBox::ScreenBox (const ScreenBox* source)
{
  *this = *source;
}


VertexFilter* ScreenBox::transformedClone (const Xform<float>& xf) const
{
  ScreenBox* sb = new ScreenBox (this);
  sb->projector.xformBy (xf);

  return sb;
}


bool ScreenBox::accept (const Pnt3& crd) const
{
  Pnt3 screen = projector (crd);
  bool in = (screen[0] <= xmax && screen[0] >= xmin
	     && screen[1] <= ymax && screen[1] >= ymin);

#ifdef DEBUG_HIT_RATE
  if (in) ++yes; else ++no;
#endif

  if (in)
    ACCEPT;
  else
    REJECT;
}



//
// ScreenBox::accept (Bbox)
//
// Decide whether or not a bbox intersects the current screen box
// selection.  Based on Ned Greene's gem in Graphics Gems IV.
//


struct _edge {
   int vIndex[2];
   int fIndex[2];
  _edge() {}
};

struct _face {
   int vIndex[4];
   Pnt3 normal;
  _face() {}
};


class IntersectPrecalculations
{
public:
  IntersectPrecalculations()
    {
      // First some fun topology
      bboxEdges = vector<_edge> (12);
      bboxFaces = vector<_face> (6);

      // Back face edges
      bboxEdges[0].vIndex[0] = 0;
      bboxEdges[0].vIndex[1] = 3;
      bboxEdges[0].fIndex[0] = 0;
      bboxEdges[0].fIndex[1] = 2;

      bboxEdges[1].vIndex[0] = 3;
      bboxEdges[1].vIndex[1] = 2;
      bboxEdges[1].fIndex[0] = 0;
      bboxEdges[1].fIndex[1] = 5;

      bboxEdges[2].vIndex[0] = 2;
      bboxEdges[2].vIndex[1] = 1;
      bboxEdges[2].fIndex[0] = 0;
      bboxEdges[2].fIndex[1] = 3;

      bboxEdges[3].vIndex[0] = 1;
      bboxEdges[3].vIndex[1] = 0;
      bboxEdges[3].fIndex[0] = 0;
      bboxEdges[3].fIndex[1] = 4;

      // Front face edges
      bboxEdges[4].vIndex[0] = 7;
      bboxEdges[4].vIndex[1] = 4;
      bboxEdges[4].fIndex[0] = 1;
      bboxEdges[4].fIndex[1] = 2;

      bboxEdges[5].vIndex[0] = 6;
      bboxEdges[5].vIndex[1] = 7;
      bboxEdges[5].fIndex[0] = 1;
      bboxEdges[5].fIndex[1] = 5;

      bboxEdges[6].vIndex[0] = 5;
      bboxEdges[6].vIndex[1] = 6;
      bboxEdges[6].fIndex[0] = 1;
      bboxEdges[6].fIndex[1] = 3;

      bboxEdges[7].vIndex[0] = 4;
      bboxEdges[7].vIndex[1] = 5;
      bboxEdges[7].fIndex[0] = 1;
      bboxEdges[7].fIndex[1] = 4;

      // The rest of the edges
      bboxEdges[8].vIndex[0] = 0;
      bboxEdges[8].vIndex[1] = 4;
      bboxEdges[8].fIndex[0] = 2;
      bboxEdges[8].fIndex[1] = 4;

      bboxEdges[9].vIndex[0] = 7;
      bboxEdges[9].vIndex[1] = 3;
      bboxEdges[9].fIndex[0] = 2;
      bboxEdges[9].fIndex[1] = 5;

      bboxEdges[10].vIndex[0] = 5;
      bboxEdges[10].vIndex[1] = 1;
      bboxEdges[10].fIndex[0] = 3;
      bboxEdges[10].fIndex[1] = 4;

      bboxEdges[11].vIndex[0] = 2;
      bboxEdges[11].vIndex[1] = 6;
      bboxEdges[11].fIndex[0] = 3;
      bboxEdges[11].fIndex[1] = 5;


      // Back face
      bboxFaces[0].vIndex[0] = 3;
      bboxFaces[0].vIndex[1] = 2;
      bboxFaces[0].vIndex[2] = 1;
      bboxFaces[0].vIndex[3] = 0;

      // Front face
      bboxFaces[1].vIndex[0] = 4;
      bboxFaces[1].vIndex[1] = 5;
      bboxFaces[1].vIndex[2] = 6;
      bboxFaces[1].vIndex[3] = 7;

      // Left face
      bboxFaces[2].vIndex[0] = 0;
      bboxFaces[2].vIndex[1] = 4;
      bboxFaces[2].vIndex[2] = 7;
      bboxFaces[2].vIndex[3] = 3;

      // Right face
      bboxFaces[3].vIndex[0] = 1;
      bboxFaces[3].vIndex[1] = 2;
      bboxFaces[3].vIndex[2] = 6;
      bboxFaces[3].vIndex[3] = 5;

      // Bottom face
      bboxFaces[4].vIndex[0] = 0;
      bboxFaces[4].vIndex[1] = 1;
      bboxFaces[4].vIndex[2] = 5;
      bboxFaces[4].vIndex[3] = 4;

      // Top face
      bboxFaces[5].vIndex[0] = 3;
      bboxFaces[5].vIndex[1] = 7;
      bboxFaces[5].vIndex[2] = 6;
      bboxFaces[5].vIndex[3] = 2;
    };

  vector<_edge> bboxEdges;
  vector<_face> bboxFaces;
};

IntersectPrecalculations ned;


bool ScreenBox::accept (const Bbox &bbox) const
{
   int i;
   Bbox projBbox;
   vector<Pnt3> verts(8);
  vector<_edge> silEdges(12);

  // Explicitly define the vertices of the 3D bbox
  // note, the ordering is important here for the code that uses these to
  // work, and the ordering is NOT the same as that returned by
  // bbox.corner().
  verts[0].set(bbox.min()[0], bbox.min()[1], bbox.min()[2]);
  verts[1].set(bbox.max()[0], bbox.min()[1], bbox.min()[2]);
  verts[2].set(bbox.max()[0], bbox.max()[1], bbox.min()[2]);
  verts[3].set(bbox.min()[0], bbox.max()[1], bbox.min()[2]);
  verts[4].set(bbox.min()[0], bbox.min()[1], bbox.max()[2]);
  verts[5].set(bbox.max()[0], bbox.min()[1], bbox.max()[2]);
  verts[6].set(bbox.max()[0], bbox.max()[1], bbox.max()[2]);
  verts[7].set(bbox.min()[0], bbox.max()[1], bbox.max()[2]);

   // Transform and project the 3D bounding box and compute a screen space
   //  bounding box

   for (i = 0; i < 8; i++) {
     verts[i] = projector (verts[i]);
     if (verts[i][2] > 1) {
       //cerr << "flip" << endl;
       verts[i] *= -1;
     }

     projBbox.add(verts[i]);
   }

#ifdef VERBOSE
   cout << projBbox << endl;
   cout << "Screen bounds: ("
	<< xmin << ", " << ymin <<")  ("
	<< xmax << ", " << ymax << ")" << endl;
#endif

   // If the screen space bounding boxes do not intersect, then report
   //   no intersection

   // if the clip region is inverted, then every region is valid, unless it is
   // completely within the clipping box. A REJECT is specified, but due to how
   // this macro works, it actually ends up meaning ACCEPT in the inverted case.
   if (inverted) {
     if (projBbox.min()[0] >= xmin &&
	 projBbox.max()[0] <= xmax &&
	 projBbox.min()[1] >= ymin &&
	 projBbox.max()[1] <= ymax) {
       if (g_verbose) cerr << "Wholly within bounding box, rejecting" << endl;
       ACCEPT;
     }else {
       REJECT;
     }
   }

   if (projBbox.min()[0] >= xmax ||
       projBbox.max()[0] <= xmin ||
       projBbox.min()[1] >= ymax ||
       projBbox.max()[1] <= ymin) {
     REJECT;
   }

   // Compute face normals in the projected coordinates

   for (i = 0; i < 6; i++) {
     _face& face = ned.bboxFaces[i];
     face.normal = normal (verts[face.vIndex[0]],
			   verts[face.vIndex[1]],
			   verts[face.vIndex[2]]);
   }


   // Silhouette edges will have neighboring faces with opposite
   //   normals in the projection direction.  Identify these edges and
   //   make sure of the ordering of vertices within the edge.

   int numEdges = 0;
   for (i = 0; i < 12; i++) {
      if (ned.bboxFaces[ned.bboxEdges[i].fIndex[0]].normal[2] >= 0 &&
	  ned.bboxFaces[ned.bboxEdges[i].fIndex[1]].normal[2] <= 0) {
	silEdges[numEdges].vIndex[0] = ned.bboxEdges[i].vIndex[0];
	silEdges[numEdges].vIndex[1] = ned.bboxEdges[i].vIndex[1];
	numEdges++;
      }
      else if (ned.bboxFaces[ned.bboxEdges[i].fIndex[0]].normal[2] <= 0 &&
	       ned.bboxFaces[ned.bboxEdges[i].fIndex[1]].normal[2] >= 0) {
	silEdges[numEdges].vIndex[0] = ned.bboxEdges[i].vIndex[1];
	silEdges[numEdges].vIndex[1] = ned.bboxEdges[i].vIndex[0];
	numEdges++;
      }
   }

#ifdef VERBOSE
      cout << "Number of edges: " << numEdges << endl;
#endif

   // Treat the projected vertices as homogeneous 2D verts
   for (i = 0; i < 8; i++) {
      verts[i][2] = 1;
   }

   Pnt3 silEdgeEq;
   Pnt3 screenBoxVert0(xmin, ymin, 1);
   Pnt3 screenBoxVert1(xmax, ymin, 1);
   Pnt3 screenBoxVert2(xmin, ymax, 1);
   Pnt3 screenBoxVert3(xmax, ymax, 1);


   // Determine if the clip box is completely outside of the projected
   //   3D bbox in the image plane.

   for (i = 0; i < numEdges; i++) {

      // Compute equation of line

      silEdgeEq = cross(verts[silEdges[i].vIndex[0]],
			verts[silEdges[i].vIndex[1]]);


      // Determine if all points are on the "wrong" side of the line

      if ((dot(silEdgeEq,screenBoxVert0) < 0) &&
	  (dot(silEdgeEq,screenBoxVert1) < 0) &&
	  (dot(silEdgeEq,screenBoxVert2) < 0) &&
	  (dot(silEdgeEq,screenBoxVert3) < 0)) {
	REJECT;
      }
   }

   ACCEPT;
}


bool ScreenBox::acceptFully (const Bbox &bbox) const
{
  // quick test to see if ALL of a bbox passes the screenbox test.
  // If it does, then things inside it don't need to be tested.

   for (int i = 0; i < 8; i++) {
      if (!accept (bbox.corner (i)))
	REJECT;
   }

   ACCEPT;
}


void ScreenBox::invert (void)
{
  inverted = !inverted;
}


//////////////////////////////////////////////////////////////////////




ScreenPolyLine::ScreenPolyLine (TbObj *ms, const vector<ScreenPnt>& pts)
  : ScreenBox (ms, 0, 0, 0, 0)
{
  // fixup minmax x,y bounds
  xmin = xmax = pts[0].x;
  ymin = ymax = pts[0].y;
  for (int i = 1; i < pts.size(); i++) {
    if (pts[i].x < xmin)
      xmin = pts[i].x;
    if (pts[i].x > xmax)
      xmax = pts[i].x;
    if (pts[i].y < ymin)
      ymin = pts[i].y;
    if (pts[i].y > ymax)
      ymax = pts[i].y;
  }

  // then get buffer info
  width = theWidth;
  height = theHeight;
  pixels = filledPolyPixels (width, height, pts);
}


ScreenPolyLine::~ScreenPolyLine()
{
  delete pixels;
}


ScreenPolyLine::ScreenPolyLine (const ScreenPolyLine* source)
  : ScreenBox (source)
{
  *this = *source;

  // make a deep copy of the pixel data
  pixels = new unsigned char [width * height];
  memcpy(pixels, source->pixels,
	 width * height * sizeof(unsigned char));
}


VertexFilter* ScreenPolyLine::transformedClone (const Xform<float>& xf) const
{
  ScreenPolyLine* spl = new ScreenPolyLine (this);
  spl->projector.xformBy (xf);

  return spl;
}


bool ScreenPolyLine::accept (const Pnt3& crd) const
{
  Pnt3 screen = projector (crd);
  int x = screen[0];
  int y = screen[1];

  if (x < xmin || x >= xmax || y < ymin || y >= ymax)
    REJECT;

  int ofs = y * width + x;
  bool in = pixels[ofs] != 0;

#ifdef DEBUG_HIT_RATE
  if (in) ++yes; else ++no;
#endif

  if (in)
    ACCEPT;
  else
    REJECT;
}

