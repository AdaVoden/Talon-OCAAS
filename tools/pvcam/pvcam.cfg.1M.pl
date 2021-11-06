#----------------------------------------------------------------
# DO NOT EDIT THIS FILE. CONFIGURATION SETTINGS ARE IN PVC1M.CFG.
#
# This "configuration" script is an adapter that converts the
# configuration found in pvc1m.cfg into the format expected by
# calimagePV, which looks at pvcam.cfg.
# This script should be found on the 1-Meter installation
# The 1/2 Meter installation has an editable configuration file by this name
#
# DO NOT EDIT THIS FILE. CONFIGURATION SETTINGS ARE IN PVC1M.CFG.
#----------------------------------------------------------------

do "$TELHOME/archive/config/pvc1m.cfg";

# convert section names from pvc1m format to pvcam format <g>

if(@ARGV != 2) {
	&usage();
	exit(0);
}

for($slaveNum=0; $slaveNum<4; $slaveNum++) {
	%gi = %{$GroupInfo->[$slaveNum]};
	$name = $gi{Name};

	# Get our group info from the config array handy
	%slave = %{$GroupInfo->[$slaveNum]};

	my $retInfo = $$ReturnInfo[$ReturnChoice];
	my $rtcpu = $$retInfo[0];

	if($name eq $rtcpu) {
   		$sectionNames = $ReturnName.",";
	} else {	
		$sectionNames = "ignore_me,";
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
	
	# we have to
	
	# now call the slave to do the actual calimagePV
	if(@GroupEnable[$slaveNum]) {
		my $cmd = $ARGV[0];
		my $num = $ARGV[1];
		my $slaveCall = "calimagePV $cmd $num $sectionNames";
		if($multicpu) { $slaveCall = "rsh $name ".$slaveCall; }
		&trace("$rt : $slaveCall");
		my $rt = system($slaveCall);
	}		
}

# the original calimagePV that invoked us, on the main computer,
# is not used... so just die now that we've invoked all the slaves
exit(0);

# --------------------
