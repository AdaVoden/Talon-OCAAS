#!/usr/bin/perl
# converts the revised IC catalog by Wolfgang Steinicke, Jan 2000
#   to xephem (*.edb) format.
# For the definitions of type= 1..10, see Steinecke's explan.htm file. 
#   type=7 are dropped here because they are duplicates;
#   type=8 are dropped here because they are in the NGC catalog.
# If no magnitude is given, 19.99 is assigned by default.
# Submitted 22 Jun 2000 by Fridger Schrempp, fridger.schrempp@desy.de

open (IC, ">IC.edb") || die "Can not create IC.edb\n";

# boilerplate
($ver = '$Revision: 1.1.1.1 $') =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print IC "# Index Catalog, revised from the NGC/IC Project.\n";
print IC "# Data From http://www.ngcic.org/steinicke/2000/revic.txt\n";
print IC "# Generated by $me $ver\n";
print IC "# Processed $year-$mon-$mday $hour:$min:$sec UTC\n";

while (<>) {
    # skip header
    next if (/^\sIC/||/^-/);
    # skip extension lines
    next if (&ss(7,7) =~ /2/);
    # IC number including C(omponent)-column
    $num = &ss(1,6);
    # get rid of extra spaces
    $num =~ s/^\s*//;
    $num =~ s/\s*$//;
    $num =~ s/\s+/ /g;
    # connect the different components (C-column) of an object with a dash 
    $num =~ s/\s(\d)$/-$1/g;

    # type ( reading out the S-column)
    $type=&ss(8,9);
    # That's all there is in Steinecke's catalog
    @Type=('|G','|U','|P','|O','|C','|F','|7','|8','|M','');  
    # individual corrections to further distinguish  xephem types

    # drop type=7,8 objects
    next if ($type =~/7/ or $type =~/8/);

    # J2000 position
    $rh = &ss(18,19);
    $rm = &ss(21,22);
    $rs = &ss(24,27);
    $dd = &ss(29,31);
    $dm = &ss(33,34);
    $ds = &ss(36,37);

    # magnitude; 
    $mag = &ss(41,44);
    $mag = 19.99 if ($mag !~ /[0-9]/);  # default if no mag

    # maj, min size, arcseconds
    $maj = &ss(46,51)*60;
    $min = &ss(53,57)*60;

    # PA, degrees E of N
    $pa = &ss(59,61);

    # ID1 alternative name field
    $altnam1 = &ss(80,93);
    # get rid of extra spaces
    $altnam1 =~ s/^\s*//;
    $altnam1 =~ s/\s*$//;
    $altnam1 =~ s/\s+/ /g;

    printf IC "IC %s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
		    $num,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
    # that's it except for "\n", if no size entries >0
    if ($maj =~/[1-9]/) {
      # Position angle specified?
      if ($pa =~ /[0-9]/) {
        printf IC ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
      } else {
        printf IC ",%0.f\n", $maj;
      }
    } else {
      print IC "\n";
    }

    # look for IC's in ID1 that will /later/ appear as type=7 objects!
    if ($altnam1 =~/IC/) {
      printf IC "%s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
		  $altnam1,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
      # that's it except for "\n", if no size entries >0
      if ($maj =~/[1-9]/) {
        # Position angle specified?
        if ($pa =~ /[0-9]/) {
          printf IC ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
        } else {
          printf IC ",%0.f\n", $maj;
        }
      } else {
        print IC "\n";
      }
    }

    # look for IC's in ID2 that will /later/ appear as type=7 objects!
    $altnam2 = &ss(96,110);
    # get rid of extra spaces
    $altnam2 =~ s/^\s*//;
    $altnam2 =~ s/\s*$//;
    $altnam2 =~ s/\s+/ /g;
    if ($altnam2 =~/IC/){
      printf IC "%s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
		  $altnam2,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
      # that's it except for "\n", if no size entries >0
      if ($maj =~/[1-9]/){
        # Position angle specified?
        if ($pa =~ /[0-9]/) {
          printf IC ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
        } else {
          printf IC ",%0.f\n", $maj;
        }
      } else {
        print IC "\n";
      }
    }

    # look for IC's in ID3 that will /later/ appear as type=7 objects!
    $altnam3 = &ss(112,126);
    # get rid of extra spaces
    $altnam3 =~ s/^\s*//;
    $altnam3 =~ s/\s*$//;
    $altnam3 =~ s/\s+/ /g;
    if ($altnam3 =~/IC/) {
      printf IC "%s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
		  $altnam3,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
      # that's it except for "\n", if no size entries >0
      if ($maj =~/[1-9]/){
        # Position angle specified?
        if ($pa =~ /[0-9]/) {
          printf IC ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
        } else {
          printf IC ",%0.f\n", $maj;
        }
      } else {
        print IC "\n";
      }
    }
}

close IC;

# like substr($_,first,last), but one-based.
sub ss {
    substr ($_, $_[0]-1, $_[1]-$_[0]+1);
}

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: revic.pl,v $ $Date: 2003/04/15 20:47:38 $ $Revision: 1.1.1.1 $ $Name:  $
