#!/opt/local/bin/perl
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

# random number for use in filename
srand time;
$rnd = int(rand(99999));

# add path to local tools
$ENV{"PATH"} = $ENV{"PATH"} . ":/net/inferno/home/ecdowney/telescope/runtime/bin" . ":/net/inferno/home/ecdowney/bin/netpbm";

# amount to compress fts when making fth
$hcomp = 100;

# get the args
$how = $ARGV[0];
$fn = $ARGV[1];

# first see if the file even exists
unless (-r $fn) {
    print "Content-type: text/html\n\n";

    print "<html><head>";
    print "<title>No ATF file</title>";
    print "</head><body bgcolor=\"white\">\n";
    print "<p><img src=\"../images/main_line.gif\">\n";
    print "<h1>$fn: not found<h1>\n";
    print "</body></html>";
    exit 0;
}

if ($how eq "any") {
    # just send a literal file
    $_ = $fn; s#.*/##;
    print "Content-type: application/octet-stream;name=$_\n\n";
    system "cat $fn";
    exit 0;
}

if ($fn =~ /fts$/) {
    if ($how eq "fts") {
	# given fts want fts
	$_ = $fn; s#.*/##;
	print "Content-type: application/octet-stream;name=$_\n\n";
	system "cat $fn";
	exit 0;
    }
    if ($how eq "fth") {
	# given fts want fth
	$tmp = "../atftmp/xftp.$rnd.fts";
	system "cp $fn $tmp; fcompress -s $hcomp -r $tmp";
	$tmp = "../atftmp/xftp.$rnd.fth";
	$_ = $fn; s#.*/##; s#fts$#fth#;
	print "Content-type: application/octet-stream;name=$_\n\n";
	system "cat $tmp; rm $tmp";
	exit 0;
    }
    if ($how =~ /[\d]+/) {
	# given fts want gif
	$ftstmp = "../atftmp/xftp.$rnd.fts";
	system "cp $fn $ftstmp; fitstogif -size $how $ftstmp";
	$giftmp = "/atftmp/xftp.$rnd.gif";
	&sendGif ($giftmp, $ftstmp, $fn);
	system "(rm $ftstmp; sleep 120; rm ../$giftmp) &";
	exit 0;
    }
}

if ($fn =~ /fth$/) {
    if ($how eq "fts") {
	# given fth want fts
	$tmp = "../atftmp/xftp.$rnd.fth";
	system "cp $fn $tmp; fdecompress -r $tmp";
	$tmp = "../atftmp/xftp.$rnd.fts";
	$_ = $fn; s#.*/##; s#fth$#fts#;
	print "Content-type: application/octet-stream;name=$_\n\n";
	system "cat $tmp; rm $tmp";
	exit 0;
    }
    if ($how eq "fth") {
	# given fth want fth
	$_ = $fn; s#.*/##;
	print "Content-type: application/octet-stream;name=$_\n\n";
	system "cat $fn";
	exit 0;
    }
    if ($how =~ /[\d]+/) {
	# given fth want gif
	$fthtmp = "../atftmp/xftp.$rnd.fth";
	system "cp $fn $fthtmp; fdecompress -r $fthtmp";
	$ftstmp = "../atftmp/xftp.$rnd.fts";
	system "fitstogif -size $how $ftstmp";
	$giftmp = "/atftmp/xftp.$rnd.gif";
	&sendGif ($giftmp, $ftstmp, $fn);
	system "(rm $ftstmp; sleep 120; rm ../$giftmp) &";
	exit 0;
    }
}

# send one gif, @_[0], with fits headers from @_[1] and named from @_[2]
sub sendGif {
    local ($gif,$fts,$ftsname) = @_;

    $_ = $ftsname; s#.*/##; s#fth$#fts#;

    print "Content-type: text/html\n\n";
    print "<html><head>";
    print "<title>ATF image $_</title>";
    print "</head><body bgcolor=\"white\">\n";
    print "<p><img src=\"../images/main_line.gif\">\n";
    print "<h1>$_</h1>\n";
    print "<br><img src=\"$gif\">\n";
    print "<br><em>Image has been equalized and inverted.</em>\n";
    print "<p><h1>FITS Header for $_</h1>\n";
    print "<p><pre>\n";
    system "fitshdr $fts";
    print "</pre>\n";
    print "</body></html>\n";
}

# if get here something did not work right.
print "Content-type: text/html\n\n";
print "<html><head>";
print "<title>ATF file error</title>";
print "</head><body bgcolor=\"white\">\n";
print "<br><img src=\"../images/main_line.gif>\n";
print "<h1>Error fetching $fn.<h1>\n";

print "</body></html>";
