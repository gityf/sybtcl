########################
#
# getSignOn
#
#   start it off
#

proc getFile {afile} {
  set contents ""
  catch { set fd [open $afile]
        set contents [read $fd]
        close $fd }
  return $contents
}


proc getSignOn { subwindowflag } {
  #upvar 1 $u uid
  #upvar 1 $p password
  #upvar 1 $s server
  global env mainwindow

  # get valid servers from interfaces file
  set syb_home [lsearch [array names env] SYBASE] 
  if {[info exist env(USER)]} {
    set uid $env(USER)
  } else {
    if {[info exist env(HOME)]} {
      set uid [file tail $env(HOME)]
    } else {
      set uid ""
    }
  }


 if {$syb_home == -1} {
      set syb_home ""
      if {$tcl_platform(platform) == "unix"} {
        catch {set syb_home [exec ypcat passwd | egrep  ^sybase: ]}
      }
      if {[string length $syb_home] > 0} {
          set syb_home [lindex [split $syb_home :] 5]
      } else {
          if {$tcl_platform(platform) == "unix"} {
            set syb_home [exec egrep ^sybase: < /etc/passwd ]
          }
          if {[string length $syb_home] > 0} {
              set syb_home [lindex [split $syb_home :] 5]
          } else {
              set syb_home ""
          }
      }
  } else {
      set syb_home $env(SYBASE)
  }


  if {[string length $syb_home] > 0} {
    set intFile $syb_home/interfaces
    set serverList ""
    if {[file isfile $intFile]} {
      set ifile [split [getFile $intFile] \n]
      foreach line $ifile {
        if {[regexp -nocase "(^\[a-z_]\[^ \t\r]*).*$" $line m s1]} {
          lappend serverList $s1
        }
      }
    } else {
      set serverList SYBASE
    }
  } else {
    set serverList SYBASE
  }

  
  if { $subwindowflag == 1 } {
     set mainwindow ".sybaselogin"
     toplevel $mainwindow
     wm geom  $mainwindow 300x330
  } else {
     set mainwindow ""
     wm geom . 300x330
  }

  frame $mainwindow.s
  message $mainwindow.s.m -justify center  -text "SQL Server Sign on" -aspect 2000 \
		-font -*-helvetica-bold-o-*-*-20-*-*-*-*-*-*-*
  frame $mainwindow.s.i
  entry $mainwindow.s.i.uid -relief sunken  -width 10 
  label $mainwindow.s.i.id  -text "  User Id" -anchor e
  frame $mainwindow.s.p
  entry $mainwindow.s.p.pw  -relief sunken -width 10 \
		 -font -*-symbol-*-r-*--20-*-*-*-*-*-*-*
  label $mainwindow.s.p.p   -text "  Password" -anchor e

  frame $mainwindow.s.s
  entry $mainwindow.s.s.ser -relief sunken -width 10 
  menubutton $mainwindow.s.s.s -text " Server  " -anchor e -menu $mainwindow.s.s.s.m -relief raised
  menu $mainwindow.s.s.s.m
  foreach s $serverList {
    $mainwindow.s.s.s.m add command -label $s \
		  -command "$mainwindow.s.s.ser delete 0 end; $mainwindow.s.s.ser insert 0 $s "
  }

  message $mainwindow.s.err -text " " -justify center -aspect 2000

  frame $mainwindow.s.b
  button $mainwindow.s.b.ok  -text "Sign on" -command { 
    set user [$mainwindow.s.i.uid get]; 
    set password [$mainwindow.s.p.pw get];
    set server [$mainwindow.s.s.ser get];

    set env(USER) $user
    set env(DSQUERY) $server

    destroy $mainwindow.s
    if { $mainwindow != "" } {
       destroy $mainwindow
    }
  }
  button $mainwindow.s.b.can -text "Cancel" -command {
							if { $mainwindow == "" } {
							   destroy .
							 } else {
							    destroy $mainwindow
							 }
						      }


  pack $mainwindow.s       -side top -expand 1 -fill both
  pack $mainwindow.s.m     -side top -fill x  -pady 20
  pack $mainwindow.s.i     -side top -pady 20 -anchor e
  pack $mainwindow.s.i.uid -side right -expand 1 -padx 20
  pack $mainwindow.s.i.id  -side left
  pack $mainwindow.s.p     -side top   -pady 10 -anchor e
  pack $mainwindow.s.p.pw  -side right -expand 1 -padx 20 
  pack $mainwindow.s.p.p   -side left

  pack $mainwindow.s.err   -side top -fill x -expand 1

  pack $mainwindow.s.b.ok  -side left -fill x -expand 1
  pack $mainwindow.s.b.can -side left -fill x -expand 1
  pack $mainwindow.s.b     -side bottom -fill x -expand 0

  pack $mainwindow.s.s.ser -side right  -expand 1 -padx 20 
  pack $mainwindow.s.s.s   -side left
  pack $mainwindow.s.s     -side bottom -pady 5 -anchor e

  $mainwindow.s.i.uid insert 0 $uid

  if {[lsearch [array names env] DSQUERY] >= 0} {
    $mainwindow.s.s.ser insert 0 $env(DSQUERY)
  } else {
    $mainwindow.s.s.ser insert 0 SYBASE
  }
  focus $mainwindow.s.p.pw

  bind $mainwindow.s.i.uid <KeyPress-Return> "focus $mainwindow.s.p.pw"
  bind $mainwindow.s.p.pw  <KeyPress-Return> "$mainwindow.s.b.ok invoke"
  bind $mainwindow.s.s.ser <KeyPress-Return> "$mainwindow.s.b.ok invoke"

tkwait window $mainwindow.s.s.ser
}



