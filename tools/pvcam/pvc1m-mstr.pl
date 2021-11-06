#!/usr/bin/perl

# PixelVision 1-Meter MASTER script
# operate a PixelVision camera via shared file interface.

#	Validate exposure information
#	Register exposure with PV software
#	Stampfits for pointing info and create name
#	Activate trigger and shutter
#	Notify slave scripts to begin process of looking for new file
#	One of the slaves will return a center section filename.
#	Open and return this file to camera.

# see &usage() for command summary.

# get parameters
$TELHOME = $ENV{TELHOME};
do "$TELHOME/archive/config/pvc1m.cfg";

# set $traceon to 1 if want trace info
# (inherits from pvc1m.cfg... can set here to override, though)
# $traceon = 1;

# sync output
$| = 1;


# information about return translated to imw, imh
if(1) {
	my $retInfo = $$ReturnInfo[$ReturnChoice];
	my $rcut = $$retInfo[1];
	my @rfrm = split / /,$rcut;
	$imw = @rfrm[2];
	$imh = @rfrm[3];
}

# get command
$cmd = (@ARGV > 0 and $ARGV[0] =~ /^-[^h]/) ? $ARGV[0] : &usage();

# return current temp
if ($cmd eq "-T") {
    &errlog ("@ARGV") if @ARGV != 1;
    print "$faketemp\n";
    exit 0;
}

# return maxen
if ($cmd eq "-f") {
    &errlog ("@ARGV") if @ARGV != 1;
    print "$imw $imh $maxbx $maxby\n";
    exit 0;
}

# -g: check exp args
if ($cmd eq "-g") {
    &errlog ("@ARGV") if @ARGV != 2;
    my ($x,$y,$w,$h,$bx,$by,$shtr) = split (/:/, $ARGV[1]);
    if ($x != 0 or $y != 0 or $w != $imw or $h != $imh) {
	print ("Subframing is not supported\n");
	exit 1;
    }
    if ($bx < 1 || $bx > $maxbx) {
	print ("X binning must be 1..$maxbx\n");
	exit 1;
    }
    if ($by < 1 || $by > $maxby) {
	print ("Y binning must be 1..$maxby\n");
	exit 1;
    }
    if ($shtr < -1 || $shtr > 3) {
	print ("Shtr must be -1..3\n");
	exit 1;
    }
    exit 0;
}

# camera id string
if ($cmd eq "-i") {
    &errlog ("@ARGV") if @ARGV != 1;
    print "$id\n";
    exit 0;
}

# kill exposure.
if ($cmd eq "-k") {
	# CANCEL REMOVED because it can not be made to work properly
	# with the way Camera works
	# To force a cancel attempt, type pvmstr -C at the command line
	exit(0);
}
	
if ($cmd eq "-C") {
    &errlog ("@ARGV") if @ARGV != 1;

#    unless(-e ($expFlagFile) ) {
#		&vtrace("Exposure not in progress -- cancel ignored");
#   	exit 1;
#    }

    unlink($expFlagFile); # we've got it..

    &vtrace("Received cancel command...checking for executing master");	
	# now kill the first instance that is doing an exposure
    my $me = $0;
    my $killer = 0;
    $me =~ s#.*/##;
    # &vtrace("Looking for $me -x");
	open P, "ps x |" or errlog("can't fork for ps: $!\n");
	while(<P>) {
		chomp();
		# &vtrace("ps: $_");
		if(m`$me -x`) {
			my $pid = substr($_,0,5);
			&vtrace("Found pid $pid at $_, marking to kill");
			$killer = 1;
			last;
		}
	}
	close P;							

	if($killer) {
		# Call the slaves and make them cancel their actions
		&trace("Calling the slaves for CANCEL");
	
		my $i;
		for($i=0; $i< 4; $i++) {
			my %slave = %{$GroupInfo->[$i]};
			my $name = $slave{Name};
			my $cmd = $slave{Cmd};
			&namecall($name,$cmd,"CANCEL &");
		}

		&vtrace("Killing master $pid...");
		`kill $pid`;
	}
		
    exit 1;
}

# open or close shutter
if ($cmd eq "-s") {
    &errlog ("@ARGV") if @ARGV != 2;
    &shutter ($ARGV[1]);
    exit 0;
}

# -x: start exp, hang around until get image, send 1 byte when see something,
# rest of file to stdout, then exit.
# N.B. any error messages should have extra leading char.
if ($cmd eq "-x") {
    &errlog ("@ARGV") if @ARGV < 2;
    &errlog ("@ARGV") if @ARGV > 3;
    my ($ms,$x,$y,$w,$h,$bx,$by,$shtr) = split (/:/, $ARGV[1]);
    my $expSync = 0;
    if(@ARGV == 3) {
    	$expSync = $ARGV[2];
	}

    # sanity check parameters
    if ($x != 0 or $y != 0 or $w != $imw or $h != $imh) {
	print ("Subframing is not supported\n");
	exit 1;
    }
    if ($bx > $maxbx or $by > $maxby) {
	print ("Binning too high\n");
	exit 1;
    }

    &trace("\nBeginning $ms ms exposure $bx:$by, shutter = $shtr");

	# remove any return file we currently have
	unlink($ReturnImgFile);
	# remove any old error file
	unlink($msterr);
	
	# create the 'exposure in progress -- can be cancelled' flag file
	&touch($expFlagFile);

	# Call slaves and tell them to get ready
	&vtrace("Calling the slaves for SETUP $ms:$bx:$by");
	
	my $i;
	for($i=0; $i< 4; $i++) {
		if(@GroupEnable[$i] || $i==0) {
			my %slave = %{$GroupInfo->[$i]};
			my $name = $slave{Name};
			my $cmd = $slave{Cmd};
			&namecall($name,$cmd,"SETUP $ms:$bx:$by &");
		}
	}
	
	sleep 1; # give it a chance...

	# now wait until they all report OK
	&vtrace("Calling the slaves for STATUS");
	
	my $working = 1;
	$i = 0;
	my $cnt = 40; # 10 seconds @ 250 ms
	while(--$cnt && $working) {
		while($i<4) {
			if(@GroupEnable[$i]) {
				my %slave = %{$GroupInfo->[$i]};
				my $name = $slave{Name};
				my $cmd = $slave{Cmd};
				$working = &namecall($name,$cmd,"STATUS");
			} else {
				$working = 0;
			}
			$i++;
			if($working) {
				last;
			}
		}
		if($i >= 4) {
			$i = 0;
		}	
		
		&mssleep(250);		
	
	}
	
	if(!$cnt) {
		&errlog("Timeout waiting for status to clear\n");
	}
	
	# call upon the sky monitor -- but only if shutter is open and not a flat
	if($shtr > 0) {
		if(@GroupEnable[0] && $useSkyMonitor) {
			&vtrace("Calling group 0 as sky monitor");
			my %slave = %{$GroupInfo->[0]};
			my $name = $slave{Name};
			my $cmd = $slave{Cmd};
			&namecall($name,$cmd,"SkyMonitor $ms:$bx:$by &");
		}
	}

	&vtrace("waiting for exposure time");
	
    # wait until we reach the precribed time
    if($expSync) {
    	while($expSync - time() > 1) {
    		sleep(1);
    	}
    	if($expSync - time() == 1) {
    		&mssleep(500);
    	}
    }
	
	&vtrace("doing actual exposure");
    &doExposure ($shtr, $ms);		# control shutter, return in $ms

    # stamp the header for this exposure
    # We don't give the w/h because there is no single "true" image like on the 1/2 meter
    &vtrace("$stampcmd -r $fitsnow $ms 0 0 $bx $by");
    system("$stampcmd -r $fitsnow $ms 0 0 $bx $by");
    # read the coordinates
	$nmstr = `$stampcmd -c $fitsnow`;
	# make a name string or shutter code out of this
	my $code = $shtr > 0 ?
					join "x", split / /,$nmstr
			 : $shtr < 0 ?
			 		"FLAT"
			 : $ms > 0 ?
			        "THERM"
			 :
			 		"BIAS"
			 ;	
	
    ####
    # Call upon the slaves to do their thing.
    # wait for respective files, cut them up, and save appropriately
    &trace("Calling on slaves for action on $code $ms:$bx:$by");
	for($i=0; $i< 4; $i++) {				
		if(@GroupEnable[$i]) {
			my %slave = %{$GroupInfo->[$i]};
			my $name = $slave{Name};
			my $cmd = $slave{Cmd};
			&namecall($name,$cmd,"$code $ms:$bx:$by &");
		}
	}

    # one of these guys will return $ReturnImgFile, according the config file
    # or there will be an error file if something choked

    &w4file($ReturnImgFile, $dlto);			# wait for image file to start to appear
    print "\n";								# one dummy char to signal end of exp
    my $sz = $imw/$bx*$imh/$by*2;			# size of return image pixels
    &rdImFile($ReturnImgFile, $sz, $shtr);	# then copy image to EOF

    unlink($expFlagFile);

    exit 0;
}

# print usage summary and exit
sub usage
{
    my $me = $0;
    $me =~ s#.*/##;
    print "Usage: $me {options}\n";
    print "Purpose: operate the 1-Meter PV mosaic camera from OCAAS via share file interface\n";
    print "Options:\n";
    print " -g x:y:w:h:bx:by:shtr      test if given exp params are ok\n";
    print " -x ms:x:y:w:h:bx:by:shtr   start the specified exposure\n";
    print "                            shtr: 0=Close 1=Open 2=OOCO 3=OOCOCO\n";
    print " -k                         kill current exp, if any (WILL NOT FUNCTION -- see -C)\n";
    print " -t temp                    set temp to given degrees C\n";
    print " -T                         current temp on stdout in C\n";
    print " -s open                    immediate shutter open or close\n";
    print " -i                         1-line camera id string on stdout\n";
    print " -f                         max `w h bx by' on stdout\n";
    print " -C                         Force cancel attempt -- issued from command-line only\n";

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
# or for an error file to appear
sub w4file
{
    my $fn = $_[0];
    my $to = $_[1];
    my $begto = $to;
    my $etime;

    &vtrace ("Waiting $to secs for $fn");

    while (!open (F, "<$fn")) {

	    # see if we have an error message instead...
	    if(-e $msterr) {
    		my $errmsg = `head -1 $msterr`;
    		chomp($errmsg);
	    	&errlog("$fn: Error returned: $errmsg");
    	}

    	# normal wait for timeout
		&errlog ("$fn: timeout waiting to appear") if ($to-- == 0);
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

    &vtrace ("Waiting $lanto secs for $fn to be >= $sz");
    # wait for file to completely arrive */
    for ($tot = 0; $tot < $lanto; $tot++) {
	my $fnsz = -s ($fn);
	&vtrace ("$tot/$lanto: $fnsz/$sz");
	last if ($fnsz >= $sz);
	sleep 1;
    }
    $tot < $lanto or &errlog("$fn: timeout waiting for complete");

    $sz = -s ($fn);

    # skip FITS header
    &vtrace ("skipping FITS header of $fn");
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
	
	$tot = $ln;
	
    # all the rest are pixels
    &trace ("sending pixels after skipping $tot");
    for (; $tot < $sz; $tot += $nrd) {
		$nrd = read (F, $buf, 16384);
		&errlog ("$fn: $!") if (!defined($nrd));
		&errlog ("Bad print @ $tot/$sz") unless print $buf;
    }
    close F;

    # ccdcamera lib kills us when gets all pixels so ok if never see this
    &trace ("$fn complete");
}

# this function operates the shutter for a given duration/mode exposure.
# the camera has already been armed for an exposure in trigger mode.
# return when the exposure is complete.
sub doExposure
{
    my $shtr = $_[0];	# mode
    my $ms = $_[1];		# ms

    &vtrace ("doExp: $ms ms, shtr code $shtr");

    if ($shtr == 0) {
	# closed for the entire time
	&shutter(0);
	&trigger(1);
	&mssleep ($ms < 3 ? 3 : $ms);	# 0 dur never seems to work
	&shutter(0);
	&trigger(0);
    } elsif ($shtr == 1 || $shtr == -1) {  # include support for "special" flat shutter code
	# open the entire time
	&shutter(1);
	&trigger(1);
	&mssleep ($ms);
	&shutter(0);
	&trigger(0);
    } elsif ($shtr == 2) {
	# Dbl: OOCO
	&shutter(1);
	&trigger(1);
	&mssleep ($ms/2);
	&shutter(0);
	&mssleep ($ms/4);
	&shutter(1);
	&mssleep ($ms/4);
	&shutter(0);
	&trigger(0);
    } elsif ($shtr == 3) {
	# Mutli: OOCOCO
	&shutter(1);
	&trigger(1);
	&mssleep ($ms/3);
	&shutter(0);
	&mssleep ($ms/6);
	&shutter(1);
	&mssleep ($ms/6);
	&shutter(0);
	&mssleep ($ms/6);
	&shutter(1);
	&mssleep ($ms/6);
	&shutter(0);
	&trigger(0);
    } else {
	&errlog ("Bad shutter: $shtr");
    }

	# force shutter to "open" (will remain closed w/o trigger)
	&shutter(1);

    &vtrace ("Exp complete");
}

sub mssleep
{
	my $ms = $_[0];
	select (undef, undef, undef, $ms/1000.);
}

sub shutter
{
	my $open = $_[0];
	my $cmd;

	if ($open > 0) {
	    $cmd = "$lpiocmd -0 $lpshut";
	} else {
	    $cmd = "$lpiocmd -1 $lpshut";
	}
	system ("$cmd");
	&trace ("$cmd");
}

sub trigger
{
	my $start = $_[0];
	my $cmd;

	if ($start > 0) {
	    $cmd = "$lpiocmd -1 $lptrig";
	} else {
	    $cmd = "$lpiocmd -0 $lptrig";
	}
	system ("$cmd");
	&trace ("$cmd");
}

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

# verbose version of trace
sub vtrace
{
	return unless ($verboseTrace);
	&trace($_[0]);
}

################

# call program on slave computer
sub namecall
{
	my $name = $_[0];
	my $cmd = $_[1];
	my $args = $_[2];
	my $syscall;
	
	if($testMode) {
		if($multicpu) {
			$syscall = "rsh $name $cmd $args";		
		} else {
			$syscall = "$cmd $args";
		}	
	}
	else {
		$syscall = "rsh $name $cmd $args";
	}
	
	my $rt = system($syscall);
	&trace("--> $rt: $syscall");
	return $rt;
}

# For CVS Only -- Do Not Edit
# $Id: pvc1m-mstr.pl,v 1.1.1.1 2003/04/15 20:48:38 lostairman Exp $
