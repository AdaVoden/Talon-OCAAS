#!/usr/bin/perl
# SLAVE script version of 1-meter PV camera control
# Called by pvc1m-mstr script

# get parameters
$TELHOME = $ENV{TELHOME};
do "$TELHOME/archive/config/pvc1m.cfg";	# unified config
do "$TELHOME/bin/naming.pl"; #unified naming

# set $traceon to 1 if want trace info
# inherits from pvc1m.cfg
# (can set here to override, though)
# $traceon = 1;

# sync output
$| = 1;

# this is used by naming.pl (holdover from 1/2 meter version). Echo $ms
$expStamp = 0;

# Figure out who we are
$slaveNum = -1;
if($testMode && !$multicpu) {
    my $me = $0;
    my @l = split /_/, $me;
    $name = @l[-1];
}
else {
	$name = `uname -n`;
}	
chomp($name);

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
$code   = @ARGV[0];		# name or shutter code
$binSet = @ARGV[1];		# binning X:Y
$ms = 0;

# information about return
if(1) {
	my $retInfo = $$ReturnInfo[$ReturnChoice];
	$ReturnCPUName = $$retInfo[0];
	my $rcut = $$retInfo[1];
	my @rfrm = split / /,$rcut;
	$ReturnX = @rfrm[0];
	$ReturnY = @rfrm[1];
	$ReturnW = @rfrm[2];
	$ReturnH = @rfrm[3];
	$ReturnOffX = $$retInfo[2];
	$ReturnOffY = $$retInfo[3];
	$ReturnRot = $$retInfo[4];

	# determine if we're the lucky slave who gets to provide the return image,
	# and if so, determine what the center section cut is.
	$retCenter = 0;
	if($name eq $ReturnCPUName) {
		&vtrace("Will process return, choice $ReturnChoice.  Return Name is $ReturnName.");
		$retCenter = 1;
		if($slaveNum == 0) {
			$useSkyMonitor = 0;
			$dlto = $skyMonitorTimeout;
		}
	}

}


# get the shared file information
$sfb = $slave{FilewaitDir};
$sfset = "$sfb/in/settings.reg";			# formerly "setting2.reg"
$sfshadow = "$sfb/in/settings.shadow";   # formerly "setting2.shadow"
$sfgo = "$sfb/in/fit16snp";
$sfim = "$sfb/out/fitout.fit";
$sferr = "$sfb/out/error.txt";
$sfcancel = "$sfb/in/cancel";
$sfsuspend = "$sfb/in/setting2.reg";	# simply touch file by this name to suspend sky monitor expose

#####
# sanity check

if($code =~ m/id/) {
	my $name = $slave{Name};
	my $cmd = $slave{Cmd};
	my $fwd = $slave{FilewaitDir};
	my $size = $slave{Size};
	my $cutlist = $slave{SectionCuts};
	
	print "Number = $slaveNum\n";
	print "Name = $name\n";
	print "Cmd = $cmd\n";
	print "FilewaitDir = $fwd\n";
	print "Size = [ ".$$size[0].", ".$$size[1]." ]\n";
	print "Sections: \n";
	while(my $sec = shift @{$cutlist}) {
		my $cut = shift @{$cutlist};
		my $offX = shift @{$cutlist};
		my $offY = shift @{$cutlist};
		my $rot = shift @{$cutlist};
		print " $sec = $cut  $offX, $offY, $rot\n";
	}
	
	if($ReturnCPUName eq $name) {
		print("\n");
		print("Returning choice $ReturnChoice:\n");
		print("Section name: $ReturnName\n");
		print("$ReturnX, $ReturnY, $ReturnW x $ReturnH\n");
		print("Offset at $ReturnOffX, $ReturnOffY\n");
		print("Rotation: $ReturnRot\n");
	}
	
	print("GroupEnable = ");
	if(@GroupEnable[$slaveNum]) {
		print("TRUE");
	} else {
		print("FALSE");
	}
	print("\n");
	
	if($slaveNum == 0) {
		print ("useSkyMonitor = $useSkyMonitor\n");
	}
	print("\n");
		
	exit(0);
}
#####

# sky monitor ignored if not needed
# exit(0) unless($slaveNum > 0 || $retCenter || $useSkyMonitor);

# If we are the sky monitor, we must always suspend first
if($slaveNum == 0 && $code =~ /SETUP/) {
	suspendSky();
}

# GroupEnable allows us to disregard certain groups
exit(0) unless(@GroupEnable[$slaveNum] > 0);

# Validate and process name code:
if($code =~ m/SETUP/ || $code =~ m/STATUS/) {
# We're being given a command... slaves lead such oppressive lives...
	if($code =~ m/SETUP/) {
		# break down setup argument
		my @list = split /:/,$binSet;
		$ms = shift @list;
		$bx = shift @list;
		$by = shift @list;
		&vtrace("Received SETUP command $ms $bx:$by");
		# we need to get ready to wait for new incoming...
				
		# we'll do everything here except pull the trigger... the master will do that
		
		# remove any lingering format "sfgo" file
		# create settings "sfset" file for PV to slurp up
		&setupExp($ms, $bx, $by);	
		# remove any existing image file		
		unlink($sfim);
		# remove any error or cancel files
		unlink($sferr);
		unlink($sfcancel);
		
		exit(0);
	}
	if($code =~ m/STATUS/) {
		&vtrace("Received STATUS command");
		
		# return 0 when image, set and go are gone
		if(-e $sfim || -e $sfset) {
			my $stat = sprintf("%s %s %s",
				(-e $sfim) ? "IMG" : "   ",
				(-e $sfset) ? "SET" : "   ");
			&trace("STAT EXIST: $stat");
			exit(1);
		}
		# create our format "go" file
		touch($sfgo);
		
		# now wait for it to disappear
		for(my $i=0; $i<5; $i++) {
			unless(-e $sfgo) {
				exit(0);
			}
			sleep(1);
		}
		
		exit(0);
		
	}	
	
	&trace("?? Invalid Setup : $code");
	exit(1);	
}
elsif($code =~ m/x/ ) {
# Extract the center coordinates, then adjust them for the offset of this chip group
# we have offsets in minutes, in config file per slave	

	exit(0) unless($slaveNum > 0 || $retCenter);

	my $nmstr = join " ", split /x/,$code;	
	&trace("Received coordinates: $code -> $nmstr");
	
	$code = $nmstr;
	
    $shtr = 1; 		# shutter open
    		
} else {
# If our code is a shutter code, we don't have any coordinates

	my $err = 0;
	
	if($code =~ m/BIAS/) {
		&trace("BIAS requested");
		$shtr = 0;  	# shutter closed
	}
	elsif($code =~ m/THERM/) {
		&trace("THERM requested");
		$shtr = 0;		# shutter closed
	}
	elsif($code =~ m/FLAT/)	{
		&trace("FLAT requested");
		$shtr = $FakeFlats ? -2 : -1;		# secret shutter code for flat
	}
	elsif($code =~ m/CANCEL/) {
		&trace("CANCEL requested");
		# give cancel command to PV
		touch($sfcancel);			
		exit(0);
	}
	elsif($code =~ m/SkyMonitor/) {
		exit(0) unless($useSkyMonitor);		
		#if we are the sky monitor we have to do our own stampfits
	    &trace("Sky Monitor $stampcmd -r $fitsnow $ms 0 0 $bx $by");
    	system("$stampcmd -r $fitsnow $ms 0 0 $bx $by");
	    # read the coordinates
		$code = `$stampcmd -c $fitsnow`;		
		$shtr = 1;
	}
	else
	{
		$err = 1;
	}
		
	if ($err) {
		&errlog("Invalid name code argument $code\n");
	}
}

# break down our binning
my @bins = split /:/, $binSet;
$ms = @bins[0];
my $bx = @bins[1];
my $by = @bins[2];
# default to 1:1
if(!$bx) { $bx = 1; }
if(!$by) { $by = 1; }

$expStamp = $ms;

# figure out the size this image is going to be
my $wh = $slave{Size};
my $width = $$wh[0]/$bx;	#/
my $height = $$wh[1]/$by;	#/
$size = $width * $height * 2;	# size in bytes = binWidth x binHeight x 2

&vtrace("Slave Number = $slaveNum, ReturnCenter = $retCenter, Shutter = $shtr");
&vtrace("Sizes and binnings: ".$width."x".$height." $bx:$by ($size bytes)");

		
# now we do our thing

# non-sky monitor:
if($slaveNum > 0 || $retCenter || $shtr <= 0) # returning sky monitors precludes using them as monitors
{
	&vtrace("processing normally");
	
	if($testMode) {
		my $xcode = join "x", split(" ",$code);
		my $cmd = "testgo $ms $slaveNum $sfb $xcode $shtr $width $height $bx $by";
		&trace("==== RUNNING TEST PROCESS FOR TRIGGER $cmd");
		system($cmd);
	}
	# normal slave process
	&trace("Waiting for file");
	&w4file($sfim, $dlto);				# wait for image file to start to appear
	&rdImFile($sfim, $size, $shtr);		# then copy image to EOF
}
elsif($useSkyMonitor)
{
	# sky monitor process
	# do this repeatedly during exposure
	&vtrace("processing as sky monitor");
	
	# remove any existing suspend file that might still be there
	unlink($sfsuspend);

	my $secs = $ms / 1000; #/#
	my $start;
	my $first = 1;

	my $skyren = $sfim."XX";

	$shtr = 1; # sky monitors always have open shutter

	my $baseStart = $base;	# because we reuse this variable...
	
	# we want to stop the skymonitor and turn it off before the end of the actual exposure
	$secs -= 1;
		
	while($now - $start < $secs) {

		# do anything we need to do to set up for triggering...
		# touch the go file
		touch($sfgo);
		#remove any previous renamed file
		unlink($skyren);
		
		if($testMode) {
			my $xcode = join "x", split(" ",$code);
			my $cmd = "testgo -1 $slaveNum $sfb $xcode $shtr $width $height $bx $by";
			&trace("=== RUNNING TEST PROCESS FOR SKY MONITOR $cmd ");
			system("$cmd &");
		}
				
		&trace("Waiting for sky monitor file");
		&w4file($sfim, $dlto);				# wait for image file to start to appear

		# rename the file to get it out of way
		system("mv $sfim $skyren");

		if($first) {
			$first = 0;
			$start = `date +%s`;	# start timing after first image appears
		}
	
		# we only need to call rdImFile if we want to save the output of sky monitors
		if($storeSkyMonitorImgs) {
			$base = $baseStart;
			&rdImFile($skyren, $size, $shtr);		# then copy image to EOF
		}
		&trace("sending $skyren to $SkyListen fifo");
		system("cp $skyren /tmp/skyimg");
		&returnToCamera("/tmp/skyimg");
		unlink($sfim);	
		
		$now = `date +%s`;	

	}
		
	# now we need to send the "suspend" command to the sky monitor cameras so that they
	# do not create crosstalk
	suspendSky();
	
}


# Done --
exit(0);

# =======================================================================

# suspend sky monitor by sending it a reg file
sub suspendSky()
{
	&vtrace(" Suspending skymonitor exposures");
    open SFN, ">$sfsuspend" or &errlog ("$sfsuspend: $!");
    print SFN "REGEDIT4\r\n";
    close SFN;
}
	
# print usage summary and exit
sub usage
{
    my $me = $0;
    $me =~ s#.*/##;
    print "This script controls the slave computers of the 1-meter PV system\n";
    print "It is not meant to be called directly by the user\n";
    exit 1;
}

# create a file
sub touch
{
    my $fn = $_[0];
    open F, ">$fn" or print EL " Can not create $fn: $!\n";
    close F;
}

# wait for the given file to exist
sub w4file
{
    my $fn = $_[0];
    my $to = $_[1];
    my $begto = $to;
    my $etime;

    &vtrace ("Waiting $to secs for $fn");

    while (!open (F, "<$fn")) {

	    # see if we have an error message instead...
	    if(-e $sferr) {
    		my $errmsg = `head -1 $sferr`;
    		chomp($errmsg);
    		system("cp $sferr $msterr"); # copy to master error location
	    	&errlog("$fn: Error returned: $errmsg");
    	}
    	
    	# see if we've cancelled this
	    if(-e $sfcancel) {
	    	&errlog("$fn: Cancelled");
    	}
    	

    	# normal wait for timeout
	if($to-- == 0) {
	     # cancel
	     touch($sfcancel);
	     &errlog ("$fn: timeout waiting to appear");
	}
	sleep 1;
    }
    $etime = $begto - $to;
    &vtrace ("File appeared in $etime seconds") if ($to > 0);
    close F;

}


# copy image pixels from file to stdout.
# N.B. be patient, it might still be coming in
# N.B. skip the header
sub rdImFile
{
    my $fhdr = 2880;			# EXPECTED (minimum) bytes in FITS header
    my $fn = $_[0];				# file name
    my $sz = $_[1] + $fhdr;		# bytes including header
    my $shtr = $_[2];			# shutter. If zero (closed) we handle differently (calib)
    my $nrd = 0;
    my $buf;
    my $tot;

    if($shtr == -2) { # produce a fake flat version
    	&trace("Producing fake flat image");
    	open FN, ">$fn" or &errlog("$fn: $!");
    	my $cnt = $sz - $fhdr;
    	my $pix = 130;
    	while($cnt--) {
    		print FN chr($pix);
    	}
    	close FN;
    	
    	$fhdr = 0;
    }
    else
    {	
	    &vtrace ("Waiting $lanto secs for $fn to be >= $sz");

    	# wait for file to completely arrive
	    for ($tot = 0; $tot < $lanto; $tot++) {
			my $fnsz = -s ($fn);	
		
	    	# see if we have an error message instead...
		    if(-e $sferr) {
    			my $errmsg = `head -1 $sferr`;
	    		chomp($errmsg);
    			system("cp $sferr $msterr"); # copy to master error location
	    		&errlog("$fn: Error returned: $errmsg");
    		}
    	
	    	# see if we've cancelled this
	    	if(-e $sfcancel) {
		    	&errlog("$fn: Cancelled");
	    	}	
			
			&vtrace ("$tot/$lanto: $fnsz/$sz");
			last if ($fnsz >= $sz);
			sleep 1;
	    }
    	$tot < $lanto or &errlog("$fn: timeout waiting for complete");
	}

&trace("Download complete");

    # sanity fix base path
    chomp($base);
   	$len = length($base);
#    &trace(sprintf("base end info: Len = %s last char = %s",$len,substr($base,$len-1,1)));

   	unless (substr($base,$len-1,1) eq '/') {
	 	$base .= '/';
   	}
   	
	#=====

	# my $nmstr = `$stampcmd -c $fitsnow`;
	my $nmstr = $code;  # happily prepared for us above	   	
	
	#--------------------
	# This section should become separate include file that
	# can be shared with 1/2 meter ??
	# if so, replace this with my @sectnamelist = ("*");
	my @sectnamelist = ();
	my $cutlist = $slave{SectionCuts};
	my $i = 0;
    while(defined @{$cutlist}[$i]) {
    	@sectnamelist[$i/5] = @{$cutlist}[$i];
    	$i += 5;
    }

    $base = nameproc($shtr, $nmstr, $base, \@sectnamelist);

    # 2002-07-26 -- rename fitout.fit to a temporary work file to allow concurrent work
    $tmpfn = `jd -t`;
    $tmpfn = $fn.".".$tmpfn;
    &vtrace("Renaming $fn to $tmpfn");
    rename $fn,$tmpfn;
    $fn = $tmpfn;
   	
   	# read and skip header   	
   	if($shtr != -2) {
   	
	    # get the size of the header for the downloaded file
    	&vtrace ("determining size for FITS header of $fn");
    	$fhdr = 0;
	    open F, "<$fn" or &errlog ("$fn: $!");
	
		my $ln = 0;
		my $xit = 0;

		while(!$xit) {
   			for($i=0; $i< 36; $i++) {
	   			$nrd = read(F, $lntxt, 80);
				&errlog ("$fn: $!") if (!defined($nrd));
	   	   		#   &trace(sprintf("%d) %s\n", $i, $lntxt));
		   		$ln += 80;
	  			if(substr($lntxt,0,3) eq "END") { $xit=1; }
	    	}
		}
			
		close F;
	
		$fhdr = $ln;
	}
		
   	# see if we've cancelled this
    if(-e $sfcancel) {
    	unlink $fn;		# remove the temp file
    	&errlog("$fn: Cancelled");
   	}
	
	&vtrace("FITS header of $fn == $fhdr bytes");
	
    # invoke Mosaic to cut up incoming image and save components.

    my $cuts = "";
    my $sectionNames = "";
    my @offX;
    my @offY;
    my @rot;
    if($retCenter) {
	    # Save a return component as section 00
    	# cut up parameters include return section as section 00
    	$cuts = "-p $ReturnX $ReturnY $ReturnW $ReturnH ";
    	$sectionNames = $ReturnName.",";
    }
    my $i = 0;
    my $cutlist = $slave{SectionCuts};
    while(defined @{$cutlist}[$i]) {
    	$sectionNames .= @{$cutlist}[$i].",";		# build our comma-delimited name list
    	$cuts .= "-p ".@{$cutlist}[$i+1]." ";		# and our section cut parameters
    	@offX[$i/5] = @{$cutlist}[$i+2];
    	@offY[$i/5] = @{$cutlist}[$i+3];
    	@rot[$i/5] = @{$cutlist}[$i+4];
    	$i += 5;
    }

    # Correct fits header to match this group
    # we have to do this on a copy because the other slaves will fight over it
    my $tmpfits = "/tmp/tmpfits".$slaveNum;
  	$rt = system("cp $fitsnow $tmpfits");
  	&vtrace("$rt : copying header stamp to $tmpfits");
    my $size = $slave{Size};
    my $width = $$size[0];
    my $height = $$size[1];
    # stamp the group size to the header
	my $cmd = "fitshdr -i NAXIS1 $width -i NAXIS2 $height $tmpfits";
	$rt = system($cmd);
	&trace("$rt : $cmd");

   	# see if we've cancelled this
    if(-e $sfcancel) {
    	unlink $fn;		# remove the temp file
    	&errlog("$fn: Cancelled");
   	}
	
	# now cut it up...
    $rt = `mosaic -n $base -i $fn -b $fhdr -h $tmpfits -s $sectionNames $cuts`;
    &trace("$rt : mosaic -n $base -i $fn -b $fhdr -h $tmpfits -s $sectionNames $cuts");

    # if we have a return section to deal with, do that first
    if($retCenter) {
    	my $retSect = $base.$ReturnName.".fts";
    	#rotate if needed
		if($ReturnRot) {
			my $rts = $ReturnRot;
			my $rc = $rts > 0 ? "-xl" : "-xr";
			if($rts < 0) { $rts = -$rts; }
			unless($rts & 1) { $rc = "" }
			if($rts & 2) { $rc .= " -c"; }
			if($rts & 4) { $rc .= " -r"; }
			$rt = system("flipfits $rc $retSect");
			&trace("$rt : flipfits $rc $retSect");										
		}
    	&trace("Offsetting return section by $ReturnOffX, $ReturnOffY pixels");
    	&adjustFITSheader($retSect, $code, $ReturnOffX, $ReturnOffY, $ReturnW, $ReturnH);
		# copy return image to where master script is looking for it					
		$rt = system("cp $retSect $ReturnImgFile");
		&trace("$rt : cp $retSect $ReturnImgFile");
	}
	
	# Thanks to our friends at PV, the output format of (most of) the group files can't simply
	# be cut up by mosaic, as they need to be separated, rotated, and placed individually

	$i = 0;
	while(defined(@offX[$i])) {
		my $detletter = @{$cutlist}[$i * 5];
		my $detname = $base.$detletter.".fts";
    	my $cutstr = @{$cutlist}[$i * 5 + 1];
    	my @frm = split / /,$cutstr;
		# rotate if needed
		if(@rot[$i]) {
			my $rts = @rot[$i];
			my $rc = ($rts & 1) ? ($rts > 0 ? "-xl" : "-xr") : "";
			if($rts < 0) { $rts = -$rts; }
			if($rts & 2) { $rc .= " -c"; }
			if($rts & 4) { $rc .= " -r"; }
			$rt = system("flipfits $rc $detname");
			&trace("$rt : flipfits $rc $detname");										
		}					
		
		# apply offset from target center and stamp this as RA DEC
	    &trace("Offsetting section $detletter by ".@offX[$i]." x ".@offY[$i]." pixels");
		&adjustFITSheader($detname, $code, @offX[$i],@offY[$i], @frm[2], @frm[3]);
		
		$i++;
	}		

    ###########
    # We invoke the rest of the post-acquistion pipeline process here
    # which is run in a separate script, in the background
	#

   	# see if we've cancelled this
    if(-e $sfcancel) {
    	unlink $fn;		# remove the temp file
    	&errlog("$fn: Cancelled");
   	}
		
	my $process = 0;
	my $record = 1;
	
	if($slaveNum == 0 && $shtr > 0) {		# don't record sky monitor images if we say not to, but always save corrections
		$record = $storeSkyMonitorImgs;
	}
	
	if($enablePostProcessing) { # heed option
		if($slaveNum > 0) { # don't pipeline sky monitor images
		    if($shtr > 0) { # don't pipeline correction images
		    	$process = 1;
			}
		}
	}	    	
	
	my $pipeCmd = "nice pipeln1m $base $process $record";
	if($testMode) {
		$pipeCmd .= " $name";
	}
	my $rt = system("$pipeCmd &");
	&trace("$rt : $pipeCmd &");

   	unlink $fn;		# remove the temp file	
		
	# That's it. Now wasn't that simple? ;-)	
}

# tell camera the new settings
sub setupExp
{
    my $ms = $_[0];
    my $bx = $_[1];
    my $by = $_[2];

    unlink($sfgo); # remove any old trigger file

# use shadow file to change setttings only if binning changes
# for the skymonitor, we have to do it each time, however.

	my $changed = 1;
	%keyval = ();
	# check shadow file
	if($slaveNum > 0) {
		&vtrace("Checking shadow file");
		if(-e $sfshadow) {
			$changed = 0;
			open SHFN, "<$sfshadow" or errlog("$sfshadow: $!");
			while($line = <SHFN>) {
				chomp($line);
				my @s = split /=/,$line;
				$keyval{@s[0]} = @s[1];
			}
			close SHFN;
		
			my $sbx = $keyval{"BINX"};
			my $sby = $keyval{"BINY"};
			if($sbx != $bx
			|| $sby != $by ) {		
				&vtrace("Shadow file $sbx:$sby vs. new $bx:$by");
				$changed = 1;
			}					
		}
	}
		
	if($changed) {
		&vtrace("Writing new values to shadow file");
		open SHFN, ">$sfshadow" or errlog("$sfshadow: $!");
		print SHFN "BINX=$bx\n";
		print SHFN "BINY=$by\n";
		close SHFN;
		
	    &trace ("setupExp: $ms ms, $bx X $by");

	    open SFN, ">$sfset" or &errlog ("$sfset: $!");
	    print SFN "REGEDIT4\r\n";
    	print SFN "\r\n";
    	print SFN "[HKEY_LOCAL_MACHINE\\SOFTWARE\\PixelVision\\PixelView\\3.2\\Library\\settings]\r\n";
#    	printf SFN "\"Exposure Time\"=dword:%08x\r\n", $ms;
   	 	printf SFN "\"Bin X Size\"=dword:%08x\r\n", $bx;
   	 	printf SFN "\"Bin Y Size\"=dword:%08x\r\n", $by;
	    close SFN;
	}
		
	if($testMode) {
		my $mss = $slaveNum > 0 ? $ms : -1;
		&trace("=== RUNNING TEST PROCESS FOR SETUP --> $mss $slaveNum $sfb ");
		system("testgo $mss $slaveNum $sfb &");
	}

}

##########
#
# Make a modified copy of the "fitsnow" file that properly
# reflects this group in relation to the target center
#
# Center coordinates of primary FITS file will be offset from
# The camera target center. Mosaic will then adjust from this.
#
# Width and height will be set in the header to match the actual
# image from PV.
#
# Changed to use offsetfits utility 12-13-2002
#
sub adjustFITSheader
{
	my $tmpfits = $_[0]; # location of fits header
	my $nmstr   = $_[1]; # our coordinate string
	my $roff    = $_[2]; # ra offset to apply
	my $doff	= $_[3]; # dec offset to apply
	my $width   = $_[4]; # width of section
	my $height  = $_[5]; # height of section
	
	my $HH = int(substr($nmstr,0,3));
	my $MM = int(substr($nmstr,4,2));
	my $SS = 0.0 + (substr($nmstr,7,5));
    my $DD = int(substr($nmstr,13,3));
	my $DM = int(substr($nmstr,17,2));
    my $DS = 0.0 + (substr($nmstr,20,4));

    my $raval = ($HH + $MM/60 + $SS/3600) * 15; #/# convert to degrees
    my $DA = abs($DD);
	my $decval = $DA + $DM/60 + $DS/3600;	    # in degrees
	if($DD < 0) {
		$decval = -$decval;
	}

    my $cmd = "offsetfits $tmpfits $roff $doff $raval $decval";
	$rt = system($cmd);
	&trace("$rt : $cmd");	

    my $nmtag = (split "/",$tmpfits)[-1];
	# stamp all of this onto new header	
	# also, remove RAEOD/DECEOD and OBJRA/OBJDEC because we don't adjust these
	# Plus, add the filename, and set BZERO to 0 for our PV camera versions.
    $cmd = "fitshdr -c 'Filename: $nmtag' -i BZERO 0 $tmpfits";
	$rt = system($cmd);
	&trace("$rt : $cmd");	
}

# Return image to camera
# Used by sky monitor to return image to camera app
sub returnToCamera
{
	my $retfile = $_[0];
	
	my $camfifo = "$TELHOME/comm/$SkyListen";
	
	&vtrace("returning $retfile to $camfifo");
	
	if(-p $camfifo) {
	
		# we spawn a process to output the filename to the fifo
		# but it might hang if the camera that owns it is gone, so we have
		# to wait a bit then look to see if it's still running
		# and if so, kill it.
	
		system("echo $retfile >> $camfifo &");
		sleep 5;
		open P, "ps T |" or errlog("can't fork for ps: $!\n");
		while(<P>) {
			chomp();
			 # &trace("ps: $_");
			if(m`sh -c echo`) {
				my $pid = substr($_,0,5);
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

sub mssleep
{
	my $ms = $_[0];
	select (undef, undef, undef, $ms/1000.);
}


# append $_[0] to STDOUT and timestamp + $_[0] to $elog and exit
sub errlog
{
	print "($name): $_[0]";
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


# For CVS Only -- Do Not Edit
# $Id: pvc1m-slave.pl,v 1.1.1.1 2003/04/15 20:48:39 lostairman Exp $

