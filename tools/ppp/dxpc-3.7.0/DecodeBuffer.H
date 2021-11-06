#ifndef DecodeBuffer_H
#define DecodeBuffer_H

class IntCache;
class CharCache;
class PixelCache;
class HuffmanCoder;


class DecodeBuffer
{
  public:
  DecodeBuffer(const unsigned char *data, unsigned int length);
   ~DecodeBuffer()
  {
  }

  int decodeValue(unsigned int &value, unsigned int numBits,
		  unsigned int blockSize = 0, int endOkay = 0);
  int decodeCachedValue(unsigned int &value, unsigned int numBits,
			IntCache & cache, unsigned int blockSize = 0,
			int endOkay = 0);
  int decodeCachedValue(unsigned char &value, unsigned int numBits,
			CharCache & cache, unsigned int blockSize = 0,
			int endOkay = 0);
  int decodeCachedValue(unsigned int &value, unsigned int numBits,
			PixelCache & cache, HuffmanCoder & escapeCoder0,
			HuffmanCoder & escapeCoder1, int endOkay = 0);

private:
  const unsigned char *buffer_;
  const unsigned char *end_;
  const unsigned char *nextSrc_;
  unsigned char srcMask_;
};

#endif /* DecodeBuffer_H */
