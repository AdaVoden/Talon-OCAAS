#
# MySQL database definition
# Tables for JSGA_IMG
#
# Torus Techologies
# S. Ohmert  9/6/2001
#
# Modified 1/17/2002 to remove ra/dec and use name instead

CREATE TABLE IF NOT EXISTS exposures
(
    exposure_id     INT UNSIGNED    NOT NULL AUTO_INCREMENT PRIMARY KEY,
    exposure_type   VARCHAR(8)      NOT NULL,       # type of exposure
#    exposure_ra     DOUBLE          DEFAULT 0,      # right ascension expressed IN DEGREES
#    exposure_dec    DOUBLE          DEFAULT 0,      # declination expressed IN DEGREES
	exposure_name	VARCHAR(16)		DEFAULT "",		# name of exposure. Usually HHMMsDDdd format, but may be name
    exposure_date   VARCHAR(8)      NOT NULL,       # date YYMMDD of exposure
    exposure_sequence SMALLINT UNSIGNED DEFAULT 0,  # sequence number
    tape_id         INT UNSIGNED    NULL            # filled when backed up
);

CREATE TABLE IF NOT EXISTS images
(
    image_id        INT UNSIGNED    NOT NULL AUTO_INCREMENT PRIMARY KEY,
    exposure_id     INT UNSIGNED    NOT NULL,       # exposures table id where this image belongs
    image_server    VARCHAR(32)     NOT NULL,       # server where file exists
    image_filename  TEXT            NOT NULL,       # pathname where file exists
    image_filesize  INT UNSIGNED    NOT NULL        # size of file in bytes
);

CREATE TABLE IF NOT EXISTS fits
(
    fits_id         INT UNSIGNED    NOT NULL AUTO_INCREMENT PRIMARY KEY,
    image_id        INT UNSIGNED    NOT NULL,       # image table id where this fits info belongs
    SIMPLE          ENUM("F","T")   DEFAULT "T",    # standard FITS
    BITPIX          SMALLINT        DEFAULT 16,     # Bits per pixel
    NAXIS           SMALLINT        DEFAULT 2,      # Number of dimensions
    NAXIS1          SMALLINT,                       # Number of columns
    NAXIS2          SMALLINT,                       # Number of rows
    BZERO           SMALLINT UNSIGNED DEFAULT 32768,# Real = Pixel*BSCALE + BZERO
    BSCALE          SMALLINT        DEFAULT 1,      # Pixel scale factor
    OFFSET1         SMALLINT        DEFAULT 0,      # Camera upper left frame x
    OFFSET2         SMALLINT        DEFAULT 0,      # Camera upper left frame y
    XFACTOR         SMALLINT        DEFAULT 1,      # Camera x binning factor
    YFACTOR         SMALLINT        DEFAULT 1,      # Camera y binning factor
    EXPTIME         DOUBLE,                         # Exposure time, in seconds
    ORIGIN          VARCHAR(32),                    # Observatory
    TELESCOP        VARCHAR(32),                    # Telescope
    CDELT1          DOUBLE,                         # RA step right, degrees/pixel
    CDELT2          DOUBLE,                         # Dec step down, degrees/pixel
    INSTRUME        VARCHAR(32),                    # Camera id
    JD              DOUBLE,                         # Julian Date / Time at end of exposure
    DATE_OBS        VARCHAR(12),                    # UTC CCYY-MM-DD
    TIME_OBS        VARCHAR(12),                    # UTC HH:MM:SS
    LST             VARCHAR(12),                    # Local Sidereal Time
    POSANGLE        VARCHAR(12),                    # Position angle, degrees +W
    LATITUDE        VARCHAR(12),                    # Site Latitude, degrees +N
    LONGITUDE       VARCHAR(12),                    # Site Longitude, degrees +E
    ELEVATION       VARCHAR(12),                    # Degrees above horizon
    AZIMUTH         VARCHAR(12),                    # Degrees E of N
    HA              VARCHAR(12),                    # Local Hour Angle
    RA              VARCHAR(12),                    # Nominal Center J2000 RA
    DECL            VARCHAR(12),                    # Nominal Center J2000 Dec
    EPOCH           VARCHAR(12),                    # RA/Dec epoch, years (obsolete)
    EQUINOX         VARCHAR(12),                    # RA/Dec equinox, years
    FILTER          VARCHAR(12),                    # Filter code
    CAMTEMP         INT,                            # Camera temp, C
    RAWHENC         DOUBLE,                         # HA Encoder, rads from home
    RAWDENC         DOUBLE,                         # Dec Encoder, rads from home
    RAWOSTP         DOUBLE,                         # Focus motor, rads from home
    FOCUSPOS        DOUBLE,                         # Focus position, um from home
    RAWISTP         DOUBLE,                         # Filter position, rads from home
    WXTEMP          DOUBLE,                         # Ambient air temperature
    WXPRES          DOUBLE,                         # Atmospheric pressure, millibars
    WXWNDSPD        INT,                            # Wind Speed, kph
    WXWNDDIR        INT,                            # Wind Dir, degrees E of N
    WXHUMID         INT,                            # Outdoor Humidity, percent
    CTYPE1          VARCHAR(12),                    # WCS Solution type (RA---TAN)
    CRVAL1          DOUBLE,                         # RA at CRPIX1, degrees
    CRPIX1          INT,                            # RA reference pixel index, 1-based
    CROTA1          DOUBLE,                         #
    CTYPE2          VARCHAR(12),                    # WCS Solution type (DEC--TAN)
    CRVAL2          DOUBLE,                         # Dec at CRPIX2, degrees
    CRPIX2          INT,                            # Dec reference pixel index, 1-based
    CROTA2          DOUBLE,                         # Rotation N through E, degrees
    FWHMH           DOUBLE,                         # Horizontal FWHM median, pixels
    FWHMHS          DOUBLE,                         # Horizontal FWHM std dev, pixels
    FWHMV           DOUBLE,                         # Vertical FWHM median, pixels
    FWHMVS          DOUBLE                          # Vertical FWHM std dev, pixels
);

CREATE TABLE IF NOT EXISTS reductions
(
    reduction_id		INT UNSIGNED    NOT NULL AUTO_INCREMENT PRIMARY KEY,
    image_id     		INT UNSIGNED    NOT NULL,       # images table id where this image belongs
    reduction_server    VARCHAR(32)     NOT NULL,       # server where file exists
    reduction_filename  TEXT            NOT NULL,       # pathname where file exists
    reduction_filesize  INT UNSIGNED    NOT NULL        # size of file in bytes
);
