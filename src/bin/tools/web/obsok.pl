#!/opt/local/bin/perl
# obsok.pl: exit 0 if obs code in $ARGV[0] is ok, else exit 1.
# Elwood Downey
# 17 Jun 96: Begin

# handy tools
require "cgi-lib-96.pl";
require "ctime.pl";

$ver = 1.0;

# file containing user codes
$codefn = "/net/gastro3/home1/ccdoper/obs.txt";

# scan for code
$obc = @ARGV[0];
exec "grep \"^$obc *|\" $codefn > /dev/null";
