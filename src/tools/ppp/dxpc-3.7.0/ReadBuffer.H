#ifndef ReadBuffer_H
#define ReadBuffer_H

class ReadBuffer
{
  public:
  ReadBuffer(int fd, unsigned int maxReadSize = 0);
    virtual ~ ReadBuffer();

  int doRead();

  const unsigned char *getMessage(unsigned int &dataLength);

    protected:
    virtual int locateMessage(const unsigned char *start,
			      const unsigned char *end,
			      unsigned int &headerLength,
			      unsigned int &dataLength,
			      unsigned int &trailerLength) = 0;
  int fd_;
  unsigned char *buffer_;
  unsigned int length_;
  unsigned int size_;
  unsigned int start_;
  unsigned int maxReadSize_;
};

#endif /* ReadBuffer_H */
