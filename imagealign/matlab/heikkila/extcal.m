function [pos,iter,res,er,C]=extcal(name,data,cpar)
%EXTCAL Calculates the extrinsic camera parameters for a single image 
%from 3D - 2D correspondences.
%
%Usage:
%   [pos,iter,res,er,C]=extcal(name,data,cpar)         
%
%where
%   name = string that is specific to the camera and the framegrabber.
%          This string must be defined in configc.m
%   data = matrix that contains the 3-D coordinates of the
%          control points (in fixed right-handed frame) and corresponding
%          image observations (in image frame, origin in the upper left
%          corner and the y-axis downwards) 
%          Dimensions: (n x 5) matrix, format: [wx wy wz ix iy]
%   cpar = intrinsic parameters of the camera (from calibration)
%   pos  = camera position and orientation
%   iter = number of iterations used
%   res  = residual (sum of squared errors)
%   er   = remaining error for each point
%   C    = error covariance matrix of the estimated parameters

%   Version 2.1b  25.08.1998
%   Janne Heikkila, University of Oulu, Finland

if ~isstr(name)
  error('The first argument should be the camera type');
end
sys=configc(name);
n=length(data);

if norm(data(:,3))
  [ipos,foc,u0,v0,b1,b2]=dlt(sys,data);
else
  [ipos,foc,u0,v0,b1,b2]=dlt2d(sys,data);
end

[pos,iter,res,er,J,succ]=lmoptc(sys,data,n,ipos(:),0,cpar);
q=ones(n,1)*std(reshape(er,n,2));
Q=spdiags(q(:).^2,1,2*n,2*n);
X=inv(J'*J)*J';
C=full(X*Q*X');
disp(sprintf('Standard error in pixels: %f',std(er(:))));
disp(sprintf('Standard deviation of the estimated extrinsic parameters:'));
disp(sprintf('%.5f ',sqrt(diag(C)')));
