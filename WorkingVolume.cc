
#include "WorkingVolume.h"
#include "CyberScan.h"

CyberWorkingVolume theCyberWorkingVolume;

void
CyberWorkingVolume::use(RigidScan *rscan)
{
   CyberScan *cscan = (CyberScan *) rscan;
   scanVertical = cscan->get_scanner_vertical();
   scanConfig   = cscan->get_scanner_config();
   scanXform    = cscan->getXform();

   bounds(wvVert, wvHorInc, wvHorMin, wvHorMax, wvRotMin, wvRotMax,
          wvMotMin, wvMotMax);
}

#define YMIN 30     // 8
#define YMID 244
#define YMAX 400    // 479
#define ZMIN 80     // 60
#define ZMID 1500
#define ZMAX 2900   // 2939
#define WV_ROT_INC 10.0      // Screw distance (mm) between points

void
CyberWorkingVolume::bounds(float vert, float hor_inc, float hor_min,
   float hor_max, float rot_min, float rot_max, float mot_min, float mot_max)
{
   CyberXform xf;
   float h, r, m;
   int mc, rc;
   float vertDelta;

   wvNumShells = wvNumRots = wvNumMots = 0;

   wvVert = vert;
   wvHorInc = hor_inc;
   wvHorMin = hor_min;
   wvHorMax = hor_max;
   wvRotMin = rot_min;
   wvRotMax = rot_max;
   wvMotMin = mot_min;
   wvMotMax = mot_max;

   vertDelta = wvVert - scanVertical;

   // Determine the horizontal translation vector by
   // checking the displacement between two points.

   xf.setup(scanConfig, 0, 0, 0);
   xf.set_screw(0.0);
   Pnt3 p1 = xf.apply_xform(YMID, ZMID);
   xf.setup(scanConfig, 825.0, 0, 0);
   xf.set_screw(0.0);
   Pnt3 p2 = xf.apply_xform(YMID, ZMID);
   wvHorTransX = (p2[0] - p1[0]) / 825.0;
   wvHorTransY = (p2[1] - p1[1]) / 825.0;
   wvHorTransZ = (p2[2] - p1[2]) / 825.0;

   // Count the number of shells in the working volume

   for (h=wvHorMin; h<=wvHorMin + wvHorInc*ceilf((wvHorMax-wvHorMin)/wvHorInc);
	h+=wvHorInc) ++wvNumShells;

   // Precompute the sample points defining the shell located
   // at horizontal position = wvHorMin.

   xf.setup(scanConfig, wvHorMin, wvRotMin, vertDelta);

   for (m=wvMotMin, mc=0; m<wvMotMax; m+=WV_ROT_INC, ++mc)  {
      xf.set_screw(m);
      xf.apply_xform(YMIN, ZMIN, corner[0][mc][0]);
      xf.apply_xform(YMIN, ZMAX, corner[0][mc][1]);
   }

   xf.set_screw(wvMotMax);
   xf.apply_xform(YMIN, ZMIN, corner[0][mc][0]);
   xf.apply_xform(YMIN, ZMAX, corner[0][mc][1]);

   for (r=wvRotMin, rc=1; r<=wvRotMax; r+=WV_ROT_INC, ++rc)
   {
      xf.setup(scanConfig, wvHorMin, r, vertDelta);

      for (m=wvMotMin, mc=0; m<wvMotMax; m+=WV_ROT_INC, ++mc)  {
          xf.set_screw(m);
          xf.apply_xform(YMID, ZMIN, corner[rc][mc][0]);
          xf.apply_xform(YMID, ZMAX, corner[rc][mc][1]);
      }

      xf.set_screw(wvMotMax);
      xf.apply_xform(YMID, ZMIN, corner[rc][mc][0]);
      xf.apply_xform(YMID, ZMAX, corner[rc][mc][1]);
   }

   xf.setup(scanConfig, wvHorMin, wvRotMax, vertDelta);

   for (m=wvMotMin, mc=0; m<wvMotMax; m+=WV_ROT_INC, ++mc)  {
      xf.set_screw(m);
      xf.apply_xform(YMAX, ZMIN, corner[rc][mc][0]);
      xf.apply_xform(YMAX, ZMAX, corner[rc][mc][1]);
   }

   xf.set_screw(wvMotMax);
   xf.apply_xform(YMAX, ZMIN, corner[rc][mc][0]);
   xf.apply_xform(YMAX, ZMAX, corner[rc][mc][1]);

   wvNumRots = rc + 1;
   wvNumMots = mc + 1;
}


void
CyberWorkingVolume::drawthis(void)
{
   int i, j, k;

   glDisable(GL_LIGHTING);
   glDisable(GL_CULL_FACE);

   // Assuming glMatrixMode is GL_MODELVIEW...

   glPushMatrix();
   glMultMatrixf((float *)scanXform);  // Apply the alignment transformation

   for (i=0; i<wvNumShells; ++i)  {

      // If in line mode, just draw line segments through each shell

      if (wvDrawMode) {
         glColor3f(1,0,0);
	 glBegin(GL_LINES);
	 for (j=0; j<wvNumRots; ++j)
            for (k=0; k<wvNumMots; ++k)  {
	       glVertex3fv(corner[j][k][0]);
	       glVertex3fv(corner[j][k][1]);
	    }
         glEnd();
      }
      else {

   // Bottom

   glColor3f(1,0,0);

   for (j=0; j<wvNumRots-1; ++j)  {
      glBegin(GL_POLYGON);
         glVertex3fv(corner[j][0][0]);
         glVertex3fv(corner[j][0][1]);
         glVertex3fv(corner[j+1][0][1]);
         glVertex3fv(corner[j+1][0][0]);
      glEnd();
   }

   // Back

   glColor3f(0,1,0);

   for (j=0; j<wvNumRots-1; ++j)
      for (k=0; k<wvNumMots-1; ++k)  {
         glBegin(GL_POLYGON);
            glVertex3fv(corner[j][k][1]);
            glVertex3fv(corner[j][k+1][1]);
            glVertex3fv(corner[j+1][k+1][1]);
            glVertex3fv(corner[j+1][k][1]);
         glEnd();
      }

   // Top

   glColor3f(0,0,1);

   for (j=0; j<wvNumRots-1; ++j)  {
      glBegin(GL_POLYGON);
         glVertex3fv(corner[j][wvNumMots-1][0]);
         glVertex3fv(corner[j+1][wvNumMots-1][0]);
         glVertex3fv(corner[j+1][wvNumMots-1][1]);
         glVertex3fv(corner[j][wvNumMots-1][1]);
      glEnd();
   }

   // Front

   glColor3f(1,1,0);

   for (j=0; j<wvNumRots-1; ++j)
      for (k=0; k<wvNumMots-1; ++k)  {
         glBegin(GL_POLYGON);
            glVertex3fv(corner[j][k][0]);
            glVertex3fv(corner[j+1][k][0]);
            glVertex3fv(corner[j+1][k+1][0]);
            glVertex3fv(corner[j][k+1][0]);
         glEnd();
      }

   // Right side

   glColor3f(1,0,1);

   for (j=0; j<wvNumMots-1; ++j)  {
      glBegin(GL_POLYGON);
         glVertex3fv(corner[wvNumRots-1][j][0]);
         glVertex3fv(corner[wvNumRots-1][j][1]);
         glVertex3fv(corner[wvNumRots-1][j+1][1]);
         glVertex3fv(corner[wvNumRots-1][j+1][0]);
      glEnd();
   }

   // Left side

   glColor3f(0,1,1);

   for (j=0; j<wvNumMots-1; ++j)  {
      glBegin(GL_POLYGON);
         glVertex3fv(corner[0][j][0]);
         glVertex3fv(corner[0][j+1][0]);
         glVertex3fv(corner[0][j+1][1]);
         glVertex3fv(corner[0][j][1]);
      glEnd();
   }
   }

   glTranslatef(wvHorInc*wvHorTransX, wvHorInc*wvHorTransY,
                wvHorInc*wvHorTransZ);

   }

   glPopMatrix();
}


CyberWorkingVolume *GetCyberWorkingVolume(void)
{
   return &theCyberWorkingVolume;
}

