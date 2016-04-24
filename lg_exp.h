const int FN_LEN=128;
const int MAX=8192;
const int PORT_LEN=6;
const int IP_LEN=15;
const int TRUE=1;
const int FALSE=0;
const int PWD_LEN=16;
const int STATIC_BYTES=8;
const int PING_COUNT=15;       // After 15 loops (15 sleeps) , ping the server to make sure it's alive
const int TIME_BUF_LN=20;
const int TMP_LN=16;

std::ifstream glb_queuefile;                                                       // Have this as a global variable
std::string   glb_outbuffer;                                                       // Same deal with this guy

int socketFD;
int      glb_switchState=0;
//---------------------------------------------
// Communication variables
//---------------------------------------------
int    SERV_TCP_PORT;
char   *SERV_HOST_ADDR;
char   service[PORT_LEN], address[IP_LEN];
struct sockaddr_in   serv_addr;
struct utsname       hostname;
char   timeBuffer[TIME_BUF_LN];
char   serverVersion[]="7.0.0.0";

//---------------------------------------------
// Prototypes 
//---------------------------------------------
int   openSocket();
int   get_dyn_statement();
void  cleanup(int socket);
void  wakeup(int signum);
int   switchQueue(int numberOfQueues, char queueDir[]);
