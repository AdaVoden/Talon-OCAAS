#!/usr/bin/perl
# write how ever many Zip floppies at IRO
#   mount floppy
#   if floppy contains a list of images and/or logs
#     remove those files from local disk
#     erase floppy
#   for each image and log on local disk
#     if floppy is nearly full
#       unmount floppy
#       ask operator to change floppy
#       mount floppy
#       if floppy contains a list of images and/or logs
#         remove those files from local disk
#         erase floppy
#     cp image or log to floppy
#   unmount floppy

$im_dir = "$ENV{'TELHOME'}/user/images";
$log_dir = "$ENV{'TELHOME'}/user/logs";
$media_dir = "/zip";
$hist_file = "$media_dir/RoundTrip";

&mountMedia;
&saveImages;
&saveLogs;
&umountMedia;

print "All finished .. thanks!\n";
exit 0;

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
    &rmHistory;
    &cleanMedia;
}

# unmount media
sub umountMedia {
    system "umount $media_dir";
}

# remove files in History list
sub rmHistory {
    if (open HF, $hist_file) {
	print "Processing round-trip record.. ";
	while (<HF>) {
	    chop;
	    local $f = "$im_dir/$_";
	    unlink $f if (-e $f);
	    local $f = "$log_dir/$_";
	    unlink $f if (-e $f);
	}
	close HF;
	unlink $hist_file;
	print "Finished.\n";
    }
}

# clean off $media_dir
sub cleanMedia {
    print "Scrubbing disk.. ";
    system "touch $media_dir/x.fts; rm -fr $media_dir/*.ft?";
    system "touch $media_dir/x.log; rm -fr $media_dir/*.log";
    print "Finished.\n";
}

# check for more room on media. if tight, unmount, prompt, remount
sub chkMedia {
    local $zf = `df | awk '/zip/ {print \$4}'`;
    if ($zf < 1000) {
	&umountMedia;
	print "\nZip floppy is full.. remove this floppy\n";
	&mountMedia;
    }
}

# cp all images in $im_dir to $media_dir
# if zip fills, prompt for new one
sub saveImages {
    local @files = <$im_dir/*.ft[sh]>;
    local $nfiles = @files + 0;
    local $i = 1;
    $| = 1;
    foreach (@files) {
	&chkMedia;
	system "cp $_ $media_dir";
	printf "\rImages: %3d of %3d", $i++, $nfiles;
    }
    if ($nfiles > 0) {
	print "\n";
    } else {
	print "No new images\n";
    }
    $| = 0;
}

# cp all logs in $log_dir to $media_dir
# if zip fills, prompt for new one
sub saveLogs {
    local @files = <$log_dir/*.log>;
    local $nfiles = @files + 0;
    local $i = 1;
    $| = 1;
    foreach (@files) {
	&chkMedia;
	system "cp $_ $media_dir";
	printf "\rLogs:   %3d of %3d", $i++, $nfiles;
    }
    if ($nfiles > 0) {
	print "\n";
    } else {
	print "No new logs\n";
    }
    $| = 0;
}

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: imwrite.pl,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $
