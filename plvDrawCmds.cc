#ifdef WIN32
#	include "winGLdecs.h"
#else
#	include <sys/time.h>
#endif
#include <GL/glu.h>
#include <tk.h>
#include <stdlib.h>

#include "togl.h"
#include "plvDraw.h"
#include "plvGlobals.h"
#include "plvDrawCmds.h"
#include "plvMeshCmds.h"
#include "plvScene.h"
#include "ToglHash.h"
#include "DisplayMesh.h"
#include "ToglCache.h"
#include "plvClipBoxCmds.h"
#include "TextureObj.h"
#include "plvViewerCmds.h"
#include "TclCmdUtils.h"


static const char ToglPaneName[] = ".toglFrame.toglPane";
static void resizeMainTogl (struct Togl* togl);


class DisplayListValidityCheck
{
public:
  DisplayListValidityCheck (RenderParams* rp)
    {
      assert (rp != NULL);
      realData = rp;
      memcpy (&oldData, rp, sizeof(oldData));
    };

  ~DisplayListValidityCheck()
    {
      if (memcmp (&oldData, realData, sizeof(oldData))) {
	//render params changed -- kill display lists
	theScene->invalidateDisplayCaches();
	if (toglCurrent && Togl_Interp (toglCurrent)) {
	  DisplayCache::InvalidateToglCache (toglCurrent);
	  Tcl_Eval (Togl_Interp (toglCurrent), "renderParamsChanged");
	}
      }
    };

private:
  RenderParams *realData;
  RenderParams oldData;
};


static void
SetResultFromColorU (Tcl_Interp* interp, uchar* color)
{
  char buffer[20];
  sprintf (buffer, "#%02x%02x%02x",
	   (int)color[0], (int)color[1], (int)color[2]);
  Tcl_SetResult (interp, buffer, TCL_VOLATILE);
}


static void
SetResultFromColorF (Tcl_Interp* interp, float* color)
{
  char buffer[20];
  sprintf (buffer, "#%02x%02x%02x",
	   (int)(255*color[0]), (int)(255*color[1]), (int)(255*color[2]));
  Tcl_SetResult (interp, buffer, TCL_VOLATILE);
}


int
PlvDrawStyleCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  if (g_bNoUI)
    return TCL_OK;

  DisplayListValidityCheck dlvc (theRenderParams);

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-shademodel")) {
      i++;
      if (!strcmp(argv[i], "realflat")) {
	theRenderParams->shadeModel = realPerFace;
      }
      else if (!strcmp(argv[i], "fakeflat")) {
	theRenderParams->shadeModel = fakePerFace;
      }
      else if (!strcmp(argv[i], "smooth")){
	theRenderParams->shadeModel = perVertex;
      }
      else {
	interp->result = "bad arg to plv_drawstyle -shademodel";
	return TCL_ERROR;
      }
    }
    else if (!strcmp(argv[i], "-polymode")) {
      i++;
      if (!strcmp(argv[i], "fill")) {
	theRenderParams->polyMode = GL_FILL;
	theRenderParams->hiddenLine = FALSE;
      }
      else if (!strcmp(argv[i], "line")){
	theRenderParams->polyMode = GL_LINE;
	theRenderParams->hiddenLine = FALSE;
      }
      else if (!strcmp(argv[i], "hiddenline")){
	theRenderParams->polyMode = GL_LINE;
	theRenderParams->hiddenLine = TRUE;
      }
      else if (!strcmp(argv[i], "point")){
	theRenderParams->polyMode = GL_POINT;
	theRenderParams->hiddenLine = FALSE;
      }
      else if (!strcmp(argv[i], "hiddenpoint")){
	theRenderParams->polyMode = GL_POINT;
	theRenderParams->hiddenLine = TRUE;
      }
      else {
	interp->result = "bad arg to plv_drawstyle -polymode";
	return TCL_ERROR;
      }
    }
    else if (!strcmp(argv[i], "-blend")) {
      SetBoolFromArgIndex (++i, theRenderParams->blend);
    }
    else if (!strcmp(argv[i], "-cull")) {
      SetBoolFromArgIndex (++i, theRenderParams->cull);
    }
    else if (!strcmp(argv[i], "-bbox")) {
      SetBoolFromArgIndex (++i, theRenderParams->boundSelection);
    }
    else if (!strcmp(argv[i], "-cachetogl")) {
      if (!toglCurrent)
	continue;  // too early

      bool bCache;
      SetBoolFromArgIndex (++i, bCache);
      DisplayCache::EnableToglCache (toglCurrent, bCache);
    }
    else if (!strcmp(argv[i], "-clearbuffer")) {
      SetBoolFromArgIndex (++i, theRenderParams->clearBeforeRender);
    }
    else if (!strcmp(argv[i], "-displaylist")) {
      bool bDispList;
      SetBoolFromArgIndex (++i, bDispList);
      for (int k = 0; k < theScene->meshSets.size(); k++) {
	theScene->meshSets[k]->useDisplayList (bDispList);
      }
    }
    else if (!strcmp(argv[i], "-flipnorm")) {
      SetBoolFromArgIndex (++i, theRenderParams->flipnorm);
    }
    else if (!strcmp(argv[i], "-lighting")) {
      SetBoolFromArgIndex (++i, theRenderParams->light);
    }
    else if (!strcmp(argv[i], "-tstrip")) {
      SetBoolFromArgIndex (++i, theRenderParams->useTstrips);
    }
    else if (!strcmp(argv[i], "-background")) {
      i++;
      if (i == argc) { // query
	SetResultFromColorU (interp, theRenderParams->background);
	return TCL_OK;
      } else {
	theRenderParams->background[0] = 255 * atof(argv[i]);  i++;
	theRenderParams->background[1] = 255 * atof(argv[i]);  i++;
	theRenderParams->background[2] = 255 * atof(argv[i]);
      }
    }
    else if (!strcmp(argv[i], "-pointsize")) {
      i++;
      theRenderParams->pointSize = atof(argv[i]);
    }
    else if (!strcmp(argv[i], "-linewidth")) {
      i++;
      theRenderParams->lineWidth = atof(argv[i]);
    }
    else if (!strcmp(argv[i], "-emissive")) {
      SetBoolFromArgIndex (++i, theRenderParams->useEmissive);
    }
    /* two sided lighting takes care of backface emissive
    else if (!strcmp(argv[i], "-backfaceemissive")) {
      SetBoolFromArgIndex (++i, theRenderParams->backFaceEmissive);
    } */
    else if (!strcmp(argv[i], "-twosidedlighting")) {
      SetBoolFromArgIndex (++i, theRenderParams->twoSidedLighting);
    }
    else if (!strcmp(argv[i], "-antialias")) {
      SetBoolFromArgIndex (++i, theRenderParams->antiAlias);
    }
    else if (!strcmp(argv[i], "-depthoffield")) {
      if (i + 4 > argc) {
	interp->result = "bad # args to plv_drawstyle -depthoffield";
	return TCL_ERROR;
      }

      // arg format: [X jitter pixels] [Y jitter pixels] [center]
      theRenderParams->dofJitterX = atof (argv[i + 1]);
      theRenderParams->dofJitterY = atof (argv[i + 2]);
      theRenderParams->dofCenter = atof (argv[i + 3]);

      i += 4;
    }
    else if (!strcmp(argv[i], "-shadows")) {
      SetBoolFromArgIndex (++i, theRenderParams->shadows);
    }
    else if (!strcmp(argv[i], "-softshadowlength")) {
      i++;
      theRenderParams->shadowLength = atof (argv[i]);
    }
    else if (!strcmp(argv[i], "-fromlightpov")) {
      SetBoolFromArgIndex (++i, theRenderParams->fromLightPOV);
    }
    else if (!strcmp(argv[i], "-aasamps")) {
      i++;
      int aasamps = atoi(argv[i]); i++;
      switch (aasamps) {

      case 2:
	theRenderParams->numAntiAliasSamps = 2;
	theRenderParams->jitterArray = (jitter_point *)j2;
	break;

      case 3:
	theRenderParams->numAntiAliasSamps = 3;
	theRenderParams->jitterArray = (jitter_point *)j3;
	break;

      case 4:
	theRenderParams->numAntiAliasSamps = 4;
	theRenderParams->jitterArray = (jitter_point *)j4;
	break;

      case 8:
	theRenderParams->numAntiAliasSamps = 8;
	theRenderParams->jitterArray = (jitter_point *)j8;
	break;

      case 15:
	theRenderParams->numAntiAliasSamps = 15;
	theRenderParams->jitterArray = (jitter_point *)j15;
	break;

      case 24:
	theRenderParams->numAntiAliasSamps = 24;
	theRenderParams->jitterArray = (jitter_point *)j24;
	break;

      case 66:
	theRenderParams->numAntiAliasSamps = 66;
	theRenderParams->jitterArray = (jitter_point *)j66;
	break;

      default:
	interp->result = "bad args to plv_drawstyle -aasamps"
	  "  -  should be [2, 3, 4, 8, 15, 24, 66]";
	return TCL_ERROR;
      }
    }
    else {
      interp->result = "bad args to plv_drawstyle";
      return TCL_ERROR;
    }
  }

  return TCL_OK;
}


int
PlvFillPhotoCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  uchar *cbuf;
  GLint lastBuffer;

  if (argc != 3) {
    interp->result = "Usage: plv_fillphoto <togl-widget> <photo-widget>";
    return TCL_ERROR;
  }

#if TK_MAJOR_VERSION >= 8
  Tk_PhotoHandle handle = Tk_FindPhoto(interp, argv[2]);
#else
  Tk_PhotoHandle handle = Tk_FindPhoto(argv[2]);
#endif
  if (handle == NULL) {
    interp->result = "Could not find photo widget.";
    return TCL_ERROR;
  }

  prepareDrawInWin(argv[1]);

  cbuf = (uchar *)malloc(theWidth*theHeight*4);

  glGetIntegerv(GL_DRAW_BUFFER, &lastBuffer);
  glDrawBuffer(GL_FRONT);
  glReadPixels(0, 0, theWidth, theHeight, GL_RGBA, GL_UNSIGNED_BYTE, cbuf);
  glDrawBuffer(GLenum(lastBuffer));

  Tk_PhotoSetSize_Panic(handle, theWidth, theHeight);

  Tk_PhotoImageBlock block;

  block.pixelPtr = cbuf;
  block.pitch = theWidth*4;

  block.pixelSize = 4;
  block.offset[0] = 0;
  block.offset[1] = 1;
  block.offset[2] = 2;
  block.width = theWidth;
  block.height = theHeight;

  Tk_PhotoPutBlock_NoComposite(handle, &block, 0, 0, theWidth, theHeight);

  free(cbuf);

  return TCL_OK;
}


int
PlvDrawCmd(ClientData clientData, Tcl_Interp *interp,
	   int argc, char *argv[])
{
  prepareDrawInWin(argv[1]);
  drawInTogl (toglCurrent);

  return TCL_OK;
}


int
PlvClearWinCmd(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  prepareDrawInWin(argv[1]);

  glClear(GL_COLOR_BUFFER_BIT);
  Togl_SwapBuffers (toglCurrent);

  return TCL_OK;
}


int
PlvMaterialCmd(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  if (g_bNoUI)
    return TCL_OK;

  DisplayListValidityCheck dlvc (theRenderParams);

  int i;
  if (argc == 1) {
    char* colorModeName = NULL;
    switch (theRenderParams->colorMode) {
    case grayColor:       colorModeName = "gray";       break;
    case falseColor:      colorModeName = "false";      break;
    case confidenceColor: colorModeName = "confidence"; break;
    case boundaryColor:   colorModeName = "boundary";   break;
    case intensityColor:  colorModeName = "intensity";  break;
    case trueColor:       colorModeName = "true";       break;
    case textureColor:    colorModeName = "texture";    break;
    case registrationColor: colorModeName = "registration"; break;
    }

    printf("Command: %s [-option value]\n", argv[0]);
    printf("  -diffuse <float> <float> <float> (%f %f %f)\n",
	   theRenderParams->diffuse[0] / 255.,
	   theRenderParams->diffuse[1] / 255.,
	   theRenderParams->diffuse[2] / 255.);
    printf("  -specular <float> <float> <float> (%f %f %f)\n",
	   theRenderParams->specular[0] / 255.,
	   theRenderParams->specular[1] / 255.,
	   theRenderParams->specular[2] / 255.);
    printf("  -shininess <float> (%f)\n", theRenderParams->shininess);
    printf("  -confscale <float> (%f)\n", theRenderParams->confScale);
    printf("  -colormode [gray|false|intensity|true|confidence|"
	   "boundary|registration|texture (%s)\n",
	   colorModeName);
  }
  else {
    for (i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "-shininess")) {
	i++;
	theRenderParams->shininess = atof(argv[i]);
      }
      else if (!strcmp(argv[i], "-specular")) {
	i++;
	if (i == argc) { // query
	  SetResultFromColorU (interp, theRenderParams->specular);
	  return TCL_OK;
	}

	theRenderParams->specular[0] = 255 * atof(argv[i]);  i++;
	theRenderParams->specular[1] = 255 * atof(argv[i]);  i++;
	theRenderParams->specular[2] = 255 * atof(argv[i]);
      }
      else if (!strcmp(argv[i], "-diffuse")) {
	i++;
	if (i == argc) { // query
	  SetResultFromColorU (interp, theRenderParams->diffuse);
	  return TCL_OK;
	}

	theRenderParams->diffuse[0] = 255 * atof(argv[i]);  i++;
	theRenderParams->diffuse[1] = 255 * atof(argv[i]);  i++;
	theRenderParams->diffuse[2] = 255 * atof(argv[i]);
      }
      // backDiffuse added by kberg 6/5/01
      else if (!strcmp(argv[i], "-backDiffuse")) {
	i++;
	if (i == argc) { // query
	  SetResultFromColorU (interp, theRenderParams->backDiffuse);
	  return TCL_OK;
	}

	theRenderParams->backDiffuse[0] = 255 * atof(argv[i]);  i++;
	theRenderParams->backDiffuse[1] = 255 * atof(argv[i]);  i++;
	theRenderParams->backDiffuse[2] = 255 * atof(argv[i]);
      }
      else if (!strcmp(argv[i], "-backSpecular")) {
	i++;
	if (i == argc) { // query
	  SetResultFromColorU (interp, theRenderParams->backSpecular);
	  return TCL_OK;
	}

	theRenderParams->backSpecular[0] = 255 * atof(argv[i]);  i++;
	theRenderParams->backSpecular[1] = 255 * atof(argv[i]);  i++;
	theRenderParams->backSpecular[2] = 255 * atof(argv[i]);
      }
      else if (!strcmp(argv[i], "-lightambient")) {
	i++;
	if (i == argc) { // query
	  SetResultFromColorF (interp, theRenderParams->lightAmbient);
	  return TCL_OK;
	}

	theRenderParams->lightAmbient[0] = atof(argv[i]);  i++;
	theRenderParams->lightAmbient[1] = atof(argv[i]);  i++;
	theRenderParams->lightAmbient[2] = atof(argv[i]);
      }
      else if (!strcmp(argv[i], "-lightspecular")) {
	i++;
	if (i == argc) { // query
	  SetResultFromColorF (interp, theRenderParams->lightSpecular);
	  return TCL_OK;
	}

	theRenderParams->lightSpecular[0] = atof(argv[i]);  i++;
	theRenderParams->lightSpecular[1] = atof(argv[i]);  i++;
	theRenderParams->lightSpecular[2] = atof(argv[i]);
      }
      else if (!strcmp(argv[i], "-lightdiffuse")) {
	i++;
	if (i == argc) { // query
	  SetResultFromColorF (interp, theRenderParams->lightDiffuse);
	  return TCL_OK;
	}

	theRenderParams->lightDiffuse[0] = atof(argv[i]);  i++;
	theRenderParams->lightDiffuse[1] = atof(argv[i]);  i++;
	theRenderParams->lightDiffuse[2] = atof(argv[i]);
      }
      else if (!strcmp(argv[i], "-backfacemode")) {
	i++;
	if (!strcmp(argv[i], "lit")) {
	  theRenderParams->backfaceMode = lit;
	}
	else if (!strcmp(argv[i], "emissive")) {
	  theRenderParams->backfaceMode = emissive;
	}
      }
      else if (!strcmp(argv[i], "-colormode")) {
	i++;
	if (     !strcmp (argv[i], "gray")) {
	  theRenderParams->colorMode = grayColor;
	}
	else if (!strcmp (argv[i], "false")) {
	  theRenderParams->colorMode = falseColor;
	}
	else if (!strcmp (argv[i], "intensity")) {
	  theRenderParams->colorMode = intensityColor;
	}
	else if (!strcmp (argv[i], "true")) {
	  theRenderParams->colorMode = trueColor;
	}
	else if (!strcmp (argv[i], "confidence")) {
	  theRenderParams->colorMode = confidenceColor;
	}
	else if (!strcmp (argv[i], "boundary")) {
	  theRenderParams->colorMode = boundaryColor;
	}
	else if (!strcmp (argv[i], "registration")) {
	  theRenderParams->colorMode = registrationColor;
	}
	else if (!strcmp (argv[i], "texture")) {
	  theRenderParams->colorMode = textureColor;
	}
	else {
	  interp->result = "Bad argument to plv_material -color";
	  return TCL_ERROR;
	}
      }
      else if (!strcmp(argv[i], "-confscale")) {
	i++;
	theRenderParams->confScale = atof(argv[i]);
      }
      else {
	interp->result = "bad arg to plv_material";
	return TCL_ERROR;
      }
    }
  }

  return TCL_OK;
}

int
PlvLoadProjectiveTexture(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  if (g_bNoUI)
    return TCL_OK;

  if (argc < 2) {
    printf("Usage: %s filename.rgb [green] [nomipmap]\n", argv[0]);
    printf("       green : Don't actually load image, just use a green projection\n");
    return TCL_OK;
  }

  int mesh;
  // This applies the perspective texture to all visible meshes, so
  // check whether anything's visible...
  int nMeshes = theScene->meshSets.size();

  if (!nMeshes)
    return TCL_OK;
  for (mesh = 0; mesh < nMeshes; mesh++) {
    if (theScene->meshSets[mesh]->getVisible())
	break;
  }
  if (mesh == nMeshes)
    return TCL_OK;


  // Try to load the texture
  bool actuallyLoad=true, use_mipmaps=true;
  for (int arg = 2; arg < argc; arg++) {
  	if (argv[arg][0] == 'g')
		actuallyLoad = false;
	else if (argv[arg][0] == 'n')
		use_mipmaps = false;
  }

  Ref<ProjectiveTexture> thetexture =
  	ProjectiveTexture::loadImage(argv[1], NULL, NULL, NULL, NULL,
				     actuallyLoad, use_mipmaps);

  if (!thetexture)
    return TCL_OK;

  for (mesh = 0; mesh < nMeshes; mesh++)
    if (theScene->meshSets[mesh]->getVisible()) {
	DisplayableMesh *themesh = theScene->meshSets[mesh];
	const float *meshxform = (float *)(themesh->getMeshData()->getXform());
	themesh->setTexture(MeshAffixedProjectiveTexture::AffixToMesh(thetexture, meshxform));
    }

  return TCL_OK;
}

int
prepareDrawInWin(char *name)
{
  toglCurrent = toglHash.FindTogl (name);

  assert (toglCurrent != NULL);
  Togl_MakeCurrent (toglCurrent);

  return 1;
}


void
catchToglCreate (struct Togl *togl)
{
  toglHash.AddToHash (Togl_Ident (togl), togl);
  if (!strcmp (Togl_Ident(togl), ToglPaneName)) {
    toglCurrent = togl;
    Togl_SetReshapeFunc (togl, resizeMainTogl);
    new DisplayCache (togl);
    Tcl_Eval (Togl_Interp (togl), "prefs_setRendererBasedDefaults");
  }
}


void
redraw (bool bForceRender)
{
  //just so other modules don't have to know about toglCurrent
  if (bForceRender)
    DisplayCache::InvalidateToglCache (toglCurrent);

  Togl_PostRedisplay (toglCurrent);
}


void
resizeMainTogl (struct Togl* togl)
{
  //printf ("from %dx%d to %dx%d\n", theWidth, theHeight,
  //  Togl_Width (togl), Togl_Height (togl));

  resizeSelectionToWindow (togl);
}


void
drawInTogl(struct Togl *togl)
{
  // don't redraw if redraw is disabled
  if (atoi (Tcl_GetVar (g_tclInterp, "noRedraw", TCL_GLOBAL_ONLY)) <= 0) {

    // ok, we have permission to draw
    drawInToglBuffer (togl, GL_FRONT, true);
  }
}

void
drawInToglBuffer (struct Togl *togl, int buffer, bool bCacheable)
{
  //puts ("drawInToglBuffer");

  // Take ms time at start of render
  unsigned long startTime = Get_Milliseconds();

  // update global size info
  theWidth = Togl_Width (togl);
  theHeight = Togl_Height (togl);

  if (togl == toglCurrent) { // only if in main window, update trackball too
    tbView->setSize (theWidth, theHeight);
  }

  //Tcl_Eval (g_tclInterp, "redrawStatus start");
#if TOGLCACHE_DEBUG
  cout << "rendering manually" << endl;
#endif
  glDrawBuffer (GL_BACK);
  drawScene();
  if (bCacheable) {
    if (isManipulatingRender()) {
#if TOGLCACHE_DEBUG
      cout << "uncacheable data: clearing cache" << endl;
#endif
      DisplayCache::InvalidateToglCache (togl);
    } else {
#if TOGLCACHE_DEBUG
      cout << "data is cacheable" << endl;
#endif
      //Tcl_Eval (g_tclInterp, "redrawStatus cache");
      DisplayCache::UpdateToglCache (togl);
    }
  }

  if (buffer == GL_FRONT)
    Togl_SwapBuffers (togl);
  else
    glFinish();

  // Take time at end of render and save
  unsigned long endTime = Get_Milliseconds();
  lastRenderTime(endTime-startTime);

  //puts ("Leaving drawinTOglBuffer");
  //Tcl_Eval (g_tclInterp, "redrawStatus end");
}


int
PlvSetSlowPolyCountCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  if (argc != 2) {
    interp->result = "Bad args to PlvSetSlowPolyCountCmd";
    return TCL_ERROR;
  }

  theScene->setSlowPolyCount (atoi (argv[1]));
  return TCL_OK;
}


int
PlvInvalidateToglCacheCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  DisplayCache::InvalidateToglCache (toglCurrent);
  return TCL_OK;
}


/* drawOverlayAndSwap
 * ------------------
 * function to draw the overlay and then swap the buffer.  Used
 * when overlay planes are not available, but are being simulated
 */
void drawOverlayAndSwap(struct Togl* togl)
{
  drawOverlay(togl);
  Togl_SwapBuffers(togl);
}

void
drawOverlay (struct Togl* togl)
{
#ifndef no_overlay_support
  Togl_UseLayer (togl, TOGL_OVERLAY);
  glClear(GL_COLOR_BUFFER_BIT);
#else
  // overlay planes not supported - use a work around

  // make the togl current (should be the main frame) since
  Togl_MakeCurrent(togl);
  //glDrawBuffer (GL_BACK);

  // save all the bits just so I don't turn off something
  // that someone else expects to be on
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glDisable (GL_DEPTH_TEST);
  glDisable (GL_LIGHTING);
  glDisable (GL_BLEND);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glViewport(0,0,theWidth, theHeight);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glLoadIdentity();
  gluOrtho2D(0, theWidth, 0, theHeight);
  glRasterPos2i(0,0);

  glDrawPixels(theRenderParams->savedImageWidth,
	       theRenderParams->savedImageHeight, GL_RGBA, GL_UNSIGNED_BYTE,
	       theRenderParams->savedImage);

  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glFinish();

#endif

  // selection tools
  drawSelection (togl);

  // mouseover hint
  drawScanHilites();

#ifndef no_overlay_support
  Togl_UseLayer (togl, TOGL_NORMAL);
#else
  glPopAttrib();
  //Togl_SwapBuffers (togl);
#endif
}


int
PlvRenderThicknessCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  Togl_MakeCurrent (toglCurrent);
  if (!drawSceneThicknessColored()) {
    return TCL_ERROR;
  }

  Togl_SwapBuffers (toglCurrent);

  return TCL_OK;
}


unsigned long Get_Milliseconds(void)
{
  unsigned long time;

#ifdef WIN32
  time = GetTickCount();
#else
  struct timeval mytime;
  static unsigned long base=0;

  gettimeofday(&mytime, NULL);

  time=(mytime.tv_sec-base)*1000 + (mytime.tv_usec/1000) ;
#endif

  return time;
}

// Read and optionally set the amount of time the last render took.
// We record this whenever a render takes place and then expose an
// interface to read this value.
long lastRenderTime(long milliseconds)
{
  static long lastDrawTime=0;

  long tmpSaveTime = lastDrawTime;

  if (milliseconds != -1)
    lastDrawTime = milliseconds;

  return tmpSaveTime;
}

int
PlvLastRenderTime(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{


  sprintf(interp->result,"%ld",lastRenderTime());

  return TCL_OK;
}
