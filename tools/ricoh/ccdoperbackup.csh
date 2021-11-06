#!/bin/csh -f
# ccdoperbackup:
#   back up stuff in ~ccdoper to /ricoh.
# V1.0 9/29/94 ECD

goto start

usage:
echo 'usage: [-full | -inc] [-clean | -week] label_file'
echo '  -full:  back up all data of ~ccdoper'
echo '  -inc:   back up just what has changed since previous backup'
echo '  -clean: remove all files older than previous backup'
echo '  -week:  remove all files older than one week (regardless)'
echo '          (unless -clean or -week is used no files are removed)'
echo 'Disk must contain a file named <label_file> or nothing is dumped.'
echo 'Time of last backup is recorded as the time of ' $stampfn
echo Directories backed up in $budir : $dirs
exit 1

start:

# for testing purposes we can allow anybody to run this
# goto anybody

if ("`whoami`" != "ccdoper") then
    echo Must be ccdoper to run $0
    exit 2
endif

anybody:

# define full path of directory we will back up and make that our cwd throughout
# set budir = /tmp
set budir = ~ccdoper
cd $budir

# define full path to file to touch whose date records time of last dump
set stampfn = $budir/LastCCDOperBackup

# define exactly which directories *relative to budir* we will back up
set dirs = (user archive/calib archive/images archive/logs)

# define full path of directory where the ricoh gets mounted
set ricohdir = /ricoh

# define full path of file to create containing cpio dump image
set define cpiofn = "$ricohdir/CCDOperBackup.`date +%y%m%d%H%M%S`.cpio.Z"

# scan arguments
set full = 0
set inc = 0
set clean = 0
set week = 0
while ($#argv > 1)
    switch ($argv[1])
    case "-full":
	set full = 1
	breaksw
    case "-inc":
	set inc = 1
	breaksw
    case "-clean":
	set clean = 1
	breaksw
    case "-week":
	set week = 1
	breaksw
    default:
	goto usage
	breaksw
    endsw
    shift
end

if ($full && $inc) goto usage
if ($#argv != 1) goto usage
if ("$argv[1]" =~ -*) goto usage
set label = "$argv[1]"

# determine whether we need to use the ricoh for any sort of dump
if ($full || $inc) then
    set dodump = 1
else
    set dodump = 0
endif

# mount the ricoh if we are doing either kind of dump.
# in case a disk is still sitting there from an earlier system crash
# and because it's exit statii are easy to interpret we start with fsck.
# if all ok insure a file exists with the name "label"
if ($dodump) then
    ricohfsck
    set s = $status
    switch ($s)
    case 32:
	echo Ricoh already mounted -- continuing
	breaksw
    case 1:
	echo No disk in Ricoh -- aborting
	exit 3
    case 0:
	ricohmnt # can't fail :-)
	breaksw
    endsw

    if (! -e $ricohdir/$label) then
	echo Required label file in $ricohdir is not present : $label
	ricohumnt
	exit 4
    endif
endif

# full dump requires no "-newer" predicate with find(1)
# incremental uses -newer unless there is no time stamp file.
set newer = ""
if ($inc && -w $stampfn) set newer = "-newer $stampfn"

# ricoh is mounted if necessary now so do the backup if so requested
set dumpok = 0
if ($dodump) then
    # compute space we'll need
    # might not need this much because we'll be compressing but then some
    # files are already compressed.
    set need = `du -s $dirs | awk '{sum+=$1} END {print sum}'`

    # get free space on optical disk
    set have = `df $ricohdir | awk '{print $3}'`
    if ($have < $need) then
	echo Need $need blocks but only have $have
	ricohumnt
	exit 5
    endif

    find $dirs $newer -print | cpio -oBac | compress > $cpiofn
    set dumpok = $status
    if ($dumpok != 0) echo Backup error
    ricohumnt
    unset need have
endif

# now remove files older than previous backup if so requested
if ($clean && -w $stampfn) then
    find $dirs -type f \! -newer $stampfn -exec rm -f {} \;
endif

# now remove files older than one week if so requested
if ($week) then
    find $dirs -type f -mtime +7 -exec rm -f {} \;
endif

# update dump time stamp if we made a dump and it went ok
if ($dodump && $dumpok == 0) then
    touch $stampfn
endif

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: ccdoperbackup.csh,v $ $Date: 2003/04/15 20:48:39 $ $Revision: 1.1.1.1 $ $Name:  $
