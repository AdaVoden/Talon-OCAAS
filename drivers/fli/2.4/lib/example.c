# if 0

  Copyright (c) 2000, 2002 Finger Lakes Instrumentation (FLI), LLC.
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

  ======================================================================

  Finger Lakes Instrumentation (FLI)
  web: http://www.fli-cam.com
  email: fli@rpa.net

# endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "fli.h"

#define DEFAULT_CCD_CHAR_FILE "/dev/ccd0"
#define DEFAULT_EXPOSURE_TIME 50

#define TRY_FUNCTION(function, args...)				\
  do {								\
    int err;							\
    if ((err = function(args)))					\
    {								\
      fprintf(stderr,						\
              "Error " #function ": %s\n", strerror(-err));	\
      exit(1);							\
    }								\
  } while(0)

void usage(char *name)
{
  printf("\n");
  printf("  Usage: %s [-e <exposure time (msec)>] "
         "[<ccd device filename>] \\\n"
         "         <output file name>\n", name);
  printf("\n");
  printf("    Examples: %s -e 50 /dev/ccd0 testfile.1.out\n", name);
  printf("              %s -e 100 /dev/ccd0 - > testfile.2.out\n", name);
  printf("\n");
  printf("    (An output file name of `-' writes data to standard output.)\n");
  printf("\n");

  exit(0);
}

int main(int argc, char *argv[])
{
  fliport_t port;
  flicam_t cam;
  int ul_x, ul_y, lr_x, lr_y;
  u_int16_t *buff;
  int buffsize, bytesgrabbed;
  FILE *outfile;
  int err;
  char *ccd_char_file, *outfilename;
  int exptime = DEFAULT_EXPOSURE_TIME;

  while (1)
  {
    int opt;
    char *endptr;

    if ((opt = getopt(argc, argv, "e:")) == -1)
      break;

    switch (opt)
    {
    case 'e':
      exptime = strtol(optarg, &endptr, 0);
      if (*endptr != '\0')
      {
        printf("Invalid exposure time [%s].\n", optarg);
        usage(argv[0]);
      }
      break;

    case '?':
      usage(argv[0]);
      break;

    default:
      printf("Unknown return value from getopt [%i], exiting.\n", opt);
      exit(1);
    }
  }

  switch (argc - optind)
  {
  case 1:
    ccd_char_file = DEFAULT_CCD_CHAR_FILE;
    outfilename = argv[optind];
    break;

  case 2:
    ccd_char_file = argv[optind++];
    outfilename = argv[optind];
    break;

  default:
    printf ("Invalid number of arguments.\n");
    usage(argv[0]);
  }

  TRY_FUNCTION(FLIInit, ccd_char_file, &port);
  TRY_FUNCTION(FLIOpen, port, NULL, &cam);
  TRY_FUNCTION(FLIGetVisibleArea, cam, &ul_x, &ul_y, &lr_x, &lr_y);
  TRY_FUNCTION(FLISetImageArea, cam, ul_x, ul_y, lr_x, lr_y);
  TRY_FUNCTION(FLISetBitDepth, cam, FLI_MODE_16BIT);
  TRY_FUNCTION(FLISetExposureTime, cam, exptime);

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
    if (strcmp(outfilename, "-") == 0)
      outfile = stdout;
    else
      if ((outfile = fopen(outfilename, "w")) == NULL)
      {
        perror("Error fopen");
        exit(1);
      }

    if (fwrite(buff, 1, bytesgrabbed, outfile) != bytesgrabbed)
      perror("Error fwrite");
  }

  TRY_FUNCTION(FLIClose, cam);
  TRY_FUNCTION(FLIExit);

  exit(0);
}
