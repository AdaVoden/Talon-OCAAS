#ifndef Constants_H
#define Constants_H

// dxpc version number
static const unsigned int DXPC_VERSION_MAJOR = 3;
static const unsigned int DXPC_VERSION_MINOR = 7;
static const unsigned int DXPC_VERSION_PATCH = 0;
static const unsigned int DXPC_VERSION_BETA = 0;	// zero if not beta

// Maximum number of X connections supported
static const unsigned int MAX_CONNECTIONS = 256;

// TCP port on which server proxy listens for connections from
// client proxy
static const unsigned int DEFAULT_PROXY_PORT = 4000;

// X display number that client proxy imitates
static const unsigned int DEFAULT_DISPLAY_NUM = 8;

// Bit masks to select the lower 'i' bits of an int, 0 <= 'i' <= 32
extern const unsigned int PARTIAL_INT_MASK[33];

// Maximum number of points in a FillPoly request that are given
// their own history caches
static const int FILL_POLY_MAX_POINTS = 10;

// Sizes of optional fields for ConfigureWindow request
extern const unsigned int CONFIGUREWINDOW_FIELD_WIDTH[7];

// Sizes of optional fields for CreateGC request
extern const unsigned int CREATEGC_FIELD_WIDTH[23];

// Mapping to reverse the bits in a byte, for image processing on
// little-endian architectures:
extern unsigned char REVERSED_BYTE[256];

// Min width/height ratio for treatment of a bitmap image as a line of
// bitmapped text
static const unsigned int PUT_IMAGE_MIN_ASPECT_RATIO = 8;

// Call this in something other than constants.C to make sure that the
// copyright and liability disclaimer end up in the resulting executable
extern const char *SaveCopyrightFromOptimizer();

#endif /* Constants_H */
