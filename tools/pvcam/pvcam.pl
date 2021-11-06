#!/usr/bin/perl
# operate a PixelVision camera via shared file interface.
# get params from $TELHOME/archive/config/pvcam.cfg
# send errors to $TELHOME/archive/logs/pvcam.log
# see &usage() for command summary.

# get parameters
$TELHOME = $ENV{TELHOME};
do "$TELHOME/archive/config/pvcam.cfg";
do "$TELHOME/bin/naming.pl"; # unified naming

# sync output
$| = 1;

# exposure notation on closed shutter images
$expStamp = 0;

# get command
$cmd = (@ARGV > 0 and $ARGV[0] =~ /^-[^h]/) ? $ARGV[0] : &usage();

my $rcut = $$ReturnInfo[$ReturnChoice];
my @rfrm = split / /,$rcut;
$imw = @rfrm[2];
$imh = @rfrm[3];

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

# kill exposure. can't really, but at least remove trigger file
# and put cancel found out there to try to stop it
if ($cmd eq "-k") {
    &errlog ("@ARGV") if @ARGV != 1;
    unlink($sfgo);

	# now kill the first instance that is doing an exposure
    my $me = $0;
    my $killer = 0;
    $me =~ s#.*/##;
    # &vtrace("Looking for $me -x");
	open P, "ps T |" or errlog("can't fork for ps: $!\n");
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
		&touch($sfcancel);
		&trace("Killing master $pid...");
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
	
    # exposure time needed later for name stamp of closed shutter exposures.
    # just record this as a global...
    $expStamp = $ms/100;

    # sanity check parameters
    if ($x != 0 or $y != 0 or $w != $imw or $h != $imh) {
	print ("Subframing is not supported\n");
	exit 1;
    }
    if ($bx > $maxbx or $by > $maxby) {
	print ("Binning too high\n");
	exit 1;
    }

    &setupExp ($ms, $bx, $by);		# set new camera parameters

    sleep 1;				# allow time to save in registry
	# remove any existing image file		
	unlink($sfim);
	# remove any error or cancel files
	unlink($sferr);
	unlink($sfcancel);
    &touch($sfgo);			# create file to signal start exposure

    sleep 1;			# allow time to absorb
#	&mssleep(250);
#    # wait for our files to be fully absorbed if they haven't already
#    $maxTime = 10; 			# if it takes this much time to do register, we're seriously bogged down.
#	while(--$maxTime && ( (-e $sfim) or !(-e $sfgo) ) )
#	{
#		sleep 1;
#	};
#	if($maxTime == 0) {
#		if(-e $sfim) { &trace("--> image file $sfim did not disappear"); }
#		unless(-e $sfgo) { &trace("--> go file $sfgo did not appear"); }
#		errlog("Setup timeout");
#		exit(1);
#	}
    #

	&trace("waiting for exposure time");
    # wait until we reach the precribed time
    if($expSync) {
    	while($expSync - time() > 1) {
    		sleep(1);
    	}
    	if($expSync - time() == 1) {
    		&mssleep(500);
    	}
    }

	&trace("doing exposure");
    &doExposure ($shtr, $ms);		# control shutter, return in $ms

    # stamp the header for this exposure 
    &trace("$stampcmd -r $fitsnow $ms $truew $trueh $bx $by");
    system("$stampcmd -r $fitsnow $ms $truew $trueh $bx $by");

    print "\n";				# one dummy char to signal end of exp

    &w4file($sfim, $dlto);		# wait for image file to start to appear
#    my $sz = $w/$bx*$h/$by*2;		# expected number of bytes for pixels
    my $sz = $truew/$bx*$trueh/$by*2; 
   &rdImFile($sfim, $sz, $shtr);		# then copy image to EOF
    exit 0;
}

# print usage summary and exit
sub usage
{
    my $me = $0;
    $me =~ s#.*/##;
    print "Usage: $me {options}\n";
    print "Purpose: operate a PV camera from OCAAS via share file interface\n";
    print "Options:\n";
    print " -g x:y:w:h:bx:by:shtr      test if given exp params are ok\n";
    print " -x ms:x:y:w:h:bx:by:shtr   start the specified exposure\n";
    print "                            shtr: 0=Close 1=Open 2=OOCO 3=OOCOCO\n";
    print " -k                         kill current exp, if any\n";
    print " -t temp                    set temp to given degrees C\n";
    print " -T                         current temp on stdout in C\n";
    print " -s open                    immediate shutter open or close\n";
    print " -i                         1-line camera id string on stdout\n";
    print " -f                         max `w h bx by' on stdout\n";

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

    	# normal wait for timeout
        if($to-- == 0) {
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

###
#    my $cx = $truew / 2 - $imw / 2;
#    my $cy = $trueh / 2 - $imh / 2;
###
    &trace ("Waiting $lanto secs for $fn to be >= $sz");
    # wait for file to completely arrive */
    for ($tot = 0; $tot < $lanto; $tot++) {
	my $fnsz = -s ($fn);
	&trace ("$tot/$lanto: $fnsz/$sz");
	last if ($fnsz >= $sz);
	sleep 1;
    }
    $tot < $lanto or &errlog("$fn: timeout waiting for complete");

    # sanity fix base path
    chomp($base);
   	$len = length($base);
#    &trace(sprintf("base end info: Len = %s last char = %s",$len,substr($base,$len-1,1)));

   	unless (substr($base,$len-1,1) eq '/') {
	 	$base .= '/';
   	}
   	
   	# ===
   	# unified naming
   	my $nmstr = `$stampcmd -c $fitsnow`;
   	my @sectnamelist = ("*");
   	$base = nameproc($shtr, $nmstr, $base, \@sectnamelist);
	# ===

   	# read and skip header   	
   	if(1) {
   	
	    # get the size of the header for the downloaded file
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
	
		close F;
	
		$fhdr = $ln;
	}
	
    # invoke Mosaic to cut up incoming image and save components.
    # Save a return component as section 00
    # cut up parameters include return section as section 00

    my $rcut = $$ReturnInfo[$ReturnChoice];
    my $cuts = "-p $rcut ";
    my $i = 0;
    while(defined $xywhdef[++$i]) {
    	$cuts .= "-p $xywhdef[$i] ";
    }

    $rt = `$mosaicCmd $orient -n $base -i $fn -b $fhdr -h $fitsnow -s $sectionNames $cuts`;
    &trace("$rt : $mosaicCmd $orient -n $base -i $fn -b $fhdr -h $fitsnow -s $sectionNames $cuts");
    ###########

    my @sect = split /,/, $sectionNames;
    
    # new:020207:  we need to run through all three sections and stamp name and bzero info to fits header
    for(my $si=0; $si<3; $si++) {
    	my $nmtag = $base.$sect[$si];
	$nmtag = $nmtag.".fts";
        my $nmf = (split "/",$nmtag)[-1];

	$cmd = "fitshdr -i BZERO 0 -c 'filename: $nmf' $nmtag";
        $rt = `$cmd`;
	&vtrace("$rt: $cmd");
    }

    # We invoke the rest of the post-acquistion pipeline process here
    # which is run in a separate script, in the background
	#
    # We don't call the pipeline script after sending pixels, because
    # ccdcamera (which invoked us) will kill this process after receiving
    # all the pixels in the return.

    if($shtr > 0) { # don't pipeline correction images

    	if($enablePostProcessing) {
		   	my $rt = system("$pipelnCmd $base &");
			&trace("$rt : $pipelnCmd $base &");
		}
	}
	
    ###########
	# Return the center section
	# from here on, we work with the center section (section 00)
    $fn = sprintf("%s%s.fts",$base,$sect[0]);
    &vtrace("center return file name = $fn");

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

    &trace ("doExp: $ms ms, shtr code $shtr");

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

    &trace ("Exp complete");
}

# tell camera the new settings
sub setupExp
{		
    my $ms = $_[0];
    my $bx = $_[1];
    my $by = $_[2];

	&vtrace ("setupExp: $ms ms, $bx X $by");

    unlink($sfgo); # remove any old trigger file

	my $changed = 1;
	%keyval = ();
	# check shadow file
#	&trace("Checking shadow file");
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
			&trace("Shadow file $sbx:$sby vs. new $bx:$by");
			$changed = 1;
		}					
	}
	
	if($changed) {
		&trace("Writing new values to shadow file");
		open SHFN, ">$sfshadow" or errlog("$sfshadow: $!");
		print SHFN "BINX=$bx\n";
		print SHFN "BINY=$by\n";
		close SHFN;
		
	    open SFN, ">$sfset" or &errlog ("$sfset: $!");
	    print SFN "REGEDIT4\r\n";
    	print SFN "\r\n";
    	print SFN "[HKEY_LOCAL_MACHINE\\SOFTWARE\\PixelVision\\PixelView\\3.2\\Library\\settings]\r\n";
#    	printf SFN "\"Exposure Time\"=dword:%08x\r\n", $ms;
   	 	printf SFN "\"Bin X Size\"=dword:%08x\r\n", $bx;
   	 	printf SFN "\"Bin Y Size\"=dword:%08x\r\n", $by;
	    close SFN;
		
    	# wait for file to disappear
	    my $to;
    	for ($to = 0; $to < 10; $to++) {
			if (open X, "<$sfset") {
			    close X;
	    		return;
			}
			sleep 1;
	    }
    	&errlog ("$sfset: did not disappear");
	}		
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
	    $cmd = "$lpiocmd -1 $lpshut";
	} else {
	    $cmd = "$lpiocmd -0 $lpshut";
	}
	system ("$cmd");
	&trace ("$cmd");
}

sub trigger
{
	my $start = $_[0];
	my $cmd;

	if ($start > 0) {
	    $cmd = "$lpiocmd -0 $lptrig";
	} else {
	    $cmd = "$lpiocmd -1 $lptrig";
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

