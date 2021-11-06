#!/usr/bin/perl
#
# Add an image to the jsga_imgs MySQL database
#
# Torus Technologies
# S. Ohmert 9/6/2001
# Updated 1/17/2002 for naming changes
#

use DBI;
#use strict;

$TELHOME = $ENV{TELHOME};
$traceon = 1;
$verboseTrace = 0;  # turn on to see --long-- SQL dumps in log
$elog = "$TELHOME/archive/logs/addexpimg.log";

# sync output
$| = 1;

$file = $ARGV[0];		# image file passed from pipeline script  /base/YYMMDD/X_HHMMSSdDDMMSS_YYMMDD_NNNN_section.fts
						#  or  /base/YYMMDD/bias_YYMMDD_NNNN_section.fts (or therm_ or flat_)

$SIG{__DIE__} = \&die_handler;
						
												
my @l = split '/', $file;
my $nmstr = @l[-1];
@l = split /_/, $nmstr;
my $ci = 0;
my $expType = $l[$ci++];
my $coords = "";			# the "Name" (usually encoded coordinates, but perhaps not)
if(substr($nmstr,1,1) eq "_") {
	$coords = $l[$ci++];
}
my $expDate = $l[$ci++];
my $expSeq = $l[$ci++];
my $sect = $l[$ci++];

# REMOVED RA/DEC since this came from "coord" name and is
# not accurate in new scheme.  Was somewhat superfluous anyway.
#my $raval = 0;
#my $decval = 0;
#if($coords ne "") {
#	my $HH = int(substr($coords,0,2));
#	my $MM = int(substr($coords,2,2));
#	my $SS = 0.0 + (substr($coords,4,2));
#	my $DD = int(substr($coords,7,2));
#	my $DM = int(substr($coords,9,2));
#	my $DS = 0.0 + (substr($coords,11,2));
#
#	$raval = ($HH*3600 + $MM*60 + $SS) / 4; #/# convert to arcminutes
#	$decval = $DD*60 + $DM + $DS/60;			 # in arcminutes
#	if(substr($coords,6,1) eq "s") {
#		$decval = -$decval;
#	}
#	$raval /= 60.0; 	#/# degrees
#	$decval /= 60.0; 	#/# degrees
#}

@l = split /\./, $sect;
$sect = @l[0];
$ext = @l[1];

&dbOpen("jsga_imgs","onem-1","jsga","password");

# get exposure or create new one
# (Note: changed ra/dec to 'name' 2002-01-17)
my $query = "SELECT exposure_id FROM exposures WHERE \n"
		  . "    exposure_type = '$expType' \n"
#		  . "AND exposure_ra = '$raval' \n"
#		  . "AND exposure_dec = '$decval' \n"
	. "AND exposure_name = '$coords' \n"
		  . "AND exposure_date = '$expDate' \n"
		  . "AND exposure_sequence = '$expSeq' \n";

&dbQuery($query);
# if it exists, use it
while(my @row = &dbGetNextRow()) {
	my $id = @row[0];
	if(!defined($expID)) {
		$expID = $id;
	}
	else {
		# this seems highly unlikely, but a bad thing to happen...
		&trace("**WARNING: DUPLICATE EXPOSURE ID ($id) MATCHES $nmstr. Continuing to use $rowID");
	}
}
&dbQueryDone();

# if it doesn't exist, create it.
if(!defined($expID)) {
#	&dbDo("INSERT INTO exposures (exposure_type, exposure_ra, exposure_dec, exposure_date, exposure_sequence) \n"
#	    . "VALUES ('$expType', '$raval', '$decval', '$expDate', '$expSeq') \n");
	&dbDo("INSERT INTO exposures (exposure_type, exposure_name, exposure_date, exposure_sequence) \n"
	    . "VALUES ('$expType', '$coords', '$expDate', '$expSeq') \n");
	
	&dbQuery("SELECT LAST_INSERT_ID()");
	my @row = &dbGetNextRow();
	$expID = @row[0];
	&dbQueryDone();
}	

# add image info
my $cpuName = `uname -n`;
chomp($cpuName);
my $filesize = -s ($file); #;

&dbDo("INSERT INTO images (exposure_id, image_server, image_filename, image_filesize) \n"
    . "VALUES ('$expID', '$cpuName', '$file', '$filesize')\n");

	
&dbQuery("SELECT LAST_INSERT_ID()");
my @row = &dbGetNextRow();
my $imageID = @row[0];
&dbQueryDone();
		
# add fits header info
my $fitsID = &addFITSinfo($file, $imageID);

# call on data reduction script and add the files it returns to the "reductions" table
my @drList = `datareduce $file`;
while(my $drFile = shift(@drList)) {
	my $drFilesize = -s ($drFile); #;
	&dbDo("INSERT INTO reductions (image_id, reduction_server, reduction_filename, reduction_filesize) "
		. "VALUES ('$imageID', '$cpuName', '$drFile', '$drFilesize')\n");
}

&dbClose();

&trace("Added exposure image for $nmstr: exposure_id=$expID, image_id=$imageID, fits_id=$fitsID");

exit(0);

# add fits header information
sub addFITSinfo()
{
	my $file = $_[0];
	my $imageID = $_[1];
	my @row;
	my @dbfield;
	
	&dbQuery("EXPLAIN fits");
	my $fc = 0;
	while(@row = &dbGetNextRow()) {
		my $f = @row[0];
		my $pad = 8 - length($f);
		$f .= " " x $pad;
		@dbfield[$fc++] = $f;
	}
	&dbQueryDone();

	my @taglist;
	my @vallist;
	my $count = 0;
	split /\n/, `fitshdr $file`;
	do {
		$line = $_[0];
					
		my $tag = substr($line,0,8);
		if($tag eq "TIME-OBS") {
			$tag = "TIME_OBS";
		}
		if($tag eq "DATE-OBS") {
			$tag = "DATE_OBS";
		}
		if($tag eq "DEC     ") {
			$tag = "DECL    ";
		}		
		
		for(my $i=0; $i<$fc; $i++) {
			if($tag eq $dbfield[$i]) {
							
				# now pull the value out of this
				my $s = "";
				my $j = 10;
				if(substr($line,$j,1) eq "'") {
					# read up to next quote
					$j++;
					while(1){
						my $c = substr($line,$j++,1);
						if($c ne "'"  and $c ne "") {
							$s .= $c;
						}
						else {
							last;
						}
					}
				}
				else
				{
					# read up to / or end of line
					while(1){
						my $c = substr($line,$j++,1);
						if($c ne "/" and $c ne "") {
							$s .= $c;
						}
						else {
							last;
						}
					}
				}
				
				@taglist[$count] = $tag;
				@vallist[$count] = $s;
				$count++;
				last;
			}
		}		
		
		shift();
	} while( $line ne "");
	
	my $stmt = "INSERT INTO fits (image_id";
	
	for(my $i=0; $i<$count; $i++) {
		$stmt .= ",\n";
		$stmt .= $taglist[$i];
	}
	$stmt .= "\n) VALUES ('$imageID'";
	for(my $i=0; $i<$count; $i++) {
		$stmt .= ",\n";
		$stmt .= "'".$vallist[$i]."'";
	}
	$stmt .= ")\n";

	&dbDo($stmt);
	&dbQuery("SELECT LAST_INSERT_ID()");
	@row = &dbGetNextRow();
	my $id = @row[0];
	&dbQueryDone();
	
	return $id;	
}

# ==========================

my $dbh; # database handle for db session
my $sth; # database handle for statement (query)
sub dbOpen()
{
	my $db = $_[0];
	my $host = $_[1];
	my $user = $_[2];
	my $password = $_[3];
	
	my $dsn = "DBI:mysql:$db:$host";
	$dbh = DBI->connect($dsn, $user, $password) or &errlog("Failed to connect to $db at $host");
}

sub dbDo()
{
	$stmt = $_[0];
	&vtrace($stmt);
	my $rows = $dbh->do($stmt);
	if(!defined($rows)) { &errlog("error executing statement $stmt"); }
}

sub dbQuery()
{
	$query = $_[0];
	&vtrace($query);
	$sth = $dbh->prepare($query) or &errlog("Cannot prepare query $query");
	$sth->execute() or &errlog("Cannot execute query $query");
}

sub dbGetNextRow()
{
	return $sth->fetchrow_array();
}

sub dbQueryDone()
{
	$sth->finish();
}

sub dbClose()
{
	$dbh->disconnect();
}

#########################

# print usage summary and exit
sub usage
{
    my $me = $0;
    $me =~ s#.*/##;
    print "$me <image file>\n";
    print "Purpose: To record an exposure image into the jsga_imgs database\n";
	print "Argument: image file passed from pipeline script: /base/YYMMDD/X_HHMMSSdDDMMSS_YYMMDD_NNNN_section.fts\n";
	
	exit 1;
}

# append $_[0] to STDOUT and timestamp + $_[0] to $elog and exit
sub errlog
{
	print "$_[0]";
	open EL, ">>$elog";
	$ts = `jd -t`;
	print EL "$ts:** $_[0]\n";
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

# intercept die() function from DBI
sub die_handler
{
	&errlog("DBI Die: (".$DBI::err.") ".$DBI::errstr."\n");
	die(); # superfluous
}

# For CVS Only -- Do Not Edit
# $Id: addexpimg.pl,v 1.1.1.1 2003/04/15 20:48:38 lostairman Exp $
