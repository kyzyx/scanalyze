#!gmake

# these aren't needed and cause lots of uncessary file opens
# that are slow across the net
.PREFIXES:
.SUFFIXES:

# Load a user pref file
-include Makefile.local


###################################################################
# Platform-specific definitions
###################################################################

SKIPCVS = 1  # can be overridden on command line, 'make SKIPCVS=0'

ifdef WINDIR
  windir=$(WINDIR)
endif

ifdef windir

include Makedefs.Win32

#default all: $(CVSFILES) scanalyze.exe
default: $(CVSFILES) $(DEFAULTVERSION)

all: $(CVSFILES) $(ALLVERSIONS)

OBJS $(addprefix OBJS/,$(ALLVERSIONS)) :
	mkdir $@

$(ALLVERSIONS): % : OBJS OBJS/%
	$(MAKE) scanalyze.$@.exe BUILD=$@

else   # UNIX, of some flavor



VPATH = ../..

JOBS = 1     # can be overridden on command line, 'make JOBS=x'

UNAME = $(subst IRIX64,IRIX,$(shell uname))

include Makedefs.$(UNAME)

default: $(CVSFILES) $(DEFAULTVERSION)

all: $(CVSFILES) $(ALLVERSIONS)

OBJS $(addprefix OBJS/,$(ALLVERSIONS)) :
	mkdir $@

$(ALLVERSIONS): % : OBJS OBJS/%
	$(MAKE) -j $(JOBS) scanalyze.$@ BUILD=$@ \
		--directory=OBJS/$@ --makefile=../../Makefile -I../.. SKIPCVS=1

endif # Win32 / UNIX


###################################################################
# Source files
###################################################################

CSRCS = togl.c accpersp.c jitter.c cyfile.c strings.c cmdassert.c
ifdef windir
CSRCS += tkConsole.c winMain.c
endif

CXXSRCS = plvMain.cc plvCmds.cc plvDraw.cc Mesh.cc \
	plvScene.cc plvGlobals.cc \
	RangeGrid.cc plvInit.cc Image.cc \
	plvImageCmds.cc plvDrawCmds.cc plvViewerCmds.cc \
	plvPlyCmds.cc plvClipBoxCmds.cc \
	plvMeshCmds.cc plvCybCmds.cc plvMMCmds.cc \
	Trackball.cc KDindtree.cc \
	plyfile.cc ToglHash.cc sczRegCmds.cc \
	absorient.cc plvAnalyze.cc ColorUtils.cc \
	GlobalReg.cc DisplayMesh.cc VertexFilter.cc \
	ResolutionCtrl.cc RigidScan.cc GenericScan.cc \
	CyberScan.cc CyraScan.cc MMScan.cc TriMeshUtils.cc \
	VolCarve.cc SyntheticScan.cc Progress.cc \
	MCEdgeTable.cc fitplane.cc \
	ToglCache.cc plycrunch.cc CyraSubsample.cc\
	TriangleCube.cc ScanFactory.cc GroupScan.cc \
	CyraResLevel.cc	KDtritree.cc CyberXform.cc CyberCalib.cc \
	CyberCmds.cc TbObj.cc GroupUI.cc BailDetector.cc \
	MeshTransport.cc SDfile.cc TextureObj.cc RefCount.cc \
	cameraparams.cc ProxyScan.cc WorkingVolume.cc \
	ToglText.cc Projector.cc OrganizingScan.cc \
	TclCmdUtils.cc

SCRIPTS = scanalyze.tcl build_ui.tcl interactors.tcl windows.tcl\
	analyze.tcl clip.tcl registration.tcl res_ctrl.tcl\
	file.tcl tcl_util.tcl scanalyze_util.tcl wrappers.tcl auto_a.tcl\
	preferences.tcl modelmaker.tcl scanalyze.csh \
	xform.tcl colorvis.tcl imagealign.tcl working_volume.tcl\
	sweeps.tcl visgroups.tcl



RESOURCES = curved_hand.xbm pointing_hand.xbm flat_hand.xbm \
	tool_line.xbm tool_rect.xbm tool_shape.xbm

H_FILES = plvGlobals.h Mesh.h plvImageCmds.h KDindtree.h \
	plvClipBoxCmds.h plvInit.h RangeGrid.h\
	plvCmds.h plvMeshCmds.h  strings.h\
	accpersp.h  plvPlyCmds.h Timer.h cyfile.h plvCybCmds.h\
	plvScene.h defines.h plvDraw.h Trackball.h\
	jitter.h  plvDrawCmds.h  plvViewerCmds.h Pnt3.h\
	togl.h Xform.h Image.h plvMMCmds.h \
	ply++.h ToglHash.h sczRegCmds.h \
	Bbox.h Median.h Random.h plvAnalyze.h ColorUtils.h \
	GlobalReg.h TbObj.h DisplayMesh.h \
	absorient.h ICP.h VertexFilter.h \
	ResolutionCtrl.h RigidScan.h GenericScan.h \
	CyberScan.h CyraScan.h MMScan.h TriMeshUtils.h \
	VolCarve.h SyntheticScan.h Progress.h \
	MCEdgeTable.h ToglCache.h \
	ScanFactory.h GroupScan.h CyraResLevel.h\
	KDtritree.h CyberXform.h CyberCalib.h FileNameUtils.h\
	DrawObj.h CyberCmds.h GroupUI.h BailDetector.h \
	MeshTransport.h ConnComp.h SDfile.h TextureObj.h RefCount.h \
	cameraparams.h ProxyScan.h DirEntries.h WorkingVolume.h \
	ToglText.h Projector.h OrganizingScan.h \
	cmdassert.h TclCmdUtils.h


ifdef windir
H_FILES += tkInt.h tkWinInt.h tkPort.h tkWinPort.h \
	tkWin.h tkFont.h winGLdecs.h
else
H_FILES += tkInt8.0p2.h
endif


# files that CVS knows about, but aren't actually necessary to build
EXTRAS = scanalyze.dsw scanalyze.dsp


#################################################################
# make rules
#################################################################

SRCS = $(CXXSRCS) $(CSRCS)

ifdef windir
OBJS_local = $(CXXSRCS:.cc=.obj) $(CSRCS:.c=.obj)
OBJS = $(addprefix OBJS/$(BUILD)/,$(OBJS_local))
else
OBJS = $(CXXSRCS:.cc=.o) $(CSRCS:.c=.o)
endif

CVSFILES = $(CXXSRCS) $(CSRCS) $(H_FILES) $(SCRIPTS) $(RESOURCES) \
	Makefile Makedefs.IRIX Makedefs.Linux Makedefs.win32

ifdef windir
scanalyze.$(BUILD).exe: $(OBJS)
	$(LINK) -out:$@ $(LINKOPT) $(OBJS) $(LIBPATHS) $(LIBS)


else # unix
scanalyze.$(BUILD): $(OBJS) $(AUXLIBS)
	rm -f ../../$@
	$(LINK) -o ../../$@ $(OBJS) $(AUXLIBS) $(LIBPATHS) $(LIBS)

endif


#################################################################
# CVS dependencies
#################################################################

ifndef SKIPCVS
CVSREPOSITORY = $(shell cat CVS/Repository)
$(CVSFILES) : % : $(CVSREPOSITORY)/%,v
	cvs update $@
	touch $@
endif

checkout: $(CVSFILES)

checkin:
	cvs commit


#################################################################
# Utility rules
#################################################################

ifdef windir
clean:
	rm OBJS/debug32/*.obj
	rm OBJS/opt32/*.obj
	rm *.ilk
	rm *.pdb

clobber: clean
	rm scanalyze.*.exe

OBJS/$(BUILD)/%.obj: %.c
	$(CC) $(CFLAGS)    /Fo$@ /c $<

OBJS/$(BUILD)/%.obj: %.cc
	$(CXX) $(CXXFLAGS) /Fo$@ /c $<

else  # UNIX

clean:
	-rm -rf OBJS/*/*

cleanold:
	-rm *.o
	-rm -rf ii_files

clobber: clean cleanold
	-rm scanalyze.d32 scanalyze.d64 scanalyze.o32 scanalyze.o64

%.o: %.c
	$(CC) $(CFLAGS)    -o $@ -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

TAGS: $(CVSFILES)
	etags $(CVSFILES)

endif


#################################################################
# Dependencies
#################################################################

-include Makedepend
-include *.d


