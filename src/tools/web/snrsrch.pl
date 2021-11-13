#!/opt/local/bin/perl
# called generate a list of the snrsrch galaxy images.
# one arg is required: ngc? directory to search/display.
# Elwood Downey 19 June 1996

# root dir for finding the galaxy images
$galdir = "/net/gastro23/export/data/snrsrch/archive/small_arc";

# stay in sync with subprocesses
$| = 1;
  
# URL of home web site
$url = "http://www-astro.physics.uiowa.edu";

# add path to local tools
$ENV{"PATH"} = $ENV{"PATH"} . ":/net/inferno/home/ecdowney/telescope/runtime/bin" . ":/net/inferno/opt/local/bin";

# get the directory
$ngcdir = $ARGV[0];
    
# give the browser some encouragement
print "Content-type: text/html\n\n";
print "<html><head>\n";
print "<title>ATF Galaxy Image Collection</title>\n";
print "</head>\n<body bgcolor=\"white\">\n";
print "<p><img src=\"../images/main_line.gif\">\n";

# build up a list of ngc*.fts file names
@galfns = `find $galdir/$ngcdir -name 'ngc*.fts' -print`;
chop (@galfns);
$ngal = @galfns + 0;

# find range message
$rmsg = $ngcdir;
$rmsg =~ s/ngc//;
$rmsg = "${rmsg}000 - ${rmsg}999";

# emit a table of object names and options to view and retrieve.
# object names are really the base portion of the filename, without the suffix.
print "<p><table border>\n";
print "<caption><b>$ngal NGC Galaxy Images in the Range $rmsg are Available:</b></caption>\n";
foreach (sort @galfns) {
    $fullfn = $_;
    /.*\/(.*)\.fts/;
    $objn = $1;
    print "<tr align=\"CENTER\">\n";
	print "  <td>$objn</td>\n";
	print "  <td><A href=\"$url/cgi-bin/atfftp.pl?256+$fullfn\">View as GIF</A></td>\n";
	print "  <td><A href=\"$url/cgi-bin/atfftp.pl?fts+$fullfn\">Send as .fts</A></td>\n";
	print "  <td><A href=\"$url/cgi-bin/atfftp.pl?fth+$fullfn\">Send as .fth</A></td>\n";
    printf "</tr>\n";
}
print "</table>\n";
 
wrapup:
  
# final boilerplate
print <<ENDOFTEXT;
<HR>
</BODY>

</HTML>

   
</body></html>
ENDOFTEXT
