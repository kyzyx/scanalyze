
#ifndef _WORKING_VOLUME_
#define _WORKING_VOLUME_

#include "RigidScan.h"
#include "DrawObj.h"

extern DrawObjects draw_other_things;

class CyberWorkingVolume : public DrawObj {
private:
   float        scanVertical;
   unsigned int scanConfig;
   Xform<float> scanXform;
   float wvVert;
   float wvHorInc, wvHorMin, wvHorMax;
   float wvRotMin, wvRotMax;
   float wvMotMin, wvMotMax;
   Pnt3 corner[50][50][2];
   int wvNumShells, wvNumRots, wvNumMots;
   float wvHorTransX, wvHorTransY, wvHorTransZ;
   int wvDrawMode;
public:
   CyberWorkingVolume(void) {
      draw_other_things.add(this);
      off();
      wvDrawMode = 0;
   }
   ~CyberWorkingVolume(void) {
      draw_other_things.remove(this);
   }

   void off(void) {disable();}
   void on(void) {enable();}
   void drawMode(int mode) {wvDrawMode = mode;}
   void use(RigidScan *scan);
   void bounds(float vert, float hor_inc, float hor_min, float hor_max,
               float rot_min, float rot_max, float mot_min, float mot_max);
   void drawthis(void);
};

CyberWorkingVolume *GetCyberWorkingVolume(void);

#endif

