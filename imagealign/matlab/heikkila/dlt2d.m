function [pos,foc,u0,v0,b1,b2]=dlt2d(sys,data)
%DLT2D Direct linear transform for coplanar control points, which
%calculates the initial camera position for nonlinear optimization.
%
%Usage:
%   [pos,foc,u0,v0,b1,b2] = dlt2d(sys,data)         
%
%where       
%   sys  = system configuration information (see configc.m) 
%   data = matrix that contains the 3-D coordinates of the
%          control points (in fixed right-handed frame) and corresponding
%          image observations (in image frame, origo in the upper left
%          corner and the y-axis downwards). 
%          Dimensions: (n x 5) matrix, format: [wx wy 0 ix iy]
%          NOTE: the calibration frame should be selected so that
%                the z-coordinates of the control points are zero
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
u0=Sx/2; v0=Sy/2;

u = Sx*data(:,4)/NDX;
v = Sy*data(:,5)/NDY;

Lu=[wx wy 0*u+1 0*u 0*u 0*u -wx.*u -wy.*u];
Lv=[0*v 0*v 0*v wx wy 0*v+1 -wx.*v -wy.*v];
L=reshape([Lu';Lv'],8,2*num)';
l=reshape([u';v'],2*num,1);

a=pinv(L)*l;
a(9)=1;
A=reshape(a,3,3)';

a11=(A(3,1)^2-A(3,2)^2)*(u0^2+v0^2)-2*(A(1,1)*A(3,1)-A(1,2)*A(3,2))*u0-...
2*(A(2,1)*A(3,1)-A(2,2)*A(3,2))*v0+A(1,1)^2-A(1,2)^2+A(2,1)^2-A(2,2)^2;

a12=(A(3,1)^2-A(3,2)^2)*(u0^2-v0^2)-2*(A(1,1)*A(3,1)-A(1,2)*A(3,2))*u0+...
2*(A(2,1)*A(3,1)-A(2,2)*A(3,2))*v0+A(1,1)^2-A(1,2)^2-A(2,1)^2+A(2,2)^2;

a13=2*(A(3,1)^2-A(3,2)^2)*u0*v0-2*(A(2,1)*A(3,1)-A(2,2)*A(3,2))*u0-...
2*(A(1,1)*A(3,1)-A(1,2)*A(3,2))*v0+2*(A(1,1)*A(2,1)-A(1,2)*A(2,2));

a14=(A(3,1)^2-A(3,2)^2)*(u0^2+v0^2+f0^2)-2*(A(1,1)*A(3,1)-A(1,2)*A(3,2))*u0-...
2*(A(2,1)*A(3,1)-A(2,2)*A(3,2))*v0+A(1,1)^2-A(1,2)^2+A(2,1)^2-A(2,2)^2;

a21=A(3,1)*A(3,2)*(u0^2+v0^2)-(A(1,1)*A(3,2)+A(1,2)*A(3,1))*u0-...
(A(2,1)*A(3,2)+A(2,2)*A(3,1))*v0+A(1,1)*A(1,2)+A(2,1)*A(2,2);

a22=A(3,1)*A(3,2)*(u0^2-v0^2)-(A(1,1)*A(3,2)+A(1,2)*A(3,1))*u0+...
(A(2,1)*A(3,2)+A(2,2)*A(3,1))*v0+A(1,1)*A(1,2)-A(2,1)*A(2,2);

a23=2*A(3,1)*A(3,2)*u0*v0-(A(2,1)*A(3,2)+A(2,2)*A(3,1))*u0-...
(A(1,1)*A(3,2)+A(1,2)*A(3,1))*v0+A(1,1)*A(2,2)+A(1,2)*A(2,1);

a24=A(3,1)*A(3,2)*(u0^2+v0^2+f0^2)-(A(1,1)*A(3,2)+A(1,2)*A(3,1))*u0-...
(A(2,1)*A(3,2)+A(2,2)*A(3,1))*v0+A(1,1)*A(1,2)+A(2,1)*A(2,2);


if a11 & a21
  t12=a12/a11; t13=a13/a11; t14=a14/a11;
  s22=a22/a21; s23=a23/a21; s24=a24/a21;
  at=4*(t12-s22)^2+4*(t13-s23)^2;
  bt=4*(t13*t14-t13*s24-s23*t14+s23*s24+2*t12^2*s23-2*t12*s22*t13-...
  2*t12*s22*s23+2*t13*s22^2);
  ct=t14^2-2*t14*s24+s24^2-4*t14*t12*s22+4*t14*s22^2+4*t12^2*s24-4*t12*s22*s24;
  b2a=(-bt-sqrt(bt^2-4*at*ct))/(2*at);
  b2b=(-bt+sqrt(bt^2-4*at*ct))/(2*at);
  b1a=-(2*(a13*a21-a23*a11)*b2a+a14*a21-a24*a11)/(2*a12*a21-2*a22*a11);

  b1b=-(2*(a13*a21-a23*a11)*b2b+a14*a21-a24*a11)/(2*a12*a21-2*a22*a11);
  if abs(b1a)<abs(b1b)
    b1=b1a; b2=b2a;
  else
    b1=b1b; b2=b2b;
  end
else
  if ~a11 & ~a21
    b2=1/2*(a12*a24-a22*a14)/(-a12*a23+a22*a13);
    b1=-1/2*(a13*a24-a14*a23)/(-a12*a23+a22*a13);
  else
    if a11
      at=4*a23^2*a11+4*a11*a22^2;
      bt=-8*a12*a22*a23+4*a24*a23*a11+8*a13*a22^2;
      ct=-4*a12*a22*a24+a11*a24^2+4*a14*a22^2;
      b2a=(-bt-sqrt(bt^2-4*at*ct))/(2*at);
      b2b=(-bt+sqrt(bt^2-4*at*ct))/(2*at);
      b1a=-1/2*(2*a23*b2a+a24)/a22;
      b1b=-1/2*(2*a23*b2b+a24)/a22;
    else
      at=4*a21*a13^2+4*a21*a12^2;
      bt=-8*a12*a22*a13+4*a21*a13*a14+8*a23*a12^2;
      ct=-4*a12*a22*a14+a21*a14^2+4*a24*a12^2;
      b2a=(-bt-sqrt(bt^2-4*at*ct))/(2*at);
      b2b=(-bt+sqrt(bt^2-4*at*ct))/(2*at);
      b1a=-1/2*(2*a13*b2a+a14)/a12;
      b1b=-1/2*(2*a13*b2b+a14)/a12;
    end
    if abs(b1a)<abs(b1b)
      b1=b1a; b2=b2a;
    else
      b1=b1b; b2=b2b;
    end
  end
end
    
m11=(A(1,1)*(1+b1)+A(2,1)*b2-A(3,1)*(1+b1)*u0-A(3,1)*b2*v0)/f0;
m21=(A(1,1)*b2+A(2,1)*(1-b1)-A(3,1)*b2*u0-A(3,1)*(1-b1)*v0)/f0;
m31=A(3,1);
m12=(A(1,2)*(1+b1)+A(2,2)*b2-A(3,2)*(1+b1)*u0-A(3,2)*b2*v0)/f0;
m22=(A(1,2)*b2+A(2,2)*(1-b1)-A(3,2)*b2*u0-A(3,2)*(1-b1)*v0)/f0;
m32=A(3,2);

lambda=sqrt(m11^2+m21^2+m31^2);
m1=[m11 m21 m31]'/lambda;
m2=[m12 m22 m32]'/lambda;
m3=cross(m1,m2);
M=[m1 m2 m3];

t0=(A(1,3)*((1+b1)*M(1,:)+b2*M(2,:))+A(2,3)*(b2*M(1,:)+(1-b1)*M(2,:))-...
A(3,3)*((1+b1)*M(1,:)*u0+b2*M(2,:)*u0+b2*M(1,:)*v0+(1-b1)*M(2,:)*v0-...
M(3,:)*f0))/(lambda*f0);

t=M*t0';
if t(3)<0, t0(3)=-t0(3); t=M*t0'; end
pa=asin(M(3,1));
wa=atan2(-M(3,2)/cos(pa),M(3,3)/cos(pa));
ra=atan2(-M(2,1)/cos(pa),M(1,1)/cos(pa));
pos=[t' -[wa pa ra]*180/pi];
foc=f0; u0=NDX/2; v0=NDY/2;
