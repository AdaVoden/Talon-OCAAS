#!/usr/bin/perl

# This will handle the calibration images for Pixel Vision mosaic cameras
# send errors to $TELHOME/archive/logs/calimagePV.log
# see &usage() for command summary.
# called from telrun (JSF version)

# get parameters
$TELHOME = $ENV{TELHOME};
# don't invoke the pvcam.cfg adapter if we have a third argument (run as slave)
if(@ARGV > 2) {
	do "$TELHOME/archive/config/pvc1m.cfg";
	$sectionNames = $ARGV[2];
} else {
	do "$TELHOME/archive/config/pvcam.cfg";
}

# set $traceon to 1 if want trace info
$traceon = 1;
# error log
$elog = "$TELHOME/archive/logs/calimagePV.log";
# sync output
$| = 1;

# get our name for benefit of logging
$ourname = `uname -n`;
chomp($ourname);

# get command
$cmd = (@ARGV > 0 and $ARGV[0] =~ /^-[^h]/) ? $ARGV[0] : &usage();

&trace($cmd);

if($cmd eq '-B') {
	
	my $num = $ARGV[1];			
	calibBuild($biasName, "-B", $num);
}

if($cmd eq '-T') {
	
	my $num = $ARGV[1];			
	calibBuild($thermName, "-T", $num);
}

if($cmd eq '-F') {
	
	my $num = $ARGV[1];			
	calibBuild($flatName, "-F", $num);
}


exit(0);

# print usage summary and exit
sub usage
{
    my $me = $0;
    $me =~ s#.*/##;
    print "Usage: $me {options}\n";
    print "Purpose: orchestrate the creation and filing of new calibration images\n";
    print " in a mosaic camera context\n";
    print "Used by xobs (telrun)\n";

    exit 1;
}

# send in name, option, and number of files
# this will perform calimage command with n files on command line
sub calibBuild
{
	my $nmstr = $_[0];
	my $option = $_[1];
	my $num = $_[2];
			
	# get the most recent files	   	
    # sanity fix base path
    chomp($base);
   	$len = length($base);
   	unless (substr($base,$len-1,1) eq '/') {
	 	$base .= '/';
   	}
	   	
	# assign base the month/day tree limb
   	my $dt = `date -u +%y%m%d`;
   	chop($dt);
	$base .= $dt;							# base/010203
	
	# assign the name for our bias
	$base .= "/".$nmstr."/".$nmstr."_".$dt."_";		# base/010203/bias/bias_dddd_
	
	&trace("ready to build $num $nmstr files with $option in $base");
		
	# look for the highest numbered of these
	my $seq = 0;
	my $t = "";
	my $r = "";
	do {
		$t = sprintf("%s%04d",$base,$seq);		# base/010203/bias/bias_dddd_0000
		my $tw = $t."*";						# base/010203/bias/bias_dddd_0000 [_A.fts]
		&trace("looking for $tw");
		$r = `ls $tw 2> /dev/null`;
		$seq++;
		
	} while($r ne "" && $r ne "no match");
	
	$seq--;
	# $seq now holds the number of sequences.
	if($num > $seq) {
		&errlog("Error: $num $nmstr files requested, $seq found\n");
	}
	
	# for each section (A, B)
	@sects = split /,/, $sectionNames;
	$first = 1;
	while(defined((@sects)[0])) {
		$sect = (@sects[0]);
		shift @sects;
		
		# allow 1M workaround for return section handling
		if($sect eq "ignore_me") {
			$first = 0;
			next;
		}
		
		# build file list for calimage
		&trace("highest sequence = $seq");
		$cur = $seq - $num;
		&trace("combining $cur - $seq");
		$fl = "";
		while($cur < $seq) {
			$fl .= sprintf("%s%04d_%s.fts ",$base,$cur,$sect);
			$cur++;
		}	
	
		# the first one is the return section... it goes to the 'normal' calib folder	
		if($first) {
			$calcmd = "nice calimage $option $fl";
			$first = 0;
		}
		else
		{
			$caldir = "$TELHOME/archive/calib/".$sect;
			# make the subfolder if it doesn't exist
			unless(-e $caldir) {
				system("mkdir $caldir");				
			}
			# make the calibration image in this folder
			$calcmd = "nice calimage -d $caldir $option $fl";
		}
		
		&trace("executing: $calcmd");
		my $rt = `$calcmd`;
		if($rt != "") { &errlog($rt); }
	}

}

# append $_[0] to STDOUT and timestamp + $_[0] to $elog and exit
sub errlog
{
	print "($ourname) $_[0]";
	open EL, ">>$elog";
	$ts = `jd -t`;
	print EL "$ts: ($ourname) $_[0]\n";
	close EL;
	exit 1;
}

# if $trace: append $_[0] to $elog
sub trace
{
	return unless ($traceon);
	open TL, ">>$elog";
	$ts = `jd -t`;
	print TL "$ts: ($ourname) $_[0]\n";
	close TL;
}
