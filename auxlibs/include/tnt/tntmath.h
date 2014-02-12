// Template Numerical Toolkit (TNT) for Linear Algebra
//
// BETA VERSION INCOMPLETE AND SUBJECT TO CHANGE
// Please see http://math.nist.gov/tnt for updates
//
// R. Pozo
// Mathematical and Computational Sciences Division
// National Institute of Standards and Technology



// Header file for scalar math functions

#ifndef TNTMATH_H
#define TNTMATH_H

// conventional functions required by several matrix algorithms



inline double sign(double a)
{
    return (a > 0 ? 1 : -1);
}

inline float sign(float a)
{
    return (a > 0 ? 1 : -1);
}




#endif
// TNTMATH_H
