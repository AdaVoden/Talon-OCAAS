#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "ReadBuffer.H"


static const unsigned int INITIAL_BUFFER_SIZE = 512;


ReadBuffer::ReadBuffer(int fd, unsigned int maxReadSize):
  fd_(fd), buffer_(new unsigned char[INITIAL_BUFFER_SIZE]), length_(0),
  size_(INITIAL_BUFFER_SIZE), start_(0), 
  maxReadSize_(maxReadSize)
{
}


ReadBuffer::~ReadBuffer()
{
  delete[]buffer_;
}


int
ReadBuffer::doRead()
{
  if ((start_ != 0) && (length_ != 0))
  {
    // if any bytes are left over from last time (due to partial message),
    // shift them to the start of the buffer
    unsigned char *nextDest = buffer_;
    unsigned char *nextSrc = buffer_ + start_;
    for (unsigned int i = 0; i < length_; i++)
      *nextDest++ = *nextSrc++;
  }
  else if (length_ == size_)
  {
    // The buffer is full; double its size so that we can read some more
    unsigned char *newBuffer = new unsigned char[size_ << 1];
    memcpy(newBuffer, buffer_, size_);
    delete[]buffer_;
    buffer_ = newBuffer;
    size_ <<= 1;
  }
  start_ = 0;

  // Read as much data as is available
  unsigned int readLength = size_ - length_;
  if (maxReadSize_ && (readLength > maxReadSize_))
    readLength = maxReadSize_;
  int bytesRead = read(fd_, buffer_ + length_, readLength);
  if (bytesRead <= 0)
    return 0;
  length_ += bytesRead;

  return 1;
}


const unsigned char *
ReadBuffer::getMessage(unsigned int &messageLength)
{
  unsigned int headerLength, dataLength, trailerLength;
  if (locateMessage(buffer_ + start_, buffer_ + start_ + length_,
		    headerLength, dataLength, trailerLength))
  {
    const unsigned char *result = buffer_ + start_;
    messageLength = dataLength;
    if (dataLength)
      result += headerLength;
    else
      messageLength += headerLength;
    start_ += (headerLength + dataLength + trailerLength);
    length_ -= (headerLength + dataLength + trailerLength);
    return result;
  }
  else
  {
    // No more complete messages remain in buffer
    return NULL;
  }
}
