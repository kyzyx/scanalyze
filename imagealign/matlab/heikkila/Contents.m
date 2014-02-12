% Calibration toolbox v2.1b
%
% Calibration:
%
% cacal    calibration routine for solving camera parameters
% circal   calibration routine for circular control points
% invmodel computes the parameters of the inverse distortion model
% imcorr   corrects distorted image coordinates
% imdist   adds radial and tangential distortion to image coordinates
% extcal   computes the extrinsic camera parameters for a calibrated camera
% configc  gives the configuration information
%
% Subroutines:
%
% dlt      direct linear transform
% dlt2d    direct linear transform for coplanar targets
% asymc    corrects the distortion in circular control point coordinates
% cmodel   camera model with radial and tangential distortion
% frames   camera model for multiple views
% lmoptc   Levenberg-Marquardt optimization for cacal and circal
% jacobc   produces the Jacobian matrix for optimization
%
% Demonstration:
%
% caldemo  performs two example calibrations

% Author: Janne Heikkila, University of Oulu, Finland

