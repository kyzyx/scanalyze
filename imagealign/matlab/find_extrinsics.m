function ret=find_extrinsics(filename, cameraname)

%
% Solve for the extrinsics of a camera (e.g. position and orientation)
% given an imXX_correspondences.m file of the form:
%   WORLDX1 WORLDY1 WORLDZ1 IMAGEX1 IMAGEY1
%   WORLDX2 WORLDY2 WORLDZ2 IMAGEX2 IMAGEY2
%   WORLDX3 WORLDY3 WORLDZ3 IMAGEX3 IMAGEY3
%   WORLDX4 WORLDY3 WORLDZ4 IMAGEX4 IMAGEY4
%   ...
% these are both returned and saved to a file with the same base name
% as the input file. Input foo.xyzuv, output foo.tsai. The input
% arg should just be 'foo'
% JED 3/12/99 - derived from lucasp stuff

% setup initial paths and internal calib
idealpar = load(strcat(cameraname,'.idealpar.fin'));
%load idealpar.fin 

% load the data for this filename
if (exist(strcat(filename,'.corr')))  
   xyzuv = loadxyzuvfile(strcat(filename,'.corr'));	
elseif (exist(strcat(filename,'.xyzuv')))
       xyzuv = loadxyzuvfile(strcat(filename,'.xyzuv'));
elseif (exist(filename))
       xyzuv = loadxyzuvfile(filename);
else
	disp('failed to load file');
	return;
end
  
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Solve for the first camera position.  Copy this code and
% Change all the "01" to "02" for the next image, etc...
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Load the camera calibration done using an "ideal" calibration target,
% not cyra data...

% Call heikkila's solver for extrinsics
[pos,iter,res,er,C]=extcal(cameraname, xyzuv, idealpar);

% Write the total camera calibration (intrinsics + extrinsics) as
% a tsai-format file that Szymon's pastecolor will understand.
sys=configc(cameraname);
tsaiparams=[sys(1:2); sys(3)/sys(1); sys(3)/sys(1); sys(4)/sys(2); sys(4)/sys(2); idealpar(3:4); idealpar(1:2); 0; pos(1:3,1); pos(4:6,1)*pi/180];
save (strcat(filename,'.tsai'),'tsaiparams','-ascii');
ret=tsaiparams;

