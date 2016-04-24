//------------------------------
// SERVER
//------------------------------

extern char *sys_errlist[];
extern int errno;
u_short portbase = 0;

#define QLEN           5
#define BUFSIZE        8192
const int TIME_BUF_LN=20;
const int TRUE=1;
const int FALSE=0;

char *pname;

int glb_sleepSignal = FALSE;
int glb_switchState = FALSE;
std::string glb_pauseFileName;

char   serverVersion[]="7.0.0.0";


//-----------------------------------------------------------
// FUNCTION PROTOTYPES
//-----------------------------------------------------------
void Cleanup(int);
void Wakeup(int);
void reaper(int);
int  checkPauseFile(const char inPauseFileName[]);


