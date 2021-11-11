#!/bin/csh -f
# usage: directory logfile sourcename
# given a directory, logfile and source name generate two files in dir from log:
#   logfile.XXX (extension is first three letters of source name) with each
#      line from logfile.log that contains both "Starting camera:" and source
#      but with "Starting camera:" discarded. We call this a master file.
#   logfile.pho with a typical photom header and just file names from the same
#      set of logfile lines as above.

# check for required number of args
if ($#argv != 3) then
    echo Usage\: $0\: directory logfile source_name
    echo "  generates directory/logfile.pho and directory/logfile.sou"
    exit 1
endif

# make args more convenient
set dir = $argv[1]
set log = $argv[2]
set src = "$argv[3]"	# beware embedded spaces

# move to $dir
cd $dir

# required string to search for in log file lines
set rqrd = "Starting camera: "

# lob off the filename extension of the log file for use in making other names
set logroot = $log:r

# create the master file name from logfile and and first three chars of source.
set mfn = $logroot"lg."`echo "$src" | nawk '{print substr($0,1,3)}'`

# fill it with a header line 
# then lines having both the required string and the source name but without
# the required string (ironically)
echo "# " "$log" "$src" > $mfn
nawk "/$rqrd.*$src/ {sub("\""$rqrd"'","",$0); print $0}' < $log >> $mfn

# create the skeleton .pho file name
set pfn = $logroot"ph."`echo "$src" | nawk '{print substr($0,1,3)}'`
# fill it with a header line 
# then filenames having both the required string and the source name
echo "# Photom input file, created from $log ,  source $src" > $pfn
echo "10 10 .03" >> $pfn
nawk "/$rqrd.*$src/ "'{print substr($0,match($0,"[^ ]*\.fts"),RLENGTH)}' < $log >> $pfn

# Convert file names to lower case for DOS file name compatibility
set lmfn = `echo $mfn | tr "[A-Z]" "[a-z]" `
mv $mfn $lmfn
set lpfn = `echo $pfn | tr "[A-Z]" "[a-z]" `
mv $pfn $lpfn

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: photprep.csh,v $ $Date: 2003/04/15 20:48:36 $ $Revision: 1.1.1.1 $ $Name:  $
