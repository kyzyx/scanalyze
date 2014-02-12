//###############################################################
// plvAnalyze.cc
// Matt Ginzton, Lucas Pereira
// Thu Jun 18 13:02:22 PDT 1998
//
// Interface for reading points or groups of points from z buffer
//###############################################################


#include "plvGlobals.h"
#include "plvDraw.h"
#include "plvDrawCmds.h"
#include "Trackball.h"
#include "togl.h"
#include "Pnt3.h"
#include "defines.h"
#include "sczRegCmds.h"
#include "plvAnalyze.h"
#include "ScanFactory.h"
#include "DisplayMesh.h"
#include "plvScene.h"
#include "plvClipBoxCmds.h"
#include "GenericScan.h"
#include "Mesh.h"
#include "DrawObj.h"
#include "plvImageCmds.h"
#include "ToglText.h"
#include "Projector.h"


#ifdef WIN32
// don't have rint, just consider it a noop
#	define rint (int)
#endif
#ifdef linux
#define MAXFLOAT FLT_MAX
#endif

// for determining background pixels in z buffer -- should be 0 to 1.0,
// or 0x0 to 0xffffffff, but on IR's the range is somewhat different,
// 0x7ffff to 0xffffff00
unsigned int g_zbufferMinUI;
unsigned int g_zbufferMaxUI;
float g_zbufferMinF;
float g_zbufferMaxF;

#define ISBACKGROUND(zfloat) ((zfloat) >= g_zbufferMaxF)


#ifndef MIN
#define MIN(a,b)  ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b)  ((a)>(b)?(a):(b))
#endif

#define DEBUG 1

static void readLinePixels (int x, int y, int w, int h,
			    GLenum component, GLenum type,
			    char* data, int dataSize);



int
WriteOrthoDepth(ClientData clientData, Tcl_Interp *interp,
                int argc, char *argv[])
{
   GLint viewport[4];
   GLdouble projmatrix[16];
   GLdouble idmatrix[16] = {1.0,0,0,0, 0,1.0,0,0, 0,0,1.0,0, 0,0,0,1.0};
   GLdouble wx, wy, wz;
   float *dvp;
   double min_eye_z = 0.0, max_eye_z = -MAXFLOAT;
   float zval;
   int val;
   int i, j;

   if (argc != 2) {
       Tcl_SetResult(interp, "Incorrect Num of Args", NULL);
       return TCL_ERROR;
   }

   FILE *fd = fopen(argv[1], "w");
   if (fd == NULL) {
       Tcl_SetResult(interp, "Can't open file for writing", NULL);
       return TCL_ERROR;
   }

   // Read the depth buffer values into a local data structure

   int width = Togl_Width (toglCurrent);
   int height = Togl_Height (toglCurrent);
   float *depthvals = new float[width*height];

   glReadBuffer(GL_FRONT);
   glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT,
                GL_FLOAT, depthvals);

   // Unproject the depth buffer values back to eye coordinates.

   glGetIntegerv(GL_VIEWPORT, viewport);
   glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);

   for (i=0; i<height; ++i)
       for (j=0; j<width; ++j) {
            dvp = &(depthvals[i*width+j]);
	    if (ISBACKGROUND(*dvp))
	       *dvp = 1000.0;
            else  {
               gluUnProject((GLdouble) j, (GLdouble) i, (GLdouble) *dvp,
                   idmatrix, projmatrix, viewport, &wx, &wy, &wz);
               if (wz > max_eye_z)
		  max_eye_z = wz;
               if (wz < min_eye_z)
		  min_eye_z = wz;
               *dvp = wz;
            }
       }

   // Write the scaled depth values out to a 16-bit .PPM format file.
   // Pixel value 0 is used as a "no data" flag.

   double dgap = max_eye_z - min_eye_z;
   double dscale = dgap / 65534.0;
   double dbias = -max_eye_z - dscale;

   fprintf(fd, "P3\n");
   fprintf(fd, "# 16-bit .PPM format displacement map (via Scanalyze)\n");
   fprintf(fd, "# Image dimensions: ");
   fprintf(fd, "width=%g height=%g\n", 2/projmatrix[0], 2/projmatrix[5]);
   fprintf(fd, "# Displacement resolution=%g\n", dscale);
   fprintf(fd, "# Pixel values of 0 (if any) correspond to missing data\n");
   fprintf(fd, "%d %d 65535\n", width, height);

   for (i=height-1; i>=0; i--)
       for (j=0; j<width; ++j)  {
           zval = depthvals[i*width + j];
	   if (zval > 0.0)
	       fprintf(fd, "0 0 0\n");
           else  {
              val = (int) rint((-zval-dbias)/dscale);
	      fprintf(fd, "%d %d %d\n", val, val, val);
	   }
       }

   delete[] depthvals;
   fclose(fd);
   return TCL_OK;
}


//////////////////////////////////////////////////////////////////////
//
// Point picking tools
//
//////////////////////////////////////////////////////////////////////


//X and Tcl are upper left origin
//GL is lower left..yah

int
WriteWorldDataFromScreen(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  Pnt3 pt;
  int width = Togl_Width(toglCurrent);
  int height = Togl_Height(toglCurrent);
  //ScreenToWorldCoordinates assumes GL lower left origin
  if(argc !=2)
    {
      Tcl_SetResult(interp, "Incorrect Num of Args", NULL);
      return TCL_ERROR;
    }
  FILE *fd = fopen(argv[1], "w");
  if(fd == NULL)
    {
      Tcl_SetResult(interp,
		    "Can't open file for writing", NULL);
      return TCL_ERROR;
    }
  //fprintf(fd, "# format is u,v x,y,z u,v, is GL (lower left) origin\n");
  fprintf(fd, "# format is u,v x,y,z   (u,v, is upper left origin)\n");
  fprintf(fd, "# next line is width, height of image\n");
  fprintf(fd, "%d %d\n",width,height);
  for(int x=0; x<width;x++)
    for(int y=0;y<height;y++)
      {
	if(ScreenToWorldCoordinates(x,y,pt))
	  {
	    //this point is on the object.
	    fprintf(fd,"%d %d %f %f %f\n",x,height-y,pt[0],pt[1],pt[2]);
	  }
      }
  fclose(fd);
  return TCL_OK;
}

/*
{
  Pnt3 pt;
  //get screen size
  int width = Togl_Width (toglCurrent);
  int height = Togl_Height (toglCurrent);
  //fprintf(stderr, "plvAnalyze.cc:WriteWorldDataFromScreen "
  //  "width,height is %d,%d\n",width,height);

  if(argc !=2)
    {
      Tcl_SetResult(interp, "Incorrect Num of Args", NULL);
      return TCL_ERROR;
    }
  FILE *fd = fopen(argv[1], "w");
#ifdef DEBUG
  char buffer[1024];
  sprintf(buffer, "%s.iv",argv[1]);
  FILE *ivfile = fopen(buffer, "w");
  fprintf(ivfile, "#Inventor V2.1 ascii#\n\n");
  //new
  fprintf(ivfile, "Separator {\nCoordinate3 { \n point [\n");
#endif
  if(fd == NULL)
    {
      Tcl_SetResult(interp,
		    "Can't open file for writing", NULL);
      return TCL_ERROR;
    }
  for(int w=0; w < width; w++)
    for(int h=0; h <height; h++)
      {
	ScreenToWorldCoordinates(w,h, pt);

	if(pt[0] != 0.0 || pt[1] != 0.0 ||
	   pt[2] != 0.0)
	  {
	    fprintf(fd, "%d %d %f %f %f\n",w,h,pt[0],
		    pt[1],pt[2]);
#ifdef DEBUG
	    fprintf(ivfile, "%f %f %f,\n",pt[0],pt[1],pt[2]);
	    //	    fprintf(ivfile, "Separator { \n");
	    //fprintf(ivfile, "Translation { translation %f %f %f } \n",
	    //	    pt[0],pt[1],pt[2]);
	    //fprintf(ivfile, "Sphere { radius .0001 }\n}\n");

#endif
	  }
      }
#ifdef DEBUG
  //add last point again, w/o the comma
  fprintf(ivfile, "%f %f %f ]\n",pt[0],pt[1],pt[2]);
  fprintf(ivfile, "}\n}\n");
  fclose(ivfile);
#endif
  fclose(fd);

  return TCL_OK;
  //TCL_ERROR
}
*/

int
ScreenToWorldCoordinates (int x, int y, Pnt3& ptWorld)
{
  // assumes GL coordinates (0,0 is lower left)
  return ScreenToWorldCoordinatesExt (x, y, ptWorld, toglCurrent, tbView, 0);
}


int
ScreenToWorldCoordinatesExt (int x, int y, Pnt3& ptWorld,
			     struct Togl* togl, Trackball* tb,
			     float fudgeFactor)
{
  // assumes GL coordinates (0,0 is lower left)

  Togl_MakeCurrent (togl);
  float pixdepth;
  if (theRenderParams->polyMode == GL_FILL || theRenderParams->hiddenLine)
  {
    // have poly data (or hidden-line/point, also filled)
    // read it from front buffer
    glReadBuffer (GL_FRONT);
    glReadPixels (x, y, 1, 1, GL_DEPTH_COMPONENT,
		  GL_FLOAT, &pixdepth);
    //printf("pixdepth (from front): %f\n", pixdepth);
  }
  else {
    // don't have poly data... need to get it
    GLenum oldPolyMode = theRenderParams->polyMode;
    bool oldHiddenLine = theRenderParams->hiddenLine;
    theRenderParams->polyMode = GL_FILL;
    theRenderParams->hiddenLine = FALSE;

    if (togl == toglCurrent)
      drawInToglBuffer (togl, GL_BACK);
    else {
      //hack warning -- assumes all togls except the main one have an
      //AlignmentToglInfo; need to change this
      DrawAlignmentMeshToBack (togl);
    }

    glReadBuffer (GL_BACK);
    glReadPixels (x, y, 1, 1, GL_DEPTH_COMPONENT,
		  GL_FLOAT, &pixdepth);
    //printf("pixdepth (from back): %f\n", pixdepth);

    theRenderParams->polyMode = oldPolyMode;
    theRenderParams->hiddenLine = oldHiddenLine;
  }

  // ok, at this point, pixdepth has the zbuffer value
  // ignore z-depth of 1.0 because that's the background
  if (ISBACKGROUND (pixdepth))
    return FALSE;

  pixdepth -= fudgeFactor;

  // need to unproject z-buffer data to world coordinates
  Unprojector unproject (tb);
  ptWorld = unproject (x, y, pixdepth);

  //printf("You selected world position: %.4f %.4f %.4f\n",
  //   worldPos[0], worldPos[1], worldPos[2]);

  return true;
}


//////////////////////////////////////////////////////////////////////
//
// Clip line analysis tools
//
//////////////////////////////////////////////////////////////////////


class ClipLineData
{
public:
  ClipLineData (int w);
  ~ClipLineData();

  GLfloat* pixels;          // z-buffer data
  GLfloat* lines;           // z-buffer data from wireframe render
  Pnt3*    objPt;           // unprojected object coordinates

  vec3uc color;
  DisplayableMesh *theMesh;
};


class ClipLineInfo
{
public:
  ClipLineInfo (int w);
  ~ClipLineInfo();

  // data corresponding to multiple scans
  vector<ClipLineData*> data;

  // info pertaining to aggregate of data collection
  int width;                //width of line, in pixels
  GLdouble zMin, zMax;      //min, max values in objZ
  GLdouble xMin, xMax;
  int nFirstGood, nLastGood;//indices of first & last
                            //non-background pixels in z-buffer

  // UI settings
  bool  bShowScale;
  bool  bShowEdges;
  bool  bShowFramebuffPts;
  bool  bShowColor;
  int   nScaleExp;
  float zRescale;
};


ClipLineInfo::ClipLineInfo (int w)
{
  width = w;

  bShowScale = true;
  bShowEdges = false;
  bShowFramebuffPts = false;
  bShowColor = true;
  nScaleExp = 0;
  zRescale = 1;
}


ClipLineInfo::~ClipLineInfo()
{
// STL Update
  for (vector<ClipLineData*>::iterator iter = data.begin(); iter < data.end(); iter++)
    delete *iter;
}


ClipLineData::ClipLineData (int w)
{
  pixels = new GLfloat [w];
  lines = new GLfloat [w];
  objPt = new Pnt3 [w];
}


ClipLineData::~ClipLineData()
{
  delete [] pixels;
  delete [] lines;
  delete [] objPt;
}


int
PlvAnalyzeLineModeCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  static float zsBase = 1;

  // args: analyzeTogl [show linetype bool] | [scale n] | [level bool]
  if (argc < 4) {
    interp->result = "Bad args to PlvAnalyzeLineModeCmd";
    return TCL_ERROR;
  }

  struct Togl* togl = toglHash.FindTogl (argv[1]);
  if (!togl) {
    interp->result = "Missing togl in PlvAnalyzeLineModeCmd";
    return TCL_ERROR;
  }
  ClipLineInfo* cli = (ClipLineInfo*)Togl_GetClientData (togl);

  if (!strcmp (argv[2], "show")) {
    if (argc < 5)
      return TCL_ERROR;
    bool bShow = atoi (argv[4]);

    if (!strcmp (argv[3], "scale"))
      cli->bShowScale = bShow;
    else if (!strcmp (argv[3], "edges"))
      cli->bShowEdges = bShow;
    else if (!strcmp (argv[3], "framebuff"))
      cli->bShowFramebuffPts = bShow;
    else if (!strcmp (argv[3], "color"))
      cli->bShowColor = bShow;
  } else if (!strcmp (argv[2], "scale")) {
    cli->nScaleExp = atoi (argv[3]);
  } else if (!strcmp (argv[2], "zscale")) {
    int zs = atoi (argv[3]);
    if (!strcmp (argv[4], "start")) {
      zsBase = zs / cli->zRescale;
    } else {
      if (!strcmp (argv[4], "reset"))
	cli->zRescale = 1;
      else
	cli->zRescale = MAX (1, zs / zsBase);
      double zScale = (cli->zMax - cli->zMin) / cli->zRescale;
      char buf[50];
      sprintf (buf, "setAnalyzeZScale %s %g", argv[1], zScale);
      Tcl_Eval (interp, buf);
    }
  } else {
    interp->result = "Bad subcommand in PlvAnalyzeModeCmd";
    return TCL_ERROR;
  }

  Togl_PostRedisplay (togl);

  return TCL_OK;
}


int
PlvExportGraphAsText(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  Togl *togl;
  char *winName;
  FILE *datafile, *ctlfile;
  ClipLineInfo *cli;
  char fileName[256];

  if ( argc < 3 ) {
    return TCL_ERROR;
  }

  char toglName[100];
  sprintf (toglName, "%s.togl", argv[1]);

  togl = toglHash.FindTogl(toglName);
  winName = argv[2];

  if ( togl == NULL ) {
    interp->result = "PlvExportGraphAsText: Null togl pointer";
    return TCL_ERROR;
  }

  // don't need to check, b/c tcl caller should do it
  ctlfile = fopen(winName, "w");
  // don't copy the extension, but remember to add terminator
  strncpy(fileName, winName, strlen(winName) - 4);
  fileName[(strlen(winName) - 4)] = '\0';

  // comment title, set postscript output
  fprintf(ctlfile, "# %s\nset term post p c s 12\n", fileName);
  fprintf(ctlfile, "set size .75, .75\n");
  // set ps output file, set title of graph
  fprintf(ctlfile, "set output \"%s.ps\"\nset title \"%s\"\n", fileName, fileName);


  cli = (ClipLineInfo*)Togl_GetClientData(togl);
  int nPix = cli->width;

  //  int k = 0;
  fprintf(ctlfile, "plot ");
// STL Update
  for (vector<ClipLineData*>::iterator pcld = cli->data.begin();
       pcld < cli->data.end(); pcld++) {
    ClipLineData* cld = *pcld;

    char buff[256];
    //    sprintf(buff, "%s.%i.dat", winName, k);
    sprintf(buff, "%s_%s.dat", fileName, cld->theMesh->getName());
    //    sprintf(buff, "%s_%s.dat", fileName, cld->theMesh->displayName);

    datafile = fopen(buff, "w");
    if ( datafile == NULL ) {
      printf ("can't open file\n");
    }
    fprintf(ctlfile, "'%s' title '%s' with lines", buff, cld->theMesh->getName());
    if ( pcld + 1 < cli->data.end() ) {
      fprintf(ctlfile, ", ");
    }

    for (int x = 0; x < nPix; x++ ) {
      if (!ISBACKGROUND(cld->pixels[x])) {
	fprintf(datafile, "\n%f %f", cld->objPt[x][0], cld->objPt[x][2]);
      }
    }
    //    k++;
    fclose(datafile);
  }

  fclose(ctlfile);
  return TCL_OK;
} // PlvExportGraphAsText


void
PlotLineDepth (struct Togl* togl)
{
  ClipLineInfo* cli = (ClipLineInfo*)Togl_GetClientData (togl);
  int nPixels = cli->width;

  int w = Togl_Width (togl);
  int h = Togl_Height (togl);

  //set child's togl as current
  glViewport (0, 0, w, h);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();

  glClear (GL_COLOR_BUFFER_BIT);

  //draw grid -- square only if correct aspect ratio
  double scaleW = (cli->xMax - cli->xMin) *
	   (cli->width / (double)(cli->nLastGood - cli->nFirstGood));
  double scaleH = cli->zMax - cli->zMin;
  float zRescaleRatio = (cli->zRescale - 1) / cli->zRescale;
  gluOrtho2D (0, scaleW, zRescaleRatio * scaleH, scaleH);

  double step = pow (10, cli->nScaleExp);

  if (cli->bShowScale) {
    glColor3f (.3, .2, 0);  // lucas brown

    glBegin (GL_LINES);
    for (double i = 0; i <= scaleH; i += step) {
      glVertex2f (0, i);
      glVertex2f (scaleW, i);
    }
    for (i = 0; i <= scaleW; i += step) {
      glVertex2f (i, 0);
      glVertex2f (i, scaleH);
    }
    glEnd();
  }

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();
  //xMin and xMax will be clamped around object, ignoring background -- we
  //want to show background -- pixel width of data is 0 to cli->width;
  //pixel width of object w/o background is cli->nFirstGood to cli->nLastGood;
  //xMin and xMax are in nFirstGood to nLastGood range and need to be scaled
  //to 0 to nPixels range.
  float scale = (cli->xMax - cli->xMin) / (cli->nLastGood - cli->nFirstGood);
  float left = cli->xMin - (scale * cli->nFirstGood);
  float right = cli->xMax + (scale * (cli->width - 1 - cli->nLastGood));
  float bottom = cli->zMin + (cli->zMax - cli->zMin) * zRescaleRatio;
  float top = cli->zMax;
  gluOrtho2D (left, right, bottom, top);

  //printf ("X: 0, %d, %d, %d\n", cli->nFirstGood, cli->nLastGood, nPixels);
  //printf ("   %g, %g, %g, %g\n", left, cli->xMin, cli->xMax, right);
  //printf ("Z: %g to %g\n", bottom, top);
  //fflush (stdout);

// STL Update
  for (vector<ClipLineData*>::iterator pcld = cli->data.begin(); pcld < cli->data.end();
       pcld++) {
    ClipLineData* cld = *pcld;

    //in background, draw mesh wireframe borders (yellow)
    if (cli->bShowEdges) {
      glColor4f (1, 1, 0, .7);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable (GL_BLEND);
      float zDiff = (cli->zMax - cli->zMin) / 20;
      for (int x = 0; x < nPixels; x++) {
	if (cld->lines[x] > 0) {    //this is the luminance value
	  if ((x+1) < nPixels && (cld->lines[x+1] > 0)) {
	    // next one is a mesh crossing too -- because these are likely
	    // to have come from a single mesh crossing at an oblique angle,
	    // shade the whole area between them.
	    glBegin (GL_QUADS);
	    glVertex2f (cld->objPt[x  ][0], cld->objPt[x][2  ] - zDiff);
	    glVertex2f (cld->objPt[x  ][0], cld->objPt[x][2  ] + zDiff);
	    glVertex2f (cld->objPt[x+1][0], cld->objPt[x+1][2] + zDiff);
	    glVertex2f (cld->objPt[x+1][0], cld->objPt[x+1][2] - zDiff);
	    glEnd();
	  } else {
	    glBegin (GL_LINES);
	    glVertex2f (cld->objPt[x][0], cld->objPt[x][2] - zDiff);
	    glVertex2f (cld->objPt[x][0], cld->objPt[x][2] + zDiff);
	    glEnd();
	  }
	}
      }
      glDisable (GL_BLEND);
    }

    //draw data points, connected by lines (mesh's false color)
    if (cli->bShowColor)
      glColor3ub (cld->color[0], cld->color[1], cld->color[2]);
    else
      glColor3f (1, 1, 1);

    glBegin (GL_LINE_STRIP);
    for (int x = 0; x < nPixels; x++) {
      if (!ISBACKGROUND(cld->pixels[x])) {
	glVertex2f (cld->objPt[x][0], cld->objPt[x][2]);
      } else {
	if (x > 0) {
	  glVertex2f (cld->objPt[x-1][0], cli->zMin - 1);
	}
	if (x + 1 < nPixels) {
	  glEnd();
	  glBegin (GL_LINE_STRIP);
	  glVertex2f (cld->objPt[x+1][0], cli->zMin - 1);
	}
      }
      //printf ("True coords: %g %g %g -- from %d, %f\n",
      //cld->objX[x], cld->objY[x], cld->objZ[x], x, cld->pixels[x]);
    }
    glEnd();

    //now draw individual data points (green)
    if (cli->bShowFramebuffPts) {
      glColor3f (0, 1, 0);
      float ptSize = (float)MIN (w, h) / nPixels;
      if (ptSize < 1.0)  //scale point size with window -- don't want to
	ptSize = 1.0;    //dominate white line, but want points visible
      glPointSize (ptSize);
      glBegin (GL_POINTS);
      for (x = 0 ; x < nPixels; x++) {
	if (!ISBACKGROUND(cld->pixels[x]))
	  glVertex2f (cld->objPt[x][0], cld->objPt[x][2]);
      }
      glEnd();
    }
  }

  char legend[200];
  sprintf (legend, "x range: %g (%g occupied); z range: %g",
	   scaleW, cli->xMax - cli->xMin, scaleH / cli->zRescale);
  glLoadIdentity();
  gluOrtho2D (0, w, 0, h);
  glColor3f (1, 1, 1);
  glRasterPos2i (2, 2);
  DrawText (togl, legend);

  Togl_SwapBuffers (togl);
}


void
FreeLineDepthInfo (struct Togl* togl)
{
  ClipLineInfo* cli = (ClipLineInfo*)Togl_GetClientData (togl);
  delete cli;
}


int nAnalysis = 0;             //need to change window name each time so
			       //tcl doesn't get mad at us for creating
			       //multiple windows with same name


int
PlvAnalyzeClipLineDepth(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  struct Togl* toglAnalyze;
  bool bFoundAnyData = false;
  char meshNames[1000] = "";

  if (theSel.type != Selection::line) {
    interp->result = "You must first select a line";
    return TCL_ERROR;
  }

  int dx = theSel[1].x - theSel[0].x;
  int dy = theSel[1].y - theSel[0].y;
  int x = theSel[0].x;
  int y = theSel[0].y;

  // force plot to go left-to-right for readability
  if (dx < 0) { // so if it's not, flip it
    x += dx;   dx *= -1;
    y += dy;   dy *= -1;
  }

  int nPixels = max (abs(dx), abs(dy));

  if (nPixels == 0) {
    interp->result = "You must first select a line";
    return TCL_ERROR;
  }

  ClipLineInfo* cli = new ClipLineInfo (nPixels);

  //need to unproject z-buffer data with identity modelview
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  Unprojector unproject;
  glPopMatrix();

  // have to render without boundSelection, or bbox will show up
  PushRenderParams();
  theRenderParams->boundSelection = false;

// STL Update
  for (vector<DisplayableMesh*>::iterator pdm = theScene->meshSets.begin();
       pdm < theScene->meshSets.end();
       pdm++) {
    if (!(*pdm)->getVisible())
      continue;

    ClipLineData* cld = new ClipLineData (nPixels);
    // render hidden line to back buffer and read data
    theRenderParams->polyMode = GL_LINE;
    theRenderParams->hiddenLine = true;
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    (*pdm)->drawSelf (false);
    glReadBuffer (GL_BACK);
    readLinePixels (x, y, dx, dy, GL_LUMINANCE, GL_FLOAT,
		    (char*)cld->lines, sizeof(cld->lines[0]));

    // render polys to back buffer and read data
    theRenderParams->polyMode = GL_FILL;
    theRenderParams->hiddenLine = false;
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    (*pdm)->drawSelf (false);
    glReadBuffer (GL_BACK);
    readLinePixels (x, y, dx, dy, GL_DEPTH_COMPONENT, GL_FLOAT,
		    (char*)cld->pixels, sizeof(cld->pixels[0]));

    //unproject data, and calculate min/max bounds on z coords
    bool bFoundThisData = false;
    for (int i = 0; i < nPixels; i++) {
      cld->objPt[i] = unproject (x + i, y, cld->pixels[i]);

      //ignore 1.0 because that's the background
      if (!ISBACKGROUND(cld->pixels[i])) {
	bFoundThisData = true;
	if (!bFoundAnyData) {
	  bFoundAnyData = true;
	  cli->zMin = cli->zMax = cld->objPt[i][2];
	  cli->xMin = cli->xMax = cld->objPt[i][0];
	  cli->nFirstGood = i;
	  cli->nLastGood = i;
	} else {
	  if (i < cli->nFirstGood)
	    cli->nFirstGood = i;
	  if (i > cli->nLastGood)
	    cli->nLastGood = i;

	  float z = cld->objPt[i][2];
	  if (z < cli->zMin)
	    cli->zMin = z;
	  if (z > cli->zMax)
	    cli->zMax = z;

	  float x = cld->objPt[i][0];
	  if (x < cli->xMin)
	    cli->xMin = x;
	  if (x > cli->xMax)
	    cli->xMax = x;
	}
      }
    }

    if (bFoundThisData) {
      const vec3uc& color = (*pdm)->getFalseColor();
      cld->color[0] = color[0];
      cld->color[1] = color[1];
      cld->color[2] = color[2];
      cld->theMesh = *pdm; // wsh added this line to be able to get names...
      cli->data.push_back (cld);

      if (bFoundAnyData)
	strcat (meshNames, " ");
	//	strcat(meshNames, "_");
      strcat (meshNames, (*pdm)->getName());
    }
  }

  PopRenderParams();

  if (!bFoundAnyData) {
    interp->result =
      "No data found.  To do line analysis, draw a line selection,\n"
      "and make sure it intersects a scan!";
    delete cli;
    return TCL_ERROR;
  }

  if (ISBACKGROUND (1 - (cli->zMax - cli->zMin))) {
    interp->result =
      "Region under clip line is too uniform to analyze!";
    delete cli;
    return TCL_ERROR;
  }
  //else printf ("min/max: %f/%f\n", cli->zMin, cli->zMax);

  //create toplevel containing togl
  //need to size window to w pixels wide, 60 pixels tall (arbitrary)
  //instead of using a script here, could call Tk_CreateWindow,
  //Tk_GeometryRequest, and Togl_Widget

  double xScale = cli->xMax - cli->xMin;
  double zScale = cli->zMax - cli->zMin;
  int defScale = (int)log10 (min (xScale, zScale) / 10);
  cli->nScaleExp = defScale;  // in case tcl is fickle and doesn't send message

  char script[1000];
  sprintf (script, "createAnalyzeWindow %d %d %g %g %g \"%s\" %d",
	   ++nAnalysis,
	   nPixels, xScale,
	   xScale * (cli->width / (double)(cli->nLastGood - cli->nFirstGood)),
	   zScale, meshNames, defScale);
  if (TCL_OK != Tcl_Eval (interp, script)) {
    delete cli;
    return TCL_ERROR;
  }

  char toglName[100];
  sprintf (toglName, ".analyze%d.togl", nAnalysis);
  toglAnalyze = toglHash.FindTogl (toglName);
  assert (toglAnalyze != NULL);

  Togl_SetClientData (toglAnalyze, cli);
  Togl_SetDisplayFunc (toglAnalyze, PlotLineDepth);
  Togl_SetDestroyFunc (toglAnalyze, FreeLineDepthInfo);

  // eval of createAnalyzeWindow set interp->result to name of window
  // created -- leave it that way
  return TCL_OK;
}


// run this function once at startup time, when we don't care what's
// onscreen since it clears it, to initialize some important variables

void storeMinMaxZBufferValues (void)
{
  if (g_bNoUI)
    return;

  assert (toglCurrent != NULL);
  Togl_MakeCurrent (toglCurrent);

  // clear z-buffer to get background z-value
  glClearDepth (0.0);
  glClear (GL_DEPTH_BUFFER_BIT);
  glReadPixels (0, 0, 1, 1,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, &g_zbufferMinUI);
  glReadPixels (0, 0, 1, 1,
		GL_DEPTH_COMPONENT, GL_FLOAT, &g_zbufferMinF);

  glClearDepth (1.0);  // this leaves it back at the default state
  glClear (GL_DEPTH_BUFFER_BIT);
  glReadPixels (0, 0, 1, 1,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, &g_zbufferMaxUI);
  glReadPixels (0, 0, 1, 1,
		GL_DEPTH_COMPONENT, GL_FLOAT, &g_zbufferMaxF);

  //fprintf (stderr, "Z buffer range: %08x to %08x (%g to %g)\n",
  //   g_zbufferMinUI, g_zbufferMaxUI,
  //   g_zbufferMinF, g_zbufferMaxF);
}


static unsigned int* ReadZBufferRect (int x, int y, int w, int h,
				      bool bRedraw = true)
{
  unsigned int* data = new unsigned int [w * h];

  if (bRedraw) {
    // draw scene
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    bool oldbs = theRenderParams->boundSelection;
    theRenderParams->boundSelection = false;
    drawInToglBuffer (toglCurrent, GL_BACK);
    theRenderParams->boundSelection = oldbs;
  }

  // and collect z-buffer
  glReadBuffer (GL_BACK);
  glReadPixels (x, y, w, h, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, data);
  if (glGetError()) {
    delete[] data;
    return NULL;
  }

  return data;
}


bool ReadZBufferArea (vector<Pnt3>& pts, const Selection& sel,
		      bool bIncludeBlanks, bool bRedraw,
		      bool bReverseBackground)
{
  Unprojector unproject (tbView);

  if (sel.type == Selection::rect) {
    int x = min (sel[0].x, sel[2].x);
    int w = abs (sel[0].x - sel[2].x);
    int y = min (sel[0].y, sel[2].y);
    int h = abs (sel[0].y - sel[2].y);

    unsigned int* data = ReadZBufferRect (x, y, w, h, bRedraw);
    if (!data)
      return false;

    for (int iy = 0; iy < h; iy++) {
      for (int ix = 0; ix < w; ix++) {
	int ofs = iy*w + ix;
	if (bReverseBackground ?
	    (data[ofs] > g_zbufferMinUI) :
	    (data[ofs] < g_zbufferMaxUI)) {
	  pts.push_back (unproject (x + ix, y + iy, data[ofs]));
	} else if (bIncludeBlanks) {
	  pts.push_back(Pnt3());
	}
      }
    }

    delete[] data;
    return true;

  } else if (sel.type == Selection::shape) {
    int width = theWidth;
    int height = theHeight;
    unsigned char* shapePix = filledPolyPixels (width, height, sel.pts);
    if (!shapePix)
      return false;
    unsigned int* data = ReadZBufferRect (0, 0, width, height);
    if (!data) {
      delete[] shapePix;
      return false;
    }

    // BUGBUG: does this need to obey bReverseBackground, bIncludeBlanks?
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++) {
	int ofs = y*width + x;
	if (shapePix[ofs] != 0 && data[ofs] < g_zbufferMaxUI) {
	  pts.push_back (unproject (x, y, data[ofs]));
	}
      }
    }

    delete[] shapePix;
    delete[] data;
    return true;
  }

  return false;
}


// from fitplane.cc
void fitPnt3Plane (const vector<Pnt3>& pts, Pnt3& n, float& d, Pnt3& outCentroid);

int
PlvAlignToMeshBoxCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad args to PlvAlignToMeshBoxCmd";
    return TCL_ERROR;
  }

  bool bDumpStats = false;
  bool bAlign = false;
  bool bCreatePlane = false;

  bool bMoveMesh = false;
  bool bDataSourceMesh = false;

  for (int i = 2; i < argc; i++) {
    if (!strcmp (argv[i], "align"))
      bAlign = true;
    else if (!strcmp (argv[i], "dumpstats"))
      bDumpStats = true;
    else if (!strcmp (argv[i], "createplane"))
      bCreatePlane = true;
    else if (!strcmp (argv[i], "movemesh"))
      bMoveMesh = true;
    else if (!strcmp (argv[i], "movecamera"))
      bMoveMesh = false;
    else if (!strcmp (argv[i], "data_mesh"))
      bDataSourceMesh = true;
    else if (!strcmp (argv[i], "data_zbuffer"))
      bDataSourceMesh = false;
    else if (!strcmp (argv[i], ""))
	; // ignore possible blank params
    else {
      char str[200];
      sprintf (str, "PlvAlignToMeshBoxCmd: unrecognized arg '%s'", argv[i]);
      Tcl_SetResult (interp, str, TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  if (!(bAlign || bDumpStats || bCreatePlane)) {
    interp->result = "PlvAlignToMeshBoxCmd: nothing to do";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (!meshDisp) {
    interp->result = "PlvAlignToMeshBoxCmd: missing mesh";
    return TCL_ERROR;
  }
  RigidScan* meshFrom = meshDisp->getMeshData();

  vector<Pnt3> pts;
  if (bDataSourceMesh) {
    VertexFilter* filter = filterFromSelection (meshFrom, theSel);
    if (!filter) {
      interp->result = "You must first select an area.";
      return TCL_ERROR;
    }

    if (!meshFrom->filter_vertices (*filter, pts)) {
      interp->result = "This scan does not implement filter_vertices.";
      return TCL_ERROR;
    }

    delete filter;

  } else {
    if (!ReadZBufferArea (pts, theSel)) {
      interp->result = "Z-Buffer read failed";
      return TCL_ERROR;
    }

    Xform<float> unxform = meshFrom->getXform();
    unxform.fast_invert();
    for_each (pts.begin(), pts.end(), unxform);
  }

  if (pts.size() == 0) {
    char* reason;
    if (bDataSourceMesh)
      reason = "there are no mesh vertices there";
    else
      reason = "the z-buffer read failed";

    char str[200];
    sprintf (str, "No data was found inside the clipping rect.  Perhaps %s?",
	     reason);
    Tcl_SetResult (interp, str, TCL_VOLATILE);
    return TCL_ERROR;
  }

  Pnt3 norm;
  float dist;
  Pnt3 centroid;
  fitPnt3Plane (pts, norm, dist, centroid);

  // there are 3 relevant coordinate systems: the mesh coordinate system,
  // which after applying mesh xform yields camera coordinate system,
  // which after applying camera xform yields eye coordinate system.
  // We just fit a plane in the mesh coordinate system.

  if (bDumpStats) {
    cout << "Fit plane to " << pts.size() << " vertices" << endl;
    SHOW (norm);
    SHOW (dist);
  }

  Pnt3 cameraNorm = norm;
  Xform<float> xf = meshFrom->getXform();
  xf.removeTranslation();
  xf (cameraNorm);
  if (bDumpStats)
    SHOW (cameraNorm);

  Pnt3 eyeNorm = cameraNorm;
  float q[4], t[3];
  tbView->getXform (q, t);

  xf.fromQuaternion (q);
  xf (eyeNorm);
  if (bDumpStats)
    SHOW (eyeNorm);

  float err = 0;
// STL Update
  for (vector<Pnt3>::iterator pt = pts.begin(); pt < pts.end(); pt++) {
    err += fabs (dot (*pt, norm) - dist);
  }
  float avgErr = err / pts.size();
  if (bDumpStats)
    SHOW (avgErr);

  if (bAlign) {
    // need to rotate such that eyeNorm points (0,0,1): at viewer
    Pnt3 target (0, 0, 1);

    // fitPnt3Plane seems to return +/- normals randomly.
    // Flip the normal if necessary, to make it point up the visible z axis.
    // This will make our life easier later on....
    if (eyeNorm[2] < 0) {
      //alex commented this back in..the cout line
      cout << "flipping plane" << endl;
      target *= -1;
    }

    float cosTh = dot (eyeNorm, target);
    if (fabs (cosTh - 1.0) > 1.e-6) {
      Pnt3 axis = cross (eyeNorm, target);

      Pnt3 center;
      ScreenPnt sc (0, 0);
      int nPts = theSel.pts.size();
      for (int i = 0; i < nPts; i++) {
	sc.x += theSel[i].x;
	sc.y += theSel[i].y;
      }
      sc.x /= nPts; sc.y /= nPts;
      if (!findZBufferNeighbor (sc.x, sc.y, center, 5000)) {
	center = pts[nPts / 2];
	cerr << "Warning: center is guess" << endl;
      }

      if (bMoveMesh) {
	tbView->newRotationCenter (center, meshFrom);
	tbView->rotateAroundAxis (axis, acos(cosTh), meshFrom);
	theScene->computeBBox();
      } else {
	tbView->newRotationCenter (center);
	tbView->rotateAroundAxis (axis, acos(cosTh));
      }

      redraw (true);
    } else {
      cout << "Scanalyze considers it futile to try to make this any flatter"
	   << endl;
    }
  }

  if (bCreatePlane) {
    vector<Pnt3> vtx;
    const float sizeX = 500;   // TODO: would be nice to get these
    const float sizeY = 500;   // sizes from projection of clipping rect
    Pnt3 center = centroid;
    Pnt3 axisFake = norm;
    if (axisFake[0] != 0)
      axisFake[0] *= -1;
    else if (axisFake[1] != 0)
      axisFake[1] *= -1;
    else
      axisFake[2] *= -1;
    Pnt3 axis1 = cross (norm, axisFake);
    Pnt3 axis2 = cross (norm, axis1);
    vtx.push_back (center - sizeX * axis1);
    vtx.push_back (center - sizeY * axis2);
    vtx.push_back (center + sizeX * axis1);
    vtx.push_back (center + sizeY * axis2);

    vector<int> tris;
    tris.push_back (1);
    tris.push_back (0);
    tris.push_back (2);
    tris.push_back (2);
    tris.push_back (0);
    tris.push_back (3);
    RigidScan* scan = CreateScanFromGeometry (vtx, tris, "fit_plane");
    scan->setXform (meshFrom->getXform());
    DisplayableMesh* dm = theScene->addMeshSet (scan, false);
    Tcl_VarEval (interp, "addMeshToWindow ", dm->getName(), NULL);
  }

  return TCL_OK;
}


static void readLinePixels (int x, int y, int w, int h,
			    GLenum component, GLenum type,
			    char* data, int dataSize)
{
  // wrapper for glReadPixels, reads from (x,y) to (x+w, y+h) along
  // straight line
  if (abs(w) > abs(h)) { // step horizontal
    int dx = 1;
    if (w < 0) { dx *= -1; w *= -1; }

    float yy = y;
    float dy = (float)h / w;

    for (int i = 0; i < w; i++) {
      glReadPixels (x, (int)yy, 1, 1, component, type,
		    (void*)data);
      x += dx;
      yy += dy;
      data += dataSize;
    }
  } else { // step vertical
    int dy = 1;
    if (h < 0) { dy *= -1; h *= -1; }

    float xx = x;
    float dx = (float)w / h;

    for (int i = 0; i < h; i++) {
      glReadPixels ((int)xx, y, 1, 1, component, type,
		    (void*)data);
      y += dy;
      xx += dx;
      data += dataSize;
    }
  }
}


bool findZBufferNeighbor (int x, int y, Pnt3& neighbor, int kMaxTries)
{
  return findZBufferNeighborExt (x, y, neighbor,
				 toglCurrent, tbView, kMaxTries);
}


bool findZBufferNeighborExt (int x, int y, Pnt3& neighbor,
			     struct Togl* togl, Trackball* tb,
			     int kMaxTries, Pnt3* rawPt, bool bRedraw,
			     bool bReadExistingFromFront)
{
  // find the closest point to (x,y) (screen coordinates);
  // unproject it and return in world coordinates
  // assumes GL screen coordinates (0,0 is lower left)
  Togl_MakeCurrent (togl);
  int oldReadBuffer;
  glGetIntegerv (GL_READ_BUFFER, &oldReadBuffer);

  if (bRedraw) {
    PushRenderParams();

    if (togl == toglCurrent)
      drawInToglBuffer (togl, GL_BACK);
    else {
      //hack warning -- assumes all togls except the main one have an
      //AlignmentToglInfo; need to change this
      DrawAlignmentMeshToBack (togl);
    }

    PopRenderParams();
    glReadBuffer (GL_BACK);
  } else {
    if (bReadExistingFromFront)
      glReadBuffer (GL_FRONT);
    else
      glReadBuffer (GL_BACK);
  }

  int inX = x, inY = y;
  float z = 1.0;
  int nTries = 0;
  int dir = 0;
  int leftThisDir = 1;
  float leftNextDir = 1.0;
  int W = Togl_Width (togl);
  int H = Togl_Height (togl);

  while (nTries++ < kMaxTries) {
    // avoid testing offscreen pixels
    if (x >= 0 && y >= 0 && x < W && y < H) {
      glReadPixels (x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
      if (!ISBACKGROUND (z))
	break;
    }

    // need to go 1R, 1U, 2L, 2D, 3R, 3U, 4L, 4D, etc.
    switch (dir) {
    case 0: x++; break;
    case 1: y++; break;
    case 2: x--; break;
    case 3: y--; break;
    }
    if (--leftThisDir == 0) {
      dir = (dir+1) % 4;
      leftThisDir = (int)(leftNextDir += .5);
    }
  }

  glReadBuffer ((GLenum) oldReadBuffer);

  if (ISBACKGROUND (z)) {
    cerr << "findZBufferNeighbor: nothing found among nearest " <<
      kMaxTries << " pixels to " << inX << ", " << inY << endl;
    return false;
  }

  Unprojector unproject (tb);
  neighbor = unproject (x, y, z);

  if (rawPt != NULL)
    rawPt->set (x, y, z);

  return true;
}

int
wsh_WarpMesh(ClientData clientData, Tcl_Interp *interp,
	     int argc, char *argv[])
{
  Mesh *theMesh = new Mesh();

  if (argc < 2) {
    interp->result = "Bad file name";
    return TCL_ERROR;
  }

  if ( theMesh->readPlyFile(argv[1]) == 0 ) {
    return TCL_ERROR;
  }

  theMesh->Warp();

  char buff[1000];

  sprintf(buff, "%s_warp", argv[1]);

  theMesh->writePlyFile(buff, 1, 0);

  return TCL_OK;
}



void
GetPtMeshMap (int w, int h, vector<DisplayableMesh*>& ptMeshMap)
{
  ptMeshMap.clear();
  ptMeshMap.insert (ptMeshMap.begin(), w*h, NULL);
  if (!theScene->meshSets.size())
    return;

  Togl_MakeCurrent (toglCurrent);
  int scale = 16;   // unimportant as long as small -- cuts into range
  drawSceneIndexColored (scale);

  vector<uchar> rgb;
  rgb.reserve (w * h * 4);

  glReadBuffer (GL_BACK);
  glPixelStorei (GL_PACK_ALIGNMENT, 4);
// STL Update
  glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, &*rgb.begin());

  int nErrors = 0;
  for (int y = 0; y < h; y++) {
    int yofs = y * w;
    for (int x = 0; x < w; x++) {
      int ofs = yofs + x;
      int ofs4 = 4 * ofs;

      uchar r = rgb[ofs4];
      uchar g = rgb[ofs4+1];
      uchar b = rgb[ofs4+2];

      int i = b * 1024 + g * 32 + r;
      i /= 8 * scale;

      i--; // since it's 0-based encoded in frame buffer, but really 0 is
      // background, and 1..n are meshes 0..n-1
      if (i < 0) {
	// background, no mesh
	ptMeshMap[ofs] = NULL;
      } else if (i >= 0 && i < theScene->meshSets.size()) {
	//printf ("Found: %d,%d has mesh #%d\n", x, y, i);
	ptMeshMap[ofs] = theScene->meshSets[i];
      } else {
	//printf ("Error: %d,%d has mesh #%d (rgb=%02x,%02x,%02x)\n",
	//	x, y, i, (int)r, (int)g, (int)b);
	++nErrors;
	ptMeshMap[ofs] = NULL;
      }
    }
  }

  if (nErrors)
    cerr << "Warning: " << nErrors << " pixels could not be read... "
	 << "perhaps the rendering window is obscured?" << endl;
}


// Code stolen from GetPtMeshMap
// In this case we just want a list of the visible meshes in the
// current view. We return a vector of meshes. Set to NULL if the
// Mesh is invisible. Set to a pointer to the mesh, if it is visible.
void
GetPtMeshVector (int xstart, int ystart, int w, int h,
		 int winwidth, int winheight,
		 vector<DisplayableMesh*>& ptMeshMap)
{
  if (xstart < 0) xstart = 0;
  if (ystart < 0) ystart = 0;
  if (xstart+w > winwidth) w = winwidth-xstart;
  if (ystart+h > winheight) h = winheight-ystart;

  // printf ("%d %d %d %d %d %d\n",xstart,ystart,w,h,winwidth,winheight);

  ptMeshMap.clear();
  // Set every mesh to NULL initially
  ptMeshMap.insert (ptMeshMap.begin(),theScene->meshSets.size(), NULL);
  if (!theScene->meshSets.size())
    return;

  Togl_MakeCurrent (toglCurrent);
  int scale = 16;   // unimportant as long as small -- cuts into range
  drawSceneIndexColored (scale);

  vector<uchar> rgb;
  rgb.reserve (winwidth * winheight * 4);

  glReadBuffer (GL_BACK);
  glPixelStorei (GL_PACK_ALIGNMENT, 4);
// STL Update
  glReadPixels (0, 0, winwidth, winheight, GL_RGBA, GL_UNSIGNED_BYTE, &*rgb.begin());

  // cerr << "Check In:" << xstart << " " << ystart << " " << w << " " << h << "\n";
  int nErrors = 0;
  for (int y = ystart; y < h+ystart; y++) {
    int yofs = y * winwidth;
    for (int x = xstart; x < w+xstart; x++) {
      int ofs = yofs + x;
      int ofs4 = 4 * ofs;

      char r = rgb[ofs4];
      char g = rgb[ofs4+1];
      char b = rgb[ofs4+2];

      int i = b * 1024 + g * 32 + r;
      i /= 8 * scale;

      i--; // since it's 0-based encoded in frame buffer, but really 0 is
      // background, and 1..n are meshes 0..n-1
      if (i < 0) {
	// background, no mesh

      } else if (i >= 0 && i < theScene->meshSets.size()) {

	ptMeshMap[i] = theScene->meshSets[i];
      } else {
	//printf ("Error: %d,%d has mesh #%d (rgb=%02x,%02x,%02x)\n",
	//	x, y, i, (int)r, (int)g, (int)b);
	++nErrors;
	ptMeshMap[ofs] = NULL;
      }
    }
  }

  if (nErrors)
    cerr << "Warning: " << nErrors << " pixels could not be read... "
	 << "perhaps the rendering window is obscured?" << endl;
}

int wsh_AlignPointsToPlane(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  if ( argc < 8 ) {
    interp->result = "wsh_AlignPointsToPlane: insufficient args";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (!meshDisp) {
    interp->result = "wsh_AlignPointsToPlane: missing mesh";
    return TCL_ERROR;
  }
  RigidScan* meshFrom = meshDisp->getMeshData();

  int x1 = atoi(argv[2]);
  int y1 = atoi(argv[3]);
  int x2 = atoi(argv[4]);
  int y2 = atoi(argv[5]);
  int x3 = atoi(argv[6]);
  int y3 = atoi(argv[7]);

  Pnt3 p1, p2, p3;

  printf("%i %i, %i %i, %i %i\n", x1, y1, x2, y2, x3, y3);

  findZBufferNeighbor(x1, y1, p1, 400);
  findZBufferNeighbor(x2, y2, p2, 400);
  findZBufferNeighbor(x3, y3, p3, 400);

  /*
  printf("%f %f %f, %f %f %f, %f %f %f\n", p1[0], p1[1], p1[2],
	 p2[0], p2[1], p2[2], p3[0], p3[1], p3[2]);
  */

  Pnt3 v1 = p1 - p3;
  Pnt3 v2 = p2 - p3;

  Pnt3 planeNorm = cross(v1, v2);

  planeNorm.normalize();

  Pnt3 target(0, 0, 1);

  if (planeNorm[2] < 0) {
    target *= -1;
  }

  float cosTh = dot(planeNorm, target);
  //  cout << cosTh << endl;
  Pnt3 axis = cross (planeNorm, target);
  if (fabs (cosTh - 1.0) > 1.e-6) {
    //    axis.normalize();
    printf("z: %f %f %f, %f\n", axis[0], axis[1], axis[2], cosTh);

    tbView->rotateAroundAxis (axis, acos(cosTh), meshFrom);
    theScene->computeBBox();
    redraw (true);
  } else {
    cout << "Scanalyze considers it futile to try to make this any flatter"
	 << endl;
  }

  Xform<double> theXform;
  theXform.rot(acos(cosTh), axis[0], axis[1], axis[2]);

  Pnt3 tp1, tp2;
  theXform.apply(p1, tp1);
  theXform.apply(p2, tp2);

  Pnt3 planex = tp2 - tp1;
  planex.normalize();
  Pnt3 htarget(1, 0, 0);

  float hcosTh = dot(planex, htarget);
  if (fabs (hcosTh - 1.0) > 1.e-6) {
    Pnt3 haxis = cross (planex, htarget);

    printf("x: %f %f %f, %f\n", haxis[0], haxis[1], haxis[2], hcosTh);

    tbView->rotateAroundAxis (haxis, acos(hcosTh), meshFrom);
    theScene->computeBBox();
    redraw (true);
  } else {
    cout << "Scanalyze considers it futile to try to make this any flatter"
	 << endl;
  }

  return TCL_OK;
} //wsh_AlignPointsToPlane


Auto_a_Line *lines_g = NULL;

int
PlvDrawAnalyzeLines(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  if ( argc < 8 ) {
    interp->result = "wsh_DrawAnalyzeLines: incorrect number of args";
    return TCL_ERROR;
  }

  struct Togl *togl = toglHash.FindTogl(argv[1]);
  char axis = argv[2][0];
  int x0 = atoi(argv[3]);
  int y0 = atoi(argv[4]);
  int len = atoi(argv[5]);
  int space = atoi(argv[6]);
  int num = atoi(argv[7]);

  // output lines to rgb file
  lines_g = new Auto_a_Line(togl, axis, x0, y0, len, space, num);
  redraw(true);

  return TCL_OK;
}

// PlvClearAnalyzeLines
// a hack: the screen dump only seems to work correctly if called from tcl only
// but since the Auto_a_Line is created in C, I need to make a call to this
// function to properly clean up.
int
PlvClearAnalyzeLines(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  if ( lines_g != NULL ) {
    delete lines_g;
    lines_g = NULL;
    redraw(true);
    return TCL_OK;
  }

  interp->result = "Can't clear analyze lines because they are NULL";
  return TCL_ERROR;
}

// -------------------------------------
// Auto_a_Line
// the 2d lines that represent the cross sections made in
// auto-analysis
// -------------------------------------
extern DrawObjects draw_other_things;

Auto_a_Line::Auto_a_Line(Togl *_togl, char _axis, int _x0, int _y0,
			 int _len, int _space, int _num)
{
  togl = _togl;
  axis = _axis;
  x0 = _x0;
  y0 = _y0;
  len = _len;
  space = _space;
  num = _num;


  draw_other_things.add(this);
}

Auto_a_Line::~Auto_a_Line()
{
  draw_other_things.remove(this);
}

void
Auto_a_Line::drawthis()
{
  int width, height;

  width = Togl_Width (togl);
  height = Togl_Height (togl);

  glPushMatrix();

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //  gluOrtho2D(-0.5, width+0.5, -0.5, height+0.5);
  gluOrtho2D(0, width, 0, height);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glPushAttrib (GL_LIGHTING_BIT);
  glDisable (GL_LIGHTING);

  glColor3f(0, 1.0, 0);
  for ( int i = 0; i < num; i++ ) {
    glBegin(GL_LINES);
    if ( axis == 'y' ) { // 1 == y axis
      glVertex2i( x0, height - y0 - i*space );
      glVertex2i( x0 + len, height - y0 - i*space );
    } else { // x axis
      glVertex2i( x0 + i*space, y0);
      glVertex2i( x0 + i*space, y0 + len);
    }
    glEnd();
    printf("here\n");
  }
  glPopAttrib();
  glPopMatrix();

  glFinish();
}
