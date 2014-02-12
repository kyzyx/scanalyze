//############################################################
//
// CyberCalib.h
//
// Daniel Wood
//
// Class for storing calibration data for the SLSS and
// reading the data from a calibration file.
//
// Basically just a bag of doubles and a reader.
//
//############################################################

#ifndef CyberCalib_h_
#define CyberCalib_h_

#include <map>
#include <ext/rope>
#include "Xform.h"

using namespace __gnu_cxx;

struct CyberCalib
{
                                // *** first the input values

                                // turn axis.  axis and point on it.
    double axturn[3];
    double axturn0[3];
                                // nod axis.  axis and point on it.
    double axnod[3];
    double axnod0[3];

                                // optical correction matrix
    double A[16];
                                // transforms from working value to scanner
    double opt_frm_v[16];
    double opt_frm_h[16];
                                // horizontal arm vector.  for each rotation times
                                // up and down

    double arm_up1[3];
    double arm_up2[3];
    double arm_up3[3];
    double arm_up4[3];

    double arm_down1[3];
    double arm_down2[3];
    double arm_down3[3];
    double arm_down4[3];

                                // screw parameters
    double dnssp;               // dist. for nod screw start pivot to axis
    double dnsep;               // dist. for nod screw end   pivot to axis
    double offn;                // offset dist. for nod when screw length = -6
    double dtssp;               // dist. for turn screw start pivot to axis
    double dtsep;               // dist. for turn screw end   pivot to axis
    double offt;                // offset dist. for turn when screw length = -6
    double minscrew;


                                // *** then the derived values

                                // pre-computed values for quickly calculating the
                                // angle from a screw value.  these are used by
                                // the SCREW_TO_ANGLE and ANGLE_TO_SCREW macros
                                // in CyberXform.cc
    double n_h1;                // offn - minscrew
    double n_h2;                // dnssp^2 + dnsep^2
    double n_h3;                // 1 / (2 * dnssp * dnsep)
    double n_h4;                //acos((dnssp*dnssp + dnsep*dnsep - offn*offn)/(2*dnssp*dnsep))
    double t_h1;                // offt - minscrew
    double t_h2;                // dtssp^2 + dtsep^2
    double t_h3;                // 1 / (2 * dtssp * dtsep)
    double t_h4;                //acos((dtssp*dtssp + dtsep*dtsep - offt*offt)/(2*dtssp*dtsep))

    CyberCalib();
private:
    typedef map<crope, vector<double> > RopeDV; // rope to double vector

    void setDefault();          // set input values to their compiled-in defaults
    bool setMap( RopeDV &s2d ); // set input values to their compiled-in defaults
    void setDerived();          // set (compute) the derived values from the compiled-in values

    bool readNumbers( istream &in, RopeDV &m );
};

#endif
