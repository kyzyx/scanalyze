function [p,iter,res,er,J,succ]=lmoptc(sys,data,nbr,p0,np,cpar)
%LMOPTC Performs the Levenberg-Marquardt nonlinear optimization for
%camera calibration.
%
%Usage:
%   [p,iter,res,er,J,succ]=lmoptc(sys,data,nbr,p0)
%
%where       
%   sys  = system configuration information (see configc.m) 
%   data = matrix that contains the 3-D coordinates of the
%          control points (in fixed right-handed frame) and corresponding
%          image observations (in image frame, origo in the upper left
%          corner) 
%          Dimensions: (n x 5) matrix, format: [wx wy wz ix iy]
%   nbr  = number of points per frame
%   p0   = initial parameter vector (8+N*6 x 1)
%          p(1:8) contains the camera intrinsic parameters
%          p(9...) contains the camera position and orientation for
%          each N images. 
%   p    = resulting parameter vector
%   iter = number of iterations used (min=10)
%   res  = residual (sum of squared errors)
%   er   = remaining error for each point
%   J    = Jacobian matrix
%   succ = successful optimization (1 = true, 0 = false)

%   Version 2.1b  25.08.1998
%   Janne Heikkila, University of Oulu, Finland

miter=10; lim=100;
n=length(p0);
p=p0(:);
if nargin==4
  np=8; cpar=[];
end
if nargin==5
  cpar=[];
end
mod=data(:,1:3); y=data(:,4:5); y=y(:);
[f,J]=jacobc(sys,mod,nbr,p,np,cpar);
er=y-f; res=er'*er; lambda=0.001;
beta=J'*er; alpha=J'*J;
pres=res; pp=p; succ=0;
%disp(sprintf('%d %.4f %.4e',0,res,lambda));
fprintf(1,'iterating');

for i=1:lim
  ind=1:(n+1):(n*n);
  tt=alpha(ind);
  alpha(ind)=tt*(1+lambda);
  dp=alpha\beta;
  p=p+dp;
  [f,J]=jacobc(sys,mod,nbr,p,np,cpar);
  er=y-f; res=er'*er;
  if abs(res-pres)<1e-4 & i>miter, succ=1; break; end
  if res<pres
    lambda=0.1*lambda;
    if lambda<1e-7,lambda=1e-7; end;
    beta=J'*er; alpha=J'*J;
    pres=res; pp=p;
  else
    lambda=10*lambda;
    alpha(ind)=tt;
    res=pres; p=pp;
  end
  %disp(sprintf('%d %.4f %.4e',i,res,lambda));
  fprintf(1,'.');
end
disp('done');  
iter=i;
