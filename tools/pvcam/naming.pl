#!/usr/bin/perl

# Perl module used to unify naming technique used on both 1/2 meter and 1 meter telescopes


sub nameproc
{
	my $shtr = $_[0];
	my $nmstr = $_[1];
	my $base = $_[2];
	my $sectnameref = $_[3];
	my @sectnamelist = @{$sectnameref};
		
   	&vtrace("start name $nmstr");

   	# assign base the month/day tree limb
   	my $dt = `date -u +%y%m%d`;
   	chop($dt);
	$base .= $dt;
   	
    if($shtr > 0) {

    	##########
    	# nmstr format
    	# _HH:MM:SS.SS _DD:MM:SS.S			// Update 2002-09-19 additional digit in RA field
    	# 012345678901234567890123456789
    	# name format
    	# m_rrrrrrpnnnnnn_dddddd_ssss_c.fts
    	# m_xxxxxxeyyyyyy_dddddd_ssss_c.fts
    	#
    	# where m is mode, and is dependent upon future input from scheduler
        # default mode is 'X'
        # r is RA hhmmss
        # p is "n or s" declination direction
        # n is dec in ddmmss
        # d is date YYMMDD
        # s is sequence number
        # c is chip suffix
        # for fixed mode exposures (again, future input from scheduler)
        # x is azimuth, y is altitude (must be pulled from header w/updated stampfits)
        ############


	    ##########
    	# extract the information we need from our saved header in order to name this
	    my $HH = int(substr($nmstr,0,3));
    	my $MM = int(substr($nmstr,4,2));
	    my $SS = int(substr($nmstr,7,5));
    	my $DD = int(substr($nmstr,13,3));
	    my $DM = int(substr($nmstr,17,2));
    	my $DS = int(substr($nmstr,20,4));

    	$modePrefix = 'X';	# until we can revise this, our mode prefix is always "X"
    	
    	if(substr($nmstr,13,1) eq '-' or substr($nmstr,14,1) eq '-') { $DD = -$DD; $DP = 's'; }      # use n/s for +/- Dec
	    else        { $DP = 'n'; }
	
	    # round
            if($SS % 10 >= 5) {
	    	$SS+=10;
            }

	    if($SS >=60) {
		$SS -= 60;
	    	$MM++;
	    	if($MM >= 60) {
	    		$MM -= 60;
	    		$HH++;
	    		if($HH >= 24) {
	    			$HH -= 24;
	    		}
	    	}
	    }
	    if($DS >=30) {
		$DM++;
		if($DM >= 60) {
			$DM -= 60;
			$DD++;
		}
	    }
		
    	#$nmstr = sprintf("%s_%02d%02d%02d%s%02d%02d%02d",$modePrefix,$HH,$MM,$SS,$DP,$DD,$DM,$DS);
    	$nmstr = sprintf("%s_%02d%02d%01d%s%02d%02d",$modePrefix,$HH,$MM,($SS/10),$DP,$DD,$DM);
	    &vtrace("end name $nmstr");
    		
	} else {
	
		if($shtr == 0) {	
			# shutter closed means either a bias or a thermal is being taken
			# and our naming convention is special for these files
			if($expStamp == 0) {
				# zero exposure time means a bias file
				$nmstr = $biasName;
			} else {
				# non-zero exposure time is a thermal
				$nmstr = $thermName;
			}
		}
		else # $shtr < 0
		{
			# -1 is a special "code" that says this is a flat exposure
			$nmstr = $flatName;
		}
	}
	
	#--

    # make a directory in the base folder with this name
	&vtrace("root base is $base");
	unless(-d $base) {
		$rt = `mkdir $base`;		
		&trace("$rt : making directory $base");
	}
    $base .= "/".$nmstr;				# folder off base root that has our images: (root)/name
    unless(-d $base) {
		$rt = `mkdir $base`;		
		&trace("$rt : making directory $base");
    }
	$base .= "/".$nmstr."_".$dt."_"; 		# base name path of our component images: (root)/name/name_date_
	
	# now look for existing sequences of this name
	my $seq = 0;
	my $t = "";
	my $r = "";
	do {
		my $i=0;
		while(defined @sectnamelist[$i]) {	# only match against our chip names... ignore other chips
			$t = sprintf("%s%04d",$base,$seq);
			my $tw = sprintf("%s_%s.*",$t,@sectnamelist[$i]);
			$r = `ls $tw 2> /dev/null`;
			$i++;
			last if(r ne "");
		}
		$seq++;			
	} while($r ne "");
	
	$base = $t;
	$base .= "_";
	
   	&trace("base string is now $base");

   	#--------------------------------------------
   	
   	return $base;
}
