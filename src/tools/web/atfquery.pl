#!/opt/local/bin/perl -- -*- ATF -*-
# atfquery.pl: called to digest atfquery.html.
# Elwood Downey
# 30 Apr 96: begin
# 17 Jun 96: really check obs codes via obs.txt
#  5 Sep 96: reverse the order of the table of completed per-user images
#            look in ccdoper/user/images too for per-user images
#            search and report N(ew) telrun.sls entries for a user
# 10 mar 97: ignore lines beginning with ! or # in obs.txt
#	     allow for location field in obs.txt

# handy tools
require "cgi-lib-96.pl";
require "ctime.pl";

$ver = 1.1;

# Admin password
$adminPW = "chipsahoy!";

# location of user logs
$logdir = "/net/gastro3/home1/ccdoper/user/logs";

# location of ccdoper staging area
$ccdoperims = "/net/gastro3/home1/ccdoper/user/images";

# location of telrun file
$telrun = "/net/gastro3/home1/ccdoper/archive/telrun/telrun.sls";

# location of user account info file
$obstxt = "/net/gastro3/home1/ccdoper/obs.txt";

# lines per run group in sls file
$lpg = 26;

# where net schedules go when first submitted
$subdir = "/net/gastro3/home1/ccdoper/user/schedin/netin";

# where net schedules go when accepted
$accdir = "/net/gastro3/home1/ccdoper/user/schedin/netin/accepted";

# dirs in which to hunt for images
@imdirs = (
    "/net/inferno/export/lucifer/astrolabs/labimage/webimages",
    "/net/rlmsun/export/vlb/ftp/pub/remote/images",
    "/net/gastro3/home1/ccdoper/user/images"
);

# URL of web site
$url = "http://www-astro.physics.uiowa.edu";

# add path to local tools
$ENV{"PATH"} = $ENV{"PATH"} . ":/net/inferno/home/ecdowney/telescope/runtime/bin" . ":/net/inferno/home/ecdowney/bin/netpbm" . ":/net/inferno/home/ecdowney/biniro";

# fetch jd now for use in image age limiting
$jdnow = `jd`;

# avoid race conditions with any subprocesses.
$| = 1;

# a temp filename portion
srand time;
$rnd = int(rand(99999));

# Read in all the variables set by the form so we can access as $input{'name'}
&ReadParse(*input);

# Print the HTML header once
print "Content-type: text/html\n\n";
print "<html><head>\n";
print "<title>ATF Status Report</title>\n";
print "</head>\n<body bgcolor=\"white\">\n";
print "<p><img src=\"../images/main_line.gif\">\n";

# see which option was selected and process accordingly
$qtype = $input{'QType'};
if ($qtype eq "PerUser") {
    $obc = $input{'Obscode'};
    if ("$obc" eq "") {
	print "<br>Please enter a valid observer code\n";
    } else {
	&perUser();
    }
    goto wrapup;
}
if ($qtype eq "System") {
    &sysStatus("System");
    goto wrapup;
}
if ($qtype eq "Image") {
    &recentImage();
    goto wrapup;
}
if ($qtype eq "Admin") {
    &sysStatus("Admin");
    goto wrapup;
}
print "<b>Bogus query type: $qtype</b>\n";
print "<b>Bogus query type: $qtype</b>\n";
goto wrapup;

wrapup:
 
# final boilerplate
print <<ENDOFTEXT;

<HR>
<I>Latest design update:  5 Sept 1996</I><P>
<p>Use your browser's \"Go back\" feature to go back.

</body></html>
ENDOFTEXT

# produce html report of current system status
# arg is "System" or "Admin". latter requires password because it shows
# account info.
sub sysStatus
{
    if ($_[0] eq "System") {
	print "<h1>System Status:</h1>\n";

	# show telshow output
	print "<p><h3>Telescope, Dome and Camera:</h3>\n";
	print "<p><pre>\n";
	print `rsh -l ccdoper atf /usr/local/telescope/bin/telshow`;
	print "</pre><br>";

	# show telrun.sls summary, without user info
	&telrunReport(0);
    } else {
	local ($pw) = $input{'AdminPW'};
	if ("$pw" ne $adminPW) {
	    print "<p><b>Password incorrect</b>\n";
	    return;
	}

	# show telrun.sls summary, with user info
	&telrunReport(1);
    }
}

# display the most recent image
sub recentImage
{
    # show most recent image, if any
    $latestccdoper = `(ls -t $ccdoperims/*.ft[sh] 2> /dev/null) | head -1`;
    chop($latestccdoper);
    if ($latestccdoper =~ /fts$/) {
	system "cp $latestccdoper ../atftmp/latest.fts";
    }
    if ($latestccdoper =~ /fth$/) {
	system "cp $latestccdoper ../atftmp/latest.fth";
	system "fdecompress -r ../atftmp/latest.fth";
    }
    if (-e "../atftmp/latest.fts") {
	print "<h3>Most recent image:</h3>\n";
	system "fitswindow < ../atftmp/latest.fts > ../atftmp/latestw.fts";
	$tmpgif = "/atftmp/latestw.$rnd.gif"; # so browser doesn't cache
	system "fitstopnm -quiet ../atftmp/latestw.fts | pnmcut 15 15 482 482 | pnmscale -xy 241 241 | ppmtogif -quiet > ../$tmpgif";
	print "<p><img src=\"$tmpgif\">\n";
	print "<br><pre>\n";
	system "fitshdr ../atftmp/latest.fts | egrep 'OBJECT|DATE|TIME|RA  |DEC  '";
	print "</pre><br><p>\n";
	system "(sleep 10; rm ../$tmpgif) & ";
    }
}

# show total telrun.sls summary
# if $_[0] is 0 do not show any account info.
sub telrunReport
{
    local ($showUI) = $_[0];
    local ($nl,@sls,$i,$base);
    local (%done_s,%fail_s,%new_s,%title);
    local (%all_obs);
    local (%names);
    local ($nn,$nd,$nf,$ntot);
    local ($tn,$td,$tf);
    local ($x,$v);
    local ($ocode);
    local ($date0,$date1,$time0,$time1);

    # read the telrun file into the sls array
    if (!open (SLS, "$telrun")) {
	print "Can not open Q file\n";
	return;
    }
    $nl = 0;
    while (<SLS>) {
	chop($_);
	$sls[$nl++] = $_;
    }
    close SLS;

    # scan for each unique obscode and collect statii
    for ($i = 0; $i < $nl; $i++) {
	# pick up first date and time
	if ($i == 0) {
	    ($x,$date0,$x) = split(/.*: /, $sls[$base+1]);       # date
	    ($x,$time0,$x) = split(/.*: /, $sls[$base+2]);       # time
	}

	# use the imagefn line to extract the obs code
	if ($i%$lpg != 25) {
	    next; 
	}
	$base = int($i/$lpg) * $lpg;
	# extract obs code
	$ocode = $sls[$base+25];
	$ocode =~ s#.*/##;
	$ocode = substr($ocode,0,3);
	($x,$v,$x) = split(/.*: /, $sls[$base+0]);       # status
	if ($v eq "F") {
	    $fail_s{$ocode}++;
	    $tf++;
	} elsif ($v eq "N") {
	    $new_s{$ocode}++;
	    $tn++;
	} elsif ($v eq "D") {
	    $done_s{$ocode}++;
	    $td++;
	} # else?? 
	($x,$title{$ocode},$x) = split(/.*: /, $sls[$base+6]);   # title
	$all_obs{$ocode}++;
    }
    # pick up last date and time
    ($x,$date1,$x) = split(/.*: /, $sls[$i-$lpg+1]);       # date
    ($x,$time1,$x) = split(/.*: /, $sls[$i-$lpg+2]);       # time

    # fetch real names if we are to show user info 
    if ($showUI) {
	if (!open (OBS, $obstxt)) {
	    print "<p><b>Can not open $obstxt</b>\n";
	    return;
	}
	while (<OBS>) {
	    if (! (/^[#!]/ || /^[ \t]*$/)) {
		local ($code,$email,$name,$loc,$comment) = split (/ *\| */);
		$names{$code} = $name;
	    }
	}
	close OBS;
    }

    # header
    print "<p><h3>\n";
    print "Observing Queue: ";
    print "$time0 $date0 through $time1 $date1 $UT\n";
    print "</h3>\n";

    # print table of obs code and count of images in each state
    print "<p><table border=5>\n";
    print "<tr>\n";
	print "  <th>Obs<br>Code</th>\n" if $showUI;
	print "  <th>Obs<br>Name</th>\n" if $showUI;
	print "  <th>Title</th>\n";
	print "  <th>Total<br>Images</th>\n";
	print "  <th>In<br>Queue</th>\n";
	print "  <th>Done</th>\n";
	print "  <th>Failed</th>\n";
	print "  <th>Percent<br>Done</th>\n";
    print "</tr>\n";
    foreach $v (sort keys %all_obs) {
	print "<tr>\n";
	if ($showUI) {
	    print "  <td>$v</td>\n";
	    $x = $names{$v} ? $names{$v} : "???";
	    print "  <td>$x</td>\n";
	}
	print "  <td>$title{$v}</td>\n";
	$nn = $new_s{$v} + 0;
	$nd = $done_s{$v} + 0;
	$nf = $fail_s{$v} + 0;
	$ntot = $nn+$nd+$nf;
	print "  <td>$ntot</td>\n";
	print "  <td>$nn</td>\n";
	print "  <td>$nd</td>\n";
	print "  <td>$nf</td>\n";
	printf "  <td>%d</td>\n", 100*$nd/$ntot+0.5;
	print "</tr>\n";
    }

    # totals in last row. TODO: how to set off nicely?
    print "<tr>\n";
    print "  <td><b>ATF</b></td>\n" if $showUI;
    print "  <td><b>ATF</b></td>\n" if $showUI;
    print "  <td><b>Totals:</b></td>\n";
    $tn += 0;
    $td += 0;
    $tf += 0;
    $ntot = $tn+$td+$tf;
    print "  <td><b>$ntot</b></td>\n";
    print "  <td><b>$tn</b></td>\n";
    print "  <td><b>$td</b></td>\n";
    print "  <td><b>$tf</b></td>\n";
    printf "  <td><b>%d</b></td>\n", 100*$td/$ntot+0.5;
    print "</tr>\n";

    print "</table>\n";
}

# produce html report of observer with code $obc
# limit report to images now more than Age days old.
sub perUser
{
    # verify valid user code
    `/net/inferno/export/brutus/Web/cgi-bin/obsok.pl "$obc"`;
    if ($?) {
	print "<h1>Observer code <em>$obc</em> is not registered.</h1>\n";
	print "<p>To request an observer code, please send your address, email address, and a brief description of observing interests to the <a href=\"mailto:ccdoper\@astro.physics.uiowa.edu\">ATF system administrator</a>.\n";
	return;
    }

    $age = $input{'Age'};

    print "<h1>Status for Observer code <em>$obc</em> as of ",`date`,":</h1>\n";

    # report status of pending submissions for $obc
    &schReport();

    # report queued runs
    &perUserTelrunReport();

    # find all images for this $obc in @obcims
    $obcsrchpat = "$obc*.ft[sh]";
    @obcims = `find @imdirs -name '$obcsrchpat' -print`;
    chop (@obcims);

    # find the log files for this user and build up a set of image info lines
    # in @images.
    $logform = sprintf ("%s*.log", $obc);
    open (FIND, "find $logdir -name '$logform' -print |");
    while (<FIND>) {
	chop();
	&oneLog ($_);
    }
    close (FIND);

    # display a table with one line of info per entry in @images
    &imReport();
}

# report status of pending submissions for $obc
sub schReport
{
    print "<b>Schedules Submitted:</b>\n";
    print "<ul>\n";
    foreach $f (<$subdir/$obc*.sch>) {
	$f =~ s#.*/##;
	print " <li>$f</li>\n";
    }
    print "</ul>\n";

    print "<b>Schedules Accepted:</b>\n";
    print "<ul>\n";
    foreach $f (<$accdir/$obc*.sch>) {
	$f =~ s#.*/##;
	print " <li>$f</li>\n";
    }
    print "</ul>\n";
}

# report all New entries in telrun.sls for $obc
sub perUserTelrunReport
{
    local ($nl,@sls,$i,$base);
    local ($x,$v);

    print "<b>Images Queued:</b>\n";

    # read the telrun file into the sls array
    if (!open (SLS, "$telrun")) {
	print "Can not open Q file\n";
	return;
    }
    $nl = 0;
    while (<SLS>) {
	chop($_);
	$sls[$nl++] = $_;
    }
    close SLS;

    # print table of all N(ew) telrun entries for obs code $obc
    print "<p><table border=5>\n";
    print "<tr>\n";
	print "  <th>Sched ID</th>\n";
	print "  <th>Filename</th>\n";
	print "  <th>Object</th>\n";
	print "  <th>UT Date</th>\n";
	print "  <th>Time</th>\n";
	print "  <th>Filter</th>\n";
	print "  <th>Exp(secs)</th>\n";
    print "</tr>\n";
    for ($i = 0; $i < $nl; $i++) {
	# ignore runs unless that have $obc in filename
	if ($i%$lpg == 25 && grep(/\/$obc[^\/]*\.fts$/, $sls[$i])) {
	    $base = int($i/$lpg) * $lpg;
	    ($x,$v,$x) = split(/.*: /, $sls[$base+0]);       # status
	    if ($v ne "N") {
		next;	# just show N(ew) entries
	    }
	    print "<tr>\n";
		($x,$v,$x) = split(/.*: /, $sls[$base+3]);   # sched
		print "  <td>$v</td>\n";
	        ($x,$v,$x) = split(/.*\//, $sls[$base+25]);  # filename
		print "  <td>$v</td>\n";
	        ($x,$v,$x) = split(/.*: /, $sls[$base+4]);   # object
		print "  <td>$v</td>\n";
	        ($x,$v,$x) = split(/.*: /, $sls[$base+1]);   # date
		print "  <td>$v</td>\n";
	        ($x,$v,$x) = split(/.*: /, $sls[$base+2]);   # time
		print "  <td>$v</td>\n";
		($x,$v,$x) = split(/.*: /, $sls[$base+20]);  # filt
		print "  <td>$v</td>\n";
	        ($x,$v,$x)  = split(/.*: /, $sls[$base+17]); # exp time
		print "  <td>$v</td>\n";
	    print "</tr>\n";
	}
    }
    print "</table>\n";
}

# process one log entry for $obc whose full filename is in $_[0].
# @obcims contains all images for $obc.
# add one line per image to @images.
# limit report to images no more than $age days old.
sub oneLog
{
    local ($lg) = $_[0];
    local ($z,$date,$time,$jd,$filt,$dur,$src,$fn,$x);
    local ($schn,$newim,$srchfn,$fullfn);

    # pull out the schedule name from the log file name
    $_ = $lg; /([\w]+)\.log/; $schn = $1;

    # scan the log file and build one entry in @images for each image
    open (LOGF, $lg);
    while (<LOGF>) {
	# dig source and Z from Starting run line
	if ($_ =~ /Starting run:/) {
	    /Z=([\d\.]+)/; $z = $1;
	    /'(.*)'/; $src = $1;
	    # print "<br>from \"$_\": z=$z src=\"$src\"\n";
	}
	# dig date, time, filt, dur and filename from Staring camera line
	if ($_ =~ /Starting camera:/) {
	    ($date,$time,$x,$x,$filt,$dur,$x) = split();
	    $time =~ s/:..:$//;
	    /([\w]+)\.ft[sh]/; $fn = $1;
	    /([\d\.]+)$/; $jd = $1;
	    $fullfn = "?";
	    foreach $x (@obcims) {
		if ($x =~ /$fn/) {
		    $fullfn = $x;
		    last;
		}
	    }
	    # print "<br>from \"$_\": date=$date time=$time filt=$filt dur=$dur fn=\"$fn\"\n";
	}
	if ($_ =~ /Run complete for/ && $jd >= $jdnow - $age) {
	    # package fields into next @image
	    # N.B. separator is |
	    $newim = "$schn|$src|$fn|$fullfn|$date|$time|$filt|$dur|$z";
	    @images = (@images,$newim);
	    # chop(); print "<br>from \"$_\": adding $newim\n";
	}
    }
    close (LOGF);
}

# display a table with one line of info per entry in @images
sub imReport
{
    local (@fullpaths);

    print "<p><b>Images Completed:</b>\n";

    print "<br><ul>\n";
    print "  <li><b>Archived:</b> No longer on line.\n";
    print "  To retrieve, send email to the <a href=\"mailto:ccdoper\@astro.physics.uiowa.edu\"> ATF system administrator</a> requesting Filenames to be reloaded.\n";
    print "  <li><b>S:</b> Display 128x128 gif image</li>\n";
    print "  <li><b>L:</b> Display 512x512 gif image</li>\n";
    print "  <li><b>F:</b> Retrieve as FITS image</li>\n";
    print "  <li><b>H:</b> Retrieve as HCompressed FITS image</li>\n";
    print "</ul>\n";

    print "<p><em>Image processing tools, including decompression, are available in the <a href=\"../IMAGE_ANALYSIS/atftools/atftools.shtml\">ATFtools</a> kit.</em>\n";

    print "<p><table border=5>\n";
    print "<caption><b>Images No More Than $age Days Old</b></caption>\n";
    print "<tr>\n";
	print "  <th>Sched ID</th>\n";
	print "  <th>Filename</th>\n";
	print "  <th>Image</th>\n";
	print "  <th>Object</th>\n";
	print "  <th>UT Date</th>\n";
	print "  <th>Time</th>\n";
	print "  <th>Z</th>\n";
	print "  <th>Filter</th>\n";
	print "  <th>Exp(secs)</th>\n";
    print "</tr>\n";
    foreach $im (reverse (sort @images)) {
	# $newim = "$schn|$src|$fn|$fullfn|$date|$time|$filt|$dur|$z";
	($schn,$src,$fn,$fullfn,$date,$time,$filt,$dur,$z) = split(/\|/,$im);
	print "<tr align=\"CENTER\">\n";
	    print "  <td>$schn</td>\n";
	    print "  <td>$fn</td>\n";
	    print "  <td>";
	    if ($fullfn eq "?") {
		print "Archived";
	    } else {
		print "<A href=\"$url/cgi-bin/atfftp.pl?128+$fullfn\">S</A> ";
		print "<A href=\"$url/cgi-bin/atfftp.pl?512+$fullfn\">L</A> ";
		print "<A href=\"$url/cgi-bin/atfftp.pl?fts+$fullfn\">F</A> ";
		print "<A href=\"$url/cgi-bin/atfftp.pl?fth+$fullfn\">H</A>";
		@fullpaths = (@fullpaths, $fullfn);
	    }
	    print "  </td>\n";
	    print "  <td>$src</td>\n";
	    print "  <td>$date</td>\n";
	    print "  <td>$time</td>\n";
	    print "  <td>$z</td>\n";
	    print "  <td>$filt</td>\n";
	    print "  <td>$dur</td>\n";
	print "</tr>\n";
    }
    print "</table>\n";

    # make an option to show an in image index
    if (@fullpaths > 0) {
	print "<br>Show a <a href=\"$url/cgi-bin/atfpostage.pl";
	foreach $p (@fullpaths) {
	    if ($p eq $fullpaths[0]) {
		print "?$p";
	    } else {
		print "+$p";
	    }
	}
	print "\">Postage Stamp Collection</a> of all available images.\n";
    }
}
