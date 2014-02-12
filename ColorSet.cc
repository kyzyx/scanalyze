//############################################################
// ColorUtils.cc
// Matt Ginzton / Kari Pulli
// Mon Jun 22 13:39:49 PDT 1998
//
// Utilities for dealing with colors
//############################################################


#include "ColorSet.h"


ColorSet::ColorSet (void)
{
  currentColor = 0;
}


static float colorSet[][3] = {
    {0.9*0.75, 0.15*0.75, 0.35*0.75},
    {0.0*0.75, 0.9*0.75, 0.0*0.75},
    {0.0*0.75, 0.9*0.75, 0.9*0.75},
    {0.9*0.75, 0.0*0.75, 0.9*0.75},
    {0.9*0.75, 0.9*0.75, 0.0*0.75},
    {0.0*0.75, 0.0*0.75, 0.9*0.75},
    {0.9*0.75, 0.5*0.75, 0.2*0.75},
    {0.5*0.75, 0.9*0.75, 0.2*0.75},
    {0.2*0.75, 0.5*0.75, 0.9*0.75},
    {0.5*0.75, 0.2*0.75, 0.9*0.75},
    {0.9*0.75, 0.2*0.75, 0.5*0.75},
    {0.2*0.75, 0.9*0.75, 0.5*0.75},
    {0.9*0.75, 0.7*0.75, 0.7*0.75},
    {0.7*0.75, 0.9*0.75, 0.7*0.75},
    {0.7*0.75, 0.7*0.75, 0.9*0.75},
    {0.7*0.75, 0.9*0.75, 0.9*0.75},
    {0.9*0.75, 0.7*0.75, 0.9*0.75},
    {0.9*0.75, 0.9*0.75, 0.7*0.75},
    {0.6*0.75, 0.6*0.75, 0.6*0.75},
    {0.6*0.75, 0.3*0.75, 0.1*0.75},
    {0.6*0.75, 0.1*0.75, 0.3*0.75},
    {0.3*0.75, 0.1*0.75, 0.6*0.75},
    {0.3*0.75, 0.6*0.75, 0.1*0.75},
    {0.1*0.75, 0.3*0.75, 0.6*0.75},
    {0.1*0.75, 0.6*0.75, 0.3*0.75},
    {0.4*0.75, 0.9*0.75, 0.3*0.75},
    {0.2*0.75, 0.9*0.75, 0.0*0.75},
};

#define NUM_COLOR_SET_COLORS (sizeof(colorSet) / sizeof(colorSet[0]))


void
ColorSet::chooseNewColor (uchar* color)
{
  currentColor = currentColor % NUM_COLOR_SET_COLORS;

  color[0] = uchar(colorSet[currentColor][0]*255);
  color[1] = uchar(colorSet[currentColor][1]*255);
  color[2] = uchar(colorSet[currentColor][2]*255);

  currentColor++;
}

void
pushColor (vector<uchar>& colors, int colorsize, float value)
{
  pushColor (colors, colorsize, 255 * value);
}


void
pushColor (vector<uchar>& colors, int colorsize, float* values)
{
  uchar cvalues[3] = { values[0] * 255, values[1] * 255, values[2] * 255 };
  pushColor (colors, colorsize, cvalues);
}


void
pushColor (vector<uchar>& colors, int colorsize,
	   float value1, float value2, float value3)
{
  uchar cvalues[3] = { value1 * 255, value2 * 255, value3 * 255 };
  pushColor (colors, colorsize, cvalues);
}


void
pushColor (vector<uchar>& colors, int colorsize, uchar value)
{
  switch (colorsize) {
  case 1:
  case 2:
    colors.push_back (value);
    break;
  case 3:
  case 4:
    colors.push_back (value);
    colors.push_back (value);
    colors.push_back (value);
    break;
  }

  if (colorsize == 2 || colorsize == 4) {
    // TODO: hardcoded alpha for now
    colors.push_back (255);
  }
}


void
pushColor (vector<uchar>& colors, int colorsize, uchar* values)
{
  switch (colorsize) {
  case 1:
  case 2:
    colors.push_back ((values[0] + values[1] + values[2]) / 3);
    break;
  case 3:
  case 4:
    colors.push_back (values[0]);
    colors.push_back (values[1]);
    colors.push_back (values[2]);
    break;
  }

  if (colorsize == 2 || colorsize == 4) {
    // TODO: hardcoded alpha for now
    colors.push_back (255);
  }
}


void
pushColor (vector<uchar>& colors, int colorsize,
	   uchar* values1, uchar* values2, uchar* values3)
{
  pushColor (colors, colorsize,
	     (values1[0] + values2[0] + values3[0]) / 3,
	     (values1[1] + values2[1] + values3[1]) / 3,
	     (values1[2] + values2[2] + values3[2]) / 3);
}

