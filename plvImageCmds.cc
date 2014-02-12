#include <assert.h>
#include <tk.h>

#include "plvGlobals.h"
#include "plvImageCmds.h"
#include "plvDrawCmds.h"
#include "Image.h"
#include "plvClipBoxCmds.h"
#include "togl.h"
#include "ToglHash.h"


typedef unsigned int uint;
static uint *theZBuf = NULL;
static uchar *theColorBuf = NULL;

int
PlvWriteIrisCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  struct Togl* togl = toglHash.FindTogl (argv[1]);
  if (!togl) {
    interp->result = "Missing togl in PlvWriteIrisCmd";
    return TCL_ERROR;
  }

  Togl_MakeCurrent (togl);
  int width = Togl_Width (togl);
  int height = Togl_Height (togl);

  Image img (width, height, 4);

  glReadBuffer(GL_FRONT);
  glReadPixels(0, 0, width, height, GL_RGBA,
	       GL_UNSIGNED_BYTE, img);

  if (!img.write(argv[2])) {
#ifdef linux
		// give an error message alerting that IFL is not supported under
		// Linux
		interp->result = "Image saving not supported under Linux";
#else
		interp->result = "file creation error";
#endif

    return TCL_ERROR;
  }

  return TCL_OK;

  //#ifdef SGI
  /*
  uchar *cbuf, *pcbuf;
  ushort *sbuf;
  int xx, yy;
  IMAGE *image;
  */


  /*
  image = iopen(argv[2], "w", RLE(1), 3, theWidth, theHeight, 3);

  if (!image)
    return TCL_ERROR;

  cbuf = (uchar *)malloc(theWidth*theHeight*4);
  sbuf = (ushort *)malloc(theWidth*sizeof(ushort));
  */
  /*
  pcbuf = cbuf;
  for (yy = 0; yy < theHeight; yy++, pcbuf += 4*theWidth) {
    for (xx = 0; xx < theWidth; xx++)
      sbuf[xx] = pcbuf[4*xx];
    putrow(image, sbuf, yy, 0);

    for (xx = 0; xx < theWidth; xx++)
      sbuf[xx] = pcbuf[4*xx+1];
    putrow(image, sbuf, yy, 1);

    for (xx = 0; xx < theWidth; xx++)
      sbuf[xx] = pcbuf[4*xx+2];
    putrow(image, sbuf, yy, 2);
  }

  iclose(image);

  free(cbuf);
  free(sbuf);
  */
  //#endif
}


int
PlvCacheBufferCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
  GLint lastBuffer;

  prepareDrawInWin(argv[1]);

  if (theColorBuf == NULL) {
    theColorBuf = new uchar[theWidth*theHeight*4];
  }

  if (theZBuf == NULL) {
    theZBuf = new uint[theWidth*theHeight];
  }

  glGetIntegerv(GL_DRAW_BUFFER, &lastBuffer);
  glDrawBuffer(GL_FRONT);
  glReadPixels(0, 0, theWidth, theHeight,
	       GL_RGBA, GL_UNSIGNED_BYTE, theColorBuf);
  glReadPixels(0, 0, theWidth, theHeight,
	       GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, theZBuf);
  glDrawBuffer(GLenum(lastBuffer));

  return TCL_OK;
}


int
PlvReloadBufferCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  GLint lastBuffer;

  prepareDrawInWin(argv[1]);

  glGetIntegerv(GL_DRAW_BUFFER, &lastBuffer);
  glDrawBuffer(GL_FRONT);
  glDrawPixels(theWidth, theHeight,
	       GL_RGBA, GL_UNSIGNED_BYTE, theColorBuf);
  glDrawPixels(theWidth, theHeight,
	       GL_DEPTH_COMPONENT, GL_INT, theZBuf);
  glDrawBuffer(GLenum(lastBuffer));

  return TCL_OK;
}

/*
void
loadTexture(MeshSet *meshSet, char *texFile, int flipX)
{
  Image img;
  img.read(texFile);
  uchar *tempTexture = new uchar[3*img.width()*img.height()];
  meshSet->texture = new uchar[3*512*512];
  meshSet->newTexture = new uchar[512*512];

  int yy, xx;
  if (img.width() != 512) {
    for (yy = 0; yy < img.height(); yy++) {
      for (xx = 0; xx < 512; xx++) {
	float xoff = xx/512.0*img.width();
	int xint = int(xoff);
	float dx = xoff - xint;
	uchar *tmp = &meshSet->texture[xx*3+yy*3*512];
	uchar *tmp0 = img.getValues(xint, yy);
	uchar *tmp1 = img.getValues(xint+1, yy);
	tmp[0] = uchar((1-dx)*tmp0[0] + dx*tmp1[0]);
	tmp[1] = uchar((1-dx)*tmp0[1] + dx*tmp1[1]);
	tmp[2] = uchar((1-dx)*tmp0[2] + dx*tmp1[2]);
      }
    }
  } else {
    if (!flipX) {
      for (yy = 0; yy < img.height(); yy++) {
	for (xx = 0; xx < img.width(); xx++) {
	  uchar *tmp = &meshSet->texture[xx*3+yy*3*512];
	  uchar *tmp0 = img.getValues(xx, yy);
	  tmp[0] = tmp0[0];
	  tmp[1] = tmp0[1];
	  tmp[2] = tmp0[2];
	}
      }
    }
    else {
      for (yy = 0; yy < img.height(); yy++) {
	for (xx = 0; xx < img.width(); xx++) {
	  uchar *tmp = &meshSet->texture[xx*3+yy*3*512];
	  uchar *tmp0 = img.getValues(img.width()-xx-1, yy);
	  tmp[0] = tmp0[0];
	  tmp[1] = tmp0[1];
	  tmp[2] = tmp0[2];
	}
      }
    }
  }

  delete [] tempTexture;

  meshSet->hasTexture = TRUE;
  meshSet->texXdim = 512;
  meshSet->texYdim = 512;

#if 0
#ifdef SGI

    int xx, yy;

    ImgUchar *texture = new ImgUchar;
    texture->readIris(texFile);
    uchar *tempTexture = new uchar[3*texture->xdim*texture->ydim];
    meshSet->texture = new uchar[3*512*512];
    meshSet->newTexture = new uchar[512*512];

    // Hack! Resample in x only for some troublesome Cyberware
    //  cyl scans
    if (texture->xdim != 512) {
	for (yy = 0; yy < texture->ydim; yy++) {
	    for (xx = 0; xx < 512; xx++) {
		float xoff = xx/512.0*texture->xdim;
		int xint = int(xoff);
		float dx = xoff - xint;
		meshSet->texture[0+xx*3+yy*3*512] =
		    uchar((1-dx)*texture->elem(xint, yy, 0)
			  + dx*texture->elem(xint+1, yy, 0));

		meshSet->texture[1+xx*3+yy*3*512] =
		    uchar((1-dx)*texture->elem(xint, yy, 1)
			  + dx*texture->elem(xint+1, yy, 1));

		meshSet->texture[2+xx*3+yy*3*512] =
		    uchar((1-dx)*texture->elem(xint, yy, 2)
			  + dx*texture->elem(xint+1, yy, 2));
	    }
	}
    } else {
	if (!flipX) {
	    for (yy = 0; yy < texture->ydim; yy++) {
		for (xx = 0; xx < texture->xdim; xx++) {
		    meshSet->texture[0+xx*3+yy*3*512] =
			texture->elem(xx, yy, 0);
		    meshSet->texture[1+xx*3+yy*3*512] =
			texture->elem(xx, yy, 1);
		    meshSet->texture[2+xx*3+yy*3*512] =
			texture->elem(xx, yy, 2);
		}
	    }
	}
	else {
	    for (yy = 0; yy < texture->ydim; yy++) {
		for (xx = 0; xx < texture->xdim; xx++) {
		    meshSet->texture[0+xx*3+yy*3*512] =
			texture->elem(texture->xdim-xx-1, yy, 0);
		    meshSet->texture[1+xx*3+yy*3*512] =
			texture->elem(texture->xdim-xx-1, yy, 1);
		    meshSet->texture[2+xx*3+yy*3*512] =
			texture->elem(texture->xdim-xx-1, yy, 2);
		}
	    }
	}
    }



// 	for (yy = 0; yy < texture->ydim; yy++) {
// 	    for (xx = 0; xx < texture->xdim; xx++) {
// 		tempTexture[0+xx*3+yy*3*texture->xdim] =
// 		    texture->elem(xx, yy, 0);
// 		tempTexture[1+xx*3+yy*3*texture->xdim] =
// 		    texture->elem(xx, yy, 1);
// 		tempTexture[2+xx*3+yy*3*texture->xdim] =
// 		    texture->elem(xx, yy, 2);
// 	    }
// 	}


// 	gluScaleImage(GL_RGB, texture->xdim, texture->ydim, GL_UNSIGNED_BYTE,
// 		      tempTexture, 512, 512,
// 		      GL_UNSIGNED_BYTE, meshSet->texture);

    delete [] tempTexture;

    meshSet->hasTexture = TRUE;
    meshSet->texXdim = 512;
    meshSet->texYdim = 512;
    delete texture;

#endif
#endif

}
*/

// Count the number of pixels in the image region
// who's RGB values fall in the given range
int countPixInRange (int x, int y, int w, int h,
		     int lowR, int lowG, int lowB,
		     int highR, int highG, int highB)
{
  typedef struct {unsigned char r,g,b,a;} pix;

    unsigned char *img = (unsigned char *) new char[(w+1)*(h+1)*4];

    glReadBuffer(GL_FRONT);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);

    int count=0;
    for (int i=0; i<w*h; i=i+4)
      {
	pix &p=*((pix *)img+i);

	if (p.r >= lowR && p.r <= highR &&
	    p.g >= lowG && p.g <= highG &&
	    p.b >= lowB && p.b <= highB)
	  {
	    count++;
	  }
      }

    delete [] img;
    return count;
}

int PlvCountPixelsCmd (ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  int x,w,y,h;

  if (argc==4 || argc==7)
    {
      // Use selection box

      // Find Rectangle
      if (theSel.type == Selection::rect)
	{

	   x = min (theSel[0].x, theSel[2].x);
	   w = abs (theSel[0].x - theSel[2].x);
	   y = min (theSel[0].y, theSel[2].y);
	   h = abs (theSel[0].y - theSel[2].y);
	}
      else
	{
	  interp->result="Need to have a rectangle selected";
	  return TCL_ERROR;
	}


    }
  else if (argc==11)
    {
      // Count pix in specified rectangle in specified range
      x = atoi(argv[7]);
      y = atoi(argv[8]);
      w = atoi(argv[9]) -x;
      h = atoi(argv[10]) -y;
    }
  else
    {
      // Usage
      char buf[2048]="";
      strcat (buf,"\n");
      strcat (buf,"Usage: plv_countPixels RGB_min [RGB_max] [Bounding_Rect_LTRB]\n");
      strcat (buf,"  plv_countPixels 0 0 0 - counts all black pixels in current selection\n");
      strcat (buf,"  plv_countPixels 0 0 200 255 255 255 - counts pixels with high blue values\n");
      strcat (buf,"  plv_countPixels 0 0 0 0 0 0 100 150 200 200 - counts black pixels in the\n    rect bounded by (100,150) and (200,200).\n");
      strcat (buf,"\n");
      Tcl_SetResult(interp,buf,TCL_VOLATILE);
      return TCL_ERROR;
    }

  // Count pix  with specified value
  int Rmin,Gmin,Bmin;
  int Rmax,Gmax,Bmax;

  Rmin=atoi(argv[1]);
  Gmin=atoi(argv[2]);
  Bmin=atoi(argv[3]);

  if (argc==4)
    {
      // default max to min
      Rmax=Rmin;
      Gmax=Gmin;
      Bmax=Bmin;
    }
  else
    {
      // explicitly set max
      Rmax=atoi(argv[4]);
      Gmax=atoi(argv[5]);
      Bmax=atoi(argv[6]);
    }

  int count = countPixInRange(x,y,w,h,Rmin,Gmin,Bmin,Rmax,Gmax,Bmax);

  sprintf(interp->result,"%d",count);

  return TCL_OK;
}
