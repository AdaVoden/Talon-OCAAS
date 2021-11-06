#include <iostream.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "ServerMultiplexer.H"
#include "ServerChannel.H"


ServerMultiplexer::ServerMultiplexer(int proxyFD, int xServerAddrFamily,
				     sockaddr * xServerAddr,
				     unsigned int xServerAddrLength,
				     unsigned int statisticsLevel):
  Multiplexer(proxyFD), xServerAddrFamily_(xServerAddrFamily),
  xServerAddr_(xServerAddr), xServerAddrLength_(xServerAddrLength), 
  statisticsLevel_(statisticsLevel)
{
  for (unsigned int i = 0; i < MAX_CONNECTIONS; i++)
  {
    fdToChannelIDMap_[i] = -1;
    channelIDToFDMap_[i] = -1;
  }
}


void
ServerMultiplexer::createNewConnection(int clientFD)
{
  clientFD = 0;
  cerr << "Internal error: in ServerMultiplexer::createNewConnection" << endl;
}


int
ServerMultiplexer::createNewConnectionFromProxy(int channelID)
{
  // Connect to the real X server
  int xServerFD = socket(xServerAddrFamily_, SOCK_STREAM, PF_UNSPEC);
  if (xServerFD == -1)
  {
    cerr << "socket() failed, errno=" << errno << endl;
    return 0;
  }
  if (connect(xServerFD, xServerAddr_, xServerAddrLength_) == -1)
  {
    cerr << "connect() to X server failed, errno=" << errno << " (" <<
      strerror(errno) << ")" << endl;
    close(xServerFD);
    return 0;
  }
  channelIDToFDMap_[channelID] = xServerFD;
  fdToChannelIDMap_[xServerFD] = channelID;
  channels_[channelID] = new ServerChannel(xServerFD, statisticsLevel_);
  return 1;
}


int
ServerMultiplexer::channelIDToFD(int channelID) const
{
  if ((channelID < 0) || ((unsigned int) channelID >= MAX_CONNECTIONS))
    return -1;
  else
    return channelIDToFDMap_[channelID];
}

int
ServerMultiplexer::fdToChannelID(int fd) const
{
  if ((fd < 0) || ((unsigned int) fd >= MAX_CONNECTIONS))
    return -1;
  else
    return fdToChannelIDMap_[fd];
}

void
ServerMultiplexer::cleanupChannelFDMapping(int channelID)
{
  int fd = channelIDToFDMap_[channelID];
  if (fd != -1)
  {
    channelIDToFDMap_[channelID] = -1;
    fdToChannelIDMap_[fd] = -1;
  }
}
