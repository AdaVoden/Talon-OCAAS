#!/usr/bin/perl -w
# distribute $TELHOME/user/{images,logs}

# must be in group talon
$gid = `id -gn`;
chomp($gid);
die "run newgrp talon\n" unless ($gid eq "talon");

# need TELHOME
defined($telhome = $ENV{TELHOME}) or die "No TELHOME\n";

# remove when older than this many days
$age = 7;

# cd to base
chdir("$telhome/user") or die "Can not cd $telhome/user\n";

# ftp base dir
$ftpdir = "/home/ftp/pub/remote";

# specific catalogies
&cpsmpl ("a", "/u1/astrophysics");
&cpsmpl ("f", "/u1/faculty");
&cpsmpl ("g", "/u1/general");
&cpsmpl ("m", "/u1/modern");
&cpsmpl ("n", "$ftpdir");
&cpsmpl ("u", "/u1/ugrad");

# supernovae search
&cpprefix ("xs", "/net/deimos/home1/snrsrch/images");
&cpprefix ("ys", "/net/deimos/home1/snrsrch/images");

# everything else just stays in user/images

# cleanup
&cleanup();

# all finished
exit 0;

# copy all files and logs with the given prefix to the given directory
sub cpsmpl
{
    my ($prefix, $basedir) = @_;
    my $dir;

    $dir = "$basedir/images";
    -x "$dir" or !system ("mkdir -p $dir") or die "Can not mkdir $dir\n";
    foreach (<images/${prefix}*.ft[sh]>) {
	system "cp $_ $dir";
    }

    $dir = "$basedir/logs";
    -x "$dir" or !system ("mkdir -p $dir") or die "Can not mkdir $dir\n";
    foreach (<logs/${prefix}*.log>) {
	system "cp $_ $dir";
    }
}

# distribute images into their own dirs based on prefix.
sub cpprefix
{
    my ($prefix, $basedir) = @_;
    my $dir;

    # images in their own directory
    foreach (<images/${prefix}*.ft[sh]>) {
	/${prefix}(.)/;
	$dir = "$basedir/${prefix}$1";
	-x "$dir" or !system ("mkdir -p $dir") or die "Can not mkdir $dir\n";
	system "cp $_ $dir";
    }

    # logs all in one directory
    foreach (<logs/${prefix}*.log>) {
	$dir = "$basedir/logs";
	-x "$dir" or !system ("mkdir -p $dir") or die "Can not mkdir $dir\n";
	system "cp $_ $dir";
    }
}

# clean up stuff that is older than $age
sub cleanup
{
	system "find images logs -mtime +$age -exec rm -f {} \\;";
	system "find $ftpdir -mtime +$age -exec rm -f {} \\;";
}

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: distimages.pl,v $ $Date: 2003/04/15 20:48:32 $ $Revision: 1.1.1.1 $ $Name:  $
