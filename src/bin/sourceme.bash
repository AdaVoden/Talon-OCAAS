export TELHOME=/usr/local/telescope
export CC=gcc
export MOTIFI="-I$HOME/telescope/Motif -I/usr/X11R6/include"
export MOTIFL="-L$TELHOME/lib -L/usr/X11R6/lib"
export MOTIFLIBS="-lXm -lXp -lXpm -lXt -lXext -lXmu -lX11"

export HISTSIZE=100

export PATH=$PATH:~/telescope/bin:$TELHOME/bin:~/telescope/CSIMC/icc/bin

export cdpath=$cdpath:~
export cdpath=$cdpath:~/telescope
export cdpath=$cdpath:~/telescope/per
export cdpath=$cdpath:~/telescope/drivers
export cdpath=$cdpath:~/telescope/daemons
export cdpath=$cdpath:~/telescope/GUI
export cdpath=$cdpath:~/telescope/CSIMC
export cdpath=$cdpath:~/telescope/tools
export cdpath=$cdpath:$TELHOME
export cdpath=$cdpath:$TELHOME/archive

