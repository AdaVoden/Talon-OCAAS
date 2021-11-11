#ifndef EncodeBuffer_H
#define EncodeBuffer_H

class IntCache;
class CharCache;
class PixelCache;
class HuffmanCoder;


class EncodeBuffer
{
  public:
  EncodeBuffer();
  ~EncodeBuffer();

  void reset();
  void encodeValue(unsigned int value, unsigned int numBits,
		   unsigned int blockSize = 0);
  void encodeCachedValue(unsigned int value, unsigned int numBits,
			 IntCache & cache, unsigned int blockSize = 0);
  void encodeCachedValue(unsigned char value, unsigned int numBits,
			 CharCache & cache, unsigned int blockSize = 0);
  void encodeCachedValue(unsigned int value, unsigned int numBits,
			 PixelCache & cache, HuffmanCoder & escapeCoder0,
			 HuffmanCoder & escapeCoder1);
  void encodeByte(unsigned char value);

  unsigned char *getData();
  unsigned int getDataLength() const;
  unsigned int getDataLengthInBits() const;
  unsigned int getCumulativeBitsWritten();

    private:
  void growBuffer_();

  unsigned int size_;
  unsigned char *buffer_;
  const unsigned char *end_;	// points to byte just beyond end of buffer

  unsigned char *nextDest_;
  unsigned char destMask_;
  unsigned int cumulativeBits_;
};

#endif /* EncodeBuffer_H */
