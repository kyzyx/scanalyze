function ic=cmodel(sys,cpts,pos,par)
%CMODEL Camera model that generates synthetic image coordinates from 
%3-D coordinates and camera parameters. 
%
%Usage:
%   ic = cmodel(sys,cpts,pos,par)
%
%where       
%   sys  = system configuration information (see configc.m) 
%   cpts = coordinates for the 3-D control points
%   pos  = camera position and orientation
%   par  = camera intrinsic parameters:
%          par(1) = scale factor ~1
%          par(2) = effective focal length
%          par(3:4) = principal point
%          par(5:6) = radial distortion coefficients
%          par(7:8) = tangential distortion coefficients
%   ic   = synthetic image coordinates

%   Version 2.0  15.5.-97
%   Janne Heikkila, University of Oulu, Finland

NDX=sys(1); NDY=sys(2); Sx=sys(3); Sy=sys(4);

if length(pos)~=6
  error('Position vector should contain [x y z w p r].');
end

if length(par)~=8
  error('Parameter vector should be 1 x 8 matrix.');
end

if size(cpts,2)~=3
  error('Control point matrix should have three columns [xp yp zp]');
end

n=size(cpts,1);
Asp=par(1); Foc=par(2);
Cpx=par(3); Cpy=par(4);
Rad1=par(5); Rad2=par(6);
Tan1=par(7); Tan2=par(8);

M=eye(4);
wa=pos(4)*pi/180;
pa=pos(5)*pi/180;
ra=pos(6)*pi/180;
cw=cos(wa); sw=sin(wa);
cp=cos(pa); sp=sin(pa);
cr=cos(ra); sr=sin(ra);

M(1,:)=[cr*cp -sr*cw+cr*sp*sw sr*sw+cr*sp*cw pos(1)];
M(2,:)=[sr*cp cr*cw+sr*sp*sw -cr*sw+sr*sp*cw pos(2)];
M(3,:)=[-sp cp*sw cp*cw pos(3)];

c=M*[cpts';ones(1,n)];

x=[Foc.*c(1,:)./c(3,:)]';
y=[Foc.*c(2,:)./c(3,:)]';

r2=x.*x+y.*y;
delta=Rad1*r2+Rad2*r2.*r2;

xn=x.*(1+delta)+2*Tan1*x.*y+Tan2*(r2+2*x.*x); 
yn=y.*(1+delta)+Tan1*(r2+2*y.*y)+2*Tan2*x.*y; 
  
ic=NDX*Asp*xn/Sx+Cpx;
ic(:,2)=NDY*yn/Sy+Cpy;

