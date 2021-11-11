#include <iostream.h>
#include <string.h>
#include "EncodeBuffer.H"
#include "IntCache.H"
#include "CharCache.H"
#include "PixelCache.H"
#include "HuffmanCoder.H"
#include "constants.H"


static const int INITIAL_BUFFER_SIZE = 256;
static const int PREFIX_SIZE = 16;


EncodeBuffer::EncodeBuffer():
  size_(INITIAL_BUFFER_SIZE),
  buffer_(new unsigned char[size_ + PREFIX_SIZE] + PREFIX_SIZE),
  end_(buffer_ + size_),
  nextDest_(buffer_),
  destMask_(0x80), 
  cumulativeBits_(0)
{
  *nextDest_ = 0;
}


EncodeBuffer::~EncodeBuffer()
{
  delete[](buffer_ - PREFIX_SIZE);
}


void
EncodeBuffer::reset()
{
  nextDest_ = buffer_;
  destMask_ = 0x80;
  cumulativeBits_ = 0;
  *nextDest_ = 0;
}


void
EncodeBuffer::encodeValue(unsigned int value, unsigned int numBits,
			  unsigned int blockSize)
{
  unsigned int srcMask = 0x1;
  unsigned int bitsWritten = 0;
  if (blockSize == 0)
    blockSize = numBits;

  unsigned int numBlocks = 1;
  do
  {
    if (numBlocks == 4)
      blockSize = numBits;
    unsigned int bitsToWrite = ((blockSize > numBits - bitsWritten) ?
				numBits - bitsWritten : blockSize);
    unsigned int count = 0;
    unsigned int lastBit;
    do
    {
      lastBit = (value & srcMask);
      if (lastBit)
	*nextDest_ |= destMask_;
      destMask_ >>= 1;
      if (destMask_ == 0)
      {
	destMask_ = 0x80;
	nextDest_++;
	if (nextDest_ == end_)
	  growBuffer_();
	*nextDest_ = 0;
      }
      srcMask <<= 1;
    }
    while (bitsToWrite > ++count);
    bitsWritten += bitsToWrite;
    if (bitsWritten < numBits)
    {
      unsigned int tmpMask = srcMask;
      unsigned int i = bitsWritten;
      if (lastBit)
      {
	do
	{
	  unsigned int nextBit = (value & tmpMask);
	  if (!nextBit)
	    break;
	  tmpMask <<= 1;
	}
	while (numBits > ++i);
      }
      else
      {
	do
	{
	  unsigned int nextBit = (value & tmpMask);
	  if (nextBit)
	    break;
	  tmpMask <<= 1;
	}
	while (numBits > ++i);
      }
      if (i < numBits)
	*nextDest_ |= destMask_;
      else
	bitsWritten = numBits;
      destMask_ >>= 1;
      if (destMask_ == 0)
      {
	destMask_ = 0x80;
	nextDest_++;
	if (nextDest_ == end_)
	  growBuffer_();
	*nextDest_ = 0;
      }
    }
    blockSize >>= 1;
    if (blockSize < 2)
      blockSize = 2;
    numBlocks++;
  }
  while (numBits > bitsWritten);
}


void
EncodeBuffer::
encodeCachedValue(unsigned int value, unsigned int numBits,
		  IntCache & cache, unsigned int blockSize)
{
  // The next line is to avoid a warning;
  blockSize = 0;

  unsigned int newBlockSize = cache.getBlockSize(numBits);
  unsigned int index;
  unsigned int sameDiff;
  if (cache.lookup(value, index, PARTIAL_INT_MASK[numBits], sameDiff))
  {
    if (index > 1)
      index++;
    for (unsigned int count = index; count; count--)
    {
      destMask_ >>= 1;
      if (destMask_ == 0)
      {
	destMask_ = 0x80;
	nextDest_++;
	if (nextDest_ == end_)
	  growBuffer_();
	*nextDest_ = 0;
      }
    }
    *nextDest_ |= destMask_;
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
  }
  else
  {
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    *nextDest_ |= destMask_;
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    if (sameDiff)
      encodeValue(1, 1);
    else
    {
      encodeValue(0, 1);
      encodeValue(value, numBits, newBlockSize);
    }
  }
}


void
EncodeBuffer::encodeCachedValue(unsigned char value, unsigned int numBits,
				CharCache & cache, unsigned int blockSize)
{
  unsigned int index;
  if (cache.lookup(value, index))
  {
    if (index > 1)
      index++;
    for (unsigned int count = index; count; count--)
    {
      destMask_ >>= 1;
      if (destMask_ == 0)
      {
	destMask_ = 0x80;
	nextDest_++;
	if (nextDest_ == end_)
	  growBuffer_();
	*nextDest_ = 0;
      }
    }
    *nextDest_ |= destMask_;
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
  }
  else
  {
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    *nextDest_ |= destMask_;
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    encodeValue(value, numBits, blockSize);
  }
}


void
EncodeBuffer::encodeCachedValue(unsigned int value, unsigned int numBits,
				PixelCache & cache,
				HuffmanCoder& escapeCoder0,
				HuffmanCoder& escapeCoder1)
{
  unsigned int index;
  if (cache.lookup(value, index))
  {
    if (index > 1)
      index++;
    for (unsigned int count = index; count; count--)
    {
      destMask_ >>= 1;
      if (destMask_ == 0)
      {
	destMask_ = 0x80;
	nextDest_++;
	if (nextDest_ == end_)
	  growBuffer_();
	*nextDest_ = 0;
      }
    }
    *nextDest_ |= destMask_;
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
  }
  else
  {
    // The value is not in the cache, so send an escape code, followed by
    // the value
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    *nextDest_ |= destMask_;
    destMask_ >>= 1;
    if (destMask_ == 0)
    {
      destMask_ = 0x80;
      nextDest_++;
      if (nextDest_ == end_)
	growBuffer_();
      *nextDest_ = 0;
    }
    // To transmit the value, use run-length coding with the static
    // Huffman code implemented by the supplied "escapeCoder" object
    //X encodeValue(value, numBits, numBits);
    unsigned int srcMask = 0x1;
    unsigned int pixelValue = ((value & srcMask) ? 1 : 0);
    encodeValue(pixelValue, 1, 1);
    for (unsigned int x = 0; x < numBits;)
    {
      unsigned int runStart = x;
      if (pixelValue)
      {
	while (x < numBits)
	{
	  if (!(value & srcMask))
	    break;
	  srcMask <<= 1;
	  x++;
	}
      }
      else
      {
	while (x < numBits)
	{
	  if (value & srcMask)
	    break;
	  srcMask <<= 1;
	  x++;
	}
      }
      unsigned int runLength = x - runStart;
      if (pixelValue)
      {
	escapeCoder1.encode(runLength - 1, *this);
	pixelValue = 0;
      }
      else
      {
	escapeCoder0.encode(runLength - 1, *this);
	pixelValue = 1;
      }
    }
  }
}


void
EncodeBuffer::encodeByte(unsigned char value)
{
  if (destMask_ != 0x80)
  {
    destMask_ = 0x80;
    nextDest_++;
    if (nextDest_ == end_)
      growBuffer_();
  }
  *nextDest_++ = value;
  if (nextDest_ == end_)
    growBuffer_();
}


unsigned int
EncodeBuffer::getDataLength() const
{
  unsigned int length = nextDest_ - buffer_;
  if (destMask_ != 0x80)
    length++;
  return length;
}

unsigned int
EncodeBuffer::getDataLengthInBits() const
{
  unsigned int length = nextDest_ - buffer_;
  length <<= 3;
  unsigned char mask = destMask_;
  while (mask != 0x80)
  {
    length++;
    mask <<= 1;
  }
  return length;
}


unsigned char *
EncodeBuffer::getData()
{
  return buffer_;
}


unsigned int
EncodeBuffer::getCumulativeBitsWritten()
{
  unsigned int bitsWritten = ((nextDest_ - buffer_) << 3);
  unsigned char mask = 0x80;
  while (mask != destMask_)
  {
    mask >>= 1;
    bitsWritten++;
  }
  unsigned int diff = bitsWritten - cumulativeBits_;
  cumulativeBits_ = bitsWritten;
  return diff;
}


void
EncodeBuffer::growBuffer_()
{
  unsigned int nextDestOffset = nextDest_ - buffer_;
  unsigned int newSize = size_ + size_;
  unsigned char *newBuffer = new unsigned char[newSize + PREFIX_SIZE] +
  PREFIX_SIZE;
  memcpy(newBuffer, buffer_, size_);
  newBuffer[size_] = 0;
  delete[](buffer_ - PREFIX_SIZE);
  buffer_ = newBuffer;
  size_ = newSize;
  end_ = buffer_ + size_;
  nextDest_ = buffer_ + nextDestOffset;
}
