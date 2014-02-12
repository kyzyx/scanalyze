proc plv_AnalyzeLineMode {an cmd opt1 {opt2 ""}} {

    if {[string range $opt1 0 0] == "$"} {
	set opt1 [globalset [string range $opt1 1 end]]
    }

    if {[string range $opt2 0 0] == "$"} {
	set opt2 [globalset [string range $opt2 1 end]]
    }

    plv_analyzeLineMode $an.togl $cmd $opt1 $opt2
}


proc createAnalyzeWindow {number width xused xrange zrange name defScale} {
    # sizing is tricky because the size we pass to the togl widget is both
    # its initial size and its minimum size, so if we want to allow it to
    # shrink, we have to pass a small value, then manually size the toplevel
    # window to the size we want, and the togl widget takes up the slack.

    set an .analyze$number
    set analyze [toplevel $an]
    set sh [frame $analyze.show]
    label $sh.cap -text "Show:"
    checkbutton $sh.scale -text Scale -variable ${an}visScale \
	-command "plv_AnalyzeLineMode $an show scale $${an}visScale"
    checkbutton $sh.edge -text "Mesh edges" -variable ${an}visEdge \
	-command "plv_AnalyzeLineMode $an show edges $${an}visEdge"
    checkbutton $sh.framebuff -text "Pts from frame buffer" \
	-variable ${an}visFBPts \
	-command "plv_AnalyzeLineMode $an show framebuff $${an}visFBPts"
    checkbutton $sh.color -text "False colors" -variable ${an}visColor \
	-command "plv_AnalyzeLineMode $an show color $${an}visColor"

    pack $sh.cap $sh.scale $sh.edge $sh.framebuff $sh.color -side left
    pack $sh -side top -anchor w
    globalset ${an}visScale 1
    globalset ${an}visColor 1

    set scaleVar ${an}scale
    set cm [frame $analyze.commands]
    label $cm.sc -text "Scale: 10^"
    label $cm.scaleT -textvariable $scaleVar -width 2 -anchor w
    label $cm.sc2 -text "mm"
    scale $cm.scale -from -4 -to 4 -orient horiz \
	-variable $scaleVar -showvalue off \
	-command "plv_AnalyzeLineMode $an scale $$scaleVar"
    checkbutton $cm.flat -text "Fit to horiz line" -state disabled
    button $cm.export -text "Export to text" \
	-command "exportAnalysisAsText $an ${name}_#${number}"
    button $cm.exportRGB -text "Export image" \
	-command "saveScreenDump $analyze.togl"
    packchildren $cm -side left
    pack $cm -side top -anchor w
    globalset $scaleVar $defScale

    togl $analyze.togl -width $width -height 10 -ident $analyze.togl
    pack $analyze.togl -fill both -expand 1 -side top

    set dims [frame $analyze.dims]
    label $dims.xru -text "x used: $xused"
    label $dims.xrwin -text "x range: $xrange"
    label $dims.zru -text "z range: $zrange"
    pack $dims.xrwin $dims.xru $dims.zru -side left
    pack $dims -side top -fill x

    wm title $analyze "Analyze $name #$number"
    wm geometry $analyze =${width}x180
    window_Register $analyze

    bind $analyze.togl <Button-1>  "plv_AnalyzeLineMode $an zscale %y start"
    bind $analyze.togl <B1-Motion> "plv_AnalyzeLineMode $an zscale %y set"
    bind $analyze.togl <Button-2>  "plv_AnalyzeLineMode $an zscale 0 reset"

    return $analyze
}


proc setAnalyzeZScale {analyzeTogl zscale} {
    set analyze [string range $analyzeTogl 0 [expr [string first .togl \
						    $analyzeTogl] - 1]]
    set widget $analyze.dims.zru
    $widget config -text "z range: $zscale"
}


proc planeFitClipRectUI {} {
    global pcfrData

    if [window_Activate .planefit] return

    set pf [toplevel .planefit]
    wm title $pf "Fit plane to cliprect"
    window_Register $pf

    label $pf.ls -text "Data source:" -anchor w -pady 6
    frame $pf.fs -relief groove -border 2
    radiobutton $pf.fs.vertices -text "Mesh vertices" \
	-var pfcrData -val data_mesh
    radiobutton $pf.fs.zbuffer -text "Rendered points" \
	-var pfcrData -val data_zbuffer

    label $pf.la -text "Actions:" -anchor w -pady 6
    frame $pf.fa -relief groove -border 2
    checkbutton $pf.fa.stats -text "Dump statistics" \
	-var pfcrActDump -onvalue dumpstats -offvalue ""
    checkbutton $pf.fa.align -text "Align to screen plane" \
	-var pfcrActAlign -onvalue align -offvalue ""
    checkbutton $pf.fa.show -text "Show plane fit" \
	-var pfcrActShow -onvalue createplane -offvalue ""
    globalset pfcrActDump ""
    globalset pfcrActAlign align
    globalset pfcrActShow ""

    label $pf.lm -text "Align by moving:" -anchor w -pady 6
    frame $pf.fm  -relief groove -border 2
    radiobutton $pf.fm.camera -text "Camera" -var pfcrMover -val movecamera
    radiobutton $pf.fm.mesh -text "Mesh" -var pfcrMover -val movemesh
    globalset pfcrMover movecamera

    button $pf.go -text "OK" -command {
	plv_clipBoxPlaneFit $theMesh \
	    $pfcrActDump $pfcrActAlign $pfcrActShow \
	    $pfcrData $pfcrMover
    }

    pack $pf.ls $pf.fs $pf.la $pf.fa $pf.lm $pf.fm \
	-side top -anchor w -padx 8 -fill x
    pack $pf.go -side top -anchor w -padx 8 -pady 6 -fill x

    pack $pf.fs.vertices $pf.fs.zbuffer -side top -anchor w
    pack $pf.fa.stats $pf.fa.align $pf.fa.show -side top -anchor w
    pack $pf.fm.camera $pf.fm.mesh -side top -anchor w
}

proc autoAnalyzeLineUI {} {
    global toglPane

    if [window_Activate .autoanalyze] return

    set aa [toplevel .autoanalyze]

    wm title $aa "Auto analyze lines"
    window_Register $aa


    # alignment group
    label $aa.l1 -text "Alignment" -anchor w
    frame $aa.f1 -relief groove -borderwidth 2

    checkbutton $aa.f1.ort -text "Orthographic" -variable isOrthographic \
	-command { if {$isOrthographic} {plv_ortho} else {plv_persp}; \
		       $toglPane render}
    button $aa.f1.app -text "Align points to XY plane" -pady 2 -command {
	wsh_AlignToPlane
    }

    # analyze group
    label $aa.l2 -text "Analyze" -anchor w
    frame $aa.f2 -relief groove -borderwidth 2

    label $aa.f2.ls -text "Spacing (in mm):" -anchor w -pady 2
    entry $aa.f2.es -textvariable spacing -relief sunken -width 4

    label $aa.f2.ln -text "Num sections:" -anchor w -pady 2
    entry $aa.f2.en -textvariable num -relief sunken -width 4

    set filename ""

    label $aa.f2.lf -text "Save file name:" -anchor w -pady 2
    entry $aa.f2.ef -textvariable filename -relief sunken -width 4

    button $aa.f2.go -text "Auto analyze" -pady 2 -command {
	auto_a $spacing $num $filename
    }

    pack $aa.f2.ls $aa.f2.es $aa.f2.ln $aa.f2.en $aa.f2.lf $aa.f2.ef $aa.f2.go \
	-side top -anchor w -padx 4 -fill x

    pack $aa.f1.ort $aa.f1.app \
	-side top -anchor w -padx 4 -fill x

    pack $aa.f1 $aa.f2 -side top -fill x -padx 4 -pady 2
}

# BUGBUG this doesn't belong here

proc polygonizeDialog {} {
    if [window_Activate .polygonize] return
    set sc [toplevel .polygonize]
    window_Register $sc

    scale $sc.levels -label "Levels:" -from 1 -to 10 -orient horiz \
	-variable polygonizeLevels
    scale $sc.intlev -label "Internal leaf depth:" -from 0 -to 6 \
	-orient horiz -variable polygonizeLeafDepth
    globalset polygonizeLevels 7
    globalset polygonizeLeafDepth 3

    button $sc.carve -text "Polygonize using visible meshes" \
	-command { addMeshToWindow [plv_spacecarve \
			$polygonizeLevels $polygonizeLeafDepth] }

    pack $sc.levels $sc.intlev $sc.carve -side top -fill x -expand true
}


