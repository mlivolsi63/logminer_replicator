#include <fstream>

const int MAX=256;
const int MSG_SIZE=8192;
const int STATIC_BYTES=8;

struct messages
{
 char msgType[6];
 char msgNumber[10];
 char msgTime[30];
 char msgCount[4];
 char SourceProgram[24];
 char Message[128];
 char Action[70];
 char nodeName[10];
};

class DataStream
{
  private:
          int fd_map, sp1, initialConnectFlag;
          char *addr;
          struct stat statbuf;
          struct messages ConsoleMsg;
          std::fstream logfile;
  public:
  DataStream();
  int    TCPechod(int fd, int verbose, char queueDir[], int numberOfQueues);
  int    SendConsoleMessages(int fd, std::string outbuf, int rc, int verbose);
  int    switchQueue(int numberOfQueues, char queueDir[]);
};

std::fstream  glb_queuefile;

