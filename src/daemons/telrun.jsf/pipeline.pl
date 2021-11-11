#!/usr/bin/perl

# This Perl script is run by the pvcam script after acquisition
# and cut up of image.  
#
# usage:
#    pipeline fullpath-basename-and-sequence
#
#-----
# Configurable options:
#
# This post-processing pipeline script may be expanded to suit as needed.
# The format should be reasonably understandable and simple to expand upon.
# This section gives some simple configuration-style control of
# what is anticipated as common options
#
# ++++
# Shared configuration file (with pvcam and calimagePV)

$TELHOME = $ENV{TELHOME};
do "$TELHOME/archive/config/pvcam.cfg";

# +++++
# Treatment of redundant 'center' section
#
# This option will DELETE the center, or return, (section 00) image from
# storage.
#
# This image is redundant, since it is a subset of the other images that 
# are stored at the same time.  It is also returned to the camera
# program, either because of direct exposure, or because $returnToCamera is set below
# and the camera is in "auto listen" mode.
# Setting this to 0 (recommended) will schedule the center section for cleanup on the next
# pass, after it is no longer used.  This can help free up disk space.

# set as 1 to preserve, 0 to delete center

$preserveCenter = 1;

# ++++
# Correction Image Control
#
# This determines if images are to be corrected using the calibration images
# set as 1 to perform corrections, 0 for no correction

$doCorrections = 1;

# ++++
# WCS control
#
# This control whether or not the wcs command program will be executed
# on the image files, saving solution fields in the FITS header.
# set as 1 to perform WCS, 0 for no WCS solutions

$doWCS = 1;

# This sets the search radius from the center of the image to consider when
# comparing to the GSC in search of solutions.  In degrees.

$wcsRadius = 0.3;

# +++
# FWHM calculation
#
# This sets whether or not the image is computed for FWHM averages, saving
# this information in the FITS header
# set as 1 to perform FWHM, 0 for no FWHM processing

$doFWHM = 1;

# ++++
# Image Compression
#
# This determines if images should be Huffman compressed into .fth files
# and if so, to what degree.
# A setting of 0 means no compression.  1 is loss-less compression, and
# settings of 2 and higher are successively higher compression / more lossy compressions.
# values over 100 are generally too lossy for practical usefulness.

$imageCompression = 0;

# ++++
# Image Returned to Camera
#
# The center section can be returned to the camera program. If camera has
# its "auto listen" feature turned on, the image will automatically appear
# in the camera window.
# Note that the standard "postprocess" script normally run on batch operations
# also includes auto-listen support.
# $resultToCamera is normally 0 so as not to interfere with the operation of
# the postprocess script callback.
#
# Note that if this setting is 0 and $preserveCenter is also 0, then no processing
# at all will be done on the center section, which may be a minor timesavings.
#
# set as 1 to return the processed center image to Camera, 0 for no camera notification
$resultToCamera = 0;

# ++++
# Trash bin file
$trashBin = "$TELHOME/archive/trash";

# Log file info
$elog = "$TELHOME/archive/logs/pipeline.log";

# set $traceon to 1 if want trace info in the log file
$traceon = 1;

# ==================================

# sync output
$| = 1;

# Sanity check: Make sure we have a filename and it exists
&errlog ("Wrong number of arguments: @ARGV\n") if @ARGV != 1;
$base = $ARGV[0];

&trace("\n===============\nprocessing $base");

# Do each section, saving the center section for last
# Only process the center section if either $preserveCenter or $resultToCamera are set
@sects = split /,/, $sectionNames;
$center = $base.(@sects[0]).".fts";
while($sect = pop(@sects))
{		
	$file = $base.$sect.".fts";
	
	if($file eq $center) {
		unless($preserveCenter || $resultToCamera)
		{
			next;
		}
	}		

	&trace("section $sect");
		
	if($doCorrections) {
		$corrDir = "$TELHOME/archive/calib";
		unless($file eq $center) {
			$corrDir .= "/".$sect;
		}
		$rt = `calimage -d $corrDir -C $file`;
		&trace("$rt : calimage -d $corrDir -C $file");
	}
	
	if($doWCS) {
		$rt = `wcs -wu $wcsRadius $file`;
		&trace("$rt : wcs -wu $wcsRadius $file");
	}
	
	if($doFWHM) {
		$rt = `fwhm -w $file`;
		&trace("$rt : fwhm -w $file");
	}
	
	# any further processing on uncompressed file should be inserted here
	
	# Never compress the center return file
	if($file ne $center)
	{			
		if($imageCompression) {
			# fcompress will remove the fts file and create an fth file
			$cmp_file = $base.$sect.".fth";
		
			# don't do this if the compressed file exists already
			unless(-e $cmp_file) {
				$rt = `fcompress -s $imageCompression -r $file`;
				&trace("$rt : fcompress -s $imageCompression -r $file");
						
				# from now on, any further processing must be on the .fth file,
				# since the .fts file is gone
				if(-e $cmp_file) {
					$file = $cmp_file;
				}
			}
			
			# any further processing for only compressed files should go here
		}
		
		# any processing on a file that may be compressed or not should go here
	}
			
}

# now what to do about the center section...

# first, empty any old trash
&emptyTrash();

# send result of center section processing to camera
if($resultToCamera) {
	
	$camfifo = "$TELHOME/comm/CameraFilename";
	
	&trace("returning $center to $camfifo");
	
	if(-p $camfifo) {
	
		# we spawn a process to output the filename to the fifo
		# but it might hang if the camera that owns it is gone, so we have
		# to wait a bit then look to see if it's still running
		# and if so, kill it.
	
		system("echo $center >> $camfifo &");
		sleep 5;
		open P, "ps T |" or errlog("can't fork for ps: $!\n");
		while(<P>) {
			chomp();
			 # &trace("ps: $_");
			if(m`sh -c echo`) {
				$pid = substr($_,0,5);
				&trace("Found pid $pid, killing...");
				`kill $pid`;
				last;
			}
		}
		close P;						
	}
	else
	{
		&trace("FIFO $camfifo not available");
	}
}
	
# we can delete the center section if we don't want it
unless($preserveCenter) {

	&throwAway($center);
}

exit(0);

# =====================

# save file name to a trash bin where it will be deleted on subsequent passes
sub throwAway
{
	$file = $_[0];
		
	open F, ">>$trashBin" or print EL " Can not open $trashBin: $!\n";
	print F $file;
	print F " ";	
    close F;

	&trace("$file scheduled for deletion");		
}	    	
		

# delete files named in trash bin
sub emptyTrash
{
	&trace("Emptying trash...");
	open F, "<$trashBin" or return;
	while($file = <F>) {
		chomp($file);
		$rt = `rm -f $file`;
		&trace("$rt : removing $file");
	}
	close F;
	unlink($trashBin);
}

# =====================

# append $_[0] to STDOUT and timestamp + $_[0] to $elog and exit
sub errlog
{
	print " $_[0]";
	open EL, ">>$elog";
	$ts = `jd -t`;
	print EL "$ts: $_[0]\n";
	close EL;
	exit 1;
}

# if $trace: append $_[0] to $elog
sub trace
{
	return unless ($traceon);
	open TL, ">>$elog";
	$ts = `jd -t`;
	print TL "$ts: $_[0]\n";
	close TL;
}
