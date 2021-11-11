#include <iostream.h>
#include "ClientMultiplexer.H"
#include "ClientChannel.H"
#include "util.H"


void
ClientMultiplexer::createNewConnection(int clientFD)
{
  int channelNum = fdToChannelID(clientFD);
  channels_[channelNum] = new ClientChannel(clientFD, statisticsLevel_);
  unsigned char message[3];
  message[0] = 0;
  message[1] = (unsigned char) CTRL_NEW_CONNECTION;
  message[2] = channelNum;
  WriteAll(proxyFD_, message, 3);
}


int
ClientMultiplexer::createNewConnectionFromProxy(int)
{
  cerr << "Internal error: in ClientMultiplexer::createNewConnectionFromProxy"
    << endl;
  return 0;
}
