#ifndef CAMERAPARAMS_H
#define CAMERAPARAMS_H
/*
Szymon Rusinkiewicz
Stanford Graphics Lab
Digital Michelangelo Project

cameraparams.h
Manipulates points and camera parameters.
*/

class CameraParams {
private:
	double *matrix;
	void compute_matrix();

public:
	int imgwidth, imgheight;
	double Tx, Ty, Tz;
	double Rx, Ry, Rz;   // in radians
	double Cx, Cy;
	double S;
	double pixels_per_radian;
	CameraParams(int _imgwidth, int _imgheight,
		     double _Tx, double _Ty, double _Tz,
		     double _Rx, double _Ry, double _Rz,
		     double _Cx, double _Cy, double _Scale,
		     double _pixels_per_radian) :
				imgwidth(_imgwidth), imgheight(_imgheight),
				Tx(_Tx), Ty(_Ty), Tz(_Tz),
				Rx(_Rx), Ry(_Ry), Rz(_Rz),
				Cx(_Cx), Cy(_Cy), S(_Scale),
				pixels_per_radian(_pixels_per_radian)
	{
		compute_matrix();
	}

	static CameraParams *Read(const char *tsaifile);

	~CameraParams()
	{
		delete [] matrix;
	}

	void Transform(const float *in, float *out) const;
	void Project(const float *in, float *out) const;
	const double *GLmodelmatrix() const;
	const double *GLprojmatrix(double z_near, double z_far) const;
};

#endif
