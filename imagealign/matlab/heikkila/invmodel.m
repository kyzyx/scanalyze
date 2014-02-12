function a=invmodel(name,par)
%INVMODEL calculates the parameters of the inverse camera model, which
%can be used to correct measured image coordinates to correspond a simple
%pinhole model.
%
%Usage:
%   a=invmodel(name,par)
%
%where
%   name = string that is specific to the camera and the framegrabber.
%          This string must be defined in configc.m
%   par  = camera intrinsic parameters
%   a    = eight implicit parameters of the inverse camera model

%   Version 2.0  15.5.-97
%   Janne Heikkila, University of Oulu, Finland

sys=configc(name);
NDX=sys(1); NDY=sys(2); Sx=sys(3); Sy=sys(4);
Asp=par(1); Foc=par(2);
Cpx=par(3); Cpy=par(4);
Rad1=par(5); Rad2=par(6);
Tan1=par(7); Tan2=par(8);

cpx=Cpx/NDX*Sx/Asp;
cpy=Cpy/NDY*Sy;

xs=-cpx*1.1; xe=(Sx-cpx)*1.1;
ys=-cpy*1.1; ye=(Sy-cpy)*1.1;
px=Sx/40; py=Sy/40;

[cx,cy]=meshgrid(xs:px:xe,ye:-py:ys); cx=cx(:); cy=cy(:);
r2=cx.*cx+cy.*cy;
delta=Rad1*r2+Rad2*r2.*r2;

dx=cx.*(1+delta)+2*Tan1*cx.*cy+Tan2*(r2+2*cx.*cx); 
dy=cy.*(1+delta)+Tan1*(r2+2*cy.*cy)+2*Tan2*cx.*cy; 

r2=dx.*dx+dy.*dy;
  
Tx=[-dx.*r2 -dx.*r2.^2 -2*dx.*dy -(r2+2*dx.^2) cx.*r2.^2 cx.*dx.*r2 ...
dy.*cx.*r2 cx.*r2];
Ty=[-dy.*r2 -dy.*r2.^2 -(r2+2*dy.^2) -2*dx.*dy cy.*r2.^2 cy.*dx.*r2 ...
cy.*dy.*r2 cy.*r2];
T=[Tx;Ty];
e=[dx-cx;dy-cy];
a=pinv(T)*e;
 
