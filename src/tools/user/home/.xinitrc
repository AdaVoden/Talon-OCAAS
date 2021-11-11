# fix some busted keycode mappings
xmodmap -e "keycode 22 = BackSpace"
xmodmap -e "keycode 107 = Delete"

# set up a background
bg='#00561c'
fg='#fec500'

# a background
# xsetroot -solid $bg
xv -maxpect -root -quit -ncols 50 rootwin.gif || \
xloadimage -onroot -colors 75 -fullscreen rootwin.gif

# start some visual candy
nice xeyes -fg blue3 -bg white -geometry 60x46-6+0 &
xclock -hd $fg -hl $fg -update 1 -bg '#112' -fg $fg -geometry 90x90-4+90 &
nice xload -fg $fg -bg $bg -hl white -scale 2 -label "CPU load" -geometry 72x54-6+262 &
(export TZ=UTC0; nice xdaliclock -seconds -24 -shape -memory medium -fn 9x15 -fg $fg -geometry --22+60 &)

# start the telescope control system GUI
camera -geometry +1074+50 &
xephem -geometry +2098+50 &
xterm -geometry 80x40+110+828 &
xobs -geometry +24+50 &

# window manager, exit when it does
exec fvwm

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: .xinitrc,v $ $Date: 2003/04/15 20:48:39 $ $Revision: 1.1.1.1 $ $Name:  $
