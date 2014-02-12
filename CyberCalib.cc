//############################################################
//
// CyberCalib.cc
//
// Daniel Wood
// Fri Dec 11 11:40:42 CET 1998
//
//
// Class for storing calibration data for the SLSS and
// reading the data from a calibration file.
//
// Basically just a bag of doubles and a reader.  Along
// with some defaults.
//
//############################################################

#include "CyberCalib.h"
#include <stdlib.h>
#include <ctype.h>
#include <iostream.h>
#include <fstream.h>

CyberCalib::CyberCalib()
{
  const char *calibFN = getenv( "SLSS_CALIBRATION_FILE" );
  if( calibFN ) {
    RopeDV r2dv;
    ifstream in( calibFN );
    if( in.fail() || !readNumbers( in, r2dv ) || !setMap(r2dv) ) {
      cerr << "Couldn't read calibration file: "
	   << calibFN << endl;
      exit( 1 );
    } else {
      cout << "Read calibration file " << calibFN << endl;
    }
  } else {
    setDefault();
  }
  setDerived();
  SHOW(offt);
}


                                // helpers
#ifdef WIN32
// MSVC can't compile the template flavor below,
// but it's only called with argument sizes 3 and 16,
// so expand these here.
bool
setval( double (&d)[3], vector<double> &vd )
// Set the size contents of the size n array d to be the same as the
// contents of the vector, if possible.
//
// If not possible return false.
{
    if( vd.size() != 3 )
        return 0;

    memcpy( d, (double *) vd.begin(), sizeof(d) );
    return 1;
}

bool
setval( double (&d)[16], vector<double> &vd )
{
    if( vd.size() != 16 )
        return 0;

    memcpy( d, (double *) vd.begin(), sizeof(d) );
    return 1;
}

#else

template< int N >
bool
setval( double (&d)[N], vector<double> &vd )
// Set the size contents of the size n array d to be the same as the
// contents of the vector, if possible.
//
// If not possible return false.
{
    if( vd.size() != N )
        return 0;

// STL Update
    memcpy( d, (double *) &*(vd.begin()), sizeof(d) );
    return 1;
}
#endif


bool
setval( double &d, vector<double> &vd )
// Set d to be the value of the one and only element of vd, if possible.
//
// If not possible return false.
{
    if( vd.size() != 1 )
        return 0;
    d = vd[0];
    return 1;
}

bool
CyberCalib::setMap( RopeDV &r2dv )
// Set all of the input calibration values using the map r2dv.  If
// something doesn't work return false, otherwise return true.
{
    return
        setval( axturn, r2dv["axturn"] ) &&
        setval( axturn0, r2dv["axturn0"] ) &&
        setval( axnod, r2dv["axnod"] ) &&
        setval( axnod0, r2dv["axnod0"] ) &&
        setval( A, r2dv["A"] ) &&
        setval( opt_frm_v, r2dv["opt_frm_v"] ) &&
        setval( opt_frm_h, r2dv["opt_frm_h"] ) &&
        setval( arm_up1, r2dv["arm_up1"] ) &&
        setval( arm_up2, r2dv["arm_up2"] ) &&
        setval( arm_up3, r2dv["arm_up3"] ) &&
        setval( arm_up4, r2dv["arm_up4"] ) &&
        setval( arm_down1, r2dv["arm_down1"] ) &&
        setval( arm_down2, r2dv["arm_down2"] ) &&
        setval( arm_down3, r2dv["arm_down3"] ) &&
        setval( arm_down4, r2dv["arm_down4"] ) &&
        setval( dnssp, r2dv["dnssp"] ) &&
        setval( dnsep, r2dv["dnsep"] ) &&
        setval( offn, r2dv["offn"] ) &&
        setval( dtssp, r2dv["dtssp"] ) &&
        setval( dtsep, r2dv["dtsep"] ) &&
        setval( offt, r2dv["offt"] ) &&
        setval( minscrew, r2dv["minscrew"] );
}

//#define OLD_DEFAULTS
#ifndef OLD_DEFAULTS
void
CyberCalib::setDefault()
// Set all of the values to their compiled in defaults.
{
                                // turn axis and point on it.
    axturn[0] =  0.01805376092094;
    axturn[1] =  0.01485705900017;
    axturn[2] = -0.99972662739095;
    axturn0[0] =  565.165097159111;
    axturn0[1] =  566.718122653641;
    axturn0[2] = -125.285854617229;

                                // nod axis.  axis and point on it.
    axnod[0] =  0.64018246222957;
    axnod[1] =  0.76822163316833;
    axnod[2] = -0.00139190008991;
    axnod0[0] =  530.462789245481;
    axnod0[1] =  453.963537662380;
    axnod0[2] = -143.122138720635;


                                // optical correction matrix
    static const double default_A[] =
    {
        1.0,               0.0,                0.0,                0.0,
        0.0,  018.579261546745,  -000.088087061526,   000.000675546148,
        0.0,  000.278844123926,  -017.924459594106,   000.014749815686,
        0.0, -021.380307002909,   162.548102595433,   001.000000000000
    };
    memcpy( A, default_A, sizeof(A) );

                                // transforms from working value to scanner
    static const double default_opt_frm_v[] =
    {
  -000.601766364057,   000.468859880636,   000.646566048768,   0,
  -000.625274483242,  -000.780237623214,  -000.016157720391,   0,
   000.496899450288,  -000.414004424676,   000.762686877199,   0,
   993.138114642769,   139.346085979040,   931.016548440975,   1
    };
    static const double default_opt_frm_h[] =
    {
   000.098796918656,   000.868618161141,   000.485532345988,   0,
  -000.995101967634,   000.084594164467,   000.051145883015,   0,
   000.003353039724,  -000.488207248486,   000.872721284059,   0,
   913.128490923706,  -110.657398062714,  -294.415768931665,   1
    };
    memcpy( opt_frm_v, default_opt_frm_v, sizeof(opt_frm_v) );
    memcpy( opt_frm_h, default_opt_frm_h, sizeof(opt_frm_h) );

                                // horizontal arm direction
    arm_up1[0] = -0.02791322161013;
    arm_up1[1] =  0.99960644654600;
    arm_up1[2] =  0.00279357889050;
    arm_up2[0] = -0.99960789687213;
    arm_up2[1] = -0.02800054167491;
    arm_up2[2] =  0.00014891875503;
    arm_up3[0] =  0.02790980783040;
    arm_up3[1] = -0.99959720870661;
    arm_up3[2] = -0.00514421741692;
    arm_up4[0] =  0.99960935645185;
    arm_up4[1] =  0.02784414108967;
    arm_up4[2] = -0.00241625762067;

    arm_down1[0] = -0.02957577082291;
    arm_down1[1] =  0.99955034883522;
    arm_down1[2] =  0.00493699540412;
    arm_down2[0] =  0.99956770750527;
    arm_down2[1] =  0.02940017976736;
    arm_down2[2] = -0.00016595877419;
    arm_down3[0] =  0.02952064420397;
    arm_down3[1] = -0.99955906286331;
    arm_down3[2] = -0.00319553025334;
    arm_down4[0] = -0.99959430058334;
    arm_down4[1] = -0.02847051690376;
    arm_down4[2] =  0.00081480582161;

                                // screw parameters
                                // from ~kapu/matlab/f99/second/xform.m
    dnssp = 212.052011651496;  // dist. for nod screw start pivot to axis
    dnsep = 181.973116924546;  // dist. for nod screw end   pivot to axis
    offn  = 041.630447912442;  // offset dist. for nod when screw length = -6
    dtssp = 208.506382766528;  // dist. for turn screw start pivot to axis
    dtsep = 181.006348828133;  // dist. for turn screw end   pivot to axis
    offt  = 035.807639467137;  // offset dist. for turn when screw length = -6
    minscrew = -6.0;

};
#else
void
CyberCalib::setDefault()
// Set all of the values to their compiled in defaults.
{
                                // turn axis and point on it.
    axturn[0] = -0.00113981711955;
    axturn[1] =  0.00438021200568;
    axturn[2] = -0.99998975722740;
    axturn0[0] =  564.786527057630;
    axturn0[1] =  568.240210030879;
    axturn0[2] = -124.331900000000;

                                // nod axis.  axis and point on it.
    axnod[0] = 0.64170209799692;
    axnod[1] = 0.76693964441216;
    axnod[2] = 0.00469033637535;
    axnod0[0] =  531.824138280106;
    axnod0[1] =  452.981000000000;
    axnod0[2] = -143.274367000629;


                                // optical correction matrix
    static const double default_A[] =
    {
        1.0,               0.0,                0.0,                0.0,
        0.0,  018.579261546745,  -000.088087061526,   000.000675546148,
        0.0,  000.278844123926,  -017.924459594106,   000.014749815686,
        0.0, -021.380307002909,   162.548102595433,   001.000000000000
    };
    memcpy( A, default_A, sizeof(A) );

                                // transforms from working value to scanner
    static const double default_opt_frm_v[] =
    {
        -000.586680931764,   000.487547825028,   000.646608538928,  0.0,
        -000.641134237921,  -000.767422605421,  -000.003071425476,  0.0,
        000.494724542821,  -000.416364819599,   000.762815812455,  0.0,
        998.060615792253,   155.870567377795,   924.974506168649,  1.0
    };
    static const double default_opt_frm_h[] =
    {
        000.087724034478,  000.837773482966,   000.538924934489,   0.0,
        -000.996126263463,  000.077077755152,   000.042325960125,   0.0,
        -000.006079557112, -000.540550285265,   000.841289740865,   0.0,
        915.077953743209, -100.746998128001,  -320.665670992974,   1.0
    };
    memcpy( opt_frm_v, default_opt_frm_v, sizeof(opt_frm_v) );
    memcpy( opt_frm_h, default_opt_frm_h, sizeof(opt_frm_h) );

                                // horizontal arm direction
    arm_up1[0] = -0.02791322161013;
    arm_up1[1] =  0.99960644654600;
    arm_up1[2] =  0.00279357889050;
    arm_up2[0] = -0.99960789687213;
    arm_up2[1] = -0.02800054167491;
    arm_up2[2] =  0.00014891875503;
    arm_up3[0] =  0.02790980783040;
    arm_up3[1] = -0.99959720870661;
    arm_up3[2] = -0.00514421741692;
    arm_up4[0] =  0.99960935645185;
    arm_up4[1] =  0.02784414108967;
    arm_up4[2] = -0.00241625762067;

    arm_down1[0] = -0.02957577082291;
    arm_down1[1] =  0.99955034883522;
    arm_down1[2] =  0.00493699540412;
    arm_down2[0] =  0.99956770750527;
    arm_down2[1] =  0.02940017976736;
    arm_down2[2] = -0.00016595877419;
    arm_down3[0] =  0.02952064420397;
    arm_down3[1] = -0.99955906286331;
    arm_down3[2] = -0.00319553025334;
    arm_down4[0] = -0.99959430058334;
    arm_down4[1] = -0.02847051690376;
    arm_down4[2] =  0.00081480582161;

                                // screw parameters
                                // from ~kapu/matlab/f99/second/xform.m
    dnssp = 210.9230287253126;  // dist. for nod screw start pivot to axis
    dnsep = 180.0069624362592;  // dist. for nod screw end   pivot to axis
    offn  = 39.38131386985114;  // offset dist. for nod when screw length = -6
    dtssp = 210.4423446244252;  // dist. for turn screw start pivot to axis
    dtsep = 180.0344270613032;  // dist. for turn screw end   pivot to axis
    offt  = 39.70382980035739;  // offset dist. for turn when screw length = -6
    minscrew = -6.0;
};
#endif


void
CyberCalib::setDerived()
// Compute the derived calibration values (which are used to optimize some
// computations in CyberXform.cc).
{
                                // pre-computed values for quickly calculating the
                                // angle from a screw value.  these are used by
                                // the SCREW_TO_ANGLE and ANGLE_TO_SCREW macros
                                // in CyberXform.cc
    n_h1 = offn - minscrew;
    n_h2 = dnssp*dnssp + dnsep*dnsep;
    n_h3 = 1.0 / (2.0 * dnssp * dnsep);
    n_h4 = acos((dnssp*dnssp + dnsep*dnsep - offn*offn)/(2*dnssp*dnsep));
    t_h1 = offt - minscrew;
    t_h2 = dtssp*dtssp + dtsep*dtsep;
    t_h3 = 1.0 / (2.0 * dtssp * dtsep);
    t_h4 = acos((dtssp*dtssp + dtsep*dtsep - offt*offt)/(2*dtssp*dtsep));
}

bool
CyberCalib::readNumbers( istream &in, RopeDV &r2dv )
// Reads in a file of the format
//
//    <string> <double> . . . .
//    [repeat ad nauseum]
//
// for each <string> makes a vector out of the following doubles and
// inserts the element <string> -> vector into the map m.
//
// RETURNS true if the file is successfully read,
//         false otherwise
{
    while( 1 )
    {
        in >> ws;
        if( in.peek() == EOF )
            return 1;
                                // read until next white space into rope
        crope s;
        while( in.peek() != EOF && !isspace(in.peek())  )
            s += (char) in.get();
        vector<double> &vd = r2dv[s];
        while( 1 )
        {
            double d;
            in >> ws >> d;
            if( in.fail() )
            {
                in.clear();
                break;
            }
            vd.push_back(d);
        }
        if( in.fail() )
            return 0;
    }
}


