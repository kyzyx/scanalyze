//############################################################
//
// TextureObj.cc
//
// Szymon Rusinkiewicz
// Fri Mar 12 03:16:27 CET 1999
//
//############################################################

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <fstream.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef linux
#       include <ifl/iflFile.h>
#endif
#ifdef WIN32
#	include "defines.h"
#	include "winGLdecs.h"
#endif
#include <GL/glu.h>
#include "TextureObj.h"


static void MakeMipmapsWithBorder(unsigned char *image_in, int width, int height)
{
	unsigned char *image1 = new unsigned char[width*height*3];
	unsigned char *image2 = new unsigned char[width*height*3];

	memcpy(image1, image_in, width*height*3);

	int logwidth = log(width) / M_LN2;
	int logheight = log(height) / M_LN2;

	int nextwidth = 1 << logwidth;
	int nextheight = 1 << logheight;

	int oldwidth = width;
	int oldheight = height;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	int level = 0;
	while (nextwidth || nextheight) {
		if (!nextwidth)
			nextwidth = 1;
		if (!nextheight)
			nextheight = 1;

		gluScaleImage(GL_RGB,
			      oldwidth, oldheight,
			      GL_UNSIGNED_BYTE,
			      image1,
			      nextwidth, nextheight,
			      GL_UNSIGNED_BYTE,
			      image2);
		memcpy(image1, image2, nextwidth*nextheight*3);

		for (int i=0; i < nextwidth; i++) {
			image2[3*i  ] = 0;
			image2[3*i+1] = 0;
			image2[3*i+2] = 255;
			image2[3*(nextwidth*nextheight-1-i)  ] = 0;
			image2[3*(nextwidth*nextheight-1-i)+1] = 0;
			image2[3*(nextwidth*nextheight-1-i)+2] = 255;
		}
		for (int j=1; j < nextheight-1; j++) {
			image2[3*(j*nextwidth)  ] = 0;
			image2[3*(j*nextwidth)+1] = 0;
			image2[3*(j*nextwidth)+2] = 255;
			image2[3*((j+1)*nextwidth-1)  ] = 0;
			image2[3*((j+1)*nextwidth-1)+1] = 0;
			image2[3*((j+1)*nextwidth-1)+2] = 255;
		}

		glTexImage2D(GL_TEXTURE_2D, level,
			     GL_RGB,
			     nextwidth, nextheight,
			     0,
			     GL_RGB, GL_UNSIGNED_BYTE,
			     image2);

		oldwidth = nextwidth;
		oldheight = nextheight;
		nextwidth /= 2;
		nextheight /= 2;
		level++;
	}

	delete [] image1;
	delete [] image2;
}

Ref<ProjectiveTexture>
ProjectiveTexture::loadImage(	const char *filename,
				const char *camfilename /* = NULL */,
				const char *camcalibxffilename /* = NULL */,
				const char *camxffilename /* = NULL */,
				const char *alignxffilename /* = NULL */,
				const bool actuallyLoadImage /* = true */,
				const bool use_mipmaps /* = true */ )
{
	char tmpfilename[255];
	struct stat statbuf;

	int width, height;
	unsigned char *image=NULL;

#ifndef linux
	if (actuallyLoadImage) {
	  // Load the image
	  iflStatus sts;
	  iflFile *imgfile = iflFile::open(filename, O_RDONLY, &sts);
	  if (sts != iflOKAY) {
	    fprintf(stderr, "Couldn't open image file %s\n", filename);
	    return NULL;
	  }

	  printf("Reading %s...", filename); fflush(stdout);
	  iflSize dims;
	  imgfile->getDimensions(dims);
	  if (dims.c != 3) {
	    fprintf(stderr, " Ack! Can't handle non-RGB images...\n");
	    fprintf(stderr, "(Error reading image file %s)\n", filename);
	    imgfile->close();
	    return NULL;
	  }
	  width = dims.x; height = dims.y;
	  image = new unsigned char[width * height * 3];

	  iflConfig cfg(iflUChar, iflInterleaved, 3, NULL, 0, iflOrientation(4));
	  sts = imgfile->getTile(0, 0, 0, width, height, 1, image, &cfg);
	  imgfile->close();
	  if (sts != iflOKAY) {
	    fprintf(stderr, " Error reading image file %s\n", filename);
	    delete [] image;
	    return NULL;
	  }
	  printf(" Done.\n");
	}
#else
	if (0) {}
#endif
	else {
	  // Make a default lime green image instead
	  puts("hello");
	  width=32;
	  height=32;
	  image = new unsigned char[width * height * 3];
	  for (int i=0; i< width*height*3; i=i+3)
	    {
	      image[i+0]=0;
	      image[i+1]=255;
	      image[i+2]=0;
	    }
	}

	// Try to load the camera parameters file...
	bool used_default_camera = false;
	if (!camfilename) {
		strcpy(tmpfilename, filename);
		char *c = strrchr(tmpfilename, '.');
		if (c) {
			c++;
			*c++ = 'c'; *c++ = 'a'; *c++ = 'm'; *c++ = 'e'; *c++ = 'r'; *c++ = 'a';
			*c = '\0';
			if (stat(tmpfilename, &statbuf) != -1) {
				camfilename = tmpfilename;
				goto try_to_load_camera;  // Considered harmful
			}

			c = strrchr(tmpfilename, '.');
			c++;
			*c++ = 't'; *c++ = 's'; *c++ = 'a'; *c++ = 'i';
			*c = '\0';
			if (stat(tmpfilename, &statbuf) != -1) {
				camfilename = tmpfilename;
				goto try_to_load_camera;
			}

			camfilename = DEFAULT_CAMERA_FILENAME;
			used_default_camera = true;
		}
	}

try_to_load_camera:
	printf("Trying to read camera parameters from %s\n", camfilename);
	CameraParams *thecamera = CameraParams::Read(camfilename);
	if (!thecamera) {
		delete [] image;
		return NULL;
	}


	// OK, look for the various transforms 'n stuff...
	// First the cam calib xform
	if (!camcalibxffilename) {
		if (used_default_camera) {
			camcalibxffilename = DEFAULT_CAMCALIBXF_FILENAME;
		} else {
			strcpy(tmpfilename, filename);
			char *c = strrchr(tmpfilename, '.');
			if (c) {
				c++;
				*c++ = 'c'; *c++ = 'a'; *c++ = 'm'; *c++ = 'c';
				*c++ = 'a'; *c++ = 'l'; *c++ = 'i'; *c++ = 'b';
				*c++ = '.'; *c++ = 'x'; *c++ = 'f';
				*c = '\0';
				camcalibxffilename = tmpfilename;
			}
		}
	}
	Xform<float> *camcalibxf = NULL;
	if (camcalibxffilename) {
		ifstream camcalibxf_is(camcalibxffilename);
		if (camcalibxf_is) {
			printf("Reading camera calibration xf from %s\n", camcalibxffilename);
			camcalibxf = new Xform<float>;
			camcalibxf_is >> (*camcalibxf);
		}
	}

	// Next the cam xform
	if (!camxffilename) {
		strcpy(tmpfilename, filename);
		char *c = strrchr(tmpfilename, '.');
		if (c) {
			c++;
			*c++ = 'x';
			*c++ = 'f';
			*c = '\0';
			camxffilename = tmpfilename;
		}
	}
	Xform<float> *camxf = NULL;
	ifstream camxf_is(camxffilename);
	if (camxf_is) {
		printf("Reading camera xf from %s\n", camxffilename);
		camxf = new Xform<float>;
		camxf_is >> (*camxf);
		camxf->fast_invert();
	}

	// Finally the alignment xform
	if (!alignxffilename) {
		// OK, there are a couple of things we can try.
		// First look for a .alignxf
		strcpy(tmpfilename, filename);
		char *c = strrchr(tmpfilename, '.');
		if (c) {
			c++;
			*c++ = 'a';
			*c++ = 'l';
			*c++ = 'i';
			*c++ = 'g';
			*c++ = 'n';
			*c++ = 'x';
			*c++ = 'f';
			*c = '\0';
			if (stat(tmpfilename, &statbuf) != -1) {
				alignxffilename = tmpfilename;
				goto try_to_load_alignxf;
			}
		}

		// The other thing we try is converting any
		// ".sd" in the name to ".xf"
		strcpy(tmpfilename, filename);
		c = strstr(tmpfilename, ".sd");
		if (c) {
			c++;
			*c++ = 'x';
			*c++ = 'f';
			*c = '\0';
			if (stat(tmpfilename, &statbuf) != -1) {
				alignxffilename = tmpfilename;
			}
		}
		// Else give up...
	}
try_to_load_alignxf:
	Xform<float> *alignxf = NULL;
	ifstream alignxf_is(alignxffilename);
	if (alignxf_is) {
		printf("Reading alignment xf from %s\n", alignxffilename);
		alignxf = new Xform<float>;
		alignxf_is >> (*alignxf);
		alignxf->fast_invert();
	}


	// If we are not going to use mipmaps, figure out how big a texture
	// image to make

	float xscale=1.0, yscale=1.0;
	int tex_x, tex_y;

	if (!use_mipmaps) {
		int logwidth = min(10, int(log(width) / M_LN2));
		int logheight = min(10, int(log(height) / M_LN2));

		tex_x = 1 << logwidth;
		tex_y = 1 << logheight;

		xscale = (float) width / tex_x;
		yscale = (float) height / tex_y;
	}


	// At this point, we've loaded the image, the camera, and all the
	// transforms we could find.  We're ready to create the TextureObj
	ProjectiveTexture *thetexture = new ProjectiveTexture(
						thecamera,
						camcalibxf, camxf, alignxf,
						xscale, yscale );

	// Tell OpenGL about the texture...
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MAG_FILTER);

	if (use_mipmaps) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MIN_FILTER);
		printf("Building mipmaps..."); fflush(stdout);

		// gluBuild2DMipmaps(GL_TEXTURE_2D,
		// 		  GL_RGB,
		// 		  width, height,
		// 		  GL_RGB, GL_UNSIGNED_BYTE,
		// 		  image);

		MakeMipmapsWithBorder(image, width, height);

		printf(" Done.\n");
	} else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		unsigned char *tempimage = new unsigned char[3 * tex_x * tex_y];

		int xoff = (width - tex_x) / 2;
		int yoff = (height - tex_y) / 2;

		for (int row=0; row < tex_y; row++) {
			memcpy( tempimage+3*row*tex_x,
				image+3*(xoff+(row+yoff)*width),
				3*tex_x );
			tempimage[3*row*tex_x  ] = 0;
			tempimage[3*row*tex_x+1] = 0;
			tempimage[3*row*tex_x+2] = 255;
			tempimage[3*(row+1)*tex_x-3] = 0;
			tempimage[3*(row+1)*tex_x-2] = 0;
			tempimage[3*(row+1)*tex_x-1] = 255;
		}
		for (int col=0; col < tex_x; col++) {
			tempimage[3*col  ] = 0;
			tempimage[3*col+1] = 0;
			tempimage[3*col+2] = 255;
			tempimage[3*(tex_y-1)*tex_x+3*col  ] = 0;
			tempimage[3*(tex_y-1)*tex_x+3*col+1] = 0;
			tempimage[3*(tex_y-1)*tex_x+3*col+2] = 255;
		}

		glTexImage2D(GL_TEXTURE_2D, 0,
			     GL_RGB,
			     tex_x, tex_y,
			     0,
			     GL_RGB, GL_UNSIGNED_BYTE,
			     tempimage);
		delete [] tempimage;
	}

	delete [] image;
	return thetexture;
}

void
ProjectiveTexture::setupTexture()
{
	bind();

	// Set up texture coord generation
	GLfloat xcoord[] = {1, 0, 0, 0};
	GLfloat ycoord[] = {0, 1, 0, 0};
	GLfloat zcoord[] = {0, 0, 1, 0};
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_S, GL_OBJECT_PLANE, xcoord);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_T, GL_OBJECT_PLANE, ycoord);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGenfv(GL_R, GL_OBJECT_PLANE, zcoord);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	float b[] = { 1, 0, 0, 0.5 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, b);

	// Set up misc texture params
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Texture matrix...
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	// The camera transform gives us coords that go -1 .. 1, but
	// for textures we need 0 .. 1
	GLfloat fudge[] = { 0.5, 0, 0, 0,
			    0, 0.5, 0, 0,
			    0, 0, 1.0, 0,
			    0.5, 0.5, 0, 1.0 };
	glMultMatrixf(fudge);
	// Scale...
	glScalef(xscale, yscale, 1.0);
	// Now the camera and its calibration transform
	glMultMatrixd(thecamera->GLprojmatrix(1,2));
	glMultMatrixd(thecamera->GLmodelmatrix());
	if (camcalibxf)
		glMultMatrixf((float *)camcalibxf);
	// Finally, the camera and alignment transforms
	if (camxf)
		glMultMatrixf((float *)camxf);
	if (alignxf)
		glMultMatrixf((float *)alignxf);


	// Enable the whole thing, and pray for the best
	glEnable(GL_TEXTURE_2D);

	// scanalyze isn't too careful about always forcing the correct
	// glMatrixMode...
	glMatrixMode(GL_MODELVIEW);
}

void
MeshAffixedProjectiveTexture::setupTexture()
{
	theprojtexture->setupTexture();
	glMatrixMode(GL_TEXTURE);
	glMultMatrixf(meshtransform);
	glMatrixMode(GL_MODELVIEW);
}
