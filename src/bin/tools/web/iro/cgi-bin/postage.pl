#!/usr/bin/perl
# called when user wants to build a postage stamp of many images.
# args are full paths to images.
# Elwood Downey 2 May 1996

# stay in sync with subprocesses
$| = 1;

# random number for use in filename
srand time;
$rnd = int(rand(99999));

# add path to local tools
$ENV{"PATH"} .= ":/net/iro/usr/local/telescope/bin";

# give the browser some encouragement while we ..
print "Content-type: text/html\n\n";
print "<html><head>\n";
print "<title>Image Collection</title>\n";
print "</head>\n<body bgcolor=\"white\">\n";
print "<p><img src=\"/images/main_line.gif\">\n";

# make the postage-stamp collage
$giffile = "/tmp/postage.$rnd.gif";
chomp (@ARGV);
system "fitsindex -o ../html/tmp/$giffile @ARGV > /dev/null 2>&1";

# now  tell 'em where it is
print "<p><img src=\"$giffile\">\n";

print "</body></html>\n";

# remove the temp
system "(sleep 100; rm ../html/tmp/$giffile) &";
