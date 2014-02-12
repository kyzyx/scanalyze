/*
Szymon Rusinkiewicz
Stanford Graphics Lab
Digital Michelangelo Project

cameraparams.cc
*/

#include <math.h>
#include <stdio.h>
#include "cameraparams.h"

CameraParams *CameraParams::Read(const char *tsaifile)
{

	FILE *infile = fopen(tsaifile, "r");
	if (!infile) {
		fprintf(stderr, "Couldn't open camera calibration file.\n");
		return NULL;
	}

	printf("Reading camera parameters file... "); fflush(stdout);

	double f, pixsize;
	double dummy1, dummy2, dummy3;
	double ix, iy;

	fscanf(infile, "%lf %lf", &ix, &iy);
	int imgwidth = int(ix+0.5), imgheight = int(iy+0.5);

	fscanf(infile, "%lf %lf %lf %lf", &pixsize, &dummy1, &dummy2, &dummy3);
	if ((dummy1 != pixsize) ||
	    (dummy2 != pixsize) ||
	    (dummy3 != pixsize)) {
		fprintf(stderr, "Yikes! The four pixel size params are not equal.\n I can't deal with that.\n");
	}

	double Cx, Cy;
	fscanf(infile, "%lf %lf", &Cx, &Cy);

	double S;
	fscanf(infile, "%lf %lf %lf", &S, &f, &dummy1);
	if (dummy1 != 0) {
		fprintf(stderr, "Yikes! The distortion is not zero.\n I can't deal with that.\n");
	}
	double pixels_per_radian = f/pixsize;

	double Tx, Ty, Tz;
	double Rx, Ry, Rz;
	fscanf(infile, "%lf %lf %lf", &Tx, &Ty, &Tz);
	fscanf(infile, "%lf %lf %lf", &Rx, &Ry, &Rz);

	fclose(infile);
	printf("Done.\n");


	return new CameraParams(imgwidth, imgheight,
				Tx, Ty, Tz,
				Rx, Ry, Rz,
				Cx, Cy, S,
				pixels_per_radian);
}

// Just emulate the modelview transform
void CameraParams::Transform(const float *in, float *out) const
{
	out[0] = matrix[0]*in[0]+matrix[4]*in[1]+matrix[8]*in[2]+Tx;
	out[1] = matrix[1]*in[0]+matrix[5]*in[1]+matrix[9]*in[2]+Ty;
	out[2] = matrix[2]*in[0]+matrix[6]*in[1]+matrix[10]*in[2]+Tz;
}

// Project world coordinates to camera space.
// Note that the Z coordinate this returns is not what you'd expect from
// OpenGL, but is just the distance from the camera to the point in world
// space.
// Also note that, unlike OpenGL, this function has its origin at the
// upper-left corner...
void CameraParams::Project(const float *in, float *out) const
{
	float cam[3];
	Transform(in, cam);

	out[0] = cam[0]/cam[2]*pixels_per_radian*S+Cx;
	out[1] = cam[1]/cam[2]*pixels_per_radian+Cy;
	out[2] = sqrt(cam[0]*cam[0]+cam[1]*cam[1]+cam[2]*cam[2]);
}

void CameraParams::compute_matrix()
{
	matrix = new double[16];

	matrix[ 0] = cos(Ry)*cos(Rz);
	matrix[ 1] = cos(Ry)*sin(Rz);
	matrix[ 2] = -sin(Ry);
	matrix[ 3] = 0;
	matrix[ 4] = cos(Rz)*sin(Rx)*sin(Ry)-cos(Rx)*sin(Rz);
	matrix[ 5] = sin(Rx)*sin(Ry)*sin(Rz)+cos(Rx)*cos(Rz);
	matrix[ 6] = cos(Ry)*sin(Rx);
	matrix[ 7] = 0;
	matrix[ 8] = sin(Rx)*sin(Rz)+cos(Rx)*cos(Rz)*sin(Ry);
	matrix[ 9] = cos(Rx)*sin(Ry)*sin(Rz)-cos(Rz)*sin(Rx);
	matrix[10] = cos(Rx)*cos(Ry);
	matrix[11] = 0;
	matrix[12] = Tx;
	matrix[13] = Ty;
	matrix[14] = Tz;
	matrix[15] = 1;
}

// Use the following as:
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	glMultMatrixd(thecamera->GLprojmatrix(neardist, fardist));
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	glMultMatrixd(thecamera->GLmodelmatrix());
//	glViewport(0.5, 0.5, thecamera->imgwidth, thecamera->imgheight);
//
// Note that you have to pass 0.5 as the first two parameters to glViewport,
// to match the pixels-at-integers vs. pixels-at-half-integers conventions
//
const double *CameraParams::GLmodelmatrix() const
{
	return matrix;
}

const double *CameraParams::GLprojmatrix(double z_near, double z_far) const
{
	static double m[16];

	// Emulate glFrustum()
	double left = -z_near/pixels_per_radian/S*Cx;
	double right = z_near/pixels_per_radian/S*(imgwidth-Cx);

	// Note to self: Yes, these really are correct!
	double bottom = -z_near/pixels_per_radian*(imgheight-Cy);
	double top = z_near/pixels_per_radian*Cy;

	// Straight out of the manpage
	double A = (2.0*z_near) / (right-left);
	double B = (2.0*z_near) / (top-bottom);
	double C = (right+left) / (right-left);
	double D = (top+bottom) / (top-bottom);
	double E = -(z_far+z_near) / (z_far-z_near);
	double F = -(2*z_far*z_near) / (z_far-z_near);

	m[ 0] = A;
	m[ 1] = 0;
	m[ 2] = 0;
	m[ 3] = 0;
	m[ 4] = 0;
	m[ 5] = B;
	m[ 6] = 0;
	m[ 7] = 0;
	m[ 8] = C;
	m[ 9] = D;
	m[10] = E;
	m[11] = -1;
	m[12] = 0;
	m[13] = 0;
	m[14] = F;
	m[15] = 0;

	// Emulate glRotated(180,1,0,0);
	// Deals with "Y axis points up" vs. "Y axis points down"
	m[ 5] *= -1;
	m[ 8] *= -1;
	m[ 9] *= -1;
	m[10] *= -1;
	m[11] *= -1;

	return m;
}









