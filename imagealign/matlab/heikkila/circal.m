function [par,pos,iter,res,er,C]=circal(name,data1,data2,data3,data4,data5,data6)
%CIRCAL Three-step calibration routine for computing the camera parameters. 
%In the first step the initial values are solved by using the DLT method.
%The second step contains nonlinear minimization and in the third step
%the accuracy is improved by taking into account the asymmetric projection
%of the circular control points.      
%
%Usage:
%   [par,pos,iter,res,er,C]=circal(name,data1,data2,data3,data4,data5,data6)
%
%where
%   name = string that is specific to the camera and the framegrabber.
%          This string must be defined in configc.m
%   data1...data6 = matrices that contain the 3-D coordinates of the
%          control points (in fixed right-handed frame) and corresponding
%          image observations (in image frame, origo in the upper left
%          corner). In addition the surface normal of each point must
%          be given.
%          dimensions: (n x 8) matrices,
%          row format: [wx wy wz ix iy nx ny nz],
%          units: mm for control points, pixels for image points,
%          min. number of images = 1 (requires 3-D control point structure)
%          max. number of images = 6
%   par  = camera intrinsic parameters
%   pos  = camera position and orientation for each image (n x 6 matrix)
%   iter = number of iterations used
%   res  = residual (sum of squared errors)
%   er   = remaining error for each point
%   C    = error covariance matrix of the estimated parameters

%   Version 2.0  15.5.-97
%   Janne Heikkila, University of Oulu, Finland

num=nargin-1;
if ~isstr(name)
  error('The first argument should be the camera type');
end
sys=configc(name);

data=[]; obs=[]; sdata=[]; snorm=[]; ipos=[]; tic;

fprintf(1,'step 1: '); %step 1
for i=1:num
  dt=eval(['data' num2str(i)]);
  if size(dt,2)~=8
    error('Data matrix should contain the surface normal');
  end
  data=[data;dt(:,1:3)]; obs=[obs;dt(:,4:5)]; snorm=[snorm;dt(:,6:8)];
  sdata=[sdata;size(dt,1)];
  if norm(dt(:,3))
    [ip,foc,u0,v0,b1,b2]=dlt(sys,dt(:,1:5));
  else
    [ip,foc,u0,v0,b1,b2]=dlt2d(sys,dt(:,1:5));
  end
  ipos=[ipos ip(:)];
end
fprintf(1,'done\n');

nbr=sdata'; n=sum(sdata);
iparam=[1 foc u0 v0 0 0 0 0 ipos(:)'];
fprintf(1,'step 2: '); %step 2
[param,iter,res,er,J,success]=lmoptc(sys,[data obs],nbr,iparam);
if success
  par=param(1:8);
  pos=reshape(param(9:length(param)),6,num);
  fprintf(1,'step 3: '); %step 3
  [cdata,dpr]=asymc(sys,par,pos,[data obs],nbr,snorm);
  [param,iter2,res,er,J,success2]=lmoptc(sys,cdata,nbr,param);
  if success2
    q=ones(n,1)*std(reshape(er,n,2));
    Q=spdiags(q(:).^2,1,2*n,2*n);
    X=inv(J'*J)*J';
    C=full(X*Q*X');
    par=param(1:8);
    pos=reshape(param(9:length(param)),6,num);
    disp('Calibration was successfully completed. Here are the results:');
    disp(sprintf('\nCamera parameters (%s):',sys(10:length(sys))));
    disp('==================');
    disp(sprintf('Scale factor: %.4f   Effective focal length: %.4f mm',...
    par(1),par(2)));
    disp(sprintf('Principal point: (%.4f,%.4f)',par(3),par(4)));
    disp(sprintf('Radial distortion:     K1 = %e  K2 = %e',par(5),par(6)));
    disp(sprintf('Tangential distortion: T1 = %e  T2 = %e',par(7),par(8)));
    disp(sprintf('\nOther information:'));
    disp('==================');
    disp(sprintf('Standard error in pixels: %f',std(er(:))));
    disp(sprintf('Standard deviation of the estimated intrinsic parameters:'));
    disp(sprintf('%.2e ',sqrt(diag(C(1:8,1:8))')));
    disp(sprintf('Number of iterations: %d',iter+iter2));
    disp(sprintf('Elapsed time: %.1f sec.',toc));
  else
    disp('Sorry, calibration failed in step 3');
  end
else
  disp('Sorry, calibration failed in step 2');
end
