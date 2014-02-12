function p=imcorr(name,par,a,dp)
%IMCORR corrects image coordinates, which are contaminated by radial
%and tangential distortion.
%
%Usage:
%   p=imcorr(name,par,a,dp)
%
%where
%   name = string that is specific to the camera and the framegrabber.
%          This string must be defined in configc.m
%   par  = camera intrinsic parameters
%   a    = eight implicit parameters of the inverse camera model. These
%          parameters are computed by using invmodel.m.
%   dp   = distorted image coordinates in pixels (n x 2 matrix)
%   p    = corrected image coordinates 

%   Version 2.0  15.5.-97
%   Janne Heikkila, University of Oulu, Finland

sys=configc(name);
NDX=sys(1); NDY=sys(2); Sx=sys(3); Sy=sys(4);
Asp=par(1); Cpx=par(3); Cpy=par(4);
a=a(:);

cx=(dp(:,1)-Cpx)*Sx/NDX/Asp;
cy=(dp(:,2)-Cpy)*Sy/NDY;

r2=cx.*cx+cy.*cy;
G=[r2.*r2 cx.*r2 cy.*r2 r2]*a(5:8)+1;
x=(cx+cx.*(a(1)*r2+a(2)*r2.*r2)+2*a(3)*cx.*cy+a(4)*(r2+2*cx.^2))./G;
y=(cy+cy.*(a(1)*r2+a(2)*r2.*r2)+2*a(4)*cx.*cy+a(3)*(r2+2*cy.^2))./G;

p=NDX*Asp*x/Sx+Cpx;
p(:,2)=NDY*y/Sy+Cpy;
