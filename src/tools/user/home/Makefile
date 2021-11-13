H = /home/talon
A = $H/APPLRESDIR

pass8a: all

all:	$A/XTerm	\
	$H/.cshrc	\
	$H/.exrc	\
	$H/.fvwmrc	\
	$H/.login	\
	$H/.xinitrc	\
	$H/rootwin.gif
	mkdir -p $H/XEphem/fits
	cp horsehead.fts $H/XEphem/fits
	mkdir -p $H/XEphem/work
	chown -R talon $H
	chgrp -R talon $H

$A/XTerm: XTerm
	mkdir -p $A
	cp XTerm $A

$H/.cshrc: .cshrc
	cp .cshrc $H

$H/.exrc: .exrc
	cp .exrc $H

$H/.fvwmrc: .fvwmrc
	cp .fvwmrc $H

$H/.login: .login
	cp .login $H

$H/.xinitrc: .xinitrc
	cp .xinitrc $H

$H/rootwin.gif: rootwin.gif
	cp rootwin.gif $H
