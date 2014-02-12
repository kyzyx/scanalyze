function [pos,foc,u0,v0,b1,b2]=dlt(sys,data)
%DLT Direct linear transform, which calculates the initial camera 
%parameters for nonlinear optimization.
%         
%Usage:
%   [pos,foc,u0,v0,b1,b2] = dlt(sys,data)  
%
%where           
%   sys  = system configuration information (see configc.m) 
%   data = matrix that contains the 3-D coordinates of the
%          control points (in fixed right-handed frame) and corresponding
%          image observations (in image frame, origo in the upper left
%          corner and the y-axis downwards) 
%          Dimensions: (n x 5) matrix, format: [wx wy wz ix iy]
%          NOTE: control points should not be coplanar
%   pos  = camera position and orientation [x y z w p r]
%   foc  = effective focal length (obtained from sys)
%   u0, v0 = principal point (obtained from sys)
%   b1, b2 = linear distortion coefficients

%Reference:
%   Trond Melen: Geometrical modelling and calibration of video
%   cameras for underwater navigation, Ph.D.-thesis, 
%   ITK-rapport 1994:103-W, NTH, Norway.           
%
%   Version 2.0  15.5.-97
%   Janne Heikkila, University of Oulu, Finland

NDX=sys(1); NDY=sys(2); Sx=sys(3); Sy=sys(4); f0=sys(5);
wx=data(:,1); wy=data(:,2); wz=data(:,3);
num=size(data,1);

u = Sx*data(:,4)/NDX;
v = Sy*data(:,5)/NDY;

Lu=[wx wy wz 0*u+1 0*u 0*u 0*u 0*u -wx.*u -wy.*u -wz.*u];
Lv=[0*v 0*v 0*v 0*v wx wy wz 0*v+1 -wx.*v -wy.*v -wz.*v];
L=reshape([Lu';Lv'],11,2*num)';
l=reshape([u';v'],2*num,1);

a=pinv(L)*l;
a(12)=1;
A=reshape(a,4,3)';
t=inv(A(:,1:3))*A(:,4);

lambda=sqrt(A(3,1)^2+A(3,2)^2+A(3,3)^2);
[Q,R]=qr(inv(A(:,1:3)/lambda));
R=inv(R); Q=Q';
J=diag(sign(diag(R)));
R=R*J;

tth=-R(1,2)/(R(1,1)+R(2,2));
sth=tth/sqrt(1+tth^2); cth=1/sqrt(1+tth^2);
S=[cth sth 0;-sth cth 0;0 0 1];
M=S'*J*Q;

lambda=det(M)*lambda;
M=det(M)*M;
G=R*S;

b1=-(G(1,1)-G(2,2))/(G(1,1)+G(2,2));
b2=-2*G(1,2)/(G(1,1)+G(2,2));
foc=2*(G(1,1)*G(2,2)-G(1,2)^2)/(G(1,1)+G(2,2));
u0=G(1,3)*NDX/Sx; v0=G(2,3)*NDY/Sy;

t=M*t;
pa=asin(M(3,1));
wa=atan2(-M(3,2)/cos(pa),M(3,3)/cos(pa));
ra=atan2(-M(2,1)/cos(pa),M(1,1)/cos(pa));
pos=[t' -[wa pa ra]*180/pi];
