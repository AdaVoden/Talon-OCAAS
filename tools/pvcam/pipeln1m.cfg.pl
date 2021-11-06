
# Configuration options of 1-meter PV Camera Pipeline process

# +++++
# Treatment of redundant 'center' section
#
# This option will DELETE the center, or return, (section 00) image from
# storage.
#
# This image is redundant, since it is a subset of the other images that
# are stored at the same time.  It is also returned to the camera
# program, either because of direct exposure, or because $returnToCamera is set below
# and the camera is in "auto listen" mode.
# Setting this to 0 (recommended) will schedule the center section for cleanup on the next
# pass, after it is no longer used.  This can help free up disk space.

# set as 1 to preserve, 0 to delete center

$preserveCenter = 1;

# ++++
# Correction Image Control
#
# This determines if images are to be corrected using the calibration images
# set as 1 to perform corrections, 0 for no correction

$doCorrections = 1;

# ++++
# WCS control
#
# This control whether or not the wcs command program will be executed
# on the image files, saving solution fields in the FITS header.
# set as 1 to perform WCS, 0 for no WCS solutions

$doWCS = 1;

# This sets the search radius from the center of the image to consider when
# comparing to the GSC in search of solutions.  In degrees.

$wcsRadius = 0.3;

# +++
# FWHM calculation
#
# This sets whether or not the image is computed for FWHM averages, saving
# this information in the FITS header
# set as 1 to perform FWHM, 0 for no FWHM processing

$doFWHM = 1;

# ++++
# Image Compression
#
# This determines if images should be Huffman compressed into .fth files
# and if so, to what degree.
# A setting of 0 means no compression.  1 is loss-less compression, and
# settings of 2 and higher are successively higher compression / more lossy compressions.
# values over 100 are generally too lossy for practical usefulness.

$imageCompression = 0;

# ++++
# Image Returned to Camera
#
# The center section can be returned to the camera program. If camera has
# its "auto listen" feature turned on, the image will automatically appear
# in the camera window.
# Note that the standard "postprocess" script normally run on batch operations
# also includes auto-listen support.
# $resultToCamera is normally 0 so as not to interfere with the operation of
# the postprocess script callback.
#
# Note that if this setting is 0 and $preserveCenter is also 0, then no processing
# at all will be done on the center section, which may be a minor timesavings.
#
# set as 1 to return the processed center image to Camera, 0 for no camera notification
$resultToCamera = 0;

# ++++
# Trash bin file
$trashBin = "$TELHOME/archive/trash";

# Log file info
$elog = "$TELHOME/archive/logs/pipeline.log";

