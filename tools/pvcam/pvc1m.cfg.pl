#################
# THIS VERSION 
# EDITED JUNE 5, 2002 -- after fix to pvslave code and feedback from 2002-05-02 update
# Addendum edit made 2002-05-06 to fix remaining hflip for chip D1
##################

# read by pvcam to set local options
# 1.0m camera

# used for testing only
$testMode = 0;
$multicpu = 1;

# Turn on for informational messages in log
$traceon = 1;

# Turn on for verbose / debug messages in log (traceon must also be 1)
$verboseTrace = 0;

# Camera ID string
$id = "Pixel Vision 4kx2kx10";

#------------------------------------------------

# CCD Chip Grouping information
# This section defines the grouping of the chips to each slave computer
# and describes the offset relationship to the image as a whole
# The offsets are in arcminutes.
# Rotation bit codes are: (2 = hflip, 4 = vflip)
#	0 = no rotation or flip
#	1 = rotate left				-1 = rotate right
#	2 = flip horizontal
#	3 = flip horizontal then rotate left	-3 = flip horizontal then rotate right
#	4 = flip vertical
#	5 = flip vertical then rotate left 	-5 = flip vertical then rotate right
#       6 = flip both horizontal and vertical
#       7 = flip both ways then rotate left	-7 = flip both ways then rotate right
#
# NOTE: GROUP 0 MUST BE THE SKY MONITOR

$GroupInfo = [

# Group 0: Sky Monitor (computer A)
{ Name			=> "onem-1",		# name of the computer that receives this group
  Cmd			=> "pvslave",		# name of script to run
  FilewaitDir 	=> "/mnt/raid/filewait",
  Size 			=> [ 700,1100 ],	# width and height of combined image file [ w,h ]
  SectionCuts 	=> [				# "name", "x y w h", offset, rotation for each section to cut	
		"a1", "0 0 700 550", 0,0 , 0,
		"a2", "0 550 700 550", 0,0, 0,
		   ],				
},

# Group 1: Computer B
{ Name			=> "onem-2",		# name of the computer that receives this group
  Cmd			=> "pvslave",		# name of script to run
  FilewaitDir 	=> "/mnt/raid/filewait",
  Size 			=> [ 8800,4150 ],	# width and height of combined image file
  SectionCuts 	=> [				# "name", "x y w h", offset, rotation for each section to cut	
	        "b1", "0130 0000 2048 4096",  3072, -3072, 2,
	        "b2", "2330 0000 2048 4096",  1024, -3072, 2,
	    	"b3", "4530 0000 2048 4096", -1024, -3072, 2,
	    	"b4", "6622 0000 2048 4096", -3072, -3072, 2,
  	    	   ],
},

# Group 2: Computer C
{ Name			=> "onem-3",		# name of the computer that receives this group
  Cmd			=> "pvslave",		# name of script to run
  FilewaitDir 	=> "/mnt/raid/filewait",
  Size 			=> [ 2200,12450 ],	# width and height of combined image file
  SectionCuts 	=> [				# "name", "x y w h", offset, rotation for each section to cut	
  	        "c1", "0126 0000 2048 4096", 2048, 0, -1,
		    "c2", "0026 4204 2048 4096", 3072, 3072, 2,
  		    "c3", "0026 8354 2048 4096", 1024, 3072, 2,
  	    	   ],
},

# Group 3: Computer D
{ Name			=> "onem-4",		# name of the computer that receives this group
  Cmd			=> "pvslave",		# name of script to run
  FilewaitDir 	=> "/mnt/raid/filewait",
  Size 			=> [ 2200,12450 ],	# width and height of combined image file
  SectionCuts 	=> [				# "name", "x y w h", offset, rotation for each section to cut	
  	        "d1", "0025 0000 2048 4096", -1024, 3072, 6,
    		"d2", "0025 4204 2048 4096", -3072, 3072, 2,
  	    	"d3", "0126 8354 2048 4096", -2048, 0, -1,
  	    	   ],
}

];	# end of GroupInfo

#------------------------------------------------

# Information about return choices
# format: "cpuName", "cutFrame", offset, rotate
$ReturnInfo = [
 # Choice 0:
	# The west side of the center (C1)
	[ "onem-3", "894 3584 512 512", 256, 0, -1 ],

 # Choice 1:
 	# The east side of the center (D3)
	[ "onem-4", "894 8354 512 512", -256, 0, -1 ],
	
 # Choice 2:
	# Northwest of center (b2)
	[ "onem-2", "3098 3584 512 512", 1024, -1280, 2 ],

 # Choice 3:
	# Northeast of center (b3)
	[ "onem-2", "5298 3584 512 512", -1024, -1280, 2 ],

 # Choice 4:
 	# Southwest of center (C3 corner)
	[ "onem-3", "1562 8364 512 512", 256, 1280, 2 ],

 # Choice 5:
	# Southeast of center (D1 corner)
	[ "onem-4", "25 3584 512 512", -256, 1280, 6 ],
	
# Return a single full chip:  Not selectable w/PVOptions... set $ReturnChoice manually
 # Choice 6 - 9 : B1 - B4
 	[ "onem-2", "0130 0000 2048 4096", 3072, -3072, 2],
	[ "onem-2", "2330 0000 2048 4096", 1024, -3072, 2 ],
	[ "onem-2", "4530 0000 2048 4096", -1024, -3072, 2 ],
	[ "onem-2", "6622 0000 2048 4096", -3072, -3072, 2 ],
	
 # Choice 10 - 12 : C1 - C3
    [ "onem-3", "0126 0000 2048 4096", 2048, 0, -1 ],
	[ "onem-3", "0026 4204 2048 4096", 3072, 3072, 2 ],
  	[ "onem-3", "0026 8354 2048 4096", 1024, 3072, 2 ],
	
 # Choice 13 - 15 : D1 - D3
  	[ "onem-4", "0025 0000 2048 4096", -1024, 3072, 6 ],
	[ "onem-4", "0025 4204 2048 4096", -3072, 3072, 2 ],
  	[ "onem-4", "0126 8354 2048 4096", -256, 0, -1],
  		
]; # end of ReturnInfo

# Choice from above to use for return section
$ReturnChoice = 0;
# name of section that is returned to system
$ReturnName = "return";

# common file location where this is copied for return to system
# ($TELHOME/archive is common)
$ReturnImgFile = "$TELHOME/archive/pvrtn.fit";

# Enable groups: Set to 0 to disable a particular group (0-3)
@GroupEnable = (0,1,1,1);

# Set to 1 to make any flat images return a "fake" return component
$FakeFlats = 0;

# Set to 0 to disable post-processing script from running
$enablePostProcessing = 0;

#----------------------------------------------------

# names for temporary calibration files
$biasName = "bias";
$thermName = "therm";
$flatName = "flat";

# set to 1 to enable sky monitor, else 0
$useSkyMonitor = 0;

# set to 1 if sky monitor images should be stored
# or 0 if they should just be returned visually
$storeSkyMonitorImgs = 0;

# name of fifo sky monitor communicates with (in $TELHOME/comm)
$SkyListen = "SkyListen";

# ==========================

# fake temperature
$faketemp = -100;

$maxbx = 2;		# 3 causes pv software to choke
$maxby = 2;

# trigger LP bits
$lpshut = 0x1;		# LP data bit to activate Shutter
$lptrig = 0x2;		# LP data bit to activate Trigger

# how to run the lpio command
# and the stampfits command
# These run on slaves, so the command is direct:
$lpiocmd = "lpio";
$stampcmd = "stampfits";

# error file returned to master to signify error
# in common location ($TELHOME/archive is shared)
$msterr = "$TELHOME/archive/logs/pvslaverr.txt";

$base = "/mnt/raid/user/images"; 	# base name and path for output files
#$base = "/home/steve/pvc1m/images";

# This must be in a location that can be seen by all the computers
# at the same pathname ($TELHOME/archive is shared)
$fitsnow = "$TELHOME/archive/fitsnow";  # temp file for placeholder fits header

# timeouts -- pertains to each group
$dlto = 240;		# time for camera to download image
$lanto = 30;	# max time to get image over lan

# err and trace log
# This also should be in a common place ($TELHOME/archive is shared)
$elog = "$TELHOME/archive/logs/pvc1m.log";

# timeout for skymonitor when used as return section
$skyMonitorTimeout = 60;
