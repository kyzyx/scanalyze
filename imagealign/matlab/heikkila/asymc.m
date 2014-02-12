function [cdata,dpr]=asymc(sys,par,pos,data,nbr,snorm)
%ASYMC Routine that corrects the effect of the asymmetric projection
%of the circular control points. 
%
%Usage:
%   [cdata,dpr]=asymc(sys,par,pos,data,nbr,snorm)
%
%where
%   sys  = system configuration information (see configc.m) 
%   par  = approximation of the intrinsic parameters of the camera
%   pos  = approximation of the camera positions and orientations 
%          (6 x m matrix)
%   data = matrix that contains the 3-D coordinates of the
%          control points (in fixed right-handed frame) and corresponding
%          image observations (in image frame, origo in the upper left
%          corner) 
%          Dimensions: (n x 5) matrix, format: [wx wy wz ix iy]
%   nbr  = number of points per frame
%   snorm = normal vectors for each point (n x 3 matrix)
%   cdata = corrected data matrix
%   dpr  = error term

%   Version 2.0  15.5.-97
%   Janne Heikkila, University of Oulu, Finland

NDX=sys(1); NDY=sys(2); Sx=sys(3); Sy=sys(4);
radius=sys(6); Asp=par(1); foc=par(2); n=length(data);
v1=-snorm;
vv=randn(3,1); vv=vv/norm(vv);
v2=ones(n,1)*vv'-v1*[vv vv vv].*v1;
nnn=sqrt(sum(v2'.^2))';
v2=v2./[nnn nnn nnn];
v3=[v1(:,2).*v2(:,3)-v1(:,3).*v2(:,2) ...
    v1(:,3).*v2(:,1)-v1(:,1).*v2(:,3) ...
    v1(:,1).*v2(:,2)-v1(:,2).*v2(:,1)];


ind=cumsum([1 nbr]);
dpr=zeros(n,2);
num=size(pos,2);

for i=1:num  
  M=eye(4);
  wa=pos(4,i)*pi/180;
  pa=pos(5,i)*pi/180;
  ra=pos(6,i)*pi/180;
  cw=cos(wa); sw=sin(wa);
  cp=cos(pa); sp=sin(pa);
  cr=cos(ra); sr=sin(ra);

  M(1,:)=[cr*cp -sr*cw+cr*sp*sw sr*sw+cr*sp*cw pos(1,i)];
  M(2,:)=[sr*cp cr*cw+sr*sp*sw -cr*sw+sr*sp*cw pos(2,i)];
  M(3,:)=[-sp cp*sw cp*cw pos(3,i)];
  iM=inv(M);
  cpos=iM(1:3,4);
  cx=-v2(ind(i):ind(i+1)-1,:)*cpos;
  cy=-v3(ind(i):ind(i+1)-1,:)*cpos;
  cz=-v1(ind(i):ind(i+1)-1,:)*cpos;
  A1=[v2(ind(i):ind(i+1)-1,:) cx]*iM;
  A2=[v3(ind(i):ind(i+1)-1,:) cy]*iM;
  A3=[v1(ind(i):ind(i+1)-1,:) cz]*iM;
  mod=data(ind(i):ind(i+1)-1,1:3);
  dx=mod(:,1).*v2(ind(i):ind(i+1)-1,1)+mod(:,2).*v2(ind(i):ind(i+1)-1,2)+...
     mod(:,3).*v2(ind(i):ind(i+1)-1,3)+cx;
  dy=mod(:,1).*v3(ind(i):ind(i+1)-1,1)+mod(:,2).*v3(ind(i):ind(i+1)-1,2)+...
     mod(:,3).*v3(ind(i):ind(i+1)-1,3)+cy;
  dz=mod(:,1).*v1(ind(i):ind(i+1)-1,1)+mod(:,2).*v1(ind(i):ind(i+1)-1,2)+...
     mod(:,3).*v1(ind(i):ind(i+1)-1,3)+cz;
  alpha=dx./dz;
  beta=dy./dz;
  gamma=radius./dz;
  kt=A1(:,1)-alpha.*A3(:,1); lt=A1(:,2)-alpha.*A3(:,2);
  mt=(A1(:,3)-alpha.*A3(:,3))*foc; nt=A2(:,1)-beta.*A3(:,1);
  pt=A2(:,2)-beta.*A3(:,2); qt=(A2(:,3)-beta.*A3(:,3))*foc;
  rt=gamma.*A3(:,1); st=gamma.*A3(:,2); tt=gamma.*A3(:,3)*foc;
  uc=(kt.*pt-nt.*lt).*(lt.*qt-pt.*mt)-(kt.*st-lt.*rt).*(tt.*lt-mt.*st)-...
     (nt.*st-pt.*rt).*(tt.*pt-qt.*st);
  vc=(kt.*pt-nt.*lt).*(mt.*nt-kt.*qt)-(kt.*st-lt.*rt).*(mt.*rt-kt.*tt)-...
     (nt.*st-pt.*rt).*(qt.*rt-nt.*tt);
  den=(kt.*pt-nt.*lt).^2-(kt.*st-lt.*rt).^2-(nt.*st-pt.*rt).^2;
  uc=uc./den; vc=vc./den;
  u0=(lt.*qt-pt.*mt)./(kt.*pt-nt.*lt);
  v0=(mt.*nt-kt.*qt)./(kt.*pt-nt.*lt);
  dpr(ind(i):ind(i+1)-1,1)=NDX*Asp*(uc-u0)/Sx;
  dpr(ind(i):ind(i+1)-1,2)=NDY*(vc-v0)/Sy;
end
cdata=[data(:,1:3) data(:,4:5)-dpr];
