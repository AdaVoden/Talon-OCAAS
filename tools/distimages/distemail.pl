#!/usr/bin/perl
# distemail.pl: send email to everyone with new images update emaillog.
# Elwood Downey
#  6 Sep 96: first release
# 10 Sep 96: change mail format slightly and enable for all users.
# 16 Sep 96: tighten up emaillog
# 17 Sep 96: improve keywords around/for each feature
#  6 Nov 98: changes for iro/atf. no more obs log

# only send mail if images are no older than this many seconds from now.
# may be overridden with first argument.
$age = defined($ARGV[0]) ? 3600*$ARGV[0] : 1e10;   # N.B. want age in seconds
if ($age <= 0) {
    $_ = $0; s#.*/##;
    print "Usage: $_ [age]\n";
    print "Purpose: send email to owners of \$TELHOME/user/images.\n";
    print "Default is to send mail to everyone regardless of file age.\n";
    print "First optional arg can specify a max age, in hours.\n";
    exit 1;
}

# need TELHOME
defined($telhome = $ENV{TELHOME}) or die "No TELHOME\n";
 
# dir with images to inspect
$imdir = "$telhome/user/images";

# file of user codes with email addresses
$obstxt = "/net/iro/home/ocaas/obs.txt";

# file in which we append email activity
$logfn = "$telhome/user/logs/summary/emaillog";

# open log file for append
open(LOG, ">>$logfn") or die "Can not append to $logfn\n";

# add date stamp and auxmsg
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime();
$mon += 1;
$year += 1900;
print LOG "DATE: $mon/$mday/$year $hour:$min:$sec MDY UTC\n";
print LOG "\n";

# read obstxt and build %email and %names.
open (OBS, $obstxt) or die "Can not open $obstxt\n";
while (<OBS>) {
    chop($_);
    ($obc,$email,$name,$comment) = split (/ *\| */);
    $email =~ s/^ *//;	# trim leading blanks
    $email =~ s/ .*$//;	# trim from first blank on to take first if many
    if ($email !~ /^[\w\.-]+@[\w\.-]+$/) {
	print "Bad email for $name: $email\n";
	next;
    }
    $email{$obc} = $email;
    $names{$obc} = $name;
}
close (OBS);

# scan for all new fts and fth files, sending mail as we find groups of codes.
$now = time();
foreach $fn (<$imdir/???*.ft[sh]>) {
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
			  $atime,$mtime,$ctime,$blksize,$blocks) = stat($fn);
    next if ($mtime + $age < $now);
    $fn =~ s#.*/##;
    $obc = substr($fn,0,3);
    if (defined($lastobc) and $obc ne $lastobc) {
	&sendEmail();
	@files = ();
    }
    $lastobc = $obc;
    @files = (@files,$fn);
}
&sendEmail() if (defined($lastobc));

# send email to $lastobc and tell them @files are ready
# also append to $logfn
sub sendEmail
{
    my ($addr) = $email{$lastobc};
    my ($name) = $names{$lastobc};
    my ($msg,$fmtfiles,$n,$f);

    # format the file names nicely in case there are lots
    $n = 0;
    foreach $f (@files) {
	$fmtfiles .= "\n " if (($n++ % 5) == 0);
	$fmtfiles .= " $f";
    }

    # build the message
    $msg = <<"xEOFx";
Hello $name,

This message is being generated automatically by the University of Iowa's
Telescope Operations Facility. The following files taken at your request
using Observer Code `$lastobc' are now available:
$fmtfiles

These images will remain online for the next several days after which they
will be archived. Archived images can be restored by special request.

To inspect and/or retrieve your images individually point your Web browser
to the following URL and follow the options to Current Status:

    http://denali.physics.uiowa.edu/IRO/remote/status.shtml

For those of you with observer codes starting with other than one of
the letters a, f, g, m, u, y or x you may access all the images and
their engineering logs using anonymous ftp to:

    ftp://iro.physics.uiowa.edu/pub/remote

Please direct any questions to mailto:liason\@iro.physics.uiowa.edu.

Thank you.
xEOFx

    # send mail
    if (!open (M, "| mail -s 'your images are ready' \"$addr\"")) {
	print STDERR "Can not send mail to $addr\n";
	return;
    }
    print M $msg;
    close (M);

    # append name, address and files to log
    print LOG "EMAIL: $addr\n";
    print LOG "NAME: $name\n";
    print LOG "FILES: @files\n";
    print LOG "\n";
}

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: distemail.pl,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $
