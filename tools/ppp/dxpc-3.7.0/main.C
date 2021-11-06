#include <iostream.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <X11/Xproto.h>
#include <pwd.h>
#ifdef _AIX
#include <strings.h>
#endif /* _AIX */

#include "constants.H"
#include "util.H"
#include "ClientMultiplexer.H"
#include "ServerMultiplexer.H"


#ifdef __EMX__

struct sockaddr_un
{
  u_short sun_family;		/* socket family: AF_UNIX */
  char sun_path[108];		/* path name (not used) */
};

#endif


#ifdef _AIX
#include <sys/select.h>
#define DXPC_ACCEPT_IS_SIZE_T 1
#endif /* _AIX */

#if defined(hpux) && !defined(RLIM_INFINITY)
/* HP-UX hides this define */
#define	RLIM_INFINITY	0x7fffffff
#endif


static void Usage(const char *const *argv);
static void Cleanup();
static void HandleSignal(int);
static int AwaitConnection(int portNum);
static int ConnectToRemote(char *remoteHost, int portNum);
static void DaemonInit();
static void KillDaemon();
static void MakeLockFileName(char*);

static int VersionNumbersMatch(int sockfd);
static int ReadDataCh(int fd, char *buf, int maxlen, char stop);

// This variable tells whether or not the client should initiate the
// connection.  (see -w option)
static int ClientInitiateConnection = 0;

// Maximum number of open file descriptors for this process
static unsigned int maxNumFDs = 0;

// Variable to tell output routines whether they should be quiet or not
// Added for -f option
// This variable is used by other files
int silent = 0;

// These two variables are needed for the forking
// We don't fork by default since old versions didn't
static int dofork = 0;
static char lockfilename[512] = {0};

// And the default lockfilename - this goes in the user's $HOME dir
static const char *LOCK_FILE_NAME = ".dxpc.pid";

// Now statistics can go to a file
char logfilename[1024] = {0};
ostream *logofs;

// Info about sockets used by the client proxy (global so that
// the cleanup signal handler can access it)
static char udomSocketPathname[100];
static int useUnixDomainSocket = 1;
sockaddr_in serverAddr;

// dxpc runs in client mode or server mode
enum ProxyMode
{
  PROXY_CLIENT, PROXY_SERVER
}
proxyMode = PROXY_SERVER;


int
main(int argc, char **argv)
{
  (void) SaveCopyrightFromOptimizer();

  udomSocketPathname[0] = 0;

  unsigned int proxyPort = DEFAULT_PROXY_PORT;
  unsigned int displayNum = DEFAULT_DISPLAY_NUM;
  unsigned int statisticsLevel = 0;
  int useTCPSocket = 1;

  char *remoteHost = NULL;
  // used to use getopt here, but it caused too many
  // portability problems
  for (int argi = 1; argi < argc; argi++)
  {
    char *nextArg = argv[argi];
    if (*nextArg == '-')
    {
      switch (nextArg[1])
      {
      case 'd':
	{
	  const char *arg = GetArg(argi, argc, argv);
	  if (arg == NULL)
	    Usage(argv);
	  else
	    displayNum = atoi(arg);
	}
	break;
      case 'f':
	dofork = 1;
	break;
      case 'k':
	KillDaemon();		// This function doesn't return

	break;
      case 'l':
	{
	  const char *arg = GetArg(argi, argc, argv);
	  if (!arg)
	    Usage(argv);
	  else
	    strcpy(logfilename, arg);
	}
	break;
      case 'p':
	{
	  const char *arg = GetArg(argi, argc, argv);
	  if (arg == NULL)
	    Usage(argv);
	  else
	    proxyPort = atoi(arg);
	}
	break;
      case 's':
	{
	  const char *arg = GetArg(argi, argc, argv);
	  if (arg == NULL)
	    Usage(argv);
	  else
	    statisticsLevel = atoi(arg);
	}
	break;
      case 't':
	useTCPSocket = 0;
	break;
      case 'u':
	useUnixDomainSocket = 0;
	break;
      case 'v':
	PrintVersionInfo();
	exit(0);
      case 'w':
	ClientInitiateConnection = 1;
	break;
      default:
	Usage(argv);
      }
    }
    else
    {
      if (remoteHost != NULL)
	Usage(argv);
      else
	remoteHost = nextArg;
    }
  }

  if (logfilename[0] != '\0')
  {
    logofs = new ofstream(logfilename, ios::out);
  }
  else
  {
    logofs = &cout;
  }

  int xServerAddrFamily = AF_INET;
  sockaddr *xServerAddr = NULL;
  unsigned int xServerAddrLength = 0;

  if ((!remoteHost && !ClientInitiateConnection) ||
      (remoteHost && ClientInitiateConnection))
  {
    cout << "dxpc proxy running in CLIENT mode" << endl;
    proxyMode = PROXY_CLIENT;
  }
  else
  {
    cout << "dxpc proxy running in SERVER mode" << endl;
    proxyMode = PROXY_SERVER;

    // Keep Cleanup() from deleting the real X server's pipe
    // when we exit...
    useUnixDomainSocket = 0;

    // $DISPLAY is the X server for which we'll act as a proxy
    char *display = getenv("DISPLAY");
    if ((display == NULL) || (*display == 0))
    {
      cerr << "$DISPLAY is not set" << endl;
      Cleanup();
    }
    char *separator = strchr(display, ':');
    if ((separator == NULL) || !isdigit(*(separator + 1)))
    {
      cerr << "invalid DISPLAY '" << display << "'" << endl;
      Cleanup();
    }
    *separator = 0;
    int displayNum = atoi(separator + 1);
    if ((separator == display) || !strcmp(display, "unix"))
    {
      // UNIX domain port
      xServerAddrFamily = AF_UNIX;
      sockaddr_un *xServerAddrUNIX = new sockaddr_un;
      xServerAddrUNIX->sun_family = AF_UNIX;
      sprintf(udomSocketPathname, "/tmp/.X11-unix/X%d", displayNum);
      struct stat statInfo;
      if (stat(udomSocketPathname, &statInfo) == -1)
      {
#if (defined(__hpux) || defined(hpux)) && !defined(PGID_USE_PID)
	sprintf(udomSocketPathname, "/usr/spool/sockets/X11/%d", displayNum);
	if (stat(udomSocketPathname, &statInfo) == -1)
	{
#endif
	  cerr << "cannot open UNIX domain connection to X server" << endl;
	  Cleanup();
#if (defined(__hpux) || defined(hpux)) && !defined(PGID_USE_PID)
	}
#endif
      }
      strcpy(xServerAddrUNIX->sun_path, udomSocketPathname);
      xServerAddr = (sockaddr *) xServerAddrUNIX;
      //      xServerAddrLength = strlen(udomSocketPathname) + 2;
      xServerAddrLength = sizeof(sockaddr_un);
    }
    else
    {
      // TCP port
      xServerAddrFamily = AF_INET;
      int ipAddr;
      hostent *hostAddr = gethostbyname(display);
      if (hostAddr == NULL)
      {
	// on some UNIXes, gethostbyname doesn't accept IP addresses,
	// so try inet_addr:
	ipAddr = (int) inet_addr(display);
	if (ipAddr == -1)
	{
	  cerr << "Unknown host '" << display << "'" << endl;
	  Cleanup();
	}
      }
      else
	ipAddr = *(int *) hostAddr->h_addr_list[0];
      sockaddr_in *xServerAddrTCP = new sockaddr_in;
      xServerAddrTCP->sin_family = AF_INET;
      xServerAddrTCP->sin_port = htons(X_TCP_PORT + displayNum);
      xServerAddrTCP->sin_addr.s_addr = ipAddr;
      xServerAddr = (sockaddr *) xServerAddrTCP;
      xServerAddrLength = sizeof(sockaddr_in);
    }
  }

  // Increase the max # of open file descriptors for this process

  maxNumFDs = 0;
#if defined(RLIMIT_NOFILE)
  rlimit limits;
  if (getrlimit(RLIMIT_NOFILE, &limits) == 0)
  {
    if (limits.rlim_max == RLIM_INFINITY)
      maxNumFDs = 0;
    else
      maxNumFDs = (unsigned int) limits.rlim_max;
  }
#endif /* RLIMIT_NOFILE */

#if defined(_SC_OPEN_MAX)
  if (maxNumFDs == 0)
    maxNumFDs = sysconf(_SC_OPEN_MAX);
#endif

#if defined(FD_SETSIZE)
  if (maxNumFDs > FD_SETSIZE)
    maxNumFDs = FD_SETSIZE;
#endif /* FD_SETSIZE */

#if defined(RLIMIT_NOFILE)
  if (limits.rlim_cur < (int) maxNumFDs)
  {
    limits.rlim_cur = (int) maxNumFDs;
    setrlimit(RLIMIT_NOFILE, &limits);
  }
#endif /* RLIMIT_NOFILE */

  if (maxNumFDs == 0)
  {
    cerr <<
      "cannot determine number of available file descriptors, exiting!"
      << endl;
    return 1;
  }
  // Install some signal handlers for graceful shutdown
  signal(SIGHUP, HandleSignal);
  signal(SIGINT, HandleSignal);
  signal(SIGTERM, HandleSignal);

  signal(SIGPIPE, (void (*)(int)) SIG_IGN);


  // If running as client proxy, open sockets that mimic an
  // X display to which X clients can connect (e.g., unix:8
  // and <hostname>:8)
  int tcpFD = -1;
  int unixFD = -1;
  if (proxyMode == PROXY_CLIENT)
  {
    if (useTCPSocket)
    {
      // Open TCP socket for display
      tcpFD = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
      if (tcpFD == -1)
      {
	cerr << "socket() failed for TCP socket, errno=" <<
	  errno << endl;
	Cleanup();
      }
      int flag = 1;
      if (setsockopt(tcpFD, SOL_SOCKET, SO_REUSEADDR, (char *) &flag,
		     sizeof(flag)) < 0)
      {
	cerr << "setsockopt(SO_REUSEADDR) failed for TCP socket,
	  errno = " << errno << endl;
      }
      sockaddr_in tcpAddr;
      tcpAddr.sin_family = AF_INET;
      unsigned int xPortTCP = X_TCP_PORT + displayNum;
      tcpAddr.sin_port = htons(xPortTCP);
      tcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      if (bind(tcpFD, (sockaddr *) & tcpAddr, sizeof(tcpAddr)) == -1)
      {
	cerr << "bind() failed for TCP port " << xPortTCP <<
	  ", errno=" << errno << endl;
	Cleanup();
      }
      if (listen(tcpFD, 5) == -1)
      {
	cerr << "listen() failed for TCP port " << xPortTCP <<
	  ", errno=" << errno << endl;
	Cleanup();
      }
    }
    if (useUnixDomainSocket)
    {
      // Open UNIX domain socket for display
      unixFD = socket(AF_UNIX, SOCK_STREAM, PF_UNSPEC);
      if (unixFD == -1)
      {
	cerr << "socket() failed for UNIX domain socket, errno=" <<
	  errno << endl;
	Cleanup();
      }
      sockaddr_un unixAddr;
      unixAddr.sun_family = AF_UNIX;
      struct stat dirStat;
      if ((stat("/tmp/.X11-unix", &dirStat) == -1) && (errno == ENOENT))
      {
	mkdir("/tmp/.X11-unix", 0777);
	chmod("/tmp/.X11-unix", 0777);
      }
      sprintf(udomSocketPathname, "/tmp/.X11-unix/X%d", displayNum);
      strcpy(unixAddr.sun_path, udomSocketPathname);
      if (bind(unixFD, (sockaddr *) & unixAddr,
	       strlen(udomSocketPathname) + 2) == -1)
      {
	cerr << "bind() failed for UNIX domain socket " <<
	  udomSocketPathname << ", errno=" << errno << endl;
	Cleanup();
      }
      if (listen(unixFD, 5) == -1)
      {
	cerr << "listen() failed for UNIX domain socket " <<
	  udomSocketPathname << ", errno=" << errno << endl;
	Cleanup();
      }
    }
  }
  if (dofork)
  {
    DaemonInit();
  }
  // Set up low-bandwidth connection between the proxies
  int proxyFD = -1;
  if (ClientInitiateConnection)
  {
    if (proxyMode == PROXY_SERVER)
    {
      proxyFD = AwaitConnection(proxyPort);

      if (!silent)
      {
	*logofs << "connected to client proxy\nready" << endl;
      }
    }
    else
    {				/* proxyMode == PROXY_CLIENT */
      proxyFD = ConnectToRemote(remoteHost, proxyPort);
      // Now we send our version number to the client.  If we don't
      // get back an identical string we exit.
      if (!VersionNumbersMatch(proxyFD))
      {
	Cleanup();
      }
      if (!silent)
      {
	*logofs << "connected to server proxy\nready" << endl;
      }
    }
  }
  else
  {				// ! ClientInitiateConnection

    if (proxyMode == PROXY_SERVER)
    {
      proxyFD = ConnectToRemote(remoteHost, proxyPort);

      // Now we send our version number to the client.  If we don't
      // get back an identical string we exit.
      if (!VersionNumbersMatch(proxyFD))
      {
	Cleanup();
      }
      if (!silent)
      {
	*logofs << "connected to client proxy\nready" << endl;
      }
      if (proxyFD < 0)
	return 1;
    }
    else
    {
      proxyFD = AwaitConnection(proxyPort);
      if (!silent)
      {
	*logofs << "connected to server proxy\nready" << endl;
      }
    }
  }

  // Create multiplexer
  Multiplexer *multiplexer;
  if (proxyMode == PROXY_SERVER)
    multiplexer = new ServerMultiplexer(proxyFD, xServerAddrFamily,
					xServerAddr, xServerAddrLength,
					statisticsLevel);
  else
    multiplexer = new ClientMultiplexer(proxyFD, statisticsLevel);


  // Loop endlessly, reading from all of the open file descriptors
  for (;;)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(proxyFD, &readSet);
    unsigned int numFDsToSelect = proxyFD + 1;
    if (proxyMode == PROXY_CLIENT)
    {
      if (useTCPSocket)
      {
	FD_SET(tcpFD, &readSet);
	if (tcpFD >= (int) numFDsToSelect)
	  numFDsToSelect = tcpFD + 1;
      }
      if (useUnixDomainSocket)
      {
	FD_SET(unixFD, &readSet);
	if (unixFD >= (int) numFDsToSelect)
	  numFDsToSelect = unixFD + 1;
      }
    }
    multiplexer->setSelectFDs(&readSet, numFDsToSelect);

    timeval delay;
    delay.tv_sec = 600;
    delay.tv_usec = 0;
    // PGID_USE_PID, defined in <sys/types.h>, is specific to HP-UX 10.x
#if (defined(__hpux) || defined(hpux)) && !defined(PGID_USE_PID)
    int result = select(numFDsToSelect, (int *) &readSet, NULL, NULL, &delay);
#else
    int result = select(numFDsToSelect, &readSet, NULL, NULL, &delay);
#endif
    if (result == -1)
    {
      if (errno == EINTR)
	continue;
      cerr << "select() failed, errno=" << errno << endl;
      Cleanup();
    }
    for (unsigned int j = 0; j < numFDsToSelect; j++)
    {
      if (!(FD_ISSET(j, &readSet)))
	continue;
      if (proxyMode == PROXY_CLIENT)
      {
	if (((int) j == tcpFD) && useTCPSocket)
	{
	  sockaddr_in newAddr;
#if defined(DXPC_ACCEPT_IS_SIZE_T)
	  size_t addrLen = sizeof(sockaddr_in);
#else
	  int addrLen = sizeof(sockaddr_in);
#endif /* defined(DXPC_ACCEPT_IS_SIZE_T) */
	  int newFD = accept(tcpFD, (sockaddr *) & newAddr, &addrLen);
	  if (newFD == -1)
	  {
	    cerr << "accept() failed, errno=" << errno << endl;
	    Cleanup();
	  }
	  multiplexer->createNewConnection(newFD);
	  continue;
	}
	if (((int) j == unixFD) && useUnixDomainSocket)
	{
	  sockaddr_un newAddr;
#if defined(DXPC_ACCEPT_IS_SIZE_T)
	  size_t addrLen = sizeof(sockaddr_un);
#else
	  int addrLen = sizeof(sockaddr_un);
#endif /* defined(DXPC_ACCEPT_IS_SIZE_T) */
	  int newFD = accept(unixFD, (sockaddr *) & newAddr, &addrLen);
	  if (newFD == -1)
	  {
	    cerr << "accept() failed, errno=" << errno << endl;
	    Cleanup();
	  }
	  multiplexer->createNewConnection(newFD);
	  continue;
	}
      }
      if (!multiplexer->handleSelect(j))
	Cleanup();
    }
  }

  return 0;
}

static void
Usage(const char *const *argv)
{
  cerr << "usage:" << endl << "(client)" << endl << "   " << argv[0] <<
    " [-d display_num] [-p port_num] [-f] [-u] [-s (1|2)] [-v] [-w]" <<
    endl << "(server)" << endl << "   " << argv[0] <<
    " [-p port_num] [-f] [-s(1|2)] [-v] [-w] client_hostname" << endl;
  exit(1);
}

static void
Cleanup()
{
  if (!silent)
    *logofs << "Closing all file descriptors and shutting down..." << endl;

  if (dofork)
    if (remove(lockfilename))
      perror("Unable to remove lockfile");

  if (useUnixDomainSocket)
    unlink(udomSocketPathname);

  for (unsigned int i = 0; i < maxNumFDs; i++)
    (void) close(i);

  exit(1);
}

static void
HandleSignal(int)
{
  Cleanup();
}

// Open TCP socket to listen for server proxy; block until server
// proxy connects, then close listener socket and return FD of socket
// on which server proxy is connected
//
static int
AwaitConnection(int portNum)
{
  int proxyFD = socket(AF_INET, SOCK_STREAM, 0);
  if (proxyFD == -1)
  {
    cerr << "socket() failed for TCP socket, errno=" << errno << endl;
    Cleanup();
  }
  int flag = 1;
  if (setsockopt(proxyFD, SOL_SOCKET, SO_REUSEADDR, (char *) &flag,
		 sizeof(flag)) < 0)
  {
    cerr << "setsockopt(SO_REUSEADDR) failed for proxy port, errno=" <<
      errno << endl;
  }
  sockaddr_in tcpAddr;
  tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_port = htons(portNum);
  tcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(proxyFD, (sockaddr *) & tcpAddr, sizeof(tcpAddr)) == -1)
  {
    cerr << "bind() failed for TCP port " << portNum <<
      ", errno=" << errno << endl;
    Cleanup();
  }
  if (listen(proxyFD, 1) == -1)
  {
    cerr << "listen() failed for TCP port " << portNum <<
      ", errno=" << errno << endl;
    Cleanup();
  }
  for (;;)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(proxyFD, &readSet);
#if (defined(__hpux) || defined(hpux)) && !defined(PGID_USE_PID)
    int result = select(proxyFD + 1, (int *) &readSet, NULL, NULL, NULL);
#else
    int result = select(proxyFD + 1, &readSet, NULL, NULL, NULL);
#endif
    if (result == -1)
    {
      if (errno == EINTR)
	continue;
      cerr << "select() failed, errno=" << errno << endl;
      Cleanup();
    }
    if (FD_ISSET(proxyFD, &readSet))
    {
      sockaddr_in newAddr;
#if defined(DXPC_ACCEPT_IS_SIZE_T)
      size_t addrLen = sizeof(sockaddr_in);
#else
      int addrLen = sizeof(sockaddr_in);
#endif /* defined(DXPC_ACCEPT_IS_SIZE_T) */
      int newFD = accept(proxyFD, (sockaddr *) & newAddr, &addrLen);
      if (newFD == -1)
      {
	cerr << "accept() failed, errno=" << errno << endl;
	Cleanup();
      }
      // Now we send our version number.
      char version[20];
      sprintf(version, "DXPC %i.%i", DXPC_VERSION_MAJOR, DXPC_VERSION_MINOR);
      write(newFD, version, strlen(version) + 1);

      // If the client doesn't like our version it will simply close the
      // socket.

      close(proxyFD);
      return newFD;
    }
  }
}


// Connect to remote proxy.  If successful, return FD of connection;
// if unsuccessful, return -1
//
static int
ConnectToRemote(char *remoteHost, int portNum)
{
  int remoteIPAddr;
  hostent *hostAddr = gethostbyname(remoteHost);
  if (hostAddr == NULL)
  {
    // on some UNIXes, gethostbyname doesn't accept IP addresses,
    // so try inet_addr:
    remoteIPAddr = (int) inet_addr(remoteHost);
    if (remoteIPAddr == -1)
    {
      cerr << "Unknown host '" << remoteHost << "'" << endl;
      Cleanup();
    }
  }
  else
    remoteIPAddr = *(int *) hostAddr->h_addr_list[0];

  int remoteProxyFD = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
  if (remoteProxyFD == -1)
  {
    cerr << "socket() failed, errno=" << errno << endl;
    Cleanup();
  }
  int flag = 1;
  if (setsockopt(remoteProxyFD, SOL_SOCKET, SO_REUSEADDR, (char *) &flag,
		 sizeof(flag)) < 0)
  {
    cerr << "setsockopt(SO_REUSEADDR) failed for proxy port, errno=" <<
      errno << endl;
  }
  if (!silent)
  {
    *logofs << "trying to connect to remote proxy..." << endl;
  }
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(portNum);
  addr.sin_addr.s_addr = remoteIPAddr;
  if (connect(remoteProxyFD, (sockaddr *) & addr, sizeof(sockaddr_in)) == -1)
  {
    cerr << "connect() failed, errno=" << errno << endl;
    close(remoteProxyFD);
    Cleanup();
  }
  flag = 1;
  setsockopt(remoteProxyFD, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,
	     sizeof(int));

  return remoteProxyFD;
}

static int
VersionNumbersMatch(int sockfd)
{
  // We consider the version numbers to match if the major and minor
  // numbers match.  Different patch levels should by definition be
  // compatible with each other.

  char version[20];
  char recvmsg[20];

  sprintf(version, "DXPC %i.%i", DXPC_VERSION_MAJOR, DXPC_VERSION_MINOR);

  if (ReadDataCh(sockfd, recvmsg, sizeof(recvmsg), '\0') < 0)
  {
    return 0;
  }
  if (strncmp(recvmsg, version, strlen(version)))
  {
    // Make sure recvmsg will be printable
    unsigned int ctr;
    for (ctr = 0;
	 ctr < strlen(recvmsg) &&
	 (isgraph(recvmsg[ctr]) || isspace(recvmsg[ctr]));
	 ctr++);

    recvmsg[ctr] = 0;

    cerr << "Error version numbers don't match!" << endl
      << "Local version: " << version << endl
      << "Remote version: " << recvmsg << endl;

    return 0;
  }
  return 1;
}

static int
ReadDataCh(int fd, char *buf, int maxlen, char stop)
{
  int ctr = 0;
  int result;

  while (ctr < maxlen)
  {
    if ((result = read(fd, buf + ctr, 1)) == -1 || !result)
    {
      return -1;
    }
    if (result && *(buf + ctr++) == stop)
    {
      return ctr;
    }
  }

  return 0;
}

static void
DaemonInit()
{
#ifdef __EMX__
  cerr << "The daemon option is disabled under OS/2" << endl;
  exit(-1);
#else

  switch (fork())
  {
  case -1:
    perror("dxpc");
    Cleanup();
  case 0:
    break;
  default:
    exit(0);
  }
  pid_t pid;

  if ((pid = setsid()) == -1)
  {
    cerr << "Error setsid() failed." << endl;
    Cleanup();
  }
  MakeLockFileName(lockfilename);

  // First we try to open the file to see if dxpc is already running
  FILE *pidfile = fopen(lockfilename, "r");

  if (pidfile)
  {
    // The open was successful
    // So we try and read a pid out of it.
    char oldpid[10];
    switch (fread(oldpid, 1, sizeof(oldpid), pidfile))
    {
    case 0:
      cerr << "Found empty pidfile " << lockfilename
	<< ".  Overriding." << endl;
      break;
    case -1:
      cerr << "Error reading from old pidfile " << lockfilename
	<< ".  Overriding." << endl;
      break;
    default:
      // Do a sanity check on the returned data
      if (!isdigit((int) ((unsigned char) oldpid[0])))
      {
	cerr << "Invalid data in pidfile " << lockfilename
	  << ".  Aborting." << endl;

	fclose(pidfile);
	Cleanup();
      }
      long oldpidval = atoi(oldpid);

      int override = 0;
      switch (kill(oldpidval, 0))
      {
      case ESRCH:
      case EPERM:
	// Either the pid doesn't exist or is owned by someone
	// else.  It's probably safe to override.
	cerr << "Stale pidfile found.  Overriding." << endl;
	override = 1;
	break;
      default:
	cerr << "Error.  It appears another dxpc is running at pid "
	     << oldpidval << endl
	     << "If this isn't correct, then delete " << lockfilename
	     << endl;
      }
      if( override ) break;
      fclose(pidfile);
      dofork = 0;		// So Cleanup() won't delete the lockfile

      Cleanup();
    }

    fclose(pidfile);
  }
  if (!(pidfile = fopen(lockfilename, "w")))
  {
    perror("dxpc");
    Cleanup();
  }
  fprintf(pidfile, "%d", pid);
  fclose(pidfile);

  // Now turn off all non-error output to the console
  silent = 1;

#endif
  return;
}

void
KillDaemon()
{
  char lockfilename[512];

  MakeLockFileName(lockfilename);

  FILE *pidfile = fopen(lockfilename, "r");

  if (pidfile)
  {
    // The open was successful
    // So we try and read a pid out of it.
    char pid[10];
    switch (fread(pid, 1, sizeof(pid), pidfile))
    {
    case 0:
    case -1:
      cerr << "Error reading pid from " << lockfilename << endl
	<< "You will have to manually kill the daemon." << endl;

      break;
    default:
      fclose(pidfile);

      // Do a sanity check on the returned data
      if (!isdigit((int) ((unsigned char) pid[0])))
      {
	cerr << "Invalid data in pidfile " << lockfilename
	  << ".  Aborting." << endl;

	exit(1);
      }
      long pidval = atoi(pid);

      cout << "Killing dxpc at pid " << pidval << endl;

      if (kill(pidval, SIGTERM) == -1)
      {
	perror("dxpc");
	cerr << "Leaving pidfile intact." << endl;
	exit(1);
      }
    }
    exit(0);
  }
  else
  {
    cerr << "No daemon is running." << endl;
    exit(1);
  }
}


static void
MakeLockFileName(char* lockfilename)
{
  char *homedir = getenv("HOME");
  if (!homedir)
  {
    cerr << "You have no environment variable HOME!" << endl
	 << "How did that happen?" << endl;
    exit(1);
  }
  strcpy(lockfilename, homedir);
  strcat(lockfilename, "/");
  strcat(lockfilename, LOCK_FILE_NAME);	// constants.h
  struct utsname unameInfo;
  if(uname(&unameInfo) == -1)
  {
    unameInfo.nodename[0] = 0;
  }
  else
  {
    char* dotptr = strchr( unameInfo.nodename, '.' );
    if( dotptr ) *dotptr = 0;
    strcat(lockfilename, "-");
    strcat(lockfilename, unameInfo.nodename);
  }

  struct passwd* passdata;
  if( passdata = getpwuid(getuid()))
  {
    strcat(lockfilename, "-");
    strcat(lockfilename, passdata->pw_name);
  }
  else
  {
    strcat(lockfilename, "");
  }
}
