#!/usr/bin/perl

# This Perl script is used to verify that the component machines of the
# One Meter control and acquisition system are set up and running the
# correct services, and tries to start them if not.
#
# usage:
#    startOnem
#

$TELHOME = $ENV{TELHOME};
do "$TELHOME/archive/config/pvc1m.cfg";

# set $traceon to 1 if want trace info in the log file
# inherits from pvc1m.cfg
# (can set here to override, though)
# $traceon = 1;

# ==================================

# sync output
$| = 1;

# count warnings per computer
$warns = [0,0,0,0];

# get the slave name for each group
for(my $i=0; $i<4; $i++) {
	%gi = %{$GroupInfo->[$i]};
	my $slavename = $gi{Name};
	
	# see if this slave is available
	my $rpt = `rsh $slavename uname -n`;
 	chomp($rpt);	
	if($rpt ne $slavename) {
		warning($i,"The computer '$slavename' does not appear to be running ($rpt)");
	}
	else {
		# see if it has its raid mounted
		$rpt = `rsh $slavename cat /proc/mounts | grep /mnt/raid`;
	 	chomp($rpt);	
		if($rpt eq "") {
			message("The computer '$slavename' does not have the RAID drive mounted.");
			message("Enter root password to attempt mounting RAID drive now:");
			$rpt = `rsh $slavename su --fast --command="mount /mnt/raid"`;
			chomp($rpt);	
			# now check again
			sleep(1);
			$rpt = `rsh $slavename cat /proc/mounts | grep /mnt/raid`;
			chomp($rpt);	
			if($rpt eq "") {
				warning($i,"The RAID drive cannot be mounted for the computer '$slavenum'");
			}
		}
		# see if it has the NFS share mounted
		$rpt = `rsh $slavename cat /proc/mounts | grep $TELHOME/archive`;
		chomp($rpt);
		if($rpt eq "" ) {
			warning($i,"The computer '$slavename' has not mounted the shared $TELHOME/archive directory.");
#			message("The computer '$slavename' has not mounted the shared $TELHOME/archive directory.");
#			message("Enter root password to attempt mounting this now:");
#			$rpt = `rsh $slavename su --fast --command="mount $TELHOME/archive"`;
#			chomp($rpt);
#			# now check again
#			sleep(1);
#			$rpt = `rsh $slavename cat /proc/mounts | grep ONEM:$TELHOME/archive`;
#			chomp($rpt);	
#			if($rpt eq "" ) {
#				warning($i,"ONEM:$TELHOME/archive cannot be mounted for the computer '$slavenum'");
#			}
		}
		# see if running samba and has a camera connected
		my $try = 0;
		while($try++ < 2) {
			$rpt = `rsh $slavename smbstatus | grep 1m-cam`;
			chomp($rpt);	
			if($rpt eq "") {
				# no camera... see if samba is running at all
				$rpt = `rsh $slavenum smbstatus`;
			 	chomp($rpt);	
				if($rpt eq "") {
#					if($try > 1) {
					if($try > 0) {
						warning($i,"Samba is not running on '$slavenum'");
						last;
					}
					else {
						# see if we can start it up
						message("The Samba server is not running on $slavename.");
						message("Enter root password to attempt to start it:");
						$rpt = `rsh $slavename su --fast --command="/etc/rc.d/init.d/smb restart"`;
						chomp($rpt);
						# now check again
						message("Waiting 20 seconds for clients to register");
						sleep(20);
					}
				}
				else {
					# samba is running but no camera is here
					warning($i, "The PixelView system does not appear to be connected to $slavename");
				}
			}
			else {
				last;
			}
		}		
	}		
}

# now give summary status, and execute if all is clear

message("\nStatus Summary:");

my $status = 0;
for(my $i=0; $i<4; $i++) {
	%gi = %{$GroupInfo->[$i]};
	my $slavename = $gi{Name};
	if($warns[$i] > 0) {
		message("The computer '$slavename' has one or more errors, see above");
		$status++;
	}
	my $cmd = $gi{Cmd};
	# give the id info for each system
	system("rsh $slavename $cmd id");
	
}

if($status != 0) {
	message("$status systems do not appear to be ready.");
	message("You may still be able to use the system by disabling these cameras via the pvoptions program.");
	message("");
	message("To start the system with all components, type the following:");
	system("  pvoptions &");
	system("  camera &");
	system("  xhost +");
	system("  rsh onem-1 setenv DISPLAY onem");
	system("  rsh onem-1 skycam");
}
else {
	#start it all up
	system("pvoptions &");
	system("camera &");
	system("xhost +");
	system("rsh onem-1 setenv DISPLAY onem");
	system("rsh onem-1 skycam");
}

exit(0);

# output a message
sub message
{
	$msg =    $_[0];
	print $msg;
	print "\n";
}


# register a warning
sub warning
{
	$cpu = $_[0];
	$msg = $_[1];
	$warns[$cpu]++;
	message($msg);
}
