#!/usr/bin/perl

# This Perl script is run by the pvcam script after acquisition
# and cut up of image.  
#
# usage:
#    pipeline fullpath-basename-and-sequence process(1|0) record(1|0)
#
#-----
# Configurable options:
#
#	configuration options may be found in $TELHOME/archive/config/pipeln1m.cfg
#
# This post-processing pipeline script may be expanded to suit as needed.
# The format should be reasonably understandable and simple to expand upon.
# This section gives some simple configuration-style control of
# what is anticipated as common options
#
# ++++
# Shared configuration file (with pvcam and calimagePV)

$TELHOME = $ENV{TELHOME};
do "$TELHOME/archive/config/pvc1m.cfg";
do "$TELHOME/archive/config/pipeln1m.cfg";

# set $traceon to 1 if want trace info in the log file
# inherits from pvc1m.cfg
# (can set here to override, though)
# $traceon = 1;

# ==================================

# sync output
$| = 1;


$numArg = 3;

# Figure out who we are
$slaveNum = -1;
if($testMode) {
	$name = $ARGV[3];
	$numArg++;
}
else {
	$name = `uname -n`;
}	
chomp($name);

# Sanity check: Make sure we have the right arguments
&errlog ("Wrong number of arguments: @ARGV\n") if @ARGV != $numArg;
$base = $ARGV[0];
$process = $ARGV[1];
$record = $ARGV[2];

# we can early reject if we're not doing anything
if(!$process && !$record) {
	&vtrace("Nothing to do for $base");
	exit(0);
}

# convert section names from pvc1m format to pvcam format <g>

for(my $i=0; $i<4; $i++) {
	%gi = %{$GroupInfo->[$i]};
	my $gname = $gi{Name};
	if($gname eq $name) {
		$slaveNum = $i;
		last;
	}
}
if($slaveNum < 0) {
	&errlog("Computer $name not found in group definition\n");
}	

# Get our group info from the config array handy
%slave = %{$GroupInfo->[$slaveNum]};

my $retInfo = $$ReturnInfo[$ReturnChoice];
my $rtcpu = $$retInfo[0];

$sectionNames = "ignore_me,";
if($name eq $rtcpu) {
   	$sectionNames = $ReturnName.",";
}

# build comma delimited version of section list
my $cutlist = $slave{SectionCuts};
while(my $sec = shift @{$cutlist}) {
	shift @{$cutlist}; #cut
	shift @{$cutlist}; #offX
	shift @{$cutlist}; #offY
	shift @{$cutlist}; #rot
	
	$sectionNames .= $sec.",";
}


# --------------------


&trace("\n===============\npipelining $base Process=$process, Record=$record");

# Do each section, saving the center section for last
# Only process the center section if either $preserveCenter or $resultToCamera are set
@sects = split /,/, $sectionNames;
$center = $base.(@sects[0]).".fts";
while($sect = pop(@sects))
{		
	if($sect eq "ignore_me") {
		next;
	}
	
	$file = $base.$sect.".fts";
		
	if($file eq $center) {
		unless($preserveCenter || $resultToCamera)
		{
			next;
		}
	}		

	&vtrace("section $sect");
		
	if($process) {
	
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
		}
	
		# any processing on a file that may be compressed or not should go here		
	}
	
	# add to the database
	if($record) {
#		$rt = `addexpimg $file`;
#		&trace("$rt : addexpimg $file");
&trace("NOT adding $file to database -- commented out in pipeln1m");
        }
	
} # end while -- next section

# done recording... nothing left unless we're processing
if(!$process) {
	exit(0);
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
				&vtrace("Found pid $pid, killing...");
				`kill $pid`;
				last;
			}
		}
		close P;						
	}
	else
	{
		&vtrace("FIFO $camfifo not available");
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
	&vtrace("Emptying trash");
	open F, "<$trashBin" or return;
	while($file = <F>) {
		chomp($file);
		$rt = `rm -f $file`;
		&vtrace("$rt : removing $file");
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
	print EL "$ts:**($name): $_[0]\n";
	close EL;
	exit 1;
}

# if $trace: append $_[0] to $elog
sub trace
{
	return unless ($traceon);
	open TL, ">>$elog";
	$ts = `jd -t`;
	print TL "$ts:  ($name) $_[0]\n";
	close TL;
}

# verbose version of trace
sub vtrace
{
	return unless ($verboseTrace);
	&trace($_[0]);
}

