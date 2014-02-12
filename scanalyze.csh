#!/bin/csh -f
#
# Scanalyze wrapper script - sets all the necessary environment vars
# and runs the right version of scanalyze.
#

if (! $?TCL_8_0_LIBRARY) then
    setenv TCL_8_0_LIBRARY /usr/common/tcl8.0/lib/tcl8.0
endif

if (! $?TK_8_0_LIBRARY) then
    setenv TK_8_0_LIBRARY /usr/common/tcl8.0/lib/tk8.0
endif

setenv TCL_LIBRARY ${TCL_8_0_LIBRARY}
setenv TK_LIBRARY ${TK_8_0_LIBRARY}

if (! $?SCANALYZE_BACKUP) then
    set SCANALYZE_BACKUP=current
endif

if (! $?SCANALYZE_DIR) then
    setenv SCANALYZE_DIR /usr/graphics/project/scanner/scanalyze/$SCANALYZE_BACKUP
endif

if (! $?SCANALYZE_VERSION) then
    if (`uname` == 'IRIX64') then
	setenv SCANALYZE_VERSION opt64
    else
	setenv SCANALYZE_VERSION opt32
    endif
endif


set SCANALYZE_BIN=$SCANALYZE_DIR/scanalyze.$SCANALYZE_VERSION
if (-e $SCANALYZE_BIN) then
    echo "Scanalyze ($SCANALYZE_VERSION) is $SCANALYZE_BIN"
    # so that others can write xforms:
    umask 003
    ${SCANALYZE_BIN} -visual best 24 $argv[*]
else
    if (-d $SCANALYZE_DIR ) then
	echo "Scanalyze version $SCANALYZE_VERSION not found in $SCANALYZE_DIR."
    else
	echo "Bad SCANALYZE_DIR $SCANALYZE_DIR."
    endif
endif
