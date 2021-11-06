#if 0

  Copyright (c) 2000 Finger Lakes Instrumentation (FLI), LLC.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

	Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.

	Redistributions in binary form must reproduce the above
	copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials
	provided with the distribution.

	Neither the name of Finger Lakes Instrumentation (FLI), LLC
	nor the names of its contributors may be used to endorse or
	promote products derived from this software without specific
	prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "fli.h"

#define CCD_CHAR_FILE "/usr/local/telescope/dev/ccdcamera"
#define EXPOSURE_TIME 100
#define OUTFILE "testfile.out"

#define TRY_FUNCTION(function, args...)                         \
  do {                                                          \
    int err;                                                    \
    if ((err = function(## args)))                              \
    {                                                           \
      fprintf(stderr,                                           \
              "Error " #function ": %s\n", strerror(-err));     \
      exit(1);                                                  \
    }                                                           \
  } while(0)

int main(int argc, char *argv[])
{
  fliport_t port;
  flicam_t cam;
  int ul_x, ul_y, lr_x, lr_y;
  u_int16_t *buff;
  char *dev;
  int buffsize, bytesgrabbed;
  FILE *outfile;
  int err;

  dev = argc > 1 ? argv[1] : CCD_CHAR_FILE;
  printf ("using %s\n", dev);

  TRY_FUNCTION(FLIInit, dev, &port);
  TRY_FUNCTION(FLIOpen, port, NULL, &cam);
  TRY_FUNCTION(FLIGetVisibleArea, cam, &ul_x, &ul_y, &lr_x, &lr_y);
  TRY_FUNCTION(FLISetImageArea, cam, ul_x, ul_y, lr_x, lr_y);
  TRY_FUNCTION(FLISetBitDepth, cam, FLI_MODE_16BIT);
  TRY_FUNCTION(FLISetExposureTime, cam, EXPOSURE_TIME);

  printf ("%d .. %d %d .. %d\n", ul_x, lr_x, ul_y, lr_y);

  buffsize = (lr_x - ul_x) * (lr_y - ul_y) * sizeof(u_int16_t);

  if ((buff = malloc(buffsize)) == NULL)
  {
    perror("Error malloc");
    exit(1);
  }

  if ((err = FLIGrabFrame(cam, buff, buffsize, &bytesgrabbed)))
    fprintf(stderr, "Error FLIGrabFrame: %s\n", strerror(-err));

  if (bytesgrabbed > 0)
  {
    if ((outfile = fopen(OUTFILE, "w")) == NULL)
    {
      perror("Error fopen");
      exit(1);
    }

    if (fwrite(buff, 1, bytesgrabbed, outfile) != buffsize)
      perror("Error fwrite");

    fclose(outfile);
  }

  free(buff);

  TRY_FUNCTION(FLIClose, cam);
  TRY_FUNCTION(FLIExit, );

  exit(0);
}
