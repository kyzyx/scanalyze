//############################################################
// Image.h
// Kari Pulli
// 10/25/95
//############################################################

#ifndef _Image_h
#define _Image_h

/*** includes ***/
#include <iostream>
using namespace std;

typedef unsigned char uchar;

/*** class definition ***/
class Image
{
private:
  int xsize, ysize;
  uchar *_data,           // actual data
        **data;           // row pointers
  int dim;                // 1 bw, 3 rgb, 4 rgba

  int inside(int x, int y)
    { return (x >= 0 && x < xsize && y >= 0 && y < ysize); }
  uchar *at(int x, int y) const
    { return &(data[y][dim*x]); }

public:
  enum Format { WSU, SGI, RAWRGB, FLOATRGB };

  Image();
  Image(const Image& a);
  Image &operator =(const Image &a);
  Image(const int w, const int h, const int d=1);
  ~Image();

  void create(const int w, const int h, const int d);

  int dimension(void) const { return dim; }
  int width(void)     const { return xsize; }
  int height(void)    const { return ysize; }

  uchar getR(int x, int y) const
    { return at(x,y)[0]; }
  uchar getG(int x, int y) const
    { return (dim>1) ? at(x,y)[1] : at(x,y)[0]; }
  uchar getB(int x, int y) const
    { return (dim>1) ? at(x,y)[2] : at(x,y)[0]; }
  uchar getValue(int x, int y)
    { return *at(x,y); }
  uchar *getValues(int x, int y)
    { return at(x,y); }
  const uchar *getValuesConst(int x, int y) const
    { return at(x,y); }
  void setValue(int x, int y, uchar a,
		uchar b=0, uchar c=0, uchar d=0);

  operator const uchar *(void) const   { return _data; }
  operator       uchar *(void)         { return _data; }

  void toRGBA(void);
  void toRGB(void);
  void RGB2HSV(void);
  void HSV2RGB(void);
  void RGB2rgI(void);
  void rgI2RGB(void);
  void resize(int w, int h);

  void flip(void); // flips the image vertically
  void distances(int iter = 4);
  void dist4fast(void);

  int read(const char *file, Format format=SGI, int w=0, int h=0);
  int write(const char *file, Format format=SGI);

  Image shrink(int factor) const;
  void  shrink_by_half(Image &simg) const;
  Image warp(float m[3][3]) const;
  Image crop(int x, int y, int dx, int dy);

  Image &fill(uchar a, uchar b=0, uchar c=0, uchar d=0);

  friend ostream &operator <<(ostream &out, Image &im);
};

inline void
Image::setValue(int x, int y, uchar a, uchar b, uchar c, uchar d)
{
  if (!inside(x,y)) return;
  uchar *tmp = at(x,y);
  tmp[0] = a;
  if (dim>1) {
    tmp[1] = b; tmp[2] = c;
    if (dim>3)  tmp[3] = d;
  }
}

#endif /* _Image_h */

