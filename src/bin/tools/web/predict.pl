#!/opt/local/bin/perl -- -*- ATF -*-
# predict.pl: called to digest predict.html.
# Elwood Downey
# 21 Jun 96: begin
# 22 Apr 97: use /home/ecdowney, not /net/inferno/home/ecdowney

# handy tools
require "cgi-lib-96.pl";
require "ctime.pl";

$ver = 1.0;

# location of star list
$fn = "/net/inferno/export/lucifer/astrolabs/labimage/varstars/ecl-bin.txt";

# program to execute
$predict = "/home/ecdowney/biniro/predict";

# predict needs LONGITUDE and LATITUDE env variables
# these should be the same as those seen by predict when it runs and determines
# the defintion of "nighttime".
$ENV{"LONGITUDE"} = "1.59752";
$ENV{"LATITUDE"} = "0.727109";

# avoid race conditions with any subprocesses.
$| = 1;

# Read in all the variables set by the form so we can access as $input{'name'}
&ReadParse(*input);

# Print the HTML header once
print "Content-type: text/html\n\n";
print "<html><head>\n";
print "<title>Eclipsing Binary Star Minima Report</title>\n";
print "</head>\n<body bgcolor=\"white\">\n";
print "<p><img src=\"../images/main_line.gif\"><p>\n";

# gather input params
$star = $input{'Star'};
$date = $input{'Date'};
$nmin = $input{'NMin'};
$night = $input{'Night'};

# do a few sanity checks
if ($star eq "") {
    print "<b>Please supply a star name</b>\n";
    goto wrapup;
}
if (! `grep -i $star $fn`) {
    print "<b>Star <em>$star</em> not found in the catalog.</b>\n";
    goto wrapup;
}
if ($date ne "" && $date !~ m%\d+/\d+/\d\d\d\d%) {
    print "<b>Invalid date format: Please use mm/dd/yyyy</b>\n";
    goto wrapup;
}
if ($nmin < 1) {
    print "<b>Number of minima must be at least 1</b>\n";
    goto wrapup;
}

# build command line to execute predict
$cmd = "$predict -f $fn -s $star -n $nmin";
$cmd = $cmd . " -d $date" if ($date ne "");
$cmd = $cmd . " -a" if ($night ne "On");

# run it, and just show as preformated text
print "<pre>\n";
system "$cmd";
print "</pre>\n";
print "<p><h2>All times and circumstances are with respect to the ATF site.</h2>\n";

wrapup:
 
# final boilerplate
print <<ENDOFTEXT;
<p>Use your browser's \"Go back\" feature to edit and resubmit.
<p>


  
</body></html>
ENDOFTEXT
