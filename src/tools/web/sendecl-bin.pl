#!/opt/local/bin/perl
# send eclipsing binary file to user
# Elwood Downey
# 21 Jun 96 Begin
#  6 May 97 New format

# handy tools
require "cgi-lib-96.pl";
require "ctime.pl";
 
# name of file to send
$fn = "/net/inferno/export/lucifer/astrolabs/labimage/varstars/ecl-bin.txt";

# avoid race conditions with any subprocesses.
$| = 1;

# Print the HTML header once
print "Content-type: text/html\n\n";
print "<html><head>\n";
print "<title>Variable Star Catalog</title>\n";
print "</head>\n<body BACKGROUND=\"textures/gray3.gif\">\n";

# make sure file exists
unless (-r $fn) {
    print "<b>Can not find <em>$fn</em></b><br>\n";
    goto wrapup;
}

# read the file and form into a nice table
print "<h1>Eclipsing Binary Stars</h1>\n";
print "<p>\n";
print "<A href=\"/IMAGE_ANALYSIS/ecl-bin-help.html\">Full explanation</a>\n";
print "<p><pre>\n";
print " Name   RA        Dec          Days       JD0          Max  PMin SMin B PDur SDur Yr Reference\n";
open (ECL, $fn);	# can't fail :-)
while (<ECL>) {
    print $_;
}
print "</pre>";

wrapup:

# final boilerplate
print <<ENDOFTEXT;
<p>Use your browser's \"Go back\" feature to enter a star to predict.
<p>
Go back to the <A href="http://www-astro.physics.uiowa.edu/index.html">
ATF Homepage</p>
</A>

</body></html>
ENDOFTEXT
