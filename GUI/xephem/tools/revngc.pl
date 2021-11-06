#!/usr/bin/perl
# converts the revised NGC catalog by Wolfgang Steinicke, Jan 2000
#   to xephem (*.edb) format.
# For the definitions of type= 1..10, see Steinecke's explan.htm file.
#   type=7 are dropped here because they are duplicates;
#   type=8 are dropped here because they are in the IC catalog.
# The Messier objects are written at the end and commented out.
# If no magnitude is given, 19.99 is assigned by default.
# Submitted 22 Jun 2000 by Fridger Schrempp, fridger.schrempp@desy.de

open (NGC, ">NGC.edb") || die "Can not create NGC.edb\n";
open (TMP, "|sort -k 3 -g >Tmp.edb")|| die "Can not create Tmp.edb\n";

# boilerplate
($ver = '$Revision: 1.1.1.1 $') =~ s/\$//g;
($me = $0) =~ s/.*\///;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
$year += 1900;
$mon += 1;
print NGC "# New General Catalog, revised from the NGC/IC Project.\n";
print NGC "# Data From http://www.ngcic.org/steinicke/2000/revngc.txt\n";
print NGC "# Generated by $me $ver\n";
print NGC "# Processed $year-$mon-$mday $hour:$min:$sec UTC\n";

while (<>) {
    # skip header
    next if (/^NGC/||/^-/);
    # skip extension lines
    next if (&ss(7,7) =~ /2/);
    # NGC number including C(omponent)-column
    $num = &ss(1,6);
    # get rid of extra spaces
    $num =~ s/^\s*//;
    $num =~ s/\s*$//;
    $num =~ s/\s+/ /g;
    # connect the different components (C-column) of an object with a dash 
    $num =~ s/\s(\d)$/-\1/g;

    # type ( reading out the S-column)
    $type=&ss(8,9);
    # That's all there is in Steinecke's catalog
    @Type=('|G','|U','|P','|O','|C','|F','|7','|8','|M','');  
    # individual corrections to further distinguish  xephem types
    # 5 Supernova remnants ('R'):
    if ($num =~/1952|6960|6992|6995/){$Type[1]='|R';}
    if ($num =~/2018/){$Type[3]='|R';}

    # drop type=7/8 objects
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

    # Correct for size of NGC 650/651 = M76A/B, using 
    # the current NGC/IC-project size 163"x107".
    # The given NGC 650/651 position corresponds to the central star;
    # c.f. B. Skiff http://www.ngcic.com/papers/plnneb.htm.
    # The minimal square box (with PA=0), entirely containing a 
    # (rotated) 163x107 object, has sides sqrt(163^2+107^2) = 195, hence
    if ($num =~ /\b65[01]\b/) {
      $maj = 195; 
    }

    # ID1 alternative name field
    $altnam1 = &ss(80,93);
    # get rid of extra spaces
    $altnam1 =~ s/^\s*//;
    $altnam1 =~ s/\s*$//;
    $altnam1 =~ s/\s+/ /g;

    # filter out Messier objects
    if ($altnam1 =~ /^M\s\d{1,3}/) {
      printf TMP "# %s NGC %s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
	      $altnam1,$num,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
      # that's it except for "\n", if no size entries >0
      if ($maj =~/[1-9]/){
        # Position angle specified?
        if ($pa =~ /[0-9]/) {
          printf TMP ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
        } else {
          printf TMP ",%0.f\n", $maj;
        }
      } else {
        print TMP "\n";
      }
    } else {
      printf NGC "NGC %s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
		  $num,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
      # that's it except for "\n", if no size entries >0
      if ($maj =~/[1-9]/) {
        # Position angle specified?
        if ($pa =~ /[0-9]/) {
          printf NGC ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
        } else {
          printf NGC ",%0.f\n", $maj;
        }
      } else {
        print NGC "\n";
      }
    }

    # look for NGC's in ID1 that will /later/ appear as type=7 objects!
    if ($altnam1 =~/NGC/) {
      printf NGC "%s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
		    $altnam1,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
      # that's it except for "\n", if no size entries >0
      if ($maj =~/[1-9]/) {
        # Position angle specified?
        if ($pa =~ /[0-9]/) {
          printf NGC ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
        } else {
          printf NGC ",%0.f\n", $maj;
        }
      } else {
        print NGC "\n";
      }
    }

    # look for NGC's in ID2 that will /later/ appear as type=7 objects!
    $altnam2 = &ss(96,110);
    # get rid of extra spaces
    $altnam2 =~ s/^\s*//;
    $altnam2 =~ s/\s*$//;
    $altnam2 =~ s/\s+/ /g;
    if ($altnam2 =~/NGC/) {
      printf NGC "%s,f%s,%d:%d:%g,%g:%d:%d,%g,2000",
      $altnam2,$Type[$type-1],$rh, $rm, $rs,$dd, $dm, $ds,$mag; 
      # that's it except for "\n", if no size entries >0
      if ($maj =~/[1-9]/){
        # Position angle specified?
        if ($pa =~ /[0-9]/) {
          printf NGC ",%.0f|%.0f|%.0f\n", $maj, $min, $pa;
        } else {
          printf NGC ",%0.f\n", $maj;
        }
      } else {
        print NGC "\n";
      }
    }
}

close TMP;
close NGC;

open (NGC, ">>NGC.edb") || die "Can not create NGC.edb\n";
open (TMP, "<Tmp.edb")|| die "Can not create Tmp.edb\n";

# add the Messier objects at the end of NGC.edb
while (<TMP>) {
    print NGC $_;
}


# like substr($_,first,last), but one-based.
sub ss {
    substr ($_, $_[0]-1, $_[1]-$_[0]+1);
}

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: revngc.pl,v $ $Date: 2003/04/15 20:47:38 $ $Revision: 1.1.1.1 $ $Name:  $
