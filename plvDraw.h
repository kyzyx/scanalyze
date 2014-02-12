#ifndef _PLV_DRAW_
#define _PLV_DRAW_

#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include "jitter.h"
#include "Pnt3.h"
#include "defines.h"

/*
 * In the case when no overlay planes are available, scanalyze provides
 * support for overlay plane features by saving the main frame window
 * to RenderParams.savedImage after every redraw.  The overlay plane
 * drawing is then done on top of this by making an explicit call to
 * either drawOverlay or drawOverlayAndUpdate.  This #define is used
 * to indicate whether support is available or not.
 */
#ifndef sgi
#define no_overlay_support
#endif

typedef enum {
  grayColor, falseColor, intensityColor, trueColor,
  confidenceColor, boundaryColor,
  registrationColor, textureColor,
  noColor, // use this to set it yourself
} ColorMode;

typedef enum {
  perVertex, fakePerFace, realPerFace
} ShadeModel;

/* BackgroundMode
 * --------------
 * Specifies how to draw back polygons when two sided lighting is enabled
 */
typedef enum {
  lit, emissive
} BackfaceMode;

struct RenderParams
{
  GLenum polyMode;
  bool hiddenLine;
  ShadeModel shadeModel;
  ColorMode colorMode;
  BackfaceMode backfaceMode;

  bool light;
  bool cull;
  bool useEmissive;
  bool twoSidedLighting; // added by kberg 6/3/01
  //bool backFaceEmissive; two-sded lighting does this
  bool blend;

  float confScale;
  float pointSize;
  float lineWidth;

  GLfloat shininess;
  uchar specular[4];
  uchar diffuse[4];
  uchar backDiffuse[4]; // added by kberg 6/5/01
  uchar backSpecular[4];
  GLfloat backShininess;
  uchar background[4];
  GLfloat lightPosition[3];
  GLfloat lightAmbient[4];
  GLfloat lightDiffuse[4];
  GLfloat lightSpecular[4];

  bool shadows;
  bool antiAlias;
  int numAntiAliasSamps;
  float jitterX, jitterY;
  jitter_point *jitterArray;
  float dofJitterX, dofJitterY, dofCenter;
  float shadowLength; // for soft shadows
  bool fromLightPOV;

  bool flipnorm;
  bool useTstrips;
  bool boundSelection;
  bool clearBeforeRender;
  bool accelerateWithBbox;

  bool bRenderManipsPoints;
  bool bRenderManipsTinyPoints;
  bool bRenderManipsUnlit;
  bool bRenderManipsLores;
  bool bRenderManipsSkipDlist;
  int iFastManipsThreshold;

#ifdef no_overlay_support
  char *savedImage;
  int savedImageWidth, savedImageHeight;
#endif
};


// save/restore render params
void PushRenderParams (void);
void PopRenderParams (void);


// one-time module initialization
bool initDrawing();
bool initDrawingPostCreation();

// set up lighting, viewing xforms, etc. if you need to render yourself
bool setupSceneDrawing();

// draw all meshes in scene (using display list if possible)
bool drawScene();
bool drawScanHilites (void);

// alternative visualizations:
// draw scene in per-scan color (indexed by position in scene mesh list)
bool drawSceneIndexColored (int scale);
// draw scene with color according to the z variation in overlapping regions
bool drawSceneThicknessColored (void);

// draw the edges of a bbox
void drawBoundingBox (const class Bbox& box, bool bUseDefaultColor = true);


#endif
