#!/usr/bin/perl
# a quick hack to plot wx.log as a gif file (wx.gif).
# we need gnuplot, ppmtogif and jd.
# Elwood Downey 3/19/98

# first arg is name of weather log, else use default.
$wxlog = $#ARGV == 0 ? $ARGV[0] : $ENV{"TELHOME"} . "/archive/logs/wx.log";

# set desired timezone offset, hours behind UT
$tzoff = 7;

# set desired plot title
$ttl = "Iowa Robotic Observatory Weather Data";

# set desired name of output file
$outfn1 = "wx1.gif";
$outfn2 = "wx2.gif";
$outfn3 = "wx3.gif";
$outfn4 = "wx4.gif";

# generally no need to change anything from here on.
# ===========================================================================
# create a temp file
$tmpfn = "/tmp/wx.$$";
open TMPF, ">$tmpfn" or die "Can not create $tmpfn";

# check for wxlog
die "$wxlog: $!" if (! -r $wxlog);

# find jd step size for 100 steps
$_ = `head -1 $wxlog`; chop(); ($fjd,$tmp) = split();
$_ = `tail -1 $wxlog`; chop(); ($ljd,$tmp) = split();
$jdstep = ($ljd - $fjd)/100;

# open and read weather log.
# convert jd to year_daynumber for a sensible abscissa.
open WXLOG, "$wxlog" or die "Can not open $wxlog";
while (<WXLOG>) {
    chop();
    ($jd,$ws,$wd,$temp,$hum,$pres,$rain) = split();
    $jd -= $tzoff/24;
    next if ($jd - $lastjd < $jdstep && $jd != $ljd);
    $lastjd = $jd;
    if (!$jd0) {
	# first time through find year, jd of jan 1, and first date.
	($x,$fmn,$fdy,$x,$x,$year) = split(/ +/, `jd $jd`);
	chop($year);
	$jd0 = `jd 01010000$year.00`;
	chop($jd0);
	$fdn = int($jd-$jd0); # first whole day number -- used for x label
	$year -= 1900;
    }
    $doy = $jd - $jd0;
    printf TMPF "$year%08.4f", $doy;
    $pres -= 1000;
    print TMPF " $ws $wd $temp $hum $pres $rain\n";
}
close WXLOG;
close TMPF;

# open a connection to gnuplot and filter through ppmtogif
# throw ppmtogif's chatter away
# this is plot 1 of 4.
open GNUPLOT, "|gnuplot|ppmtogif > $outfn1 2> /dev/null" or die "Can not connect to gnuplot";
select GNUPLOT;
$| = 1;

# generate pbm on stdout
print "set terminal pbm small color\n";

# set some plot options
print "set xlabel 'YrDay, Day $fdn = $fmn $fdy; TZ = UT - $tzoff Hrs'\n";
print "set data style lines\n";
print "set title '$ttl'\n";

# go
print "plot";
print "'$tmpfn' using 1:4 title 'Temp, C'";
print ", '$tmpfn' using 1:6 title 'Pressure, mBar-1000'";
print "\n";

# closing will block until gnuplot is finished -- just what we want
close GNUPLOT;

# open a connection to gnuplot and filter through ppmtogif
# throw ppmtogif's chatter away.
# This is plot 2 of 4

open GNUPLOT, "|gnuplot|ppmtogif > $outfn2 2> /dev/null" or die "Can not connect to gnuplot";
select GNUPLOT;
$| = 1;

# generate pbm on stdout
print "set terminal pbm small color\n";

# set some plot options
print "set xlabel 'YrDay, Day $fdn = $fmn $fdy; TZ = UT - $tzoff Hrs'\n";
print "set data style lines\n";
print "set title '$ttl'\n";

# go
print "plot";
print "'$tmpfn' using 1:4 title 'Temp, C'";
print ", '$tmpfn' using 1:5 title 'Humidity, %'";
print "\n";

# closing will block until gnuplot is finished -- just what we want
close GNUPLOT;

# open a connection to gnuplot and filter through ppmtogif
# throw ppmtogif's chatter away.
# This is plot 3 of 4
open GNUPLOT, "|gnuplot|ppmtogif > $outfn3 2> /dev/null" or die "Can not connect to gnuplot";
select GNUPLOT;
$| = 1;

# generate pbm on stdout
print "set terminal pbm small color\n";

# set some plot options
print "set xlabel 'YrDay, Day $fdn = $fmn $fdy; TZ = UT - $tzoff Hrs'\n";
print "set data style lines\n";
print "set title '$ttl'\n";

# go
print "plot";
print "'$tmpfn' using 1:7 title 'Rain, mm'";
print "\n";

# closing will block until gnuplot is finished -- just what we want
close GNUPLOT;

# open a connection to gnuplot and filter through ppmtogif
# throw ppmtogif's chatter away.
# This is plot 4 of 4

open GNUPLOT, "|gnuplot|ppmtogif > $outfn4 2> /dev/null" or die "Can not connect to gnuplot";
select GNUPLOT;
$| = 1;

# generate pbm on stdout
print "set terminal pbm small color\n";

# set some plot options
print "set xlabel 'YrDay, Day $fdn = $fmn $fdy; TZ = UT - $tzoff Hrs'\n";
print "set data style lines\n";
print "set title '$ttl'\n";

# go
print "plot";
print "  '$tmpfn' using 1:2 title 'Wind speed, kph'";
print "\n";

# closing will block until gnuplot is finished -- just what we want
close GNUPLOT;


# clean up temp file
#unlink ("$tmpfn");
