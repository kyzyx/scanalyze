@echo off
del scanalyze-win32.zip
pkzip25 -add scanalyze-win32.zip scanalyze.*.exe *.tcl *.xbm
pkzip25 -add scanalyze-win32.zip ..\runtime-dll\*
pkzip25 -add -recurse -dir=specify -exclude=cvs scanalyze-win32.zip imagealign\*
