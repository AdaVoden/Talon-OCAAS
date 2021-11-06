#!/bin/csh -f
# Installation script to be run at ATF
# N.B. we insist on running as root and in dir of telescope.tar.gz

# make sure we are running root
if (`whoami` != "root") then
    echo Must be root -- try using su
    exit 1
endif

# make sure we can find the tar file
set tarfile = telescope.tar.gz
if (! -r ./$tarfile) then
    echo Can not find $tarfile
    exit 1
endif

set dst = /usr/local/telescope
if (-e $dst) then
    pushd $dst
    set ds = `date +%y%m%d`
    # save some old directories
    foreach dir (comm dev bin xephem archive/config)
	set old = $dir.$ds
	echo "replacing $dir -- old in $old"
	if (-e $old) rm -fr $old
	mv $dir $old
	mkdir $dir
    end
    popd
endif

# go
echo Installing new software at $dst ..
gunzip < $tarfile | (cd $dst; tar xf - )

# check for proper accounts
echo checking accounts
grep -q teloper /etc/passwd
if ($status) echo 'teloper:*:1041:40000:Anonymous Telescope Operator:/net/iro/home/ccdoper:/bin/false' >> /etc/passwd
grep -q teloper /etc/group
if ($status) echo 'teloper::40000:teloper' >> /etc/group

# now set permssions
echo setting permissions
chown -R teloper $dst
chgrp -R teloper $dst
chmod -R ug+w $dst
chmod -R o-w $dst

# check for bootup
echo installing boot parameters
grep -q telescope /etc/rc.d/rc.local
if ($status) then
    echo ' ' >> /etc/group
    echo 'Start telescope system' >> /etc/group
    echo '/usr/local/telescope/archive/config/boot.csh' >> /etc/group
endif

# done
echo Done.

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: install.csh,v $ $Date: 2003/04/15 20:48:20 $ $Revision: 1.1.1.1 $ $Name:  $
