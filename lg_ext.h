//-------------------------------------------------------------------------------
// DISCLAIMER - Though the use of const int is prefered, the limitations of pro*c
// make the use of defines a must
//--------------------------------------------------------------------------------
#define     UNAME_LEN          20
#define     PWD_LEN            40
#define     LOG_MEMBER_LENGTH 512
#define     ARRAY_LENGTH     1024
#define     SQL_LENGTH       4000
#define     ARRAY_SIZE        250
#define     UNSIGNED_LONG_INT 4294967295
#define     MAX_SCN_LENGTH     16
//---------------------------------------------
// Regular C/C++ consts
//---------------------------------------------
const int   OBJ_NM_LEN      =   30;
const int   TRUE            =    1;
const int   FALSE           =    0;
const int   TIME_BUF_LN     =   20;
const int   FN_LEN          =  128;
const int   MAX_REDO_ENTRIES=  128;
const int   MAX_IDLE_TIME   =    8;
const int   REDO_LENGTH     = 4096;

struct RedoStruct
{
  char			redoChangeNum[MAX_REDO_ENTRIES][MAX_SCN_LENGTH];
  unsigned int          redoGroupNum[MAX_REDO_ENTRIES];
  char                  redoLogMember[MAX_REDO_ENTRIES][128];
  int                   status[MAX_REDO_ENTRIES];
};

char          RedoMember[128];
int           glb_switchState=0;
int           glb_contFlag=0;
std::fstream  glb_queuefile;

//====================================================
// PROTOTYPES OF CALLING FUNCTIONS
//====================================================
/* Prototypes */
#if defined(__EXT__)
int   sql_proc(int connectFlag, char *inMsg); 
char* PrintRows(int n, char *MaxCSCN);
void  Cleanup(int);
int   switchQueue(int numberOfQueues, char queueDir[]);
void  Wakeup(int);
#else
int   sql_proc(int connectFlag, char *inMsg); 
char* PrintRows(int n, char *MaxCSCN);
void  Cleanup(int);
int   switchQueue(int numberOfQueues, char queueDir[]);
void  Wakeup(int);
#endif
