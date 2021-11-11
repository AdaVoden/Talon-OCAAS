#!/bin/csh -f
# this script is run by telrun after each new raw image has been created.
# usage:
#   postprocess fullpath cal scale
#
# Updated for JSF version 9/2001 to call offsetFITS
# This will correctly align return sections according to their
# offsets as given in the pvc1m.cfg file before applying WCS to them
#
#
# 1) if cal > 0, apply image corrections, add wcs and fwhm to file
# 2) if scale > 0 fcompress file, rename to .fth, discard .fts file
# 3) inform camera of new file

if ($#argv != 3) then
    jd -t; echo " Bad args"": " $argv
    jd -t; echo " $0"": " "fullpath corrections scale" 
    exit 1
endif

set file = "$argv[1]"
set filet = "$file:t"":"
set cal = "$argv[2]"
set scale = "$argv[3]"

echo ""
jd -t; echo " $filet Starting postprocessing"

# 1) if cal > 0, apply image corrections, add wcs and fwhm to file
if ("$cal" > 0) then
    jd -t; echo " $filet Applying image corrections"
    calimage -C $file
    # if we're running on the 1-meter, do the offset for the return section
    if (-e $TELHOME/archive/config/pvc1m.cfg) then
	jd -t; echo " $filet Offsetting return section"
        offsetfits -r $file
    endif
    jd -t; echo " $filet Adding WCS headers"
    wcs -wu .3 $file
    jd -t; echo " $filet Adding FWHM headers"
    fwhm -w $file
else
    jd -t; echo " $filet No image corrections requested"
endif

# 2) if scale > 0 fcompress file, rename to .fth, discard .fts file
if ("$scale" > 0) then
    jd -t; echo " $filet Compressing with scale $scale"
    # fcompress -r adds HCOMSTAT and HCOMSCAL fits headers and removes the .fts.
    # it also refuses to overwrite an existing .fth file which can happen for
    # repeats.
    set cmp_fn = "$file:r".fth
    rm -f $cmp_fn
    fcompress -s $scale -r $file 
    set file = $cmp_fn
endif

jd -t; echo " $filet Postprocessing complete"

# 3) inform camera of new file
if (! $?TELHOME) setenv TELHOME /usr/local/telescope
set camfifo = $TELHOME/comm/CameraFilename
if (-w $camfifo && -p $camfifo) then
    jd -t; echo " $filet Informing Camera (if any)"
    # N.B. echo hangs if there is no camera running with the fifo open
    (echo $file >> $camfifo & sleep 5; kill %1) >& /dev/null
endif

exit 0

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: postprocess.csh,v $ $Date: 2003/04/15 20:48:01 $ $Revision: 1.1.1.1 $ $Name:  $
