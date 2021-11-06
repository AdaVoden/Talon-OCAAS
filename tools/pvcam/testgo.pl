#!/usr/bin/perl
#
# This will serve as a simple test case and surrogate PV camera
#
# Called when we set up an exposure on a slave, and again after triggering has occurred
#
# When called for setup, this will:
# 	remove sfset
#   (at this point, the real camera would get triggered by the master)
#
# When called for exposure:
# The static version of this will:
#	wait for a period to pretend like we're exposing
#   wait for a period to pretend like we're downloading
#	copy the file for this group from the fake stash to the destination
#
# The online version of this will:
# 	adjust the given coordinates to select an offset that matches the group we're simulating
#	call the stsci for an image from this location of the appropriate size into the destination
#
#
# The static data is pre ordered so that it can test rotations and other oddities of PV layout methods
# The online version brings in dynamic data, but it is not rotated.

$ONLINE_VERSION = 0; # set to 1 for stsci internet images, 0 for static images stored on disk
# Make sure you have the appropriate configure file set for each mode!

$elog = "/usr/local/telescope/archive/logs/testgo.log";

# settings for vcam
$url = "http://stdatu.stsci.edu/cgi-bin/dss_search";
$survey = 1;
$harcmin = 2119/60;
$varcmin = 2119/60;

# offsets per group number
@roffsets = (0, 0, -14.858, 14.858 );
@doffsets = (0, 17.830, -26.745, -26.745);

$ms = @ARGV[0];				# exposure time
$groupNum = @ARGV[1];		# group number
$toFW = @ARGV[2];			# path of filewait directory

$roff = @roffsets[$groupNum];
$doff = @doffsets[$groupNum];

$sfim = $toFW."/out/fitout.fit";
$expStamp = $ms/100;

if(@ARGV > 3) {
	$code = @ARGV[3];	
	$shtr = @ARGV[4];
	$width = @ARGV[5];
	$height = @ARGV[6];
	$bx = @ARGV[7];
	$by = @ARGV[8];
}
else
{	
	# setup only
	&trace("Called for setup: $ms $groupNum $toFW");
	# remove sfset
	unlink($toFW."/in/settings.reg");
	
	exit(0);
}

&trace("Called for exposure: $ms $groupNum $toFW $code $shtr $width $height $bx $by");
if($ONLINE_VERSION) {

#TODO: something intelligent about sky monitor re-load
	getNetImage($shtr, $width, $height, $bx, $by, $code) unless(-e $toFW."/in/cancel");
	
} else {

	if($ms >= 0) {
		# we get triggered
		sleep 1;

		# we expose
		&mssleep($ms);

		# we integrate
		sleep 5; # ha!
	}
	
	# we download to filewait/out...
	@lets = ("A","B","C","D");
	#$from = "/home/steve/pvc1m/fakesrc/group".@lets[$groupNum].".fts";
	$from = "/home/ocaas/fakesrc/group".@lets[$groupNum].".fts";

	if(0) { #(rand() * 100) < 2) {
		#system("cp /home/steve/pvc1m/error.txt $toFW"."/out/error.txt");
	}
	else {
		if(-e $toFW."/in/cancel") {
			&trace("Cancel detected -- no copy");
		} else {
			&trace("Copying $from to $sfim");	
			system("cp $from $sfim") unless(-e $toFW."/in/cancel");
		}	
	}
}

exit (0);

sub mssleep
{
	my $ms = $_[0];
	select (undef, undef, undef, $ms/1000.);
}

sub getNetImage
{
	my $shtr = $_[0];
	my $width = $_[1];
	my $height = $_[2];
	my $bx = $_[3];
	my $by = $_[4];
	my $nmstr = $_[5];
	
	&trace("GroupNum = $groupNum. Offsets = $roff x $doff");
	&trace("GetNetImage: $width x $height, $shtr, $bx:$by, $nmstr");
	
	if($shtr > 0) {
		
	    # get the image from our friends at the STScI
    	&trace("start name $nmstr");
	    my $rah;
    	my $ram;
	    my $ras;
    	my $dcd;
	    my $dcm;
    	my $dcs;
    	my @l = split "x", $nmstr;
    	my $colsep = join ":", @l;
    	($rah, $ram, $ras, $dcd, $dcm, $dcs) = split ":", $colsep;
    	
    	&trace("$rah:$ram:$ras  $dcd:$dcm:$dcs");

	    my $raval = ($rah*3600 + $ram*60 + $ras) / 4; #/# convert to arcminutes
	    my $DA = abs($dcd);
		my $decval = $DA*60 + $dcm + $dcs/60;			 # in arcminutes
		if($dcd < 0) {
			$decval = -$decval;
		}
		
	    $raval += $roff;
    	$decval += $doff;

	    $raval /= 900; #/# convert back to RA hours
    	$rah = int($raval);
	    $raval -= $rah;
    	$raval *= 60;
    	$ram = int($raval);
	    $raval -= $ram;
	    $raval *= 60;
    	$ras = $raval;

	    $decval /= 60;	#/# convert to arc degrees
    	$dcd = int($decval);
	    $decval -= $dcd;
	    $decval = abs($decval);
    	$decval *= 60;
    	$dcm = int($decval);
    	$decval -= $dcm;
    	$decval *= 60;
    	$dcs = $decval;

		# frame request
		my $hminrq = ($width + 0.5) / $harcmin ;  #/ arc minutes horizontal requested
		my $vminrq = ($height + 0.5) / $varcmin;  #/ arc minutes vertical requested

		my $rqst = sprintf("%s?v=%d&r=%02d+%02d+%02d&d=%02d+%02d+%02d&e=J2000&h=%.2f&w=%.2f&f=fits&c=none&fov=NONE&v3=",
    		               $url,$survey,$rah,$ram,$ras,$dcd,$dcm,$dcs,$vminrq,$hminrq);

		&trace(sprintf("%s\n",$rqst));

		# download the image
		`lynx -dump "$rqst" > $sfim`;

		my $sz = -s ($sfim); #;		
		if($sz < $width * $height) {
			open ETL, ">$toFW/out/error.txt";
			print ETL "STScI Unavailable ";
			close ETL;
		}						
	}
	else
	{
		my $pix = 128 + int ($expStamp/16);
		if($shtr < 0) { $pix += 32; } # a fakier fake flat...
		# closed shutter returns all zeros	
		my $size = $width/$bx * $height/$by * 2;
		&trace(sprintf("Closed shutter. Filling $sfim with $size %x bytes",$pix));
		my $fits = chr($pix) x $size;
		open F, ">$sfim" or &errlog("Can't open $sfim to write fill");
		print F $fits;
		close F;
	}
}

# if $trace: append $_[0] to $elog
sub trace
{
	open TL, ">>$elog";
	$ts = `jd -t`;
	print TL "$ts: (Group $groupNum) ".$_[0]."\n";
	close TL;
}
