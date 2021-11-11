#ifndef ClientMultiplexer_H
#define ClientMultiplexer_H

#include "Multiplexer.H"


class ClientMultiplexer:public Multiplexer
{
  public:
  ClientMultiplexer(int proxyFD, int statisticsLevel):
    Multiplexer(proxyFD), statisticsLevel_(statisticsLevel)
  {
  }
  virtual ~ ClientMultiplexer()
  {
  }

protected:
  virtual void createNewConnection(int fd);
  virtual int createNewConnectionFromProxy(int channelID);

  virtual int channelIDToFD(int channelID) const
  {
    return channelID;
  }
  virtual int fdToChannelID(int fd) const
  {
    return fd;
  }
  virtual void cleanupChannelFDMapping(int channelFD)
  {
    channelFD = 0;
  }

  unsigned int statisticsLevel_;
};


#endif /* ClientMultiplexer_H */
