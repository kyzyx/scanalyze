//############################################################
// Image.cc
// Kari Pulli
// 10/25/95
//############################################################

#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "Image.h"
#ifndef linux
#include <ifl/iflFile.h>
#endif


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


void
bwtorgba(unsigned char *b,unsigned char *l,int n)
{
  while(n--) {
    l[0] = *b;
    l[1] = *b;
    l[2] = *b;
    l[3] = 0xff;
    l += 4; b++;
  }
}

void
bwtorgb(unsigned char *b,unsigned char *l,int n)
{
  while(n--) {
    l[0] = *b;
    l[1] = *b;
    l[2] = *b;
    l += 3; b++;
  }
}

void
rgbtorgba(unsigned char *r,unsigned char *g,unsigned char *b,
	  unsigned char *l,int n)
{
  while(n--) {
    l[0] = r[0];
    l[1] = g[0];
    l[2] = b[0];
    l[3] = 0xff;
    l += 4; r++; g++; b++;
  }
}

void
rgbatorgba(unsigned char *r,unsigned char *g,unsigned char *b,
	   unsigned char *a,unsigned char *l,int n)
{
  while(n--) {
    l[0] = r[0];
    l[1] = g[0];
    l[2] = b[0];
    l[3] = a[0];
    l += 4; r++; g++; b++; a++;
  }
}

void
rgbtorgb(unsigned char *r,unsigned char *g,unsigned char *b,
	 unsigned char *l,int n)
{
  while(n--) {
    l[0] = r[0];
    l[1] = g[0];
    l[2] = b[0];
    l += 3; r++; g++; b++;
  }
}

typedef struct _ImageRec {
  unsigned short imagic;
  unsigned short type;
  unsigned short dim;
  unsigned short xsize, ysize, zsize;
  unsigned int min, max;
  unsigned int wasteBytes;
  char name[80];
  unsigned long colorMap;
  FILE *file;
  unsigned char *tmp, *tmpR, *tmpG, *tmpB;
  unsigned long rleEnd;
  unsigned int *rowStart;
  int *rowSize;
} ImageRec;

static void
ConvertShort(unsigned short *array, long length)
{
  unsigned long b1, b2;
  unsigned char *ptr;

  ptr = (unsigned char *)array;
  while (length--) {
    b1 = *ptr++;
    b2 = *ptr++;
    *array++ = (b1 << 8) | (b2);
  }
}

static void
ConvertLong(unsigned *array, long length)
{
  unsigned long b1, b2, b3, b4;
  unsigned char *ptr;

  ptr = (unsigned char *)array;
  while (length--) {
    b1 = *ptr++;
    b2 = *ptr++;
    b3 = *ptr++;
    b4 = *ptr++;
    *array++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4);
  }
}

static ImageRec *
ImageOpen(const char *fileName)
{
  union {
    int  testWord;
    char testByte[4];
  } endianTest;
  ImageRec *image;
  int swapFlag;
  int x;

  endianTest.testWord = 1;
  if (endianTest.testByte[0] == 1) {
    swapFlag = 1;
  } else {
    swapFlag = 0;
  }

  image = (ImageRec *)malloc(sizeof(ImageRec));
  if (image == NULL) {
    fprintf(stderr, "Image: Out of memory!\n");
    exit(1);
  }
  if ((image->file = fopen(fileName, "rb")) == NULL) {
    perror(fileName);
    exit(1);
  }

  fread(image, 1, 12, image->file);

  if (swapFlag) {
    ConvertShort(&image->imagic, 6);
  }

  image->tmp  = (unsigned char *)malloc(image->xsize*256);
  image->tmpR = (unsigned char *)malloc(image->xsize*256);
  image->tmpG = (unsigned char *)malloc(image->xsize*256);
  image->tmpB = (unsigned char *)malloc(image->xsize*256);
  if (image->tmp  == NULL || image->tmpR == NULL ||
      image->tmpG == NULL || image->tmpB == NULL) {
    fprintf(stderr, "Image: Out of memory!\n");
    exit(1);
  }

  if ((image->type & 0xFF00) == 0x0100) {
    x = image->ysize * image->zsize * sizeof(unsigned);
    image->rowStart = (unsigned *)malloc(x);
    image->rowSize = (int *)malloc(x);
    if (image->rowStart == NULL || image->rowSize == NULL) {
      fprintf(stderr, "Image: Out of memory!\n");
      exit(1);
    }
    image->rleEnd = 512 + (2 * x);
    fseek(image->file, 512, SEEK_SET);
    fread(image->rowStart, 1, x, image->file);
    fread(image->rowSize, 1, x, image->file);
    if (swapFlag) {
      ConvertLong(image->rowStart, x/sizeof(unsigned));
      ConvertLong((unsigned *)image->rowSize, x/sizeof(int));
    }
  }
  return image;
}

static void
ImageClose(ImageRec *image)
{
  fclose(image->file);
  free(image->tmp);
  free(image->tmpR);
  free(image->tmpG);
  free(image->tmpB);
  free(image);
}

static void
ImageGetRow(ImageRec *image, unsigned char *buf, int y, int z)
{
  unsigned char *iPtr, *oPtr, pixel;
  int count;

  if ((image->type & 0xFF00) == 0x0100) {
    fseek(image->file, image->rowStart[y+z*image->ysize], SEEK_SET);
    fread(image->tmp, 1, (unsigned int)
	  image->rowSize[y+z*image->ysize], image->file);

    iPtr = image->tmp;
    oPtr = buf;
    while (1) {
      pixel = *iPtr++;
      count = (int)(pixel & 0x7F);
      if (!count) {
	return;
      }
      if (pixel & 0x80) {
	while (count--) {
	  *oPtr++ = *iPtr++;
	}
      } else {
	pixel = *iPtr++;
	while (count--) {
	  *oPtr++ = pixel;
	}
      }
    }
  } else {
    fseek(image->file, 512+(y*image->xsize)+
	  (z*image->xsize*image->ysize), SEEK_SET);
    fread(buf, 1, image->xsize, image->file);
  }
}

unsigned *
read_texture(char *name, int *width, int *height, int *components)
{
  unsigned *base, *lptr;
  unsigned char *rbuf, *gbuf, *bbuf, *abuf;
  ImageRec *image;
  int y;

  image = ImageOpen(name);

  if(!image)
    return NULL;
  (*width)=image->xsize;
  (*height)=image->ysize;
  (*components)=image->zsize;
  base = (unsigned *)malloc((*width)*(*height)*sizeof(unsigned));
  rbuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  gbuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  bbuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  abuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  if(!base || !rbuf || !gbuf || !bbuf)
    return NULL;
  lptr = base;
  for(y=0; y<image->ysize; y++) {
    if(image->zsize>=4) {
      ImageGetRow(image,rbuf,y,0);
      ImageGetRow(image,gbuf,y,1);
      ImageGetRow(image,bbuf,y,2);
      ImageGetRow(image,abuf,y,3);
      rgbatorgba(rbuf,gbuf,bbuf,abuf,(unsigned char *)lptr,
	       image->xsize);
      lptr += image->xsize;
    } else if(image->zsize==3) {
      ImageGetRow(image,rbuf,y,0);
      ImageGetRow(image,gbuf,y,1);
      ImageGetRow(image,bbuf,y,2);
      rgbtorgba(rbuf,gbuf,bbuf,(unsigned char *)lptr,image->xsize);
      lptr += image->xsize;
    } else {
      ImageGetRow(image,rbuf,y,0);
      bwtorgba(rbuf,(unsigned char *)lptr,image->xsize);
      lptr += image->xsize;
    }
  }
  ImageClose(image);
  free(rbuf);
  free(gbuf);
  free(bbuf);
  free(abuf);

  return (unsigned *) base;
}

unsigned *
read_textureRGB(char *name, int *width, int *height)
{
  unsigned *base, *lptr;
  unsigned char *rbuf, *gbuf, *bbuf, *abuf;
  ImageRec *image;
  int y;

  image = ImageOpen(name);

  if(!image)
    return NULL;
  (*width)=image->xsize;
  (*height)=image->ysize;
  base = (unsigned *)malloc((*width)*(*height)*sizeof(unsigned));
  rbuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  gbuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  bbuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  abuf = (unsigned char *)malloc((*width)*sizeof(unsigned char));
  if(!base || !rbuf || !gbuf || !bbuf)
    return NULL;
  lptr = base;
  for(y=0; y<image->ysize; y++) {
    if(image->zsize>=4) {
      ImageGetRow(image,rbuf,y,0);
      ImageGetRow(image,gbuf,y,1);
      ImageGetRow(image,bbuf,y,2);
      ImageGetRow(image,abuf,y,3);
      rgbtorgb(rbuf,gbuf,bbuf,(unsigned char *)lptr,image->xsize);
      lptr += image->xsize;
    } else if(image->zsize==3) {
      ImageGetRow(image,rbuf,y,0);
      ImageGetRow(image,gbuf,y,1);
      ImageGetRow(image,bbuf,y,2);
      rgbtorgb(rbuf,gbuf,bbuf,(unsigned char *)lptr,image->xsize);
      lptr += image->xsize;
    } else {
      ImageGetRow(image,rbuf,y,0);
      bwtorgb(rbuf,(unsigned char *)lptr,image->xsize);
      lptr += image->xsize;
    }
  }
  ImageClose(image);
  free(rbuf);
  free(gbuf);
  free(bbuf);
  free(abuf);

  return (unsigned *) base;
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////



void
Image::create(const int w, const int h, const int d)
{
  if (w*h*d > xsize*ysize*dim) {
    // need more room
    delete[] data;
    delete[] _data;
    _data = new uchar[w*h*d];
    data =  new uchar *[h];
  }
  if (w != xsize || d != dim) {
    for (int i=0; i<h; i++) data[i] = &_data[w*d*i];
    xsize = w;  dim = d;
  }
  ysize = h;
}

Image::Image()
: xsize(0), ysize(0), data(NULL), _data(NULL), dim(0)
{
}


Image::Image(const Image& a)
: xsize(0), ysize(0), dim(0), data(NULL), _data(NULL)
{
  create(a.xsize, a.ysize, a.dim);
  //bcopy(a._data, _data, xsize*ysize*dim);
  memcpy(_data, a._data, xsize*ysize*dim);
}

Image &
Image::operator=(const Image& a)
{
  if (&a != this) {
    //xsize = ysize = dim = 0;
    //data = NULL;
    //_data = NULL;
    create(a.xsize, a.ysize, a.dim);
    //bcopy(a._data, _data, xsize*ysize*dim);
    memcpy(_data, a._data, xsize*ysize*dim);
  }
  return *this;
}


Image::Image(const int w, const int h, const int d)
: data(NULL), _data(NULL), dim(0)
{
  create (w,h,d);
}


Image::~Image()
{
  delete[] data;
  delete[] _data;
}


ostream &operator <<(ostream &out, Image &im)
{
  out << im.xsize << " x " << im.ysize << " image";
  return out;
}

/*
void
Image::rgb2bw()
{
  unsigned char tmp;
  if (bw == 1) return;
  for (int i=0; i<xsize*ysize; i++) {
    tmp = (unsigned char)(double(_data[i].red())   * 0.30 +
			  double(_data[i].green()) * 0.59 +
			  double(_data[i].blue())  * 0.11);
    _data[i] = Pixel(tmp, tmp, tmp);
  }
  bw = 1;
}
*/

void
Image::toRGBA(void)
{
  assert(data && _data);

  if (dim == 4) return;

  uchar *new_data = new uchar[xsize*ysize*4];

  uchar *ptr_old = _data;
  uchar *ptr_new = new_data;
  for (int i=0; i<xsize*ysize; i++) {
    uchar tmp = *ptr_old++;
    if        (dim == 3) {
      *ptr_new++ = tmp;
      *ptr_new++ = *ptr_old++;
      *ptr_new++ = *ptr_old++;
      *ptr_new++ = 255;
    } else if (dim == 1) {
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
      *ptr_new++ = 255;
    } else if (dim == 2) {
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
      *ptr_new++ = *ptr_old++;
    }
  }
  delete[] _data;
  _data = new_data;
  dim = 4;
  for (i=0; i<ysize; i++) data[i] = &_data[xsize*dim*i];
}


void
Image::toRGB(void)
{
  assert(data && _data);

  if (dim == 3) return;

  uchar *new_data = new uchar[xsize*ysize*3];

  uchar *ptr_old = _data;
  uchar *ptr_new = new_data;
  for (int i=0; i<xsize*ysize; i++) {
    uchar tmp = *ptr_old++;
    if        (dim == 4) {
      *ptr_new++ = tmp;
      *ptr_new++ = *ptr_old++;
      *ptr_new++ = *ptr_old++;
      ptr_old++;
    } else if (dim == 1) {
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
    } else if (dim == 2) {
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
      *ptr_new++ = tmp;
      ptr_old++;
    }
  }
  delete[] _data;
  _data = new_data;
  dim = 3;
  for (i=0; i<ysize; i++) data[i] = &_data[xsize*dim*i];
}

void
Image::RGB2HSV(void)
{
  assert(dim == 3);
  float h,s,min,max; // max == v
  for (int v=0; v<ysize; v++) {
    for (int u=0; u<xsize; u++) {
      uchar *tmp = getValues(u,v);
      min = max = tmp[0];
      if      (tmp[1] < min) min = tmp[1];
      else if (tmp[1] > max) max = tmp[1];
      if      (tmp[2] < min) min = tmp[2];
      else if (tmp[2] > max) max = tmp[2];

      if (max != 0) s = (max - min)/max;
      else          s = 0;

      if (s == 0)   h = 0;
      else {
	float delta = max - min;
	if      (tmp[0] == max) h = (tmp[1]-tmp[2])/delta;
	else if (tmp[1] == max) h = 2 + (tmp[2]-tmp[0])/delta;
	else if (tmp[2] == max) h = 4 + (tmp[0]-tmp[1])/delta;
	if (h < 0.0) h += 6.0;
	h /= 6.0;
      }
      tmp[0] = h*255.0;
      //tmp[1] = s*255.0;
      //tmp[0] = max*h;
      tmp[1] = max*s;
      tmp[2] = max;
    }
  }
}

void
Image::HSV2RGB(void)
{
  assert(dim == 3);
  float h,s;
  for (int v=0; v<ysize; v++) {
    for (int u=0; u<xsize; u++) {
      uchar *tmp = getValues(u,v);
      if (tmp[1] == 0) {
	// zero saturation
	tmp[0] = tmp[1] = tmp[2];
      } else {
	if (tmp[0] == 255) h = 0.0;
	else               h = tmp[0]/255.0 * 6.0;
	int i = floor(h);
	float f = h - i;
	s = float(tmp[1]) / float(tmp[2]);
	float p = tmp[2] * (1.0-s);
	float q = tmp[2] * (1.0-s * f);
	float t = tmp[2] * (1.0-s * (1.0-f));
	switch(i) {
	case 0:
	  tmp[0] = tmp[2];
	  tmp[1] = t;
	  tmp[2] = p;
	  break;
	case 1:
	  tmp[0] = q;
	  tmp[1] = tmp[2];
	  tmp[2] = p;
	  break;
	case 2:
	  tmp[0] = p;
	  tmp[1] = tmp[2];
	  tmp[2] = t;
	  break;
	case 3:
	  tmp[0] = p;
	  tmp[1] = q;
	  //tmp[2] = tmp[2];
	  break;
	case 4:
	  tmp[0] = t;
	  tmp[1] = p;
	  //tmp[2] = tmp[2];
	  break;
	case 5:
	  tmp[0] = tmp[2];
	  tmp[1] = p;
	  tmp[2] = q;
	  break;
	}
      }
    }
  }
}

void
Image::RGB2rgI(void)
{
  assert(dim == 3);
  for (int v=0; v<ysize; v++) {
    for (int u=0; u<xsize; u++) {
      uchar *tmp = getValues(u,v);
      int sum = int(tmp[0]) + int(tmp[1]) + int(tmp[2]);
      if (sum < 10) {
	tmp[0] = tmp[1] = 85;
	tmp[2] = (sum / 3.0) + .5;
      } else {
	tmp[0] = 255.0 * float(tmp[0]) / sum;
	tmp[1] = 255.0 * float(tmp[1]) / sum;
	tmp[2] = (sum / 3.0) + .5;
	if (sum < 45) {
	  float frac = float(45-sum)/36.0;
	  tmp[0] += (85-int(tmp[0])) * frac;
	  tmp[1] += (85-int(tmp[1])) * frac;
	}
      }
    }
  }
}

void
Image::rgI2RGB(void)
{
  assert(dim == 3);
  for (int v=0; v<ysize; v++) {
    for (int u=0; u<xsize; u++) {
      uchar *tmp = getValues(u,v);
      uchar b = 255 - tmp[0] - tmp[1];
      float frac = 3.0 * tmp[2] / 255.0;
      tmp[0] = frac * tmp[0];
      tmp[1] = frac * tmp[1];
      tmp[2] = frac * b;
    }
  }
}

void
Image::resize(int w, int h)
{
  // get new memory
  uchar *n_data = new uchar[w*h*dim];
  uchar **ndata = new uchar *[h];
  for (int i=0; i<h; i++) ndata[i] = &n_data[w*dim*i];
  // copy the data
  int maxy = (h > ysize ? ysize : h);
  int maxx = (w > xsize ? xsize : w);
  for (i=0; i<maxy; i++)
    //bcopy(&_data[i*xsize*dim], &n_data[i*w*dim], maxx*dim);
    memcpy(&n_data[i*w*dim], &_data[i*xsize*dim], maxx*dim);
  // delete old space
  delete[] data;
  delete[] _data;
  // write the correct info
  _data = n_data;
  data  = ndata;
  xsize = w;
  ysize = h;

}


void
Image::flip(void)
{
  int row = dim*xsize;
  uchar *buf = new uchar[row];

  for (int i=0; i<ysize/2; i++) {
    memcpy(buf, data[i], row);
    memcpy(data[i], data[ysize-i-1], row);
    memcpy(data[ysize-i-1], buf, row);
  }

  delete[] buf;
}

// assume input is a single channel image with values 0 and 255
// calculate distances from 0 by looking at neighbors and
// replacing the value with the min(nbors) + 1
// do i iterations
// use 8-nborhood
// note: writes over itself while processing
void
Image::distances(int iter)
{
  for (int i=0; i<iter; i++) {
    int vi = i%2;
    for (int v=vi?0:ysize-1;
	 vi?(v<ysize):(v>=0);
	 vi?v++:v--) {
      int ui = (i%4)>>1;
      for (int u=ui?0:xsize-1;
	   ui?(u<xsize):(u>=0);
	   ui?u++:u--) {
	if (getR(u,v) == 0) continue;
	int min = 255;
	for (int j=-1; j<=1; j++) {
	  for (int k=-1; k<=1; k++) {
	    if (k==0 && j==0) continue;
	    if (inside(u+j, v+k)) {
	      uchar tmp = getR(u+j, v+k);
	      if (tmp < min) min = tmp;
	    }
	  }
	}
	if (min < 255) setValue(u,v,min+1);
      }
    }
  }
}

// assume input is a single channel image with values 0 and 255
// calculate distances from 0, use 4-nborhood
// a fast inaccurate scanline version
// note: writes over itself while processing
void
Image::dist4fast(void)
{
  int u,v;
  // scan left to right
  for (v=0; v<ysize; v++) {
    uchar *ptr = &data[v][0];
    for (u=1; u<xsize; u++, ptr++) {
      if (ptr[1] > ptr[0]) ptr[1] = ptr[0]+1;
    }
  }
  // scan down to up
  for (u=0; u<xsize; u++) {
    uchar *ptr = &data[0][u];
    for (v=1; v<ysize; v++, ptr+=xsize) {
      if (ptr[xsize] > ptr[0]) ptr[xsize] = ptr[0]+1;
    }
  }
  // scan right to left
  for (v=0; v<ysize; v++) {
    uchar *ptr = &data[v][xsize-2];
    for (u=xsize-2; u>=0; u--, ptr--) {
      if (ptr[0] > ptr[1]) ptr[0] = ptr[1]+1;
    }
  }
  // scan up to down
  for (u=0; u<xsize; u++) {
    uchar *ptr = &data[ysize-2][u];
    for (v=ysize-2; v>=0; v--, ptr-=xsize) {
      if (ptr[0] > ptr[xsize]) ptr[0] = ptr[xsize]+1;
    }
  }
}

int
Image::read(const char *fname, Format format, int w, int h)
{
  int i;
  if (w==0) {
    w=xsize; h=ysize;
  }

  switch (format) {
  case WSU:
    {
      ifstream from (fname);
      if (!from) {
	cerr << "Cannot open inputfile " << fname << endl;
	return 0;
      }
      create (w, h, 1);
      for (i=h-1; i>=0; i--)
	from.read(&_data[i*w], w);
    }
    break;
  case SGI:
#ifndef linux
    {
      iflStatus sts;
      iflFile* file = iflFile::open(fname, O_RDONLY, &sts);
      if (sts != iflOKAY) {
	cerr << "Cannot open inputfile " << fname << endl;
	return 0;
      }
      // read the entire image (just the first plane in z if image has depth)
      // into a buffer of unsiged chars
      iflSize dims;
      file->getDimensions(dims);
//      unsigned char* data = new unsigned char[dims.x*dims.y*dims.c];
//      assert(dims.c == 1 || dims.c == 3);
//      assert(w == dims.x && h == dims.y);
      create(dims.x, dims.y, dims.c);
      iflConfig cfg(iflUChar, iflInterleaved);
      sts = file->getTile(0, 0, 0, dims.x, dims.y, 1, _data, &cfg);
      if (sts != iflOKAY) {
	cerr << "Failed reading inputfile " << fname << endl;
	return 0;
      }
      // close the file
      file->close();

      /*
      ImageRec *image = ImageOpen(fname);

      if (!image) return 0;

      create(image->xsize, image->ysize, image->zsize);

      uchar *rbuf = new uchar[xsize];
      uchar *gbuf = new uchar[xsize];
      uchar *bbuf = new uchar[xsize];
      if (!rbuf || !gbuf || !bbuf) return NULL;

      uchar *lptr = _data;
      int step = 3*xsize;
      for (int y=0; y<image->ysize; y++) {
	ImageGetRow(image,rbuf,y,0);
	if (image->zsize>=4) {
	  ImageGetRow(image,gbuf,y,1);
	  ImageGetRow(image,bbuf,y,2);
	  rgbtorgb(rbuf,gbuf,bbuf,lptr,xsize);
	  ImageGetRow(image,rbuf,y,3);
	} else if (image->zsize==3) {
	  ImageGetRow(image,gbuf,y,1);
	  ImageGetRow(image,bbuf,y,2);
	  rgbtorgb(rbuf,gbuf,bbuf,lptr,xsize);
	} else {
	  bwtorgb(rbuf,lptr,xsize);
	}
	lptr += step;
      }
      ImageClose(image);
      delete[] rbuf;
      delete[] gbuf;
      delete[] bbuf;
      */
    }
    break;
#else
    return 0;
#endif
  case RAWRGB:
    {
      ifstream from (fname);
      if (!from) {
	cerr << "Cannot open inputfile " << fname << endl;
	return 0;
      }
      create(w,h,3);
      from.read(_data, w*h*3);
    }
    break;
  case FLOATRGB:
    {
      ifstream from (fname);
      if (!from) {
	cerr << "Cannot open inputfile " << fname << endl;
	return 0;
      }
      int cnt = w*h*3;
      float *tmp = new float[cnt];
      create(w,h,3);
      from.read((char *)tmp, cnt*sizeof(float));
      for (i=0; i<cnt; i++) {
	_data[i] = (uchar)(255.999999 * tmp[i]);
      }
    }
    break;
  default:
    cerr << "Unrecognized format" << endl;
    return 0;
  }
  return 1;
}

int
Image::write(const char *fname, Format format)
{
  switch (format) {
  case WSU:
    {
      cout << "Saving raw" << endl;
      ofstream to (fname);
      if (!to) {
	cerr << "Cannot open outputfile " << fname << endl;
	return 0;
      }
      for (int i=ysize-1; i>=0; i--)
	to.write(&_data[i*xsize], xsize);
    }
    break;
  case SGI:
#ifndef linux
    {
      cout << "Saving rgb" << endl;
      // create a one-channel, unsigned char image file
      iflSize dims(xsize, ysize, dim);
      iflFileConfig fc(&dims, iflUChar);
      fc.setCompression(iflSGIRLE);
      iflStatus sts;
      iflFile* file = iflFile::create(fname, NULL, &fc,NULL,&sts);
      if (sts != iflOKAY) {
	cerr << "Error in creating file " << fname << endl;
	return 0;
      }
      // write a tile of data to it
      sts = file->setTile(0, 0, 0, xsize, ysize, 1, _data);
      if (sts != iflOKAY) {
	cerr <<"Error in setting the tile for file "<<fname<<endl;
	return 0;
      }
      file->close();
    }
    break;
#else
    return 0;
#endif
  default:
    cerr << "Unknown format" << endl;
    return 0;
  }
  int i=0;
  i++;
  return i;
}


Image
Image::shrink(int factor) const
{
  int sx = xsize/factor;
  int sy = ysize/factor;
  Image simg(sx, sy, dim);

  int fact2 = factor*factor;
  uchar col[4];
  col[0] = col[1] = col[2] = col[3] = 0;
  for (int v=0; v<sy; v++) {
    int y = v*factor;
    for (int u=0; u<sx; u++) {
      int x = u*factor;
      for (int d=0; d<dim; d++) {
	int tmp = 0;
	for (int i=0; i<factor; i++) {
	  for (int j=0; j<factor; j++) {
	    tmp += at(x+i, y+j)[d];
	  }
	}
	col[d] = (tmp + fact2/2) / fact2;
      }
      simg.setValue(u,v,col[0],col[1],col[2],col[3]);
    }
  }
  return simg;
}


void
Image::shrink_by_half(Image &simg) const
{
  int sx = xsize>>1;
  int sy = ysize>>1;
  simg.create(sx, sy, dim);

  int tmp[4];
  int u,v,x,y;
  uchar *ptr;
  if (dim == 4) {
    for (v=0,y=0; v<sy; v++, y+=2) {
      for (u=0,x=0; u<sx; u++, x+=2) {
	ptr = at(x,y);
	tmp[0] = ptr[0] + ptr[4];
	tmp[1] = ptr[1] + ptr[5];
	tmp[2] = ptr[2] + ptr[6];
	tmp[3] = ptr[3] + ptr[7];
	ptr += xsize*4;
	tmp[0] += ptr[0] + ptr[4];
	tmp[1] += ptr[1] + ptr[5];
	tmp[2] += ptr[2] + ptr[6];
	tmp[3] += ptr[3] + ptr[7];
	ptr = simg.getValues(u,v);
	ptr[0] = (tmp[0]+2)>>2;
	ptr[1] = (tmp[1]+2)>>2;
	ptr[2] = (tmp[2]+2)>>2;
	ptr[3] = (tmp[3]+2)>>2;
      }
    }
  } else if (dim == 3) {
    for (v=0,y=0; v<sy; v++, y+=2) {
      for (u=0,x=0; u<sx; u++, x+=2) {
	ptr = at(x,y);
	tmp[0] = ptr[0] + ptr[3];
	tmp[1] = ptr[1] + ptr[4];
	tmp[2] = ptr[2] + ptr[5];
	ptr += xsize*3;
	tmp[0] += ptr[0] + ptr[3];
	tmp[1] += ptr[1] + ptr[4];
	tmp[2] += ptr[2] + ptr[5];
	ptr = simg.getValues(u,v);
	ptr[0] = (tmp[0]+2)>>2;
	ptr[1] = (tmp[1]+2)>>2;
	ptr[2] = (tmp[2]+2)>>2;
      }
    }
  } else if (dim == 2) {
    for (v=0,y=0; v<sy; v++, y+=2) {
      for (u=0,x=0; u<sx; u++, x+=2) {
	ptr = at(x,y);
	tmp[0] = ptr[0] + ptr[2];
	tmp[1] = ptr[1] + ptr[3];
	ptr += xsize*2;
	tmp[0] += ptr[0] + ptr[2];
	tmp[1] += ptr[1] + ptr[3];
	ptr = simg.getValues(u,v);
	ptr[0] = (tmp[0]+2)>>2;
	ptr[1] = (tmp[1]+2)>>2;
      }
    }
  } else {
    for (v=0,y=0; v<sy; v++, y+=2) {
      for (u=0,x=0; u<sx; u++, x+=2) {
	ptr = at(x,y);
	tmp[0] = ptr[0] + ptr[1];
	ptr += xsize;
	tmp[0] += ptr[0] + ptr[1];
	ptr = simg.getValues(u,v);
	ptr[0] = (tmp[0]+2)>>2;
      }
    }
  }
}


// return an image warped by the projective transformation
// [x' y' w'] = [u v w] M
Image
Image::warp(float m[3][3]) const
{
  // first, invert the matrix by taking its adjoint
  // works because it's a projection, scale invariant
  float Mi[3][3];
  Mi[0][0] = m[1][1]*m[2][2] - m[1][2]*m[2][1];
  Mi[0][1] = m[0][2]*m[2][1] - m[0][1]*m[2][2];
  Mi[0][2] = m[0][1]*m[1][2] - m[0][2]*m[1][1];
  Mi[1][0] = m[1][2]*m[2][0] - m[1][0]*m[2][2];
  Mi[1][1] = m[0][0]*m[2][2] - m[0][2]*m[2][0];
  Mi[1][2] = m[0][2]*m[1][0] - m[0][0]*m[1][2];
  Mi[2][0] = m[1][0]*m[2][1] - m[1][1]*m[2][0];
  Mi[2][1] = m[0][1]*m[2][0] - m[0][0]*m[2][1];
  Mi[2][2] = m[0][0]*m[1][1] - m[0][1]*m[1][0];

  Image im(xsize, ysize, dim);
  int tmp[4], cnt;
  float x,y;
  uchar *val;

  for (int v=0; v<ysize; v++) {
    for (int u=0; u<xsize; u++) {
      tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0;
      cnt = 0;
      for (int i=0; i<=2; i++) {
	for (int j=0; j<=2; j++) {
	  float ui = u+i/3.0+1./6.;
	  float vj = v+j/3.0+1./6.;
	  float w = 1.0f / (ui*Mi[0][2] + vj*Mi[1][2] + Mi[2][2]);
	  x = (ui*Mi[0][0] + vj*Mi[1][0] + Mi[2][0]) * w;
	  y = (ui*Mi[0][1] + vj*Mi[1][1] + Mi[2][1]) * w;
	  if (x < 0.0 || y < 0.0 || x >= xsize || y >= ysize)
	    continue;
	  cnt++;
	  val = at(x,y);
	  for (int d=0; d<dim; d++)
	    tmp[d] += int(val[d]);
	}
      }
      val = im.getValues(u,v);
      for (int d=0; d<dim; d++)
	val[d] = (cnt ? tmp[d] / float(cnt) : 0);
    }
  }
  return im;
}

Image
Image::crop(int x, int y, int dx, int dy)
{
  Image im(dx,dy,dim);
  for (int v=0; v<dy; v++) {
    uchar *tmpo = getValues(x,y+v);
    uchar *tmpn = im.getValues(0,v);
    for (int i=0; i<dx*dim; i++) {
      tmpn[i] = tmpo[i];
    }
  }
  return im;
}

Image&
Image::fill(uchar a, uchar b, uchar c, uchar d)
{
  int size = xsize*ysize;
  for (int i=0; i<size; i++) {
    uchar *tmp = &_data[i*dim];
    if (dim == 4) tmp[3] = d;
    if (dim >= 3) tmp[2] = c;
    if (dim >= 2) tmp[1] = b;
    if (dim >= 1) tmp[0] = a;
  }
  return *this;
}
