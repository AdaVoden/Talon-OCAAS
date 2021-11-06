#!/usr/bin/perl
# read a zip floppy arriving from IRO to local disk
#   mount media
#   cp images to local disk, adding name to list if ok
#   cp logs to local disk, adding name to list if ok
#   erase media
#   cp list to media
#   unmount media
# 29 Oct 97 Elwood Downey

# set directories
$im_dir = "$ENV{'TELHOME'}/user/images";
$log_dir = "$ENV{'TELHOME'}/user/logs";
$media_dir = "/zip";
$hist_file = "$media_dir/RoundTrip";

# do it
&openTMPF;
&mountMedia;
&saveImages;
&saveLogs;
&cleanMedia;
&saveRT;
&umountMedia;

# all done
print "All finished .. thanks!\n";
exit 0;

# open a temp file for what will become the round-trip record
sub openTMPF {
    srand time;
    $rnd = int(rand(99999));
    $tmp_file = "/tmp/imread.$rnd";
    open TMPF, ">$tmp_file" || die "Can not create $tmp_file\n";
}

# mount media
sub mountMedia {
    while (1) {
	print "Insert Zip floppy and press return when ready ... ";
	<STDIN>;
	local $mnt = system "mount $media_dir > /dev/null 2>&1";
	if ($mnt != 0 && $mnt != 768) { # 768 means already mounted
	    print "Zip error ... please try again\n";
	} else {
	    last;
	}
    }
}

# unmount media
sub umountMedia {
    system "umount $media_dir";
}

# cp all images in $media_dir to $im_dir, add basenames to tmp_file
sub saveImages {
    local @files = <$media_dir/*.ft[sh]>;
    local $nfiles = @files + 0;
    local $i = 1;
    $| = 1;
    foreach (@files) {
	system "cp $_ $im_dir";
	s,.*/,,;	# form basename
	print TMPF "$_\n";
	printf "\rImages: %3d of %3d", $i++, $nfiles;
    }
    if ($nfiles > 0) {
	print "\n";
    } else {
	print "No images\n";
    }
    $| = 0;
}

# cp all logs in $media_dir to $log_dir, add basenames to tmp_file
sub saveLogs {
    local @files = <$media_dir/*.log>;
    local $nfiles = @files + 0;
    local $i = 1;
    $| = 1;
    foreach (@files) {
	system "cp $_ $log_dir";
	s,.*/,,;	# form basename
	print TMPF "$_\n";
	printf "\rLogs:   %3d of %3d", $i++, $nfiles;
    }
    if ($nfiles > 0) {
	print "\n";
    } else {
	print "No logs\n";
    }
    $| = 0;
}

# clean off $media_dir
sub cleanMedia {
    system "touch $media_dir/x.x; rm -fr $media_dir/*";
}

# save TMPF as the round-trip file on med_dir
sub saveRT {
    close TMPF;
    system "cp $tmp_file $hist_file";
    unlink $tmp_file;
}

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: imread.pl,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $
