if (! -e /tmp/.X0-lock && `tty` !~ *p* ) exec startx
