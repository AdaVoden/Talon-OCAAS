if ( ! $?TELHOME ) export TELHOME = /usr/local/telescope
if ( ! $?CC ) export CC = gcc
if ( ! $?MOTIFI ) export MOTIFI = "-I$HOME/telescope/Motif -I/usr/X11R6/include"
if ( ! $?MOTIFL ) export MOTIFL = "-L$TELHOME/lib -L/usr/X11R6/lib"
if ( ! $?MOTIFLIBS ) export MOTIFLIBS = "-lXm -lXp -lXpm -lXt -lXext -lXmu -lX11"

set history = 100

PATH = (PATH:~/telescope/bin)
PATH = ($PATH:$TELHOME/bin)
PATH = ($PATH:~/telescope/CSIMC/icc/bin)

if (! $?cdpath) set cdpath
set cdpath = ($cdpath:~)
set cdpath = ($cdpath:~/telescope)
set cdpath = ($cdpath:~/telescope/per)
set cdpath = ($cdpath:~/telescope/drivers)
set cdpath = ($cdpath:~/telescope/daemons)
set cdpath = ($cdpath:~/telescope/GUI)
set cdpath = ($cdpath:~/telescope/CSIMC)
set cdpath = ($cdpath:~/telescope/tools)
set cdpath = ($cdpath:$TELHOME)
set cdpath = ($cdpath:$TELHOME/archive)
