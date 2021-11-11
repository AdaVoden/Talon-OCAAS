#!/usr/bin/perl
# called when user wants to fetch an image or any file back.
#
# two args are required:
#  how =
#    <n>   convert $fn (an fts or fth file) to an nXn gif.
#    fts   ftp $fn as a fts file (converting from fth if necessary)
#    fth   ftp $fn as a fth file (converting from fth if necessary)
#    any   ftp $fn without any changes
#  fn =
#    full path to file
#
# if sending a fts or fth file as a gif file, embed it in an html file with its
#    full fits header.
#
# Elwood Downey
#  2 May 96 begin
# 19 Jun 96 add "any"
#  4 Apr 97 some sites are seeing the fits headers but not the gifs: increase
#           time before removing temp gif from 10 to 120 secs.

# stay in sync with subprocesses
$| = 1;

# path to staging directory wrt cgi-bin script's cwd
$cwdtmp = "../html/tmp";

# path to staging directory wrt url base
$urltmp = "/tmp";

# random number for use in filename
srand time;
$rnd = int(rand(99999));

# add path to local tools
$ENV{"PATH"} .= ":/net/iro/usr/local/telescope/bin";

# amount to compress fts when making fth
$hcomp = 100;

# get the args
$how = $ARGV[0];
$fn = $ARGV[1];

# first see if the file even exists
unless (-r $fn) {
    print "Content-type: text/html\n\n";

    print "<html><head>";
    print "<title>No file</title>";
    print "</head><body bgcolor=\"white\">\n";
    print "<p><img src=\"/images/main_line.gif\">\n";
    print "<h2>$fn: not found<h2>\n";
    print "</body></html>";
    &cleanold;
    exit 0;
}

if ($how eq "any") {
    # just send a literal file
    $_ = $fn; s#.*/##;
    print "Content-type: application/octet-stream\n\n";
    system "cat $fn";
    &cleanold;
    exit 0;
}

if ($fn =~ /fts$/) {
    if ($how eq "fts") {
	# given fts want fts
	$_ = $fn; s#.*/##;
	print "Content-type: application/octet-stream\n\n";
	system "cat $fn";
	&cleanold;
	exit 0;
    }
    if ($how eq "fth") {
	# given fts want fth
	$tmp = "$cwdtmp/xftp.$rnd.fts";
	system "cp $fn $tmp; fcompress -s $hcomp -r $tmp";
	$tmp = "$cwdtmp/xftp.$rnd.fth";
	$_ = $fn; s#.*/##; s#fts$#fth#;
	print "Content-type: application/octet-stream\n\n";
	system "cat $tmp; rm $tmp";
	&cleanold;
	exit 0;
    }
    if ($how =~ /[\d]+/) {
	# given fts want gif
	$ftstmp = "$cwdtmp/xftp.$rnd.fts";
	system "cp $fn $ftstmp; fitstogif -size $how -invert $ftstmp";
	$giftmp = "$urltmp/xftp.$rnd.gif";
	&sendGif ($giftmp, $ftstmp, $fn);
	&cleanold;
	exit 0;
    }
}

if ($fn =~ /fth$/) {
    if ($how eq "fts") {
	# given fth want fts
	$tmp = "$cwdtmp/xftp.$rnd.fth";
	system "cp $fn $tmp; fdecompress -r $tmp";
	$tmp = "$cwdtmp/xftp.$rnd.fts";
	$_ = $fn; s#.*/##; s#fth$#fts#;
	print "Content-type: application/octet-stream\n\n";
	system "cat $tmp; rm $tmp";
	&cleanold;
	exit 0;
    }
    if ($how eq "fth") {
	# given fth want fth
	$_ = $fn; s#.*/##;
	print "Content-type: application/octet-stream\n\n";
	system "cat $fn";
	&cleanold;
	exit 0;
    }
    if ($how =~ /[\d]+/) {
	# given fth want gif
	$fthtmp = "$cwdtmp/xftp.$rnd.fth";
	system "cp $fn $fthtmp; fdecompress -r $fthtmp";
	$ftstmp = "$cwdtmp/xftp.$rnd.fts";
	system "fitstogif -size $how -invert $ftstmp";
	$giftmp = "$urltmp/xftp.$rnd.gif";
	&sendGif ($giftmp, $ftstmp, $fn);
	&cleanold;
	exit 0;
    }
}

# send one gif, @_[0], with fits headers from @_[1] and named from @_[2]
sub sendGif {
    local ($gif,$fts,$ftsname) = @_;

    $_ = $ftsname; s#.*/##; s#fth$#fts#;

    print "Content-type: text/html\n\n";
    print "<html><head>";
    print "<title>image $_</title>";
    print "</head><body bgcolor=\"white\">\n";
    print "<p><img src=\"/images/main_line.gif\">\n";
    print "<font size=6>$_</font>\n";
    print "<br><img src=\"$gif\">\n";
    print "<br><em>Image has been equalized and inverted.</em>\n";
    print "<p><font size=6>FITS Header for $_</font>\n";
    print "<p><pre>\n";
    system "fitshdr $fts";
    print "</pre>\n";
    print "</body></html>\n";
}

# clean up old tmp files
# N.B. don't do the system/sleep thing because httpd waits even if use &
sub cleanold
{
    system "find $cwdtmp -type f -mtime +1 -exec rm -f {} \\;";
}

# if get here something did not work right.
print "Content-type: text/html\n\n";
print "<html><head>";
print "<title>ATF file error</title>";
print "</head><body bgcolor=\"white\">\n";
print "<br><img src=\"/images/main_line.gif>\n";
print "<h1>Error fetching $fn.<h1>\n";

print "</body></html>";
