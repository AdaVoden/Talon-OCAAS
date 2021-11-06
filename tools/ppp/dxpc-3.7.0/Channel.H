#ifndef Channel_H
#define Channel_H

class EncodeBuffer;

class Channel
{
  public:
  Channel():framingBitsOut_(0)
  {
  }
  virtual ~ Channel()
  {
  }

  virtual int doRead(EncodeBuffer &) = 0;
  virtual int doWrite(const unsigned char *message, unsigned int length) = 0;

  void recordFramingBits(unsigned int numBits)
  {
    framingBitsOut_ += numBits;
  }

  protected:
  unsigned int framingBitsOut_;
};

#endif /* Channel_H */
