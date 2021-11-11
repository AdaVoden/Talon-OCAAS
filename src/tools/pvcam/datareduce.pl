#! /usr/bin/perl
#
# Process and image, creating a resulting second file, or a list of files (space delimited)
# Return the list of files via stdout
#
# Called by the addexpimg script
#
# Torus Technologies
# S. Ohmert 9/13/2001
#

# add any new commands to run here.
# follow the format for each line
# { cmd => " ", ext => " " },
# and put your command and options in the first set of quotes
# and the extension to append to input file to produce output filename in the second

$dataReductions = [
#	{ cmd => "findstars -h ", ext => ".fs" },
#	{ cmd => "fitshdr ", ext => ".hdr" },
];	

##################3
$TELHOME = $ENV{TELHOME};
$elog = "$TELHOME/archive/logs/datareduce.log";

$infile = $ARGV[0];
$name = `uname -n`;
chomp($name);

&usage() if($infile eq "" or @ARGV != 1);

my $i = 0;	
while (1){
	my %dr = %{$dataReductions->[$i]};
	if(defined($dr{cmd})) {
		my $cmd = $dr{cmd};
		my $ext = $dr{ext};
		$cmd .= $infile;
		my $outfile = $infile.$ext;
		my $rt = system("$cmd > $outfile");
		&trace("$rt : $cmd > $outfile");
		if($rt >= 0) {
			print "$outfile\n";
		} else {
			print ("ERROR: $rt\n");
			&errlog("ERROR $rt producing $outfile");
		}
	} else {
		last;
	}
	
	$i++;
}

exit(0);

# print usage summary and exit
sub usage
{
    my $me = $0;
    $me =~ s#.*/##;
    print "$me <image file>\n";
    print "This script handles the data reduction processes on images.\n";
    print "It is not meant to be called directly by the user\n";
    print "However, you may edit this file to add/change reduction commands.\n";
    exit 1;
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
			
