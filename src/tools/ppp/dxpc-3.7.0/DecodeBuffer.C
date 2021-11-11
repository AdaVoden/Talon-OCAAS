#include <iostream.h>
#include <stdlib.h>
#include <string.h>
#include "DecodeBuffer.H"
#include "IntCache.H"
#include "CharCache.H"
#include "PixelCache.H"
#include "HuffmanCoder.H"
#include "constants.H"


DecodeBuffer::DecodeBuffer(const unsigned char *data, unsigned int length):
  buffer_(data),
  end_(buffer_ + length),
  nextSrc_(buffer_), 
  srcMask_(0x80)
{
}


int
DecodeBuffer::decodeValue(unsigned int &value, unsigned int numBits,
			  unsigned int blockSize, int endOkay)
{
  unsigned int result = 0;
  unsigned int destMask = 0x1;
  unsigned int bitsRead = 0;
  if (blockSize == 0)
    blockSize = numBits;

  unsigned char nextSrcChar = *nextSrc_;
  unsigned int numBlocks = 1;
  do
  {
    if (numBlocks == 4)
      blockSize = numBits;
    unsigned int bitsToRead = ((blockSize > numBits - bitsRead) ?
			       numBits - bitsRead : blockSize);
    unsigned int count = 0;
    unsigned char lastBit;
    do
    {
      if (nextSrc_ >= end_)
      {
	if (!endOkay)
	{
	  cerr << "assertion 1 failed in DecodeBuffer::decodeValue()" << endl;
	  cerr << "  nextSrc_ " << (nextSrc_ - buffer_) << ", end_ " <<
	    (end_ - buffer_) << endl;
	  abort();
	}
	return 0;
      }
      lastBit = (nextSrcChar & srcMask_);
      if (lastBit)
	result |= destMask;
      srcMask_ >>= 1;
      if (srcMask_ == 0)
      {
	srcMask_ = 0x80;
	nextSrc_++;
	nextSrcChar = *nextSrc_;
      }
      destMask <<= 1;
    }
    while (bitsToRead > ++count);
    bitsRead += bitsToRead;
    if (bitsRead < numBits)
    {
      if (nextSrc_ >= end_)
      {
	if (!endOkay)
	{
	  cerr << "assertion 2 failed in DecodeBuffer::decodeValue()" << endl;
	  cerr << "  nextSrc_ " << (nextSrc_ - buffer_) << ", end_ " <<
	    (end_ - buffer_) << endl;
	  abort();
	}
	return 0;
      }
      unsigned char moreData = (nextSrcChar & srcMask_);
      srcMask_ >>= 1;
      if (srcMask_ == 0)
      {
	srcMask_ = 0x80;
	nextSrc_++;
	nextSrcChar = *nextSrc_;
      }
      if (!moreData)
      {
	if (lastBit)
	{
	  do
	  {
	    result |= destMask;
	    destMask <<= 1;
	  }
	  while (numBits > ++bitsRead);
	}
	else
	  bitsRead = numBits;
      }
    }
    blockSize >>= 1;
    if (blockSize < 2)
      blockSize = 2;
    numBlocks++;
  }
  while (numBits > bitsRead);
  value = result;
  return 1;
}


int
DecodeBuffer::decodeCachedValue(unsigned int &value, unsigned int numBits,
				IntCache & cache, unsigned int blockSize,
				int endOkay)
{
  if (nextSrc_ >= end_)
    return 0;
  unsigned int index = 0;
  unsigned char nextSrcChar = *nextSrc_;
  while (!(nextSrcChar & srcMask_))
  {
    index++;
    srcMask_ >>= 1;
    if (srcMask_ == 0)
    {
      srcMask_ = 0x80;
      nextSrc_++;
      if (nextSrc_ >= end_)
      {
	if (!endOkay)
	{
	  cerr << "assertion 1 failed in DecodeBuffer::decodeCachedValue()" <<
	    endl;
	  cerr << "  nextSrc_ " << (nextSrc_ - buffer_) << ", end_ " <<
	    (end_ - buffer_) << endl;
	  abort();
	}
	return 0;
      }
      nextSrcChar = *nextSrc_;
    }
  }
  srcMask_ >>= 1;
  if (srcMask_ == 0)
  {
    srcMask_ = 0x80;
    nextSrc_++;
  }
  if (index == 2)
  {
    unsigned int sameDiff;
    decodeValue(sameDiff, 1);
    if (sameDiff)
    {
      value = cache.getLastDiff(PARTIAL_INT_MASK[numBits]);
      cache.insert(value, PARTIAL_INT_MASK[numBits]);
    }
    else
    {
      blockSize = cache.getBlockSize(numBits);
      if (decodeValue(value, numBits, blockSize, endOkay))
      {
	cache.insert(value, PARTIAL_INT_MASK[numBits]);
	return 1;
      }
    }
    return 0;
  }
  else
  {
    if (index > 2)
      index--;
    if (index > cache.getSize())
    {
      cerr << "Assertion 2 failed in DecodeCachedValue: index=" << index <<
	", cache size=" << cache.getSize() << endl;
      abort();
    }
    value = cache.get(index);
    return 1;
  }
}


int
DecodeBuffer::decodeCachedValue(unsigned char &value, unsigned int numBits,
				CharCache & cache, unsigned int blockSize,
				int endOkay)
{
  if (nextSrc_ >= end_)
    return 0;
  unsigned int index = 0;
  unsigned char nextSrcChar = *nextSrc_;
  while (!(nextSrcChar & srcMask_))
  {
    index++;
    srcMask_ >>= 1;
    if (srcMask_ == 0)
    {
      srcMask_ = 0x80;
      nextSrc_++;
      if (nextSrc_ >= end_)
      {
	if (!endOkay)
	{
	  cerr << "assertion 3 failed in DecodeBuffer::decodeCachedValue()" <<
	    endl;
	  cerr << "  nextSrc_ " << (nextSrc_ - buffer_) << ", end_ " <<
	    (end_ - buffer_) << endl;
	  abort();
	}
	return 0;
      }
      nextSrcChar = *nextSrc_;
    }
  }
  srcMask_ >>= 1;
  if (srcMask_ == 0)
  {
    srcMask_ = 0x80;
    nextSrc_++;
  }
  if (index == 2)
  {
    unsigned int val;
    if (decodeValue(val, numBits, blockSize, endOkay))
    {
      value = (unsigned char) val;
      cache.insert(value);
      return 1;
    }
    else
      return 0;
  }
  else
  {
    if (index > 2)
      index--;
    if (index > cache.getSize())
    {
      cerr << "Assertion 4 failed in DecodeCachedValue: index=" << index <<
	", cache size=" << cache.getSize() << endl;
      abort();
    }
    value = cache.get(index);
    return 1;
  }
}


int
DecodeBuffer::decodeCachedValue(unsigned int& value, unsigned int numBits,
				PixelCache& cache,
				HuffmanCoder& escapeCoder0,
				HuffmanCoder& escapeCoder1, int endOkay)
{
  if (nextSrc_ >= end_)
    return 0;
  unsigned int index = 0;
  unsigned char nextSrcChar = *nextSrc_;
  while (!(nextSrcChar & srcMask_))
  {
    index++;
    srcMask_ >>= 1;
    if (srcMask_ == 0)
    {
      srcMask_ = 0x80;
      nextSrc_++;
      if (nextSrc_ >= end_)
      {
	if (!endOkay)
	{
	  cerr << "assertion 5 failed in DecodeBuffer::decodeCachedValue()" <<
	    endl;
	  cerr << "  nextSrc_ " << (nextSrc_ - buffer_) << ", end_ " <<
	    (end_ - buffer_) << endl;
	  abort();
	}
	return 0;
      }
      nextSrcChar = *nextSrc_;
    }
  }
  srcMask_ >>= 1;
  if (srcMask_ == 0)
  {
    srcMask_ = 0x80;
    nextSrc_++;
  }
  if (index == 2)
  {
    value = 0;
    unsigned int pixelValue;
    if (!decodeValue(pixelValue, 1, 1, endOkay))
      return 0;
    unsigned int mask = 0x1;
    for (unsigned int x = 0; x < numBits;)
    {
      unsigned int runLength;
      if (pixelValue)
      {
	runLength = escapeCoder1.decode(*this) + 1;
	for (unsigned int i = runLength; i; i--)
	{
	  value |= mask;
	  mask <<= 1;
	}
	pixelValue = 0;
      }
      else
      {
	runLength = escapeCoder0.decode(*this) + 1;
	mask <<= runLength;
	pixelValue = 1;
      }
      x += runLength;
    }
    cache.insert(value);
    return 1;
  }
  else
  {
    if (index > 2)
      index--;
    if (index > cache.getSize())
    {
      cerr << "Assertion 6 failed in DecodeCachedValue: index=" << index <<
	", cache size=" << cache.getSize() << endl;
      abort();
    }
    value = cache.get(index);
    return 1;
  }
}
