#!/usr/bin/perl
# send IRO weather stats as a graph with content-type gif

# add a reasonable TELHOME to env
$ENV{"TELHOME"} = "/usr/local/telescope";

# add local/bin and telescope/bin to path
$ENV{"PATH"} .= ":" . $ENV{"TELHOME"} . "/bin:/usr/local/bin";

# name of weather gif
# must be in a place writable by nobody
$wgd = ".";
$wg = "$wgd/wx.gif";

# recreate the gif if more than about an hour old.
# wx.pl creates it in its cwd.
system ("cd $wgd; wx.pl") if (! -r $wg || -M $wg > .04);

# send it
$| = 1;
print ("Content-type: image/gif\n\n");
system ("cat $wg");
