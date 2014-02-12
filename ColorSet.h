//############################################################
// ColorUtils.h
// Brian Curless / Matt Ginzton / Kari Pulli
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



void
pushColor (vector<uchar>& colors, int colorsize, float value);
void
pushColor (vector<uchar>& colors, int colorsize, float* values);
void
pushColor (vector<uchar>& colors, int colorsize,
	   float value1, float value2, float value3);
void
pushColor (vector<uchar>& colors, int colorsize, uchar value);
void
pushColor (vector<uchar>& colors, int colorsize, uchar* values);
void
pushColor (vector<uchar>& colors, int colorsize,
	   uchar* values1, uchar* values2, uchar* values3);

#endif
