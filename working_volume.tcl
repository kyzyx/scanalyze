
proc cyberscanPreviewUI {} {

   global theMesh previewOnOff previewDrawMode

   if {$theMesh == ""} {
      puts "You need at least one reference scan to preview"
      return
   }

   plv_working_volume use $theMesh
   trace variable theMesh w referenceScanWrite

   set previewOnOff "off"
   set previewDrawMode "solid"

   if {[window_Activate .cyberscanPreview]} { return }
   toplevel .cyberscanPreview

   # Radiobutton for working volume display on/off

   set f [frame .cyberscanPreview.onoff]
   label $f.l -text "Working Volume: " -padx 3
   radiobutton $f.on -text "On" -variable previewOnOff \
      -value on -command previewDisplayOnOff
   radiobutton $f.off -text "Off" -variable previewOnOff \
      -value off -command previewDisplayOnOff

   pack $f.l $f.on $f.off -side left
   pack $f -side top -anchor w

   # Radiobutton for render mode (solid working volume, or lines)

   set f [frame .cyberscanPreview.drawMode]
   label $f.l -text "Display Mode: " -padx 3
   radiobutton $f.solid -text "Solid" -variable previewDrawMode \
      -value solid -command previewDrawModeChange
   radiobutton $f.lines -text "Lines" -variable previewDrawMode \
      -value lines -command previewDrawModeChange

   pack $f.l $f.solid $f.lines -side left
   pack $f -side top -anchor w

   # Scales for scanner working volume boundaries

   set scale_frame [frame .cyberscanPreview.scales \
      -borderwidth 4 -relief ridge]
   set labelWidth 8

   set motMin [frame $scale_frame.motMin]
   label $motMin.l -text "Mot Min" -width $labelWidth -anchor w -padx 3
   scale $motMin.s -from -6 -to 260 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $motMin.s set 0
   pack $motMin -side top -expand true -fill x
   pack $motMin.l -side left -anchor sw
   pack $motMin.s -side right -expand true -fill x

   set motMax [frame $scale_frame.motMax]
   label $motMax.l -text "Mot Max" -width $labelWidth -anchor w -padx 3
   scale $motMax.s -from -6 -to 260 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $motMax.s set 250
   pack $motMax -side top -expand true -fill x
   pack $motMax.l -side left -anchor sw
   pack $motMax.s -side right -expand true -fill x

   set rotMin [frame $scale_frame.rotMin]
   label $rotMin.l -text "Rot Min" -width $labelWidth -anchor w -padx 3
   scale $rotMin.s -from -6 -to 260 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $rotMin.s set 0
   pack $rotMin -side top -expand true -fill x
   pack $rotMin.l -side left -anchor sw
   pack $rotMin.s -side right -expand true -fill x

   set rotMax [frame $scale_frame.rotMax]
   label $rotMax.l -text "Rot Max" -width $labelWidth -anchor w -padx 3
   scale $rotMax.s -from -6 -to 260 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $rotMax.s set 250
   pack $rotMax -side top -expand true -fill x
   pack $rotMax.l -side left -anchor sw
   pack $rotMax.s -side right -expand true -fill x

   set horMin [frame $scale_frame.horMin]
   label $horMin.l -text "Hor Min" -width $labelWidth -anchor w -padx 3
   scale $horMin.s -from -7 -to 825 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $horMin.s set 100
   pack $horMin -side top -expand true -fill x
   pack $horMin.l -side left -anchor sw
   pack $horMin.s -side right -expand true -fill x

   set horMax [frame $scale_frame.horMax]
   label $horMax.l -text "Hor Max" -width $labelWidth -anchor w -padx 3
   scale $horMax.s -from -7 -to 825 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $horMax.s set 500
   pack $horMax -side top -expand true -fill x
   pack $horMax.l -side left -anchor sw
   pack $horMax.s -side right -expand true -fill x

   set horInc [frame $scale_frame.horInc]
   label $horInc.l -text "Hor Inc" -width $labelWidth -anchor w -padx 3
   scale $horInc.s -from 50 -to 150 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $horInc.s set 120
   pack $horInc -side top -expand true -fill x
   pack $horInc.l -side left -anchor sw
   pack $horInc.s -side right -expand true -fill x

   set vert [frame $scale_frame.vert]
   label $vert.l -text "Vertical" -width $labelWidth -anchor w -padx 3
   scale $vert.s -from -2000 -to 2000 -resolution 1 -orient horizontal \
      -length 150 -command previewBounds
   $vert.s set 0
   pack $vert -side top -expand true -fill x
   pack $vert.l -side left -anchor sw
   pack $vert.s -side right -expand true -fill x

   pack $scale_frame -side top -expand true -fill x

   wm title .cyberscanPreview "CyberScan Preview"
   bind .cyberscanPreview <Destroy> "+destroyCyberScanPreviewUI %W"
   window_Register .cyberscanPreview
}


proc referenceScanWrite {name element op} {
    global theMesh
    plv_working_volume use $theMesh

    redraw 1
}


proc previewDisplayOnOff {} {
   global previewOnOff

   if {$previewOnOff == "on"} {plv_working_volume on} \
   else {plv_working_volume off}

   redraw 1
}


proc previewDrawModeChange {} {
   global previewDrawMode

   if {$previewDrawMode == "lines"} {plv_working_volume lines} \
   else {plv_working_volume solid}

   redraw 1
}


proc previewBounds {value} {

   set p .cyberscanPreview.scales

   if {[$p.rotMin.s get] > [$p.rotMax.s get]} \
      {$p.rotMax.s set [$p.rotMin.s get]}

   plv_working_volume [$p.vert.s get] [$p.horInc.s get] [$p.horMin.s get] \
      [$p.horMax.s get] [$p.rotMin.s get] [$p.rotMax.s get] \
      [$p.motMin.s get] [$p.motMax.s get]

   redraw 1
}


proc destroyCyberScanPreviewUI {widget} {
   global theMesh

   if {$widget != ".cyberscanPreview"} {return}

   trace vdelete theMesh w referenceScanWrite

}
