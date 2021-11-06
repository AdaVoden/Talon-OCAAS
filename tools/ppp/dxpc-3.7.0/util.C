#include <iostream.h>
#include <stdlib.h>
#include <unistd.h>
#include "constants.H"
#include "util.H"


unsigned int
GetUINT(unsigned const char *buffer, int bigEndian)
{
  unsigned int result;
  if (bigEndian)
  {
    result = *buffer;
    result <<= 8;
    result += buffer[1];
  }
  else
  {
    result = buffer[1];
    result <<= 8;
    result += *buffer;
  }
  return result;
}


unsigned int
GetULONG(unsigned const char *buffer, int bigEndian)
{
  const unsigned char *next = (bigEndian ? buffer : buffer + 3);
  unsigned int result = 0;
  for (int i = 0; i < 4; i++)
  {
    result <<= 8;
    result += *next;
    if (bigEndian)
      next++;
    else
      next--;
  }
  return result;
}


void
PutUINT(unsigned int value, unsigned char *buffer, int bigEndian)
{
  if (bigEndian)
  {
    buffer[1] = (unsigned char) (value & 0xff);
    value >>= 8;
    *buffer = (unsigned char) value;
  }
  else
  {
    *buffer = (unsigned char) (value & 0xff);
    value >>= 8;
    buffer[1] = (unsigned char) value;
  }
}


void
PutULONG(unsigned int value, unsigned char *buffer, int bigEndian)
{
  if (bigEndian)
  {
    buffer += 3;
    for (int i = 4; i; i--)
    {
      *buffer-- = (unsigned char) (value & 0xff);
      value >>= 8;
    }
  }
  else
  {
    for (int i = 4; i; i--)
    {
      *buffer++ = (unsigned char) (value & 0xff);
      value >>= 8;
    }
  }
}


unsigned int
RoundUp4(unsigned int x)
{
  unsigned int y = x / 4;
  y *= 4;
  if (y != x)
    y += 4;
  return y;
}

void
PrintVersionInfo()
{
  cout << "dxpc - Differential X Protocol Compressor - " <<
    "Version " << DXPC_VERSION_MAJOR << '.' << DXPC_VERSION_MINOR << '.' <<
    DXPC_VERSION_PATCH;
  if (DXPC_VERSION_BETA != 0)
    cout << "beta" << DXPC_VERSION_BETA;
  cout << endl;
  cout << "Copyright (c) 1995,1996 Brian Pane" << endl <<
    "Copyright (c) 1996,1997 Zachary Vonler" << endl;
}


void
DumpMessage(const unsigned char *src, unsigned int numBytes)
{
  for (unsigned int i = 0; i < numBytes; i++)
    cout << i << '\t' << (unsigned int) (src[i]) << endl;
}


const char *
GetArg(int &argi, int argc, const char *const *argv)
{
  const char *nextArg = argv[argi] + 2;		// skip "-" and flag character

  if (*nextArg == 0)
  {
    if (argi + 1 == argc)
      return NULL;
    else
    {
      argi++;
      return argv[argi];
    }
  }
  else
    return nextArg;
}


int
WriteAll(int fd, const unsigned char *data, unsigned int length)
{
  unsigned int bytesWritten = 0;
  while (bytesWritten < length)
  {
    int result =::write(fd, data + bytesWritten,
			length - bytesWritten);
    if (result <= 0)
      return -1;
    bytesWritten += result;
  }
  return length;
}
