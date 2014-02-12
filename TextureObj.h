//############################################################
//
// TextureObj.h
//
// Szymon Rusinkiewicz
// Fri Mar 12 01:47:50 CET 1999
//
// Texture maps for scans.
//
//############################################################

#ifndef _TEXTUREOBJ_H_
#define _TEXTUREOBJ_H_

#define DEFAULT_CAMERA_FILENAME "/usr/graphics/lib/plyshade/pastecolor/dmich.cameraparams"
#define DEFAULT_CAMCALIBXF_FILENAME "/usr/graphics/lib/plyshade/pastecolor/dmich.cameracalib.xf"

#ifdef sgi
#define MAG_FILTER GL_LINEAR_SHARPEN_SGIS
#else
#define MAG_FILTER GL_LINEAR
#endif

#define MIN_FILTER GL_LINEAR_MIPMAP_LINEAR

#include <string.h>
#include <stdlib.h>
#include <GL/gl.h>
#include "Xform.h"
#include "RefCount.h"
#include "cameraparams.h"

class TextureObj : public RefCount {
public:
	virtual void setupTexture() = 0;
	virtual ~TextureObj() {}
};

class GL_Bound_TextureObj : public TextureObj {
private:
	GLuint textureobjnum;

protected:
	void bind()
	{
		glBindTexture(GL_TEXTURE_2D, textureobjnum);
	}
	void unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	GL_Bound_TextureObj()
	{
		glGenTextures(1, &textureobjnum);
		bind();
	}

public:
	virtual ~GL_Bound_TextureObj()
	{
		if (textureobjnum)
			glDeleteTextures(1, &textureobjnum);
	}
};


class ProjectiveTexture : public GL_Bound_TextureObj {
private:
	Xform<float> *camcalibxf, *camxf, *alignxf;
	float xscale, yscale;
	CameraParams *thecamera;

protected:
	ProjectiveTexture(CameraParams *_cam,
			  Xform<float> *_camcalibxf = NULL,
			  Xform<float> *_camxf = NULL,
			  Xform<float> *_alignxf = NULL,
			  float _xscale = 1.0,
			  float _yscale = 1.0) :
		camcalibxf(_camcalibxf), camxf(_camxf), alignxf(_alignxf),
		thecamera(_cam), xscale(_xscale), yscale(_yscale)
	{}

public:
	static Ref<ProjectiveTexture> loadImage(const char *filename,
						const char *camfilename = NULL,
						const char *camcalibxffilename = NULL,
						const char *camxffilename = NULL,
						const char *alignxffilename = NULL,
						const bool actuallyLoadImage = true,
						const bool use_mipmaps = true );
	virtual void setupTexture();
	virtual ~ProjectiveTexture()
	{
		delete thecamera;
		if (alignxf)  delete alignxf;
		if (camxf)  delete camxf;
		if (camcalibxf)  delete camcalibxf;
	}
};

class MeshAffixedProjectiveTexture : public TextureObj {
private:
	Ref<ProjectiveTexture> theprojtexture;
	float meshtransform[16];
	MeshAffixedProjectiveTexture(Ref<ProjectiveTexture> _theprojtexture,
				     const float *_meshtransform) :
		theprojtexture(_theprojtexture)
	{
		memcpy(meshtransform, _meshtransform, 16*sizeof(float));
	}
public:
	static Ref<TextureObj> AffixToMesh(Ref<ProjectiveTexture> projtex,
					   const float *transform)
	{
		return new MeshAffixedProjectiveTexture(projtex, transform);
	}
	virtual void setupTexture();
};

#endif
