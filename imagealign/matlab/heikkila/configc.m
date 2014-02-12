function sys=configc(name)
%CONFIGC Loads system configuration information based on given name. 
%User may edit here new setup data.

%%% Full-size DKCST5, landscape orientation
if strcmp(name,'DKCST5_rawpixels')
  sys = [
      1030,    %number of pixels in horizontal direction
      1300,    %number of pixels in vertical direction
      6,           %effective CCD chip size in horizontal direction
      7.57281553,  %effective CCD chip size in vertical direction
      12.5,    %nominal focal length
      11,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

%%% Full-size DKCST5, landscape orientation
if strcmp(name,'DKCST5')
  sys = [
      2560,    %number of pixels in horizontal direction
      2048,    %number of pixels in vertical direction
      7.5,     %effective CCD chip size in horizontal direction
      6,       %effective CCD chip size in vertical direction
      12.5,    %nominal focal length
      11,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

%%% Full-size DKCST5, vertical (portrait) orientation
if strcmp(name,'DKCST5_V')
  sys = [
      2048,    %number of pixels in horizontal direction
      2560,    %number of pixels in vertical direction
      6,     %effective CCD chip size in horizontal direction
      7.5,       %effective CCD chip size in vertical direction
      12.5,    %nominal focal length
      11,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

%%% Half-size DKCST5, landscape orientation
if strcmp(name,'DKCST5_2')
  sys = [
      1280,    %number of pixels in horizontal direction
      1024,    %number of pixels in vertical direction
      7.5,     %effective CCD chip size in horizontal direction
      6,       %effective CCD chip size in vertical direction
      12.5,    %nominal focal length
      11,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end


%%% Half-size DKCST5, Vertical orientation
if strcmp(name,'DKCST5_2V')
  sys = [
      1024,    %number of pixels in horizontal direction
      1280,    %number of pixels in vertical direction
      6,     %effective CCD chip size in horizontal direction
      7.5,       %effective CCD chip size in vertical direction
      12.5,    %nominal focal length
      11,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

%%% DKCST5, after playing with it for video resolution.
%%% Basically, padded with black bars to be 1366x1024,
%%% and then scaled back down to 640x480
if strcmp(name,'DKCST5_640x480')
  sys = [
      640,    %number of pixels in horizontal direction
      480,    %number of pixels in vertical direction
      8,     %effective CCD chip size in horizontal direction
      6,       %effective CCD chip size in vertical direction
      13.6,    %nominal focal length
      11,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

if strcmp(name,'DKC5000')
  sys = [
      1520,    %number of pixels in horizontal direction
      1144,    %number of pixels in vertical direction
      6.3840,  %effective CCD chip size in horizontal direction
      4.8048,  %effective CCD chip size in vertical direction
      25.0,    %nominal focal length
      2,       %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

if strcmp(name,'DKC5000_gantry')
  sys = [
      1520,    %number of pixels in horizontal direction
      1144,    %number of pixels in vertical direction
      6.3840,  %effective CCD chip size in horizontal direction
      4.8048,  %effective CCD chip size in vertical direction
      25.0,    %nominal focal length
      2,       %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

if strcmp(name,'DKC5000_chapel')
  sys = [
      1520,    %number of pixels in horizontal direction
      1144,    %number of pixels in vertical direction
      6.3840,  %effective CCD chip size in horizontal direction
      4.8048,  %effective CCD chip size in vertical direction
      25.0,    %nominal focal length
      2,       %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

if strcmp(name,'Toshiba')
  sys = [
      640,     %number of pixels in horizontal direction
      480,     %number of pixels in vertical direction
      4.8,     %effective CCD chip size in horizontal direction
      3.6,     %effective CCD chip size in vertical direction
      20.0,    %nominal focal length
      3,       %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end



if strcmp(name,'sony')
  sys = [
      768,     %number of pixels in horizontal direction
      576,     %number of pixels in vertical direction
      6.2031,  %effective CCD chip size in horizontal direction
      4.6515,  %effective CCD chip size in vertical direction
      8.5,     %nominal focal length
      5,       %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end
      
if strcmp(name,'pulnix')
  sys = [
      512,     %number of pixels in horizontal direction
      512,     %number of pixels in vertical direction
      4.3569,  %effective CCD chip size in horizontal direction
      4.2496,  %effective CCD chip size in vertical direction
      16,      %nominal focal length
      15,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

if strcmp(name,'generic')
  sys = [
      640,     %number of pixels in horizontal direction
      480,     %number of pixels in vertical direction
      6.3840,  %effective CCD chip size in horizontal direction
      4.8048,  %effective CCD chip size in vertical direction
      8,       %nominal focal length
      15,      %radius of the circular control points
      0,       %for future expansions
      0,
      0,
      abs(name)'
  ];
  return;
end

error('Unknown camera type')
