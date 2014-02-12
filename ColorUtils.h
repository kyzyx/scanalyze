//############################################################
// ColorUtils.h
// Matt Ginzton / Kari Pulli
// Mon Jun 22 13:39:49 PDT 1998
//
// Utilities for dealing with colors
//############################################################


#ifndef _COLORUTILS_H_
#define _COLORUTILS_H_

#include "vector.h"
#include "defines.h"

class ColorSet
{
public:
  ColorSet();
  void chooseNewColor (uchar* color);

private:
  int currentColor;
};


// the rules on these overloaded functions:
// 1) floats get multiplied by 255 to convert to uchar
// 2) a pointer is taken to be a 3-array
// 3) the 3-float version is taken to be 3 separate values, like the array

void pushColor (vector<uchar>& colors, int colorsize, float value);
void pushColor (vector<uchar>& colors, int colorsize, float* values);
void pushColor (vector<uchar>& colors, int colorsize,
		float value1, float value2, float value3);
void pushColor (vector<uchar>& colors, int colorsize, uchar value);
void pushColor (vector<uchar>& colors, int colorsize, uchar* values);
void pushColor (vector<uchar>& colors, int colorsize,
		uchar* values1, uchar* values2, uchar* values3);

void pushConf (vector<uchar>& colors, int colorsize, float conf);
void pushConf (vector<uchar>& colors, int colorsize, uchar conf);

uchar
intensityFromRGB (uchar* rgb);
#endif
